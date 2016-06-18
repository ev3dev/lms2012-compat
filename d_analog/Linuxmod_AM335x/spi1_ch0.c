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





#include <linux/delay.h>





MODULE_LICENSE("GPL");
MODULE_AUTHOR("LIU");
MODULE_DESCRIPTION(MODULE_NAME);
MODULE_SUPPORTED_DEVICE(DEVICE1_NAME);

module_init(ModuleInit);
module_exit(ModuleExit);








static volatile ULONG *CM_PER;
static volatile ULONG *CM;
static volatile ULONG *MCSPI1;

/*
static ULONG *GPIOBANK0;
static ULONG *GPIOBANK1;
static ULONG *GPIOBANK2;
static ULONG *GPIOBANK3;
*/






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




static    struct hrtimer Device1Timer;
static    ktime_t        Device1Time;
























#define SYSCONFIG	(0x110>>2)
#define SYST		(0x124>>2)

#define IRQSTATUS	(0x118>>2)
#define IRQENABLE	(0x11C>>2)
#define MODULCTRL	(0x128>>2)
#define	CH0CONF		(0x12C>>2)
#define	CH0STAT		(0x130>>2)
#define CH0CTRL		(0x134>>2)









#define   SPIRxempty                    ((MCSPI1[CH0STAT] & 0x1))
#define   SPITxfull                    ((MCSPI1[CH0STAT] & 0x2))



#define	  TX0		(0x138>>2)
#define	  RX0		(0x13C>>2)





UWORD     SpiUpdate(UWORD DataOut)
{


//  while (!SPITxfull)
//	;


  MCSPI1[CH0CTRL] = 1;
  MCSPI1[TX0]   = (ULONG)DataOut;

/*
  while (!SPIRxempty)
  	;

  return ((UWORD)(MCSPI1[RX1] & 0x0000FFFF));
*/
  return 0;

}






static UWORD    rx_data = 0;
static UWORD	tx_data = 0;

static enum hrtimer_restart Device1TimerInterrupt1(struct hrtimer *pTimer)
{
  	// restart timer
  	hrtimer_forward_now(pTimer,Device1Time);
	
	MCSPI1[CH0CONF] |= 1<<20;
	MCSPI1[CH0CTRL] |= 1;
	rx_data = MCSPI1[CH0STAT];
//	printk("Reg CH0CTRL = %x\n", MCSPI1[CH0CTRL]);
//	printk("Reg irqstat = %x\n", MCSPI1[IRQSTATUS]);
//	printk("Reg  irqena = %x\n", MCSPI1[IRQENABLE]);
	
//	printk("\nReg CH0STAT = %x\n", rx_data); 

//	udelay(1000);
	while(!(rx_data & 0x2))
		rx_data = MCSPI1[CH0STAT];
	MCSPI1[TX0] = (ULONG)tx_data++;
	if(tx_data > 0xFFF0)
		tx_data = 0;

	MCSPI1[CH0CONF] &= ~(1<<20);
//	MCSPI1[CH0CTRL] = 0;

	return (HRTIMER_RESTART);
}




static void Device1Init(void)
{
	int Result = 0;
	
	Result = misc_register(&Device1); 

	GetPeriphealBasePtr(0x44E00000, 0x154, (ULONG **)&CM_PER);
	GetPeriphealBasePtr(0x481A0000, 0x1A4, (ULONG **)&MCSPI1);

	CM_PER[0x50 >> 2] |= 0x2;	//CM_PER_SPI1_CLKCTRL
	while(0x2 != (ioread32(&CM_PER[0x50 >> 2]) & 0x2))
		;

	printk("%s init done\n", DEVICE1_NAME);


      	Device1Time  =  ktime_set(0, 1000000 * 100);
      	hrtimer_init(&Device1Timer,CLOCK_MONOTONIC,HRTIMER_MODE_REL);
      	Device1Timer.function  =  Device1TimerInterrupt1;
      	hrtimer_start(&Device1Timer,Device1Time,HRTIMER_MODE_REL);

}




















static void Device1Exit(void)
{
	hrtimer_cancel(&Device1Timer);
	misc_deregister(&Device1);
	printk("%s exit\n", DEVICE1_NAME);
}





static int ModuleInit(void)
{

	GetPeriphealBasePtr(0x44E10000, 0x1448, (ULONG **)&CM);
/*	GetPeriphealBasePtr(0x44E07000, 0x198, (ULONG **)&GPIOBANK0);	
	GetPeriphealBasePtr(0x4804C000, 0x198, (ULONG **)&GPIOBANK1);	
	GetPeriphealBasePtr(0x481AC000, 0x198, (ULONG **)&GPIOBANK2);	
	GetPeriphealBasePtr(0x481AE000, 0x198, (ULONG **)&GPIOBANK3);	
*/	

	Device1Init();
	
	return (0);
}


static void ModuleExit(void)
{

//	Device2Exit();
	Device1Exit();
	
	iounmap(MCSPI1);
	iounmap(CM_PER);
	iounmap(CM);

}
