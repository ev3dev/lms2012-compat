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


/*! \page UiModule Ui Button/Led Module
 *
 *  Manages button and LEDs\n
 *
 *-  \subpage UiDriver
 *-  \subpage UiModuleMemory
 *-  \subpage UiModuleResources
 *
 */


#ifndef PCASM
#include  <asm/types.h>
#endif

#define   HW_ID_SUPPORT

#include "../../lms2012/source/lms2012.h"
#include "../../lms2012/source/am335x.h"
#include "vol_char_8x20.c"
#include "logo_41x54.c"
#include "fatcatlab_178x24.c"

#define   MODULE_NAME                   "ui_module"
#define   DEVICE1_NAME                  UI_DEVICE

static    int  ModuleInit(void);
static    void ModuleExit(void);

#include  <linux/kernel.h>
#include  <linux/fs.h>

#include  <linux/sched.h>

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

#include  <linux/fb.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("The LEGO Group");
MODULE_DESCRIPTION(MODULE_NAME);
MODULE_SUPPORTED_DEVICE(DEVICE1_NAME);

module_init(ModuleInit);
module_exit(ModuleExit);


int       Hw = 6;

enum      UiButPins
{
  BUT0,     // UP
  BUT1,     // ENTER
  BUT2,     // DOWN
  BUT3,     // RIGHT
  BUT4,     // LEFT
  BUT5,     // BACK
  BUT_PINS
};


INPIN     UiButPin[BUT_PINS];


#define   NO_OF_LEDS                    LEDS
#define   NO_OF_BUTTONS                 BUTTONS

//static volatile ULONG *CM;
//static volatile ULONG *CM_PER;
static volatile ULONG *CM_WKUP;

static volatile ULONG *ADC_TSC;


#define CTRL 		(0x40 >> 2)
#define ADC_CLKDIV 	(0x4C >> 2)
#define STEPENABLE 	(0x54 >> 2)
#define STEPCONFIG1 	(0x64 >> 2)
#define STEPCONFIG2 	(0x6C >> 2)
#define STEPDELAY1 	(0x68 >> 2)
#define FIFO0DATA 	(0x100 >> 2)
#define FIFO1DATA 	(0x200 >> 2)


extern struct fb_info *ili9225fb_extern_info;
extern void ili9225fb_extern_touch(int, int, int, int);

static unsigned char *fbmem;


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

static    struct hrtimer Device1Timer;
static    ktime_t        Device1Time;
static    struct hrtimer Device2Timer;
static    ktime_t        Device2Time;

static    UI UiDefault;
static    UI *pUi = &UiDefault;


unsigned long     LEDPATTERNDATA[NO_OF_LEDS + 1][LEDPATTERNS] =
{ //  LED_BLACK   LED_GREEN   LED_RED    LED_ORANGE           LED_GREEN_FLASH                     LED_RED_FLASH                     LED_ORANGE_FLASH                      LED_GREEN_PULSE                       LED_RED_PULSE                      LED_ORANGE_PULSE
  {  0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0b00000000000000000000000000000000, 0b00000000000000000111110000011111, 0b00000000000000000111110000011111, 0b00000000000000000000000000000000, 0b00000000000000000000000001110111, 0b00000000000000000000000001110111 }, // RR
  {  0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0b00000000000000000111110000011111, 0b00000000000000000000000000000000, 0b00000000000000000111110000011111, 0b00000000000000000000000001110111, 0b00000000000000000000000000000000, 0b00000000000000000000000001110111 }, // RG
  {  0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0b00000000000000000000000000000000, 0b00000000000000000111110000011111, 0b00000000000000000111110000011111, 0b00000000000000000000000000000000, 0b00000000000000000000000001110111, 0b00000000000000000000000001110111 }, // LR
  {  0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0b00000000000000000111110000011111, 0b00000000000000000000000000000000, 0b00000000000000000111110000011111, 0b00000000000000000000000001110111, 0b00000000000000000000000000000000, 0b00000000000000000000000001110111 }, // LG
  { 0 }
};


UBYTE     PatternBlock    = 0;          // Block pattern update
UBYTE     PatternBits     = 20;         // Pattern bits
UBYTE     PatternBit      = 0;          // Pattern bit pointer
ULONG     ActPattern[NO_OF_LEDS];
ULONG     TmpPattern[NO_OF_LEDS];

void lcd_put_pixel(int x, int y, unsigned char c0, unsigned char c1)
{
	int location;

	location = (x + y * 220) * 2;
	*(fbmem + location++) = c0;
	*(fbmem + location) = c1;
}


void lcd_put_vol_char(int x, int y, unsigned char letter, unsigned char c0, unsigned char c1)
{
	int base = letter * 20;
	int line, bit;
	unsigned char byte;

	for (line = 0; line < 20; line++)
	{
		byte = vol_char_8x20[base + line];
		for (bit = 7; bit >= 0; bit--)
		{
			if(byte & (1 << bit))
			{
				lcd_put_pixel(x + 7 - bit, y + line, c0, c1);
			}
			else
			{
				lcd_put_pixel(x + 7 - bit, y + line, 0, 0);
			}
		}
	}
}


static unsigned char bat_vol_update = 0;


static enum hrtimer_restart Device1TimerInterrupt1(struct hrtimer *pTimer)
{
  UBYTE   Tmp;

  unsigned char col1_vol = 0, col2_vol = 0;
  static int vol_times = 0, vol_sum = 0; 
  static unsigned char col1 = 0, col2 = 0, col1_old = 0, col2_old = 0;
  int x, y, location, vol, num1, num2;


  if (PatternBlock)
  {
	    PatternBlock  =  0;
  }
  else
  {
	    for (Tmp = 0;Tmp < NO_OF_LEDS;Tmp++)
	    {
		      if (PatternBit == 0)
		      {
			TmpPattern[Tmp]  =  ActPattern[Tmp];
		      }
		      if ((TmpPattern[Tmp] & 0x00000001))
		      {
		//	DIODEOn(Tmp);
			if(Tmp == 0 || Tmp == 2)
			{
				col1 |= 0xF8;
			}
			else
			{
				col1 |= 0x07;
				col2 |= 0xE0;
			}
		      }
		      else
		      {
		//	DIODEOff(Tmp);
			if(Tmp == 0 || Tmp == 2)
			{
				col1 &= 0x07;
			}
			else
			{
				col1 &= 0xF8;
				col2 = 0x0;
			}
		      }
		      TmpPattern[Tmp] >>=  1;
	    }

	    if (++PatternBit >= PatternBits)
	    {
	      PatternBit  =  0;
	    }

	if(col1 != col1_old || col2 != col2_old)
	{
		col1_old = col1;
		col2_old = col2;
		for(y = 0; y < 40; y++)
		{
			for(x = 180; x < 220; x++)
			{
				location = (x + y * 220) * 2;

				*(fbmem + location++) = col1;
				*(fbmem + location) = col2;

			}
		}

		ili9225fb_extern_touch(180, 0, 40, 40);
	}

  }

	if (bat_vol_update)
	{
		vol = (ADC_TSC[FIFO1DATA] & 0xFFF) * 1800 / 4095;
		vol = vol * 1072 / 165;		//(vol * 67 / 11) * 16 / 15;
		//vol = vol * 67 / 11;
		vol_sum += vol;
		

		if(++vol_times > 3)
		{
			vol_times = 0;
		//	printk("\nvol_sum = %d\n", vol_sum);
			vol_sum = (vol_sum >> 2) + 49;
			vol_sum = vol_sum > 9999 ? 9999 : vol_sum;

			if ( vol_sum > 7400)
			{
				col1_vol = 0x07;
				col2_vol = 0xE0;
			}
			else if (vol_sum > 6400)
			{
				col1_vol = 0xFF;
				col2_vol = 0xE0;
			}
			else
			{
				col1_vol = 0xF8;
				col2_vol = 0x0;
			}

			num1 = vol_sum / 1000;
			num2 = (vol_sum - num1 * 1000) / 100;
		 
			lcd_put_vol_char(180             , 60, 11, col1_vol, col2_vol);
			lcd_put_vol_char(180 + 11        , 60, 12, col1_vol, col2_vol);
			lcd_put_vol_char(180 + 11 * 2    , 60, 13, col1_vol, col2_vol);
			lcd_put_vol_char(180 + 11 * 3 - 1, 60, 14, col1_vol, col2_vol);
			
			
			lcd_put_vol_char(180         , 80, (unsigned char)num1, col1_vol, col2_vol);
			lcd_put_vol_char(180 + 10 * 2, 80, (unsigned char)num2, col1_vol, col2_vol);
			lcd_put_vol_char(180 + 10 * 1, 80, 10, col1_vol, col2_vol);
			lcd_put_vol_char(180 + 10 * 3, 80, 11, col1_vol, col2_vol);

			ili9225fb_extern_touch(180, 60, 11 * 4, 20 * 2);


			vol_sum = 0;
		}
	}


  // restart timer
  hrtimer_forward_now(pTimer,ktime_set(0, 50000000));

  return (HRTIMER_RESTART);
}







static enum hrtimer_restart Device2TimerInterrupt1(struct hrtimer *pTimer)
{
  UWORD   Tmp;
  ULONG   vol, vol_min, vol_max;
  static  ULONG vol_old = 0, stat = BUT_PINS;

static ULONG old_stat = 0;

	vol = (ADC_TSC[FIFO0DATA] & 0xFFF) * 1800 / 4095;
	
	bat_vol_update = vol > 100 ? 0 : 1;

	vol_min = vol < vol_old ? vol : vol_old;
	vol_max = vol > vol_old ? vol : vol_old;


	if ((vol_max - vol_min) < 20)
	{
		//if (vol < 214 && vol > 114)
		if (vol < 274 && vol > 174)
		{
			stat = BUT0;			//up, 47K --- 163 || 33K --- 224
		}
		//else if (vol < 393 && vol > 293)
		else if (vol < 422 && vol > 322)
		{	
			stat = BUT1;			//enter, 20K, 342 || 18K --- 372
		}
		else if (vol < 625 && vol > 525)
		{
			stat = BUT2;			//down, 10K, 575
		}
		else if (vol < 950 && vol > 850)
		{
			stat = BUT3;			//right, 4.7K, 900
		}
		//else if (vol < 1310 && vol > 1210)
		else if (vol < 1329 && vol > 1229)
		{
			stat = BUT4;			//left, 2K, 1262 || 1.91K --- 1279
		}
		else if (vol < 1534 && vol > 1434)
		{
			stat = BUT5;			//back, 1K, 1484
		}
		else
		{
			stat = BUT_PINS;
		}

		for(Tmp = 0; Tmp < BUT_PINS; Tmp++)
		{
			if(Tmp == stat)
			{//Button active
				(*pUi).Pressed[Tmp] = 1;
			}
			else
			{//Button inactive
				(*pUi).Pressed[Tmp] = 0;
			}
		}


//		if(stat != old_stat)
//			printk("new stat = %d, vol_old = %d, vol = %d\n", stat, vol_old, vol);

		old_stat = stat;

	}
	
	vol_old = vol;
	



  // restart timer
  hrtimer_forward_now(pTimer,ktime_set(0,10000000));

  return (HRTIMER_RESTART);
}


/*! \page UiDriver
 *
 *  <hr size="1"/>
 *  <b>     write </b>
 *
 *
 *  CP        C = Color, P = Pattern
 *
 *  C = 0     Off
 *  C = 1     Green
 *  C = 2     Red
 *  C = 3     Orange
 *
 *  P = 0     Off
 *  P = 1     On
 *  P = 2     50% (250mS on, 250mS off)
 *  P = 3     30% (150mS on, 50mS off, 150mS on, 650mS off)
 *
 *- 0pct      Set all LED = pct ["0".."100"]
 *- 1pct      Set LED 1 = pct
 *- 2pct      Set LED 2 = pct
 *- 3pct      Set LED 3 = pct
 *- 4pct      Set LED 4 = pct
 */
/*! \brief    Device1Write
 *
 *
 *
 */
static ssize_t Device1Write(struct file *File,const char *Buffer,size_t Count,loff_t *Data)
{
  char    Buf[2];
  UBYTE   No;
  UBYTE   Tmp;
  int     Lng     = 0;

  if (Count >= 2)
  {
    Lng     =  Count;
    copy_from_user(Buf,Buffer,2);
    No      =  Buf[0] - '0';
    if ((No >= 0) && (No < LEDPATTERNS))
    {
      PatternBlock  =  1;

      PatternBits   =  20;
      PatternBit    =  0;

      for (Tmp = 0;Tmp < NO_OF_LEDS;Tmp++)
      {
        ActPattern[Tmp]  =  LEDPATTERNDATA[Tmp][No];
      }

      PatternBlock  =  0;
    }
  }

  return (Lng);
}


static ssize_t Device1Read(struct file *File,char *Buffer,size_t Count,loff_t *Offset)
{
  int     Lng = 0;

  //Lng  =  snprintf(Buffer,Count,"V%c.%c0",HwId[0],HwId[1]);
  Lng  =  snprintf(Buffer,Count,"V%c.%c0",0,6);

  return (Lng);
}


#define     SHM_LENGTH    (sizeof(UiDefault))
#define     NPAGES        ((SHM_LENGTH + PAGE_SIZE - 1) / PAGE_SIZE)
static void *kmalloc_ptr;

static int Device1Mmap(struct file *filp, struct vm_area_struct *vma)
{
   int ret;

   ret = remap_pfn_range(vma,vma->vm_start,virt_to_phys((void*)((unsigned long)pUi)) >> PAGE_SHIFT,vma->vm_end-vma->vm_start,PAGE_SHARED);

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
  .mmap         = Device1Mmap
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
	  int     i, location, x, y;
	  UWORD   *pTmp;

	

	GetPeriphealBasePtr(0x44E0D000, 0x204, (ULONG **)&ADC_TSC);
	

	 Result = misc_register(&Device1);
	  if (Result)
	  {
	    printk("  %s device register failed\n",DEVICE1_NAME);
	  }

	else
	{


		//config adc reg:

		//CLOCK = 3MHz
		ADC_TSC[ADC_CLKDIV] = 7;


		ADC_TSC[CTRL] = (1 << 1) | (1 << 2);


		//set stepconfig1 for button
		ADC_TSC[STEPCONFIG1] = (1 << 5) | (1 << 6) | 1;

		//set stepconfig2 for battery
		ADC_TSC[STEPCONFIG2] = (1 << 26) | (1 << 15) | (1 << 19) | (1 << 5) | (1 << 6) | 1;


		//enable ch1
		ADC_TSC[STEPENABLE] = (1 << 1) | (1 << 2);


		//turn on adc
		ADC_TSC[CTRL] |= 1;


	    if ((kmalloc_ptr = kmalloc((NPAGES + 2) * PAGE_SIZE, GFP_KERNEL)) != NULL)
	    {
		      pTmp = (UWORD*)((((unsigned long)kmalloc_ptr) + PAGE_SIZE - 1) & PAGE_MASK);
		      for (i = 0; i < NPAGES * PAGE_SIZE; i += PAGE_SIZE)
		      {
				SetPageReserved(virt_to_page(((unsigned long)pTmp) + i));
		      }
		      pUi      =  (UI*)pTmp;
		      memset(pUi,0,sizeof(UI));


			fbmem = (unsigned char *)ili9225fb_extern_info->screen_base;



		      // setup ui update timer interrupt
		      Device2Time  =  ktime_set(0,10000000);
		      hrtimer_init(&Device2Timer,CLOCK_MONOTONIC,HRTIMER_MODE_REL);
		      Device2Timer.function  =  Device2TimerInterrupt1;
		      hrtimer_start(&Device2Timer,Device2Time,HRTIMER_MODE_REL);
		      // setup ui update timer interrupt
		       Device1Time  =  ktime_set(0,50000000);
		       hrtimer_init(&Device1Timer,CLOCK_MONOTONIC,HRTIMER_MODE_REL);
		       Device1Timer.function  =  Device1TimerInterrupt1;
		       hrtimer_start(&Device1Timer,Device1Time,HRTIMER_MODE_REL);	

			for(y = 0; y <= 175; y++)
			{
				for(x = 0; x <= 219; x++)
				{
					location = (x + y * 220) * 2;
					*(fbmem + location++) = 0;
					*(fbmem + location) = 0;
				}
			}
			ili9225fb_extern_touch(0, 0, 220, 176);

			i = 0;
			for(y = 140; y <=163; y++)
			{
				for(x = 0; x <= 177; x++)
				{
					location = (x + y * 220) * 2;
					*(fbmem + location++) = gImage_fatcatlab[i++];
					*(fbmem + location) = gImage_fatcatlab[i++];
				}
			}

			i = 0;
			for(y = 122; y <= 175; y++)
			{
				for(x = 179; x <= 219; x++)
				{
					location = (x + y * 220) * 2;
					*(fbmem + location++) = gImage_logo[i++];
					*(fbmem + location) = gImage_logo[i++];
				}
			}
			//ili9225fb_extern_touch(179, 121, 41, 55);
			ili9225fb_extern_touch(0, 0, 220, 176);

			//printk (DEVICE1_NAME" initialized!\n");
	    }
	}
	return Result;
}


static void Device1Exit(void)
{
  //int     Tmp;
  int     i;
  UWORD   *pTmp;

  int x, y, location;


	hrtimer_cancel(&Device1Timer);
  	hrtimer_cancel(&Device2Timer);

	for(y = 0; y <= 175; y++)
	{
		for(x = 0; x <= 219; x++)
		{
			location = (x + y * 220) * 2;
			*(fbmem + location++) = 0;
			*(fbmem + location) = 0;
		}
	}
	ili9225fb_extern_touch(0, 0, 220, 176);


	//turn off adc
	ADC_TSC[CTRL] &= ~1;

	 //free shared memory
	 pTmp = (UWORD *)pUi;
	 pUi = &UiDefault;

	  for (i = 0; i < NPAGES * PAGE_SIZE; i+= PAGE_SIZE)
	  {
	    ClearPageReserved(virt_to_page(((unsigned long)pTmp) + i));
	  }
	  kfree(kmalloc_ptr);


	misc_deregister(&Device1);

	//printk (DEVICE1_NAME" exit!\n");
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

	CM_WKUP[0xBC >> 2] |= 0x2;	//CM_WKUP_ADC_TSC_CLKCTRL
	while(0x2 != (ioread32(&CM_WKUP[0xBC >> 2]) & 0x2))
		;
	

      Device1Init();

  return (0);
}


static void ModuleExit(void)
{

  	Device1Exit();
//	iounmap(CM);
//	iounmap(CM_PER);
	iounmap(CM_WKUP);
	iounmap(ADC_TSC);

}
