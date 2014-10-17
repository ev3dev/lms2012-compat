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


#ifndef PCASM
#include  <asm/types.h>
#endif

#define   HW_ID_SUPPORT

#include  "../../lms2012/source/lms2012.h"
#include  "../../lms2012/source/am335x.h"

int       Hw  =  6;

#define   MODULE_NAME                   "bluetooth_module"
#define   DEVICE1_NAME                  BT_DEVICE
#define   DEVICE2_NAME                  UPDATE_DEVICE

static    int  ModuleInit(void);
static    void ModuleExit(void);

#define   __USE_POSIX

#include  <linux/kernel.h>
#include  <linux/fs.h>

#include  <linux/sched.h>


#ifndef   PCASM
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

#else
// Keep Eclipse happy
#endif






// DEVICE1 ********************************************************************


static ssize_t Device1Write(struct file *File,const char *Buffer,size_t Count,loff_t *Data)
{
  SBYTE   Buf[20];
  int     Lng;

  Lng  =  Count;
  copy_from_user(Buf,Buffer,Count);

  return (Lng);
}


static ssize_t Device1Read(struct file *File,char *Buffer,size_t Count,loff_t *Offset)
{

  return(1);
}


static ssize_t Device2Write(struct file *File,const char *Buffer,size_t Count,loff_t *Data)
{

  return(1);
}


static ssize_t Device2Read(struct file *File,char *Buffer,size_t Count,loff_t *Offset)
{

  return(1);
}


static    const struct file_operations Device1Entries =
{
  .owner   =  THIS_MODULE,
  .read    =  Device1Read,
  .write   =  Device1Write
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


/* Device 2 is to handle pirmware update status writing into */
/* internal RAM                                              */
static    const struct file_operations Device2Entries =
{
  .owner   =  THIS_MODULE,
  .read    =  Device2Read,
  .write   =  Device2Write
};


static    struct miscdevice Device2 =
{
  MISC_DYNAMIC_MINOR,
  DEVICE2_NAME,
  &Device2Entries
};


static int Device2Init(void)
{
  int     Result = -1;

  Result  =  misc_register(&Device2);
  if (Result)
  {
    printk("  %s device register failed\n",DEVICE2_NAME);
  }
  else
  {
    #ifdef DEBUG
      printk("  %s device register succes\n",DEVICE2_NAME);
    #endif
  }

  return (Result);
}


static void Device2Exit(void)
{
  misc_deregister(&Device2);
#ifdef DEBUG
  printk("  %s device unregistered\n",DEVICE2_NAME);
#endif
}


// MODULE *********************************************************************


#ifndef PCASM
module_param (HwId, charp, 0);
#endif

static int ModuleInit(void)
{
  Hw  =  HWID;

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
      Device2Init();

  return (0);
}


static void ModuleExit(void)
{
  #ifdef DEBUG
    printk("%s exit started\n",MODULE_NAME);
  #endif


  Device1Exit();
  Device2Exit();

}
