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


/*! \page IicModule IIC Module
 *
 *  <hr size="1"/>
 *
 *  Manages communication with intelligent IIC devices on an input port.\n
 *
 *-  \subpage IicDriver
 *-  \subpage IicModuleMemory
 *-  \subpage IicModuleResources
 *
 */


/*! \page IicDriver IIC Device Controller
 *
 *  <hr size="1"/>
 *
 *  Manages the sequence necessary when adding a IIC devices to an input port.\n
 *
 * \n
 *
 *  The IIC Device Controller gets information from the DCM driver \ref DcmDriver "Device Detection Manager Driver"
 *  about the port state and when the DCM driver detects a IIC device on the port the sequence below the following defines
 *  is started inside the IIC Device Controller.
 *
 * \verbatim
*/

/*!
\endverbatim
 *
 *  \n
 */



#ifndef PCASM
#include  <asm/types.h>
#endif

#define   HW_ID_SUPPORT

#include  "../../lms2012/source/lms2012.h"
#include  "../../lms2012/source/am335x.h"




#define   MODULE_NAME                   "iic_module"
#define   DEVICE1_NAME                  IIC_DEVICE

static    int  ModuleInit(void);
static    void ModuleExit(void);

#define   __USE_POSIX

#include  <linux/kernel.h>
#include  <linux/fs.h>

#include  <linux/sched.h>


#include  <linux/circ_buf.h>
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

/*! \page IicModuleResources Gpios and Resources used for Module
 *
 *  Describes use of gpio and resources\n
 *
 *  \verbatim
 */


static long Device1Ioctl(struct file *File, unsigned int Request, unsigned long Pointer)
{

  return (0);
}


static ssize_t Device1Write(struct file *File,const char *Buffer,size_t Count,loff_t *Data)
{
  int     Lng     = 0;

  Lng           =  Count;

  return (Lng);
}


static ssize_t Device1Read(struct file *File,char *Buffer,size_t Count,loff_t *Offset)
{
  int     Lng     = 0;

  return (Lng);
}

static int Device1Mmap(struct file *filp, struct vm_area_struct *vma)
{
   int ret = 0;


   return (ret);
}


static    const struct file_operations Device1Entries =
{
  .owner        = THIS_MODULE,
  .read         = Device1Read,
  .write        = Device1Write,
  .mmap         = Device1Mmap,
  .unlocked_ioctl        = Device1Ioctl,
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

#ifdef DEBUG
  printk("%s init started\n",MODULE_NAME);
#endif


      Device1Init();

  return (0);
}


static void ModuleExit(void)
{
#ifdef DEBUG
  printk("%s exit started\n",MODULE_NAME);
#endif

  Device1Exit();
}

