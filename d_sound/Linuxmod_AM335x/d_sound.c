/*
 * LEGO® MINDSTORMS EV3
 *
 * Copyright (C) 2010-2013 The LEGO Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*! \page Sound Driver Module
 *
 *
 *-  \subpage SoundDriverModuleResources
 */


#define   HW_ID_SUPPORT

#include  "../../lms2012/source/lms2012.h"
#include  "../../lms2012/source/am335x.h"


#define   MODULE_NAME                   "sound_module"
#define   DEVICE1_NAME                  SOUND_DEVICE

static    int  ModuleInit(void);
static    void ModuleExit(void);

#define   __USE_POSIX

#include  <linux/kernel.h>
#include  <linux/fs.h>

#include  <linux/sched.h>


#include  <linux/mm.h>
#include  <linux/hrtimer.h>

#include  <linux/init.h>
#include  <linux/uaccess.h>
#include  <linux/debugfs.h>

#include  <linux/delay.h>		//!!!!!!!!!!!!!!!!!!!!!!!

#include  <linux/ioport.h>
#include  <asm/gpio.h>
#include  <asm/io.h>
#include  <linux/module.h>
#include  <linux/miscdevice.h>
#include  <asm/uaccess.h>


//*******************************************
#include <linux/slab.h>
//*******************************************

MODULE_LICENSE("GPL");
MODULE_AUTHOR("The LEGO Group");
MODULE_DESCRIPTION(MODULE_NAME);
MODULE_SUPPORTED_DEVICE(DEVICE1_NAME);

module_init(ModuleInit);
module_exit(ModuleExit);


enum
{
  TIMING_SAMPLES,
  ONE_SHOT,
  MANUAL,
  READY_FOR_SAMPLES,
  IDLE
} TIMER_MODES;

/*
 * Variables
 */

static volatile ULONG *CM_PER;
static volatile ULONG *CM;

static volatile ULONG *GPIOBANK0;
static volatile ULONG *GPIOBANK1;
static volatile ULONG *GPIOBANK2;
static volatile ULONG *GPIOBANK3;

static volatile ULONG *PWMSS0;
static volatile ULONG *ECAP0;

static    UWORD   Duration;
static    UWORD   Period;
static    UBYTE   Level;
static 	  UBYTE	  TimerMode = READY_FOR_SAMPLES;
static	  SOUND		SoundDefault;
static	  SOUND   *pSound = &SoundDefault;
static    struct hrtimer  Device1Timer;
static    ktime_t         Device1Time;


static UWORD	SoundBuffers[SOUND_BUFFER_COUNT][SOUND_FILE_BUFFER_SIZE];
static UBYTE  SoundChunkSize[SOUND_BUFFER_COUNT];
static UWORD  BufferReadPointer;
static UBYTE	BufferReadIndex;
static UBYTE	BufferWriteIndex;



#define	TSCTR	(0x0)
#define	CTRPHS	(0x4 >> 2)
#define	CAP1	(0x8 >> 2)
#define	CAP2	(0xC >> 2)
#define	CAP3	(0x10 >> 2)
#define	CAP4	(0x14 >> 2)

	
//********** This 2 pointers is UWORD **********
#define	ECCTL1		((UWORD *)(ECAP0 + (0x28 >> 2)))
#define ECCTL2		((UWORD *)(ECCTL1 + 1))
















/*
 * Macro defines related to hardware PWM output
 */

#define   EHRPWMClkDisable             (*ECCTL2) = 0x280;



#define   EHRPWMClkEnable              (*ECCTL2) = 0x290;


#define   EHRPWMClkEnableTone          (*ECCTL2) = 0x290;


#define   SETPwmPeriod(Prd)            ECAP0[CAP1] = Prd; /* A factor of sample-rate  */\

#define   SETSoundLevel(Level)         ECAP0[CAP2] = Level;  /* The amplitude for this */\ 



#define   STOPPwm                       {\
                                          EHRPWMClkDisable;\
					  PWMSS0[0x2] &= ~0x1;\
                                        }

#define   SOUNDPwmModuleSetupPcm        { \
                                          EHRPWMClkDisable;\
                                          ECAP0[CAP1] = 0; \
                                          ECAP0[TSCTR] = 0; \
                                          EHRPWMClkEnable;\
                                        }

#define   SOUNDPwmModuleSetupTone        SOUNDPwmModuleSetupPcm       

#define   SOUNDEnable           {\
					(SoundPin[SOUNDEN].pGpio)[GPIO_SETDATAOUT] = SoundPin[SOUNDEN].Mask;\
					(SoundPin[SOUNDEN].pGpio)[GPIO_OE] &= ~SoundPin[SOUNDEN].Mask;\
				}

#define   SOUNDDisable          {\
					(SoundPin[SOUNDEN].pGpio)[GPIO_CLEARDATAOUT] = SoundPin[SOUNDEN].Mask;\
					(SoundPin[SOUNDEN].pGpio)[GPIO_OE] &= ~SoundPin[SOUNDEN].Mask;\
				}


#define   SOUNDPwmPoweron       {\
					CM[0x199]   |= 0x1;/* Timebase for PWMSS0 enable */\
					PWMSS0[0x2] |= 0x1;\
                                }






#define	SOUND_MASTER_CLOCK_BBB	100000000 




enum      SoundPins
{
  SOUNDEN,        // Sound Enable to Amp
  SOUND_ARMA,     // PWM audio signal from ARM
  SOUND_PINS
};



INPIN     SoundPin[SOUND_PINS] =
{
  { GP0_20  , NULL, 0 }, // SOUNDEN
  { GP0_7 , NULL, 0 }, // SOUND_ARMA
};





void SetGpio(int Pin)
{
	int Tmp = 0;

	if ( Pin >= 0)
	{
		while((MuxRegMap[Tmp].Pin != Pin) && (MuxRegMap[Tmp].Pin != -1))
		{
			Tmp++;
		}
		if (MuxRegMap[Tmp].Pin == Pin)
		{
			CM[MuxRegMap[Tmp].Addr >> 2] = MuxRegMap[Tmp].Mode;
		}
	}
}



void InitGpio(void)
{
	int Pin;

	  for (Pin = 0; Pin < SOUND_PINS; Pin++)
	  {
		if(SoundPin[Pin].Pin >= 0 && SoundPin[Pin].Pin < 128)
		{
			if (SoundPin[Pin].Pin < 32)
				SoundPin[Pin].pGpio = GPIOBANK0;
			else if (SoundPin[Pin].Pin < 64)
				SoundPin[Pin].pGpio = GPIOBANK1;
			else if (SoundPin[Pin].Pin < 96)
				SoundPin[Pin].pGpio = GPIOBANK2;
			else
				SoundPin[Pin].pGpio = GPIOBANK3;
			
			SoundPin[Pin].Mask = (1 << (SoundPin[Pin].Pin & 0x1F));
			SetGpio(SoundPin[Pin].Pin);
		}	
	  }
}


void GetPeriphealBasePtr(ULONG Address, ULONG Size, ULONG **Ptr)
{
	if(request_mem_region(Address, Size, MODULE_NAME) >= 0)
	{
		*Ptr = (ULONG *)ioremap(Address, Size);
		if (*Ptr == NULL)
			printk("%s memory remap ERROR!\n", DEVICE1_NAME);
	}
	else
	{
		printk("Region request ERROR!\n");
	}
}




// DEVICE1 ********************************************************************



static void Device1TimerSetTiming(ULONG Secs, ULONG NanoSecs )
{
  Device1Time  =  ktime_set(Secs, NanoSecs);
}

static void Device1TimerStart(void)     // (Re) start the high-resolution timer
{
  hrtimer_start(&Device1Timer,Device1Time,HRTIMER_MODE_REL);
}

static void Device1TimerInitDuration(void)
{
  UWORD Sec;
  ULONG nSec;

  Sec = (UWORD)(Duration / 1000);                       // Whole secs
  nSec = (ULONG)((Duration - Sec * 1000) * 1000000);    // Raised to mSec
  TimerMode = ONE_SHOT;
  Device1TimerSetTiming(Sec, nSec);
  Device1TimerStart();                                  // Start / reStart
}

static void Device1TimerCancel(void)
{
  TimerMode = IDLE;
  hrtimer_cancel(&Device1Timer);
}

static void Device1TimerExit(void)
{
  Device1TimerCancel();
}

static enum hrtimer_restart Device1TimerInterrupt1(struct hrtimer *pTimer)
{
  ULONG TempLevel;

  enum hrtimer_restart ret = HRTIMER_RESTART;

  if (0 < hrtimer_forward_now(pTimer,Device1Time))
  {
    // Take care sample missed!!!!!
  }

  switch(TimerMode)
  {
    case READY_FOR_SAMPLES: if(SoundChunkSize[BufferReadIndex] > 0) // Any samples D/L yet?
                            {

                              TimerMode = TIMING_SAMPLES;           // We're ready, next interrupt will
                                                                    // bring the sound alive
                            }
                            break;

    case TIMING_SAMPLES:    // Get new sample - if any

                            if(SoundChunkSize[BufferReadIndex] > 0) // Anything to use in Buffers?
                            {
                              // Get raw sample
                              TempLevel = (ULONG)SoundBuffers[BufferReadIndex][BufferReadPointer++];
                              if(TempLevel == 0) TempLevel++;
                              // Add volume i.e. scale the PWM level

                              TempLevel = (ULONG)(Level * TempLevel);
                              SETSoundLevel(TempLevel);

                              SoundChunkSize[BufferReadIndex]--;

                              if(SoundChunkSize[BufferReadIndex] < 1)
                              {
                                BufferReadPointer = 0;
                                BufferReadIndex++;
                                if(BufferReadIndex >= SOUND_BUFFER_COUNT)
                                  BufferReadIndex = 0;
                              }
                            }
                            else
                            {
                              ret = HRTIMER_NORESTART;

                              SOUNDDisable;
                              TimerMode = IDLE;
                              (*pSound).Status  =  OK;
                              // ret(urn value) already set
                            }
                            break;

    case ONE_SHOT:          // Stop tone - duration elapsed!
                            ret = HRTIMER_NORESTART;
                            EHRPWMClkDisable;
                            SOUNDDisable;
                            TimerMode = IDLE;
                            (*pSound).Status  =  OK;
                            // ret(urn value) already set

    case IDLE:              break;
    case MANUAL:            break;
    default:                ret = HRTIMER_NORESTART;
                            break;
    }

    return (ret);   // Keep on or stop doing the job
}

static void Device1TimerInit8KHz(void)
{
  /* Setup 125 uS timer interrupt*/

    TimerMode = READY_FOR_SAMPLES;    // Allow for D/L to SoundBuffer(s)
    Device1TimerSetTiming(0, 125000); // 8 KHz. sample-rate
    Device1TimerStart();              // Start / reStart
}

static ssize_t Device1Write(struct file *File, const char *Buffer, size_t Count, loff_t *Data)
{

    char    CommandBuffer[SOUND_FILE_BUFFER_SIZE + 1];	// Space for command @ byte 0
    ULONG   Frequency;		//changed for bbb
    ULONG   PwmLevel;		//changed for bbb
    int		BytesWritten = 0;
    int 	i = 0;

    copy_from_user(CommandBuffer, Buffer, Count);

    switch(CommandBuffer[0])
    {
      case SERVICE:
                      if(Count > 1)
      	  	  	  	  	{
    	  	  	  	    	  if(SoundChunkSize[BufferWriteIndex] == 0) // Is the requested RingBuffer[Index] ready for write?
    	  	  	  	    	  {
    	  	  	  	    	    for(i = 1; i < Count; i++)
    	  	  	  	    	      SoundBuffers[BufferWriteIndex][i-1] = CommandBuffer[i];
    	  	  	  	    	    SoundChunkSize[BufferWriteIndex] = Count - 1;
    	  	  	  	    	    BufferWriteIndex++;
    	  	  	  	    	    if(BufferWriteIndex >= SOUND_BUFFER_COUNT)
    	  	  	  	    	      BufferWriteIndex = 0;
    	  	  	  	    	    BytesWritten = Count - 1;
    	  	  	  	    	  }
      	  	  	  	  	}
          	  	  	  	break;

      case TONE:
    	  	  	SOUNDDisable;
                        SOUNDPwmModuleSetupTone;
                        Level = CommandBuffer[1];
                        Frequency = (ULONG)(CommandBuffer[2] + (CommandBuffer[3] << 8));	//changed for bbb
                        Duration = (UWORD)(CommandBuffer[4] + (CommandBuffer[5] << 8));	

                        if (Frequency < SOUND_MIN_FRQ)
                        {
                          Frequency = SOUND_MIN_FRQ;
                        }
                        else if (Frequency > SOUND_MAX_FRQ)
                        {
                          Frequency = SOUND_MAX_FRQ;
                        }

                        Period = (ULONG)((SOUND_MASTER_CLOCK_BBB / Frequency) - 1);	//changed for bbb
                        SETPwmPeriod(Period);

                        if (Level > 100) Level = 100;

                        PwmLevel = (ULONG)(((ulong)(Period) * (ulong)(Level)) /(ulong)(200)); // Level from % to ticks (Duty Cycle)

                        SETSoundLevel(PwmLevel);

                        if(Duration > 0)  // Infinite or specific?
                        {
                          Device1TimerInitDuration();
                          TimerMode = ONE_SHOT;
                        }
                        else
                          TimerMode = MANUAL;

                        SOUNDEnable;

                        break;

      case REPEAT:	  // Handled in Green Layer
    	  	  	  	    break;

      case PLAY:
    	  	  	  	  SOUNDDisable;
                      SOUNDPwmModuleSetupPcm;
                      Level = CommandBuffer[1];
                      Period = (ULONG)(SOUND_MASTER_CLOCK_BBB / 64000); // 64 KHz => 8 shots of each sample
      	  	  	  	  SETPwmPeriod(Period);                         // helps filtering

    	  	  	  	    BufferWriteIndex = 0;		// Reset to first ring Buffer
      	  	  	  	  BufferReadIndex = 0;		// -
      	  	  	  	  BufferReadPointer = 0;	// Ready for very first sample
      	  	  	  	  for (i = 0; i < SOUND_BUFFER_COUNT; i++)
      	  	  	  		  SoundChunkSize[i] = 0;	// Reset all Buffer sizes

      	  	  	  	  Device1TimerInit8KHz();   // TimerMode = READY_FOR_SAMPLES;
      	  	  	  	  if(Level != 0)
      	  	  	  	    SOUNDEnable;
      	  	  	  	  // Else we remove any "selfmade" noise
      	  	  	  	  // Better n/s+n ratio ;-)
      	  	          BytesWritten = 2; // CMD and Level
    	  	  	  	    break;

      case BREAK:     Device1TimerCancel();
                      SOUNDDisable;
                      EHRPWMClkDisable;
                      TimerMode = IDLE;
                      (*pSound).Status  =  OK;
                      break;

      default:
                      break;
    }

  return (BytesWritten);
}


static ssize_t Device1Read(struct file *File,char *Buffer,size_t Count,loff_t *Offset)
{
  int     Lng     = 0;

  Lng  =  snprintf(Buffer,Count,"%s\r",DEVICE1_NAME);

  return (Lng);
}

#define     SHM_LENGTH    (sizeof(SoundDefault))
#define     NPAGES        ((SHM_LENGTH + PAGE_SIZE - 1) / PAGE_SIZE)
static void *kmalloc_ptr;

static int Device1Mmap(struct file *filp, struct vm_area_struct *vma)
{
   int ret;

   ret = remap_pfn_range(vma,vma->vm_start,virt_to_phys((void*)((unsigned long)pSound)) >> PAGE_SHIFT,vma->vm_end-vma->vm_start,PAGE_SHARED);

   if (ret != 0)
   {
     ret  =  -EAGAIN;
   }

   return (ret);
}

static    const struct file_operations Device1Entries =
{
  .owner        = THIS_MODULE,
  .read         = Device1Read,
  .write        = Device1Write,
  .mmap         = Device1Mmap,
};


static    struct miscdevice Device1 =
{
  MISC_DYNAMIC_MINOR,
  DEVICE1_NAME,
  &Device1Entries
};


static int Device1Init(void)
{
  int     Result = -1;
  int i = 0;
  UWORD   *pTmp;

  	
  	// Get pointers to memory-mapped registers
	GetPeriphealBasePtr(0x48300000, 0x10, (ULONG **)&PWMSS0);
	GetPeriphealBasePtr(0x48300100, 0x60, (ULONG **)&ECAP0);



  Result  =  misc_register(&Device1);
  if (Result)
  {

      printk("  %s device register failed\n",DEVICE1_NAME);

  }
  else
  {
    // allocate kernel shared memory for SoundFlags used by Test etc.
    // showing the state of the sound-module used by async and waiting
    // use from VM

        if ((kmalloc_ptr = kmalloc((NPAGES + 2) * PAGE_SIZE, GFP_KERNEL)) != NULL)
        {
          pTmp = (UWORD*)((((unsigned long)kmalloc_ptr) + PAGE_SIZE - 1) & PAGE_MASK);
          for (i = 0; i < NPAGES * PAGE_SIZE; i += PAGE_SIZE)
          {
            SetPageReserved(virt_to_page(((unsigned long)pTmp) + i));
          }
          pSound =  (SOUND*)pTmp;

          SOUNDPwmPoweron;

          /* Setup the Sound PWM peripherals */

          SOUNDPwmModuleSetupPcm;

          /* Setup 125 uS timer interrupt*/
          Device1TimerSetTiming(0, 125000); // Default to 8 KHz. sample-rate
          hrtimer_init(&Device1Timer,CLOCK_MONOTONIC,HRTIMER_MODE_REL);
          // Timer Callback function - do the sequential job
          Device1Timer.function  =  Device1TimerInterrupt1;
          Device1TimerCancel();

          SOUNDDisable; // Disable the Sound Power Amp

		      #ifdef DEBUG
        	  printk("  %s device register succes\n",DEVICE1_NAME);
		      #endif

        	(*pSound).Status  =  OK;  // We're ready for making "noise"
        }
  }

  return (Result);
}

static void Device1Exit(void)
{
  UWORD   *pTmp;
  int i = 0;

  Device1TimerExit();
  misc_deregister(&Device1);



  // free shared memory
  pTmp    =  (UWORD*)pSound;
  pSound  =  &SoundDefault;

  for (i = 0; i < NPAGES * PAGE_SIZE; i+= PAGE_SIZE)
  {
    ClearPageReserved(virt_to_page(((unsigned long)pTmp) + i));
  }

  kfree(kmalloc_ptr);


  iounmap(PWMSS0);
  iounmap(ECAP0);

}


// MODULE *********************************************************************


static int ModuleInit(void)
{

	GetPeriphealBasePtr(0x44E10000, 0x1448, (ULONG **)&CM);
	GetPeriphealBasePtr(0x44E00000, 0x154, (ULONG **)&CM_PER);
	GetPeriphealBasePtr(0x44E07000, 0x198, (ULONG **)&GPIOBANK0);	
	GetPeriphealBasePtr(0x4804C000, 0x198, (ULONG **)&GPIOBANK1);	
	GetPeriphealBasePtr(0x481AC000, 0x198, (ULONG **)&GPIOBANK2);	
	GetPeriphealBasePtr(0x481AE000, 0x198, (ULONG **)&GPIOBANK3);	



	CM_PER[0x35] |= 0x2;	//CM_PER_EPWMSS0_CLKCTRL
	while(0x2 != (ioread32(&CM_PER[0x35]) & 0x2))
		;

	CM_PER[0xAC >> 2] |= 0x2;	//CM_PER_GPIO1_CLKCTRL
	while(0x2 != (ioread32(&CM_PER[0xAC >> 2]) & 0x2))
		;

	CM_PER[0xB0 >> 2] |= 0x2;	//CM_PER_GPIO2_CLKCTRL
	while(0x2 != (ioread32(&CM_PER[0xB0 >> 2]) & 0x2))
		;

	CM_PER[0xB4 >> 2] |= 0x2;	//CM_PER_GPIO3_CLKCTRL
	while(0x2 != (ioread32(&CM_PER[0xB4 >> 2]) & 0x2))
		;



      InitGpio();
      Device1Init();

 
      return (0);
}


static void ModuleExit(void)
{
  SOUNDDisable; // Disable the Sound Power Amp
  Device1Exit();
  iounmap(GPIOBANK3);
  iounmap(GPIOBANK2);
  iounmap(GPIOBANK1);
  iounmap(GPIOBANK0);
  iounmap(CM_PER);
  iounmap(CM);
}

