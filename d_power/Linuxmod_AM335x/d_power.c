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


/*! \page PowerModule Power Control Module
 *
 *  Manages the power supervising and budget.\n
 *
 *-  \subpage PowerModuleResources
 */


#ifndef PCASM
#include  <asm/types.h>
#endif

#define   HW_ID_SUPPORT

#include  "../../lms2012/source/lms2012.h"
#include  "../../lms2012/source/am335x.h"


#define   MODULE_NAME                   "power_module"
#define   DEVICE1_NAME                  POWER_DEVICE

static    int  ModuleInit(void);
static    void ModuleExit(void);

#define   __USE_POSIX

#include  <linux/kernel.h>
#include  <linux/fs.h>

#include  <linux/sched.h>


#include  <linux/hrtimer.h>

#include  <linux/mm.h>
#include  <linux/hrtimer.h>

#include  <linux/init.h>
#include  <linux/uaccess.h>
#include  <linux/debugfs.h>

#include  <linux/ioport.h>
#include  <asm/gpio.h>
#include  <asm/io.h>
#include  <linux/module.h>
#include  <linux/miscdevice.h>
#include  <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("The LEGO Group");
MODULE_DESCRIPTION(MODULE_NAME);
MODULE_SUPPORTED_DEVICE(DEVICE1_NAME);

module_init(ModuleInit);
module_exit(ModuleExit);



int       Hw = 6;

enum      PowerPins
{
//  P_EN,
  PENON,
//  SW_RECHARGE,
  TESTPIN,
  POWER_PINS
};


INPIN     PowerPin[POWER_PINS];

/*! \page PowerModuleResources Gpios and Resources used for Module
 *
 *  Describes use of gpio and resources\n
 *
 *  \verbatim
 */

INPIN     PowerPin[POWER_PINS] =
{
//  {-1, NULL, 0 }, // P_EN
  { GP2_6 , NULL, 0 }, // 5VPENON
//  { GP8_8  , NULL, 0 }, // SW_RECHARGE
  { GP1_0 , NULL, 0 }  // ADCACK
};



static volatile ULONG *CM_PER;
static volatile ULONG *CM;

static volatile ULONG *GPIOBANK0;
static volatile ULONG *GPIOBANK1;
static volatile ULONG *GPIOBANK2;
static volatile ULONG *GPIOBANK3;




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

	  for (Pin = 0; Pin < POWER_PINS; Pin++)
	  {
		if(PowerPin[Pin].Pin >= 0 && PowerPin[Pin].Pin < 128)
		{
			if (PowerPin[Pin].Pin < 32)
				PowerPin[Pin].pGpio = GPIOBANK0;
			else if (PowerPin[Pin].Pin < 64)
				PowerPin[Pin].pGpio = GPIOBANK1;
			else if (PowerPin[Pin].Pin < 96)
				PowerPin[Pin].pGpio = GPIOBANK2;
			else
				PowerPin[Pin].pGpio = GPIOBANK3;
			
			PowerPin[Pin].Mask = (1 << (PowerPin[Pin].Pin & 0x1F));
			SetGpio(PowerPin[Pin].Pin);
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



#define   POWEROn           	{\
					(PowerPin[PENON].pGpio)[GPIO_SETDATAOUT] = PowerPin[PENON].Mask;\
					(PowerPin[PENON].pGpio)[GPIO_OE] &= ~PowerPin[PENON].Mask;\
				}



#define   TESTOn           	{\
					(PowerPin[TESTPIN].pGpio)[GPIO_SETDATAOUT] = PowerPin[TESTPIN].Mask;\
					(PowerPin[TESTPIN].pGpio)[GPIO_OE] &= ~PowerPin[TESTPIN].Mask;\
				}






#define   TESTOff          	{\
					(PowerPin[TESTPIN].pGpio)[GPIO_CLEARDATAOUT] = PowerPin[TESTPIN].Mask;\
					(PowerPin[TESTPIN].pGpio)[GPIO_OE] &= ~PowerPin[TESTPIN].Mask;\
				}



















// DEVICE1 ********************************************************************
static    UBYTE PowerOffFlag = 0;

static long Device1Ioctl(struct file *File, unsigned int Request, unsigned long Pointer)
{
  PowerOffFlag  =  1;

  return (0);
}


static ssize_t Device1Write(struct file *File,const char *Buffer,size_t Count,loff_t *Data)
{
  char    Buf[1];
  int     Lng     = 0;

  if (Count == 1)
  {
    copy_from_user(Buf,Buffer,Count);
    if (Buf[0])
    {
      TESTOn;
    }
    else
    {
      TESTOff;
    }
  }

  Lng           =  Count;

  return (Lng);
}


static ssize_t Device1Read(struct file *File,char *Buffer,size_t Count,loff_t *Offset)
{
  int     Lng     = 0;
  char    Accu;
  int	  battery = 1;

  //if (ACCURead)
  if (battery)
  {
    Accu  =  '0';
  }
  else
  {
    Accu  =  '1';
  }

  Lng  =  snprintf(Buffer,Count,"%c\r",Accu);

  return (Lng);
}


static    const struct file_operations Device1Entries =
{
  .owner        = THIS_MODULE,
  .read         = Device1Read,
  .write        = Device1Write,
  .unlocked_ioctl        = Device1Ioctl
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

  Result  =  misc_register(&Device1);
  if (Result)
  {
    printk("  %s device register failed\n",DEVICE1_NAME);
  }
  else
  {
    PowerOffFlag  =  0;

#ifdef DEBUG
    printk("  %s device register succes\n",DEVICE1_NAME);
#endif
  }

  return (Result);
}


static void Device1Exit(void)
{
  misc_deregister(&Device1);
#ifdef DEBUG
  printk("  %s device unregistered\n",DEVICE1_NAME);
#endif
}


// MODULE *********************************************************************


static int ModuleInit(void)
{


  if (Hw < PLATFORM_START)
  {
    Hw  =  PLATFORM_START;
  }
  if (Hw > PLATFORM_END)
  {
    Hw  =  PLATFORM_END;
  }


	GetPeriphealBasePtr(0x44E10000, 0x1448, (ULONG **)&CM);
	GetPeriphealBasePtr(0x44E00000, 0x154, (ULONG **)&CM_PER);
	GetPeriphealBasePtr(0x44E07000, 0x198, (ULONG **)&GPIOBANK0);	
	GetPeriphealBasePtr(0x4804C000, 0x198, (ULONG **)&GPIOBANK1);	
	GetPeriphealBasePtr(0x481AC000, 0x198, (ULONG **)&GPIOBANK2);	
	GetPeriphealBasePtr(0x481AE000, 0x198, (ULONG **)&GPIOBANK3);	



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
      POWEROn;
      TESTOff;
  //    ACCUFloat;

      Device1Init();

  return (0);
}


static void ModuleExit(void)
{
  ULONG   Tmp1;
  ULONG   Tmp2;

#ifdef DEBUG
  printk("%s exit started\n",MODULE_NAME);
#endif

  TESTOff;
  Device1Exit();

  if (PowerOffFlag)
  {
    for (Tmp1 = 0;Tmp1 < 0xFFFFFFFF;Tmp1++)
    {
      for (Tmp2 = 0;Tmp2 < 0xFFFFFFFF;Tmp2++)
      {
        //POWEROff;				//deleted for EVB
      }
    }
  }

  iounmap(GPIOBANK3);
  iounmap(GPIOBANK2);
  iounmap(GPIOBANK1);
  iounmap(GPIOBANK0);
  iounmap(CM_PER);
  iounmap(CM);


}
