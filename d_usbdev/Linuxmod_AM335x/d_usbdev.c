/*
 * zero.c -- Gadget Zero, for USB development
 *
 * Copyright (C) 2003-2008 David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * Gadget Zero only needs two bulk endpoints, and is an example of how you
 * can write a hardware-agnostic gadget driver running inside a USB device.
 * Some hardware details are visible, but don't affect most of the driver.
 *
 * Use it with the Linux host/master side "usbtest" driver to get a basic
 * functional test of your device-side usb stack, or with "usb-skeleton".
 *
 * It supports two similar configurations.  One sinks whatever the usb host
 * writes, and in return sources zeroes.  The other loops whatever the host
 * writes back, so the host can read it.
 *
 * Many drivers will only have one configuration, letting them be much
 * simpler if they also don't support high speed operation (like this
 * driver does).
 *
 * Why is *this* driver using two configurations, rather than setting up
 * two interfaces with different functions?  To help verify that multiple
 * configuration infrastucture is working correctly; also, so that it can
 * work with low capability USB controllers without four bulk endpoints.
 */

/*
 * driver assumes self-powered hardware, and
 * has no way for users to trigger remote wakeup.
 */

/* #define VERBOSE_DEBUG */

//added by liu start
#include  <asm/types.h>

#include  "../../lms2012/source/lms2012.h"
#include  "../../lms2012/source/am335x.h"


#define   MODULE_NAME                   "usbdev_module"
#define   DEVICE1_NAME                  USBDEV_DEVICE

static    int  ModuleInit(void);
static    void ModuleExit(void);

#define   __USE_POSIX
//added by liu end



#include <linux/kernel.h>

//added by liu, start
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
#include  <linux/miscdevice.h>
#include  <asm/uaccess.h>
#include  <linux/hid.h>
#include  <linux/utsname.h>
//added by liu, end

#include <linux/slab.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/err.h>






//changed by liu, start
//#include <linux/usb/computil.c>
//#include "g_zero.h"
#include "computil.c"
#include <../drivers/usb/gadget/gadget_chips.h>
#include <../drivers/usb/gadget/usbstring.c>
#include <../drivers/usb/gadget/config.c>
#include <../drivers/usb/gadget/epautoconf.c>
#include <../drivers/usb/gadget/functions.c>
//#include <../drivers/usb/gadget/configfs.c>
#include <../drivers/usb/gadget/g_zero.h>

//changed by liu, end


//added by liu, start
#define MAX_EP_SIZE 1024
#define MAX_FULLSPEED_EP_SIZE 64
unsigned buflen = MAX_EP_SIZE ;
char usb_char_buffer_in[MAX_EP_SIZE];
char usb_full_buffer_in[MAX_FULLSPEED_EP_SIZE];
int usb_char_in_length = 0;
char usb_char_buffer_out[MAX_EP_SIZE];
char usb_full_buffer_out[MAX_FULLSPEED_EP_SIZE];
int usb_char_out_length = 0;

#define     SHM_LENGTH    (sizeof(UsbSpeedDefault))
#define     NPAGES        ((SHM_LENGTH + PAGE_SIZE - 1) / PAGE_SIZE)
static void *kmalloc_ptr;

#include  "usb_function.c"    // Specific USB functionality

MODULE_LICENSE("GPL");
MODULE_AUTHOR("The LEGO Group");
MODULE_DESCRIPTION(MODULE_NAME);
MODULE_SUPPORTED_DEVICE(DEVICE1_NAME);

module_init(ModuleInit);
module_exit(ModuleExit);

//added by liu, end


/*-------------------------------------------------------------------------*/
USB_GADGET_COMPOSITE_OPTIONS();

#define DRIVER_VERSION		"31jan2011->"



/*
 * Normally the "loopback" configuration is second (index 1) so
 * it's not the default.  Here's where to change that order, to
 * work better with hosts where config changes are problematic or
 * controllers (like original superh) that only support one config.
 */
static bool loopdefault = 0;
module_param(loopdefault, bool, S_IRUGO|S_IWUSR);

/*
static struct usb_zero_options gzero_options = {
	.isoc_interval = 4,
	.isoc_maxpacket = 1024,
	.bulk_buflen = 4096,
	.qlen = 32,
};
*/


/*-------------------------------------------------------------------------*/

/* Thanks to NetChip Technologies for donating this product ID.
 *
 * DO NOT REUSE THESE IDs with a protocol-incompatible driver!!  Ever!!
 * Instead:  allocate your own, using normal USB-IF procedures.
 */

#define DRIVER_VENDOR_NUM     0x0694      // LEGO Group
#define DRIVER_PRODUCT_NUM    0x0005      // No. 5 in a row
#define DEFAULT_AUTORESUME    0


/* If the optional "autoresume" mode is enabled, it provides good
 * functional coverage for the "USBCV" test harness from USB-IF.
 * It's always set if OTG mode is enabled.
 */
unsigned autoresume = DEFAULT_AUTORESUME;
module_param(autoresume, uint, S_IRUGO);
MODULE_PARM_DESC(autoresume, "zero, or seconds before remote wakeup");

/* Maximum Autoresume time */
unsigned max_autoresume;
module_param(max_autoresume, uint, S_IRUGO);
MODULE_PARM_DESC(max_autoresume, "maximum seconds before remote wakeup");

/* Interval between two remote wakeups */
unsigned autoresume_interval_ms;
module_param(autoresume_interval_ms, uint, S_IRUGO);
MODULE_PARM_DESC(autoresume_interval_ms,
		"milliseconds to increase successive wakeup delays");

static unsigned autoresume_step_ms;
/*-------------------------------------------------------------------------*/

static struct usb_device_descriptor device_desc = {
	.bLength =		sizeof device_desc,
	.bDescriptorType =	USB_DT_DEVICE,

	.bcdUSB =		cpu_to_le16(0x0200),
	.bDeviceClass =		0,
	.bDeviceSubClass =  	0,
	.bDeviceProtocol =  	0,

	.idVendor =		cpu_to_le16(DRIVER_VENDOR_NUM),
	.idProduct =		cpu_to_le16(DRIVER_PRODUCT_NUM),
	.bNumConfigurations =	1,
};

#ifdef CONFIG_USB_OTG
static struct usb_otg_descriptor otg_descriptor = {
	.bLength =		sizeof otg_descriptor,
	.bDescriptorType =	USB_DT_OTG,

	/* REVISIT SRP-only hardware is possible, although
	 * it would not be called "OTG" ...
	 */
	.bmAttributes =		USB_OTG_SRP | USB_OTG_HNP,
};

static const struct usb_descriptor_header *otg_desc[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	NULL,
};
#else
#define otg_desc	NULL
#endif

/* string IDs are assigned dynamically */
/* default serial number takes at least two packets */
static char manufacturer[] = "LEGO Group";
static char serial[] = "123456789ABC ";
static const char longname[] = "EV3 brick    ";

#define USB_GZERO_SS_DESC	(USB_GADGET_FIRST_AVAIL_IDX + 0)


static struct usb_string strings_dev[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = manufacturer,
	[USB_GADGET_PRODUCT_IDX].s = longname,
	[USB_GADGET_SERIAL_IDX].s = serial,
	[USB_GZERO_SS_DESC].s	= "source and sink data",
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

/*-------------------------------------------------------------------------*/

static struct timer_list	autoresume_timer;

static void zero_autoresume(unsigned long _c)
{
	struct usb_composite_dev	*cdev = (void *)_c;
	struct usb_gadget		*g = cdev->gadget;

	/* unconfigured devices can't issue wakeups */
	if (!cdev->config)
		return;

	/* Normally the host would be woken up for something
	 * more significant than just a timer firing; likely
	 * because of some direct user request.
	 */
	if (g->speed != USB_SPEED_UNKNOWN) {
		int status = usb_gadget_wakeup(g);
		INFO(cdev, "%s --> %d\n", __func__, status);
	}
}

static void zero_suspend(struct usb_composite_dev *cdev)
{
	if (cdev->gadget->speed == USB_SPEED_UNKNOWN)
		return;

	if (autoresume) {
		if (max_autoresume &&
			(autoresume_step_ms > max_autoresume * 1000))
				autoresume_step_ms = autoresume * 1000;

		mod_timer(&autoresume_timer, jiffies +
			msecs_to_jiffies(autoresume_step_ms));
		DBG(cdev, "suspend, wakeup in %d milliseconds\n",
			autoresume_step_ms);

		autoresume_step_ms += autoresume_interval_ms;
	} else
		DBG(cdev, "%s\n", __func__);
}

static void zero_resume(struct usb_composite_dev *cdev)
{
	DBG(cdev, "%s\n", __func__);
	del_timer(&autoresume_timer);
}

/*-------------------------------------------------------------------------*/



static struct usb_function *func_ss;
static struct usb_function_instance *func_inst_ss;

static int ss_config_setup(struct usb_configuration *c,
		const struct usb_ctrlrequest *ctrl)
{
	switch (ctrl->bRequest) {
	case 0x5b:
	case 0x5c:
		return func_ss->setup(func_ss, ctrl);
	default:
		return -EOPNOTSUPP;
	}
}

static struct usb_configuration sourcesink_driver = {
	.label                  = "rudolf driver",
	.setup                  = ss_config_setup,
	.bConfigurationValue    = 1,
	.bmAttributes           = USB_CONFIG_ATT_SELFPOWER,
	/* .iConfiguration      = DYNAMIC */
};



static int zero_bind(struct usb_composite_dev *cdev)
{
	struct f_ss_opts	*ss_opts;
	int			status;

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */
	// printk("liu, zero.c---zero_bind---00\n");
	status = usb_string_ids_tab(cdev, strings_dev);
	if (status < 0)
		return status;

	// printk("liu, zero.c---zero_bind---11---usb_string_ids_tab OK!\n");
	device_desc.iManufacturer = strings_dev[USB_GADGET_MANUFACTURER_IDX].id;
	device_desc.iProduct = strings_dev[USB_GADGET_PRODUCT_IDX].id;
	device_desc.iSerialNumber = strings_dev[USB_GADGET_SERIAL_IDX].id;

	setup_timer(&autoresume_timer, zero_autoresume, (unsigned long) cdev);



	status = usb_function_register(&SourceSinkusb_func);
	if (status)
		return status;

	// printk("liu, zero.c---zero_bind---22---usb_function_register OK!\n");


	func_inst_ss = usb_get_function_instance("SourceSink");
	if (IS_ERR(func_inst_ss))
		return PTR_ERR(func_inst_ss);

	 //printk("liu, zero.c---zero_bind---33---get_function_instance OK!\n");

	ss_opts =  container_of(func_inst_ss, struct f_ss_opts, func_inst);

	func_ss = usb_get_function(func_inst_ss);
	if (IS_ERR(func_ss)) {
		status = PTR_ERR(func_ss);
		goto err_put_func_inst_ss;
	}

	 //printk("liu, zero.c---zero_bind---44---get_function OK!\n");

	sourcesink_driver.iConfiguration = strings_dev[USB_GZERO_SS_DESC].id;

	/* support autoresume for remote wakeup testing */
	sourcesink_driver.bmAttributes &= ~USB_CONFIG_ATT_WAKEUP;
	sourcesink_driver.descriptors = NULL;
	if (autoresume) {
		sourcesink_driver.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
		autoresume_step_ms = autoresume * 1000;
	}

	/* support OTG systems */
	if (gadget_is_otg(cdev->gadget)) {
		sourcesink_driver.descriptors = otg_desc;
		sourcesink_driver.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}

	/* Register primary, then secondary configuration.  Note that
	 * SH3 only allows one config...
	 */
	usb_add_config_only(cdev, &sourcesink_driver);

	status = usb_add_function(&sourcesink_driver, func_ss);
	if (status)
		goto err_put_func_ss;
	 //printk("liu, zero.c---zero_bind---55---usb_add_function OK!\n");

	usb_ep_autoconfig_reset(cdev->gadget);

	usb_composite_overwrite_options(cdev, &coverwrite);

	//INFO(cdev, "%s, version: " DRIVER_VERSION "\n", longname);

	return 0;

err_put_func_ss:
	usb_put_function(func_ss);
	func_ss = NULL;
err_put_func_inst_ss:
	usb_put_function_instance(func_inst_ss);
	func_inst_ss = NULL;
	return status;
}

static int zero_unbind(struct usb_composite_dev *cdev)
{
	del_timer_sync(&autoresume_timer);
	if (!IS_ERR_OR_NULL(func_ss))
		usb_put_function(func_ss);
	usb_put_function_instance(func_inst_ss);
	return 0;
}

static  struct usb_composite_driver zero_driver = {
	.name		= "zero",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.max_speed	= USB_SPEED_HIGH,
	.bind		= zero_bind,
	.unbind		= zero_unbind,
	.suspend	= zero_suspend,
	.resume		= zero_resume,
};

static int dUsbInit(void)
{
  //#define DEBUG
  #undef DEBUG
  #ifdef DEBUG
    printk("dUsbInit\n\r");
  #endif

  UsbSpeed.Speed = FULL_SPEED;          // default to FULL_SPEED if not connected to a HIGH-SPEED
  (*pUsbSpeed).Speed = FULL_SPEED;      // HOST. If not connected to HIGH-SPEED we assume we're
                                        // wanting (or at least doing) Daisy Chain
  return usb_composite_probe(&zero_driver);
}

static void dUsbExit(void)
{
  usb_composite_unregister(&zero_driver);
}

// DEVICE1 char device stuff ********************************************************************

static ssize_t Device1Write(struct file *File,const char *Buffer,size_t Count,loff_t *Data)
{
  // Write data for the HOST to poll - Stuff sent to the HOST

  int BytesWritten = 0;

  #undef DEBUG
  //#define DEBUG
  #ifdef DEBUG
    printk("Device1Write - usb_char_in_length = %d\n", usb_char_in_length);
  #endif

  if (usb_char_in_length == 0)  // ready for more
  {                             // else wait in USER layer
      BytesWritten = Count;
      copy_from_user(usb_char_buffer_in, Buffer, BytesWritten);
      usb_char_in_length = BytesWritten;

	  //#define DEBUG
    #undef DEBUG
    #ifdef DEBUG
      	  printk("WR = %d, %d -- ", usb_char_buffer_in[2], usb_char_buffer_in[3]);
	  #endif

      if(USB_DATA_PENDING == input_state)
      {
        // Already we've a failed tx (HOST part starwing??

        input_state = USB_DATA_READY;
        #undef DEBUG
        //#define DEBUG
        #ifdef DEBUG
          printk("DATA_PENDING SECOND time and reset!! in Device1Write\n\r");
        #endif
      }

      if(USB_DATA_READY == input_state)
      {
        #undef DEBUG
        //#define DEBUG
        #ifdef DEBUG
          printk("USB_DATA_READY in Device1Write\n\r");
        #endif

        input_state = USB_DATA_BUSY;
        write_data_to_the_host(save_in_ep, save_in_req);
        usb_req_arm(save_in_ep, save_in_req); // new request
      }
      else
      {
        input_state = USB_DATA_PENDING;

        #undef DEBUG
        //#define DEBUG
        #ifdef DEBUG
          printk("DATA_PENDING in Device1Write\n\r");
        #endif
      }
  }

  //#define DEBUG
  #undef DEBUG
  #ifdef DEBUG
    printk("usbdev %d written\n\r", BytesWritten);
  #endif
 
  return (BytesWritten); // Zero means USB was not ready yet
}

static ssize_t Device1Read(struct file *File,char *Buffer,size_t Count,loff_t *Offset)
{
  // Read the bits'n'bytes from the HOST
  int     BytesRead     = 0;

  if (usb_char_out_length > 0)     // Something to look at
  {
    #undef DEBUG
    //#define DEBUG
	  #ifdef DEBUG
	    printk("Some bytes to READ?\n\r");
    #endif

	  copy_to_user(Buffer, usb_char_buffer_out, Count);
    BytesRead = usb_char_out_length;
    usb_char_out_length = 0;
  }
  return (BytesRead);
}

static int Device1Mmap(struct file *filp, struct vm_area_struct *vma)
{
   int ret;

   ret = remap_pfn_range(vma,vma->vm_start,virt_to_phys((void*)((unsigned long)pUsbSpeed)) >> PAGE_SHIFT,vma->vm_end-vma->vm_start,PAGE_SHARED);

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
  UWORD   *pTemp;
  int i;

  Result  =  misc_register(&Device1);
  if (Result)
  {
    //#define DEBUG
    #undef DEBUG
    #ifdef DEBUG
      printk("  %s device register failed\n",DEVICE1_NAME);
    #endif
  }
  else
  {
    //#define DEBUG
    #undef DEBUG
    #ifdef DEBUG
      printk("  %s device register OK\n",DEVICE1_NAME);
    #endif

    // allocate kernel shared memory for DaisyChain Speed info

    if ((kmalloc_ptr = kmalloc((NPAGES + 2) * PAGE_SIZE, GFP_KERNEL)) != NULL)
    {

      pTemp = (UWORD*)((((unsigned long)kmalloc_ptr) + PAGE_SIZE - 1) & PAGE_MASK);

      for (i = 0; i < NPAGES * PAGE_SIZE; i += PAGE_SIZE)
      {
        SetPageReserved(virt_to_page(((unsigned long)pTemp) + i));
      }

      pUsbSpeed = (USB_SPEED*)pTemp;
    }

    dUsbInit();
  }

  return (Result);
}

static void Device1Exit(void)
{
  int i;
  UWORD *pTemp = (UWORD*)pUsbSpeed;

  dUsbExit();

  pUsbSpeed = &UsbSpeedDefault;

  for (i = 0; i < NPAGES * PAGE_SIZE; i+= PAGE_SIZE)
  {
    ClearPageReserved(virt_to_page(((unsigned long)pTemp) + i));

    //#define DEBUG
    #undef DEBUG
    #ifdef DEBUG
      printk("  %s memory page %d unmapped\n",DEVICE1_NAME,i);
    #endif
  }

  kfree(kmalloc_ptr);

  misc_deregister(&Device1);

  //#define DEBUG
  #undef DEBUG
  #ifdef DEBUG
    printk("  %s device unregistered\n",DEVICE1_NAME);
  #endif
}


// MODULE *********************************************************************

char *HostStr;    // Used for HostName - or NOT used at all
char *SerialStr;  // Used for Serial number (I.e. BT number)

module_param (HostStr, charp, 0);
module_param (SerialStr, charp, 0);

static int ModuleInit(void)
{

  //#define DEBUG
  #undef DEBUG
  #ifdef DEBUG
    printk("%s Module init started\r\n",MODULE_NAME);
  #endif

  //#define DEBUG
  #undef DEBUG
  #ifdef DEBUG
    printk("This is DEFAULT NAME: %s\n\r", longname);
  #endif

  //#define DEBUG
  #undef DEBUG
  #ifdef DEBUG
    printk("\n\rThis is the HostStr: %s\n\r", HostStr);
  #endif

  strcpy(longname, HostStr);

  //#define DEBUG
  #undef DEBUG
  #ifdef DEBUG
    printk("\n\rThis is the INSMODed NAME: %s\n\r", longname);
  #endif

  //#define DEBUG
  #undef DEBUG
  #ifdef DEBUG
    printk("\n\rThis is the DEFAULT SerialNumber: %s\n\r", serial);
  #endif

  //#define DEBUG
  #undef DEBUG
  #ifdef DEBUG
    printk("\n\rThis is the SerialStr: %s\n\r", SerialStr);
  #endif

  strcpy(serial, SerialStr);

  //#define DEBUG
  #undef DEBUG
  #ifdef DEBUG
    printk("\n\rThis is the INSMODed SerialNumber (BT mac): %s\n\r", serial);
  #endif

  Device1Init();

  return (0);
}

static void ModuleExit(void)
{
  //#define DEBUG
  #undef DEBUG
  #ifdef DEBUG
    printk("%s exit started\n",MODULE_NAME);
  #endif

  Device1Exit();

}

