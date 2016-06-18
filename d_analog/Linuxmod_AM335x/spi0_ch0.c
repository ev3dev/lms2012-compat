#ifndef PCASM
#include  <asm/types.h>
#include  <linux/time.h>
#endif

#define   HW_ID_SUPPORT

#include  "lms2012.h"
#include  "am335x.h"





#define   MODULE_NAME 			"analog_module"
#define   DEVICE1_NAME                  ANALOG_DEVICE

static    int  ModuleInit(void);
static    void ModuleExit(void);

#include  <linux/kernel.h>
#include  <linux/fs.h>

#include  <linux/sched.h>

#include  <linux/mm.h>
#include  <linux/hrtimer.h>
#include  <linux/interrupt.h>

#include  <linux/init.h>
#include  <linux/uaccess.h>
#include  <linux/debugfs.h>

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
MODULE_AUTHOR("LIU");
MODULE_DESCRIPTION(MODULE_NAME);
MODULE_SUPPORTED_DEVICE(DEVICE1_NAME);

module_init(ModuleInit);
module_exit(ModuleExit);








static volatile ULONG *CM_PER;
static volatile ULONG *CM;

static volatile ULONG *MCSPI0;
static volatile ULONG *GPIOBANK0;
static volatile ULONG *GPIOBANK1;
static volatile ULONG *GPIOBANK2;
static volatile ULONG *GPIOBANK3;









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




void SetGpio(void)
{
	int Tmp = 0;

	int i, Pin;

	for (i = 2; i <= 5; i++)
	{
		Pin = i;
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
}





static    const struct file_operations Device1Entries =
{
  .owner        = THIS_MODULE,
};




static    struct miscdevice Device1 =
{
  MISC_DYNAMIC_MINOR,
  DEVICE1_NAME,
  &Device1Entries
};

























#define SYSCONFIG	(0x110>>2)
#define SYSSTATUS	(0x114>>2)
#define SYST		(0x124>>2)

#define IRQSTATUS	(0x118>>2)
#define IRQENABLE	(0x11C>>2)
#define MODULCTRL	(0x128>>2)
#define	CH0CONF		(0x12C>>2)
#define	CH0STAT		(0x130>>2)
#define CH0CTRL		(0x134>>2)

#define XFERLEVEL	(0x17C>>2)




#define	  TX0		(0x138>>2)
#define	  RX0		(0x13C>>2)









static    struct hrtimer Device1Timer;
static    ktime_t        Device1Time;

static 	  UWORD rx_data = 0;
static 	  UWORD tx_data = 0;

static enum hrtimer_restart Device1TimerInterrupt1(struct hrtimer *pTimer)
{

  	// restart timer
  	hrtimer_forward_now(pTimer,Device1Time);

	MCSPI0[CH0CONF] |= (0x1 << 20);
	MCSPI0[IRQENABLE] |= (1 << 2) | 1; 
	MCSPI0[CH0CTRL] |= 0x1; 




	while((MCSPI0[CH0STAT] & 0x2) == 0);

	MCSPI0[TX0] = (ULONG)(tx_data);

//	printk("ch0stat = %x\n", MCSPI0[CH0STAT]);
//	printk("irqstat = %x\n", MCSPI0[IRQSTATUS]);





	while((MCSPI0[CH0STAT] & 0x1) == 0);
//	printk("ch0stat = %x\n", MCSPI0[CH0STAT]);
//	printk("irqstat = %x\n", MCSPI0[IRQSTATUS]);
	rx_data = (UWORD)(MCSPI0[RX0] & 0x0000FFFF);  



//	printk("ch0stat = %x\n", MCSPI0[CH0STAT]);
//	printk("irqstat = %x\n", MCSPI0[IRQSTATUS]);


	MCSPI0[CH0CONF] &= ~(0x1 << 20);
	MCSPI0[CH0CTRL] &= ~0x1;

	MCSPI0[SYST] &= ~(1 << 11);
	MCSPI0[IRQSTATUS] |= 1 | (1 << 2);
	MCSPI0[IRQENABLE] &= ~((1 << 2) | 1); 

	printk("tx_data = %x, rx_data = %x\n\n\n\n",(ULONG)tx_data, (ULONG)rx_data);

	if(tx_data++ > 0xFFF0)
		tx_data = 0;

  	return (HRTIMER_RESTART);
}





static void Device1Init(void)
{
	int Result = 0;
	unsigned int i;
	
	Result = misc_register(&Device1); 
	

	
	MCSPI0[MODULCTRL] = 1;			//Channel

//	MCSPI0[CH0CONF] = 0x107D9 | (1 << 25) | (1 <<26);
	MCSPI0[CH0CONF] = (ULONG)(0x107D9 & 0xFFFFFFFE);

	printk("con0conf = %x\n", MCSPI0[CH0CONF]);


	
  	Device1Time  =  ktime_set(0, 1000000 * 100);
      	hrtimer_init(&Device1Timer,CLOCK_MONOTONIC,HRTIMER_MODE_REL);
      	Device1Timer.function  =  Device1TimerInterrupt1;
      	hrtimer_start(&Device1Timer,Device1Time,HRTIMER_MODE_REL);

}







































static int ModuleInit(void)
{
	GetPeriphealBasePtr(0x44E10000, 0x1448, (ULONG **)&CM);
/*	GetPeriphealBasePtr(0x44E07000, 0x198, (ULONG **)&GPIOBANK0);	
	GetPeriphealBasePtr(0x4804C000, 0x198, (ULONG **)&GPIOBANK1);	
	GetPeriphealBasePtr(0x481AC000, 0x198, (ULONG **)&GPIOBANK2);	
	GetPeriphealBasePtr(0x481AE000, 0x198, (ULONG **)&GPIOBANK3);	
	
*/


	SetGpio();



	GetPeriphealBasePtr(0x44E00000, 0x154, (ULONG **)&CM_PER);
	GetPeriphealBasePtr(0x48030000, 0x1A4, (ULONG **)&MCSPI0);
	CM_PER[0x4C >> 2] |= 0x2;	//CM_PER_SPI0_CLKCTRL
	while(0x2 != (ioread32(&CM_PER[0x4C >> 2]) & 0x2))
		;



	Device1Init();
	
	printk("\nModuleInit OVER!\n");
	return (0);
}










static void Device1Exit(void)
{

	
  	hrtimer_cancel(&Device1Timer);


	  misc_deregister(&Device1);

	  printk("%s exit\n", DEVICE1_NAME);
}





static void ModuleExit(void)
{

	Device1Exit();

	
//	iounmap(GPIOBANK0);
//	iounmap(GPIOBANK1);
//	iounmap(GPIOBANK2);
//	iounmap(GPIOBANK3);
	iounmap(MCSPI0);
	iounmap(CM_PER);
	iounmap(CM);

}
