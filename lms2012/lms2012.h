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
 *
 * As a special exception, if other files instantiate templates or use macros or
 * inline functions from this file, or you  compile this file and link it with
 * other works to produce a work based on this file, this file does not by itself
 * cause the resulting work to be covered by the GNU General Public License.
 * However the source code for this file must still be made available in accordance
 * with section (3) of the GNU General Public License.
 *
 */

#ifndef   LMS2012_H_
#define   LMS2012_H_

#include <libudev.h>

#include  "lmstypes.h"
#include  "bytecodes.h"

// Hardware

#define   OUTPUTS                       vmOUTPUTS                     //!< Number of output ports in the system
#define   INPUTS                        vmINPUTS                      //!< Number of input  ports in the system
#define   BUTTONS                       vmBUTTONS                     //!< Number of buttons in the system
#define   LEDS                          vmLEDS                        //!< Number of LEDs in the system

#define   LCD_WIDTH                     vmLCD_WIDTH                   //!< LCD horizontal pixels
#define   LCD_HEIGHT                    vmLCD_HEIGHT                  //!< LCD vertical pixels
#define   TOPLINE_HEIGHT                vmTOPLINE_HEIGHT              //!< Top line vertical pixels
#define   LCD_STORE_LEVELS              vmLCD_STORE_LEVELS            //!< Store levels

// Software

#define   FG_COLOR                      vmFG_COLOR                    //!<  Foreground color
#define   BG_COLOR                      vmBG_COLOR                    //!<  Background color

#define   CHAIN_DEPT                    vmCHAIN_DEPT                  //!< Number of bricks in the USB daisy chain (master + slaves)

typedef enum {
    EVENT_NONE          = vmEVENT_NONE,
    EVENT_BT_PIN        = vmEVENT_BT_PIN,
    EVENT_BT_REQ_CONF   = vmEVENT_BT_REQ_CONF,
} COM_EVENT;

// Folders

#define   MEMORY_FOLDER                 vmMEMORY_FOLDER
#define   PROGRAM_FOLDER                vmPROGRAM_FOLDER
#define   DATALOG_FOLDER                vmDATALOG_FOLDER
#define   SDCARD_FOLDER                 vmSDCARD_FOLDER
#define   USBSTICK_FOLDER               vmUSBSTICK_FOLDER

// Files
#define   DETAILS_FILE                  vmDETAILS_FILE                //!< File containing firmware version

// Extensions

#define   EXT_SOUND                     vmEXT_SOUND                   //!< Rudolf sound file
#define   EXT_GRAPHICS                  vmEXT_GRAPHICS                //!< Rudolf graphics file
#define   EXT_BYTECODE                  vmEXT_BYTECODE                //!< Rudolf byte code file
#define   EXT_TEXT                      vmEXT_TEXT                    //!< Rudolf text file
#define   EXT_DATALOG                   vmEXT_DATALOG                 //!< Rudolf datalog file
#define   EXT_PROGRAM                   vmEXT_PROGRAM                 //!< Rudolf program byte code file
#define   EXT_CONFIG                    vmEXT_CONFIG                  //!< rudolf configuration file

// Permissions

#define   FILEPERMISSIONS               (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define   DIRPERMISSIONS                (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define   SYSPERMISSIONS                (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/*! \page system System Configuration
 *
 *  <hr size="1"/>
 *
 *  Following defines sets the system configuration and limitations.\n
 *  Defines with preceding "vm" is visible for the byte code assembler\n
 *
 *  \verbatim
 */

#ifdef RETAILVERS //!< LEGO Retail version

#define   SPECIALVERS                   'H'
#define   LEGO_BUNDLE_SEED_ID           "9RNK8ZF528"
#define   LEGO_BUNDLE_ID                "com.lego.lms"

#endif

#ifdef EDUCATIONVERS //!< LEGO Education version

#define   SPECIALVERS                   'E'
#define   LEGO_BUNDLE_SEED_ID           "9YZJD9MXPZ"
#define   LEGO_BUNDLE_ID                "com.lego.education.ev3"

#endif

#ifdef DEVELOPERVERS //!< LEGO Developer version (telnet with password)

#define   SPECIALVERS                   'D'
#define   LEGO_BUNDLE_SEED_ID           "9RNK8ZF528"
#define   LEGO_BUNDLE_ID                "com.lego.lms"

#endif

#ifdef XTENDEDVERS //!< Robomatter/NI version

#define   SPECIALVERS                   'X'
#define   LEGO_BUNDLE_SEED_ID           "9YZJD9MXPZ"
#define   LEGO_BUNDLE_ID                "com.lego.education.ev3"

#endif

#ifdef COMPATVERS //!< ev3dev version

#define   SPECIALVERS                   'C'
#define   LEGO_BUNDLE_SEED_ID           "9YZJD9MXPZ"
#define   LEGO_BUNDLE_ID                "com.lego.education.ev3"

#endif

#ifdef TESTVERS

#define   SPECIALVERS                   TESTVERS
#define   LEGO_BUNDLE_SEED_ID           "BA9Q76VTQG"
#define   LEGO_BUNDLE_ID                "com.lego.education.ev3"
/*
#define   SPECIALVERS                   TESTVERS
#define   LEGO_BUNDLE_SEED_ID           "BA9Q76VTQG"
#define   LEGO_BUNDLE_ID                "com.lego.edu.ev3"

#define   SPECIALVERS                   TESTVERS
#define   LEGO_BUNDLE_SEED_ID           "BA9Q76VTQG"
#define   LEGO_BUNDLE_ID                "com.lego.education.ev3programming"
*/
#endif



#define   MAX_PROGRAMS          SLOTS                 //!< Max number of programs (including UI and direct commands) running at a time
#define   MAX_BREAKPOINTS       4                     //!< Max number of breakpoints (opCODES depends on this value)
#define   MAX_LABELS            32                    //!< Max number of labels per program
#define   MAX_DEVICE_TYPE       127                   //!< Highest type number (positive)
#define   MAX_VALID_TYPE        vmMAX_VALID_TYPE      //!< Highest valid type
#define   MAX_DEVICE_MODES      8                     //!< Max number of modes in one device
#define   MAX_DEVICE_DATASETS   8                     //!< Max number of data sets in one device
#define   MAX_DEVICE_DATALENGTH 32                    //!< Max device data length

#define   MAX_DEVICE_BUSY_TIME  1200                  //!< Max number of mS a device can be busy when read

#define   MAX_DEVICE_TYPES      ((MAX_DEVICE_TYPE + 1) * MAX_DEVICE_MODES)//!< Max number of different device types and modes (max type data list size)

#define   MAX_FRAMES_PER_SEC    10                    //!< Max frames per second update in display

#define   CACHE_DEEPT           10                    //!< Max number of programs cached (in RECENT FILES MENU)
#define   MAX_HANDLES           500                   //!< Max number of handles to memory pools and arrays in one program

#define   MAX_ARRAY_SIZE        1000000000            //!< Max array size
#define   MIN_ARRAY_ELEMENTS    0                     //!< Min elements in a DATA8 array

#define   INSTALLED_MEMORY      6000                  //!< Flash allocated to hold user programs/data
#define   LOW_MEMORY            500                   //!< Low memory warning [KB]

#define   LOGBUFFER_SIZE        1000                  //!< Min log buffer size
#define   DEVICE_LOGBUF_SIZE    300                   //!< Device log buffer size (black layer buffer)
#define   MIN_LIVE_UPDATE_TIME  10                    //!< [mS] Min sample time when live update

#define   MIN_IIC_REPEAT_TIME   10                    //!< [mS] Min IIC device repeat time
#define   MAX_IIC_REPEAT_TIME   1000                  //!< [mS] Max IIC device repeat time

#define   MAX_COMMAND_BYTECODES 64                    //!< Max number of byte codes in a debug terminal direct command
#define   MAX_COMMAND_LOCALS    64                    //!< Max number of bytes allocated for direct command local variables
#define   MAX_COMMAND_GLOBALS   1021                  //!< Max number of bytes allocated for direct command global variables

#define   UI_PRIORITY           20                    //!< UI byte codes before switching VM thread
#define   C_PRIORITY            200                   //!< C call byte codes
#define   PRG_PRIORITY          200                   //!< Prg byte codes before switching VM thread

#define   BUTTON_DEBOUNCE_TIME        30
#define   BUTTON_START_REPEAT_TIME    400
#define   BUTTON_REPEAT_TIME          200

#define   LONG_PRESS_TIME       3000                  //!< [mS] Time pressed before long press recognised

#define   ADC_REF               5000                  //!< [mV]  maximal value on ADC
#define   ADC_RES               4095                  //!< [CNT] maximal count on ADC

#define   IN1_ID_HYSTERESIS     50                    //!< [mV]  half of the span one Id takes up on input connection 1 voltage
#define   OUT5_ID_HYSTERESIS    100                   //!< [mV]  half of the span one Id takes up on output connection 5 voltage

#define   DEVICE_UPDATE_TIME    1000000               //!< Min device (sensor) update time [nS]
#define   DELAY_TO_TYPEDATA     10000                 //!< Time from daisy chain active to upload type data [mS]
#define   DAISYCHAIN_MODE_TIME  10                    //!< Time for daisy chain change mode [mS]
#define   MAX_FILE_HANDLES      64                    //!< Max number of down load file handles
#define   MIN_HANDLE            3                     //!< Min file handle to close

#define   ID_LENGTH             7                     //!< Id length  (BT MAC id) (incl. zero terminator)
#define   NAME_LENGTH           12                    //!< Name length (not including zero termination)

#define   ERROR_BUFFER_SIZE     8                     //!< Number of errors in buffer

#define   PWM_DEVICE            "lms_pwm"             //!< PWM device name
#define   PWM_DEVICE_NAME       "/dev/lms_pwm"        //!< PWM device file name

#define   MOTOR_DEVICE          "lms_motor"           //!< TACHO device name
#define   MOTOR_DEVICE_NAME     "/dev/lms_motor"      //!< TACHO device file name

#define   ANALOG_DEVICE         "lms_analog"          //!< ANALOG device name
#define   ANALOG_DEVICE_NAME    "/dev/lms_analog"     //!< ANALOG device file name

#define   DCM_DEVICE            "lms_dcm"             //!< DCM device name
#define   DCM_DEVICE_NAME       "/dev/lms_dcm"        //!< DCM device file name

#define   UART_DEVICE           "lms_uart"            //!< UART device name
#define   UART_DEVICE_NAME      "/dev/lms_uart"       //!< UART device file name

#define   IIC_DEVICE            "lms_iic"             //!< IIC device name
#define   IIC_DEVICE_NAME       "/dev/lms_iic"        //!< IIC device

#define   BT_DEVICE             "lms_bt"              //!< BT device name
#define   BT_DEVICE_NAME        "/dev/lms_bt"         //!< BT device

#define   UPDATE_DEVICE          "lms_update"         //!< UPDATE device name
#define   UPDATE_DEVICE_NAME     "/dev/lms_update"    //!< UPDATE device

#define   TEST_PIN_DEVICE       "lms_tst_pin"         //!< TEST pin device name
#define   TEST_PIN_DEVICE_NAME  "/dev/lms_tst_pin"    //!< TEST pin device file name

#define   TEST_UART_DEVICE      "lms_tst_uart"        //!< TEST UART device name
#define   TEST_UART_DEVICE_NAME "/dev/lms_tst_uart"   //!< TEST UART device file name


#define   DIR_DEEPT             vmDIR_DEEPT           //!< Max directory items allocated

// File

//***********************************************************************************************************************
//! \todo Filename sizes should only use vmPATHSIZE, vmNAMESIZE, vmEXTSIZE and vmFILENAMESIZE

#define   FILENAMESIZE          vmFILENAMESIZE        //!< All inclusive (path, filename, extension and zero termination)
#define   FILENAME_SIZE         52                    //!< User filename size without extension including zero
#define   FOLDERNAME_SIZE       10                    //!< Folder name size relative to "lms2012" folder including zero
#define   SUBFOLDERNAME_SIZE    FILENAME_SIZE         //!< Sub folder name size without "/" including zero

#define   MAX_FILENAME_SIZE     (FOLDERNAME_SIZE + SUBFOLDERNAME_SIZE + FILENAME_SIZE + 5)

//***********************************************************************************************************************

#define   TYPEDATE_FILE_NAME    "typedata"            //!< TypeData filename
#define   ICON_FILE_NAME        "icon"                //!< Icon image filename
#define   TEXT_FILE_NAME        "text"                //!< Text filename

#define   DEMO_FILE_NAME        "../prjs/BrkProg_SAVE/Demo.rpf"

// Memory





// SD card

#ifdef Linux_X86
#define   SDCARD_DEVICE1        "/dev/mmcblk0"
#define   SDCARD_DEVICE2        "/dev/mmcblk0p1"
#else
#define   SDCARD_DEVICE1        "/dev/mmcblk0"
#define   SDCARD_DEVICE2        "/dev/mmcblk0p1"
#endif

#define   SDCARD_MOUNT          "./mount_sdcard"
#define   SDCARD_UNMOUNT        "./unmount_sdcard"

// USB stick

#ifndef Linux_X86
#define   USBSTICK_DEVICE       "/dev/sda"
#else
#define   USBSTICK_DEVICE       "/dev/sdf1"
#endif

#define   USBSTICK_MOUNT        "./mount_usbstick"
#define   USBSTICK_UNMOUNT      "./unmount_usbstick"

#define   DEFAULT_FOLDER        "ui"                  //!< Folder containing the first small programs
#define   DEFAULT_UI            "ui"                  //!< Default user interface

#define   DEFAULT_VOLUME        vmDEFAULT_VOLUME

/*! \endverbatim
 *
 */

/*! \page powermanagement Power Management
 *
 *  This section describes various constants used in the power management
 *
 *
 *\n
 *  <hr size="1"/>
 *  <b>Battery Indicator</b>
 *\n
 *  <hr size="1"/>
 *  \verbatim
 */

#define   BATT_INDICATOR_HIGH           7500          //!< Battery indicator high [mV]
#define   BATT_INDICATOR_LOW            6200          //!< Battery indicator low [mV]

#define   ACCU_INDICATOR_HIGH           7500          //!< Rechargeable battery indicator high [mV]
#define   ACCU_INDICATOR_LOW            7100          //!< Rechargeable battery indicator low [mV]

/*! \endverbatim
 *  \subpage pmbattind
 *\n
 *  <hr size="1"/>
 *  <b>Low Voltage Shutdown</b>
 *\n
 *  <hr size="1"/>
 *  \verbatim
 */

#define   LOW_VOLTAGE_SHUTDOWN_TIME     10000         //!< Time from shutdown lower limit to shutdown [mS]

#define   BATT_WARNING_HIGH             6.2           //!< Battery voltage warning upper limit [V]
#define   BATT_WARNING_LOW              5.5           //!< Battery voltage warning lower limit [V]
#define   BATT_SHUTDOWN_HIGH            5.5           //!< Battery voltage shutdown upper limit [V]
#define   BATT_SHUTDOWN_LOW             4.5           //!< Battery voltage shutdown lower limit [V]

#define   ACCU_WARNING_HIGH             7.1           //!< Rechargeable battery voltage warning upper limit [V]
#define   ACCU_WARNING_LOW              6.5           //!< Rechargeable battery voltage warning lower limit [V]
#define   ACCU_SHUTDOWN_HIGH            6.5           //!< Rechargeable battery voltage shutdown upper limit [V]
#define   ACCU_SHUTDOWN_LOW             6.0           //!< Rechargeable battery voltage shutdown lower limit [V]

 /*! \endverbatim
  *  \subpage pmbattsd
  *\n
  */
#ifdef ENABLE_HIGH_CURRENT
 /*! \page powermanagement
  *
  *  <hr size="1"/>
  *  <b>High Load Shutdown</b>
  *\n
  *  <hr size="1"/>
  *  \verbatim
  */

#define   LOAD_SHUTDOWN_FAIL            4.0           //!< Current limit for instantly shutdown [A]
#define   LOAD_SHUTDOWN_HIGH            3.0           //!< Current limit for integrated current shutdown [A*S]
#define   LOAD_BREAK_EVEN               1.75          //!< Current limit for integrated current break even [A*S]

#define   LOAD_SLOPE_UP                 0.2           //!< Current slope when current is above break even [A/S]
#define   LOAD_SLOPE_DOWN               0.1           //!< Current slope when current is below break even [A/S]

 /*! \endverbatim
  *  \subpage pmloadsd
  *\n
  */
#endif

#ifndef DISABLE_VIRTUAL_BATT_TEMP
 /*! \page powermanagement
  *
  *  <hr size="1"/>
  *  <b>High Temperature Shutdown</b>
  *\n
  *  <hr size="1"/>
  *  \verbatim
  */

#define   TEMP_SHUTDOWN_FAIL            45.0          //!< Temperature rise before fail shutdown  [C]
#define   TEMP_SHUTDOWN_WARNING         40.0          //!< Temperature rise before warning        [C]

 /*! \endverbatim
  *  \subpage pmtempsd
  *\n
  */
#endif

#define   UPDATE_TIME1          2                     //!< Update repeat time1  [mS]
#define   UPDATE_TIME2          10                    //!< Update repeat time2  [mS]
#define   UPDATE_MEMORY         200                   //!< Update memory size   [mS]
#define   UPDATE_SDCARD         500                   //!< Update sdcard size   [mS]
#define   UPDATE_USBSTICK       500                   //!< Update usbstick size [mS]


/*!
 *
 * \n
 */


#define   VtoC(V)               ((UWORD)((V * ADC_RES) / ADC_REF))
#define   CtoV(C)               ((UWORD)((C * ADC_REF) / ADC_RES))
#define   MtoV(M)               ((UWORD)((M * ADC_REF * 100) / (ADC_RES * 52)))

#define   KB                    1024

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

/*! \page connections Connections
 *
 *  \anchor connectiontypes
 *
 *  Following defines sets the input and output port connection type\n
 *
 *  These connection types are evaluated in the DCM driver \ref DcmDriver "Device Detection Manager Driver"
 *
 *  \verbatim
 */

typedef   enum
{
  CONN_UNKNOWN                  = 111,  //!< Connection is fake (test)

  CONN_DAISYCHAIN               = 117,  //!< Connection is daisy chained
  CONN_NXT_COLOR                = 118,  //!< Connection type is NXT color sensor
  CONN_NXT_DUMB                 = 119,  //!< Connection type is NXT analog sensor
  CONN_NXT_IIC                  = 120,  //!< Connection type is NXT IIC sensor

  CONN_INPUT_DUMB               = 121,  //!< Connection type is LMS2012 input device with ID resistor
  CONN_INPUT_UART               = 122,  //!< Connection type is LMS2012 UART sensor

  CONN_OUTPUT_DUMB              = 123,  //!< Connection type is LMS2012 output device with ID resistor
  CONN_OUTPUT_INTELLIGENT       = 124,  //!< Connection type is LMS2012 output device with communication
  CONN_OUTPUT_TACHO             = 125,  //!< Connection type is LMS2012 tacho motor with series ID resistance

  CONN_NONE                     = 126,  //!< Port empty or not available
  CONN_ERROR                    = 127,  //!< Port not empty and type is invalid
}
CONN;

/*\endverbatim
 *
 *  \n
 */



/*! \page objectstatus Object Status
 *
\verbatim

Image load  ->   Initialize all objects


Initialize  ->   if TriggerCount == 0 ->  RUNNING   (VMTHREAD)
                 else                 ->  STOPPED   (waiting for 1. trigger)

1.Trigger   ->   Initialise           ->  WAITING

Triggered   ->   Enqueue              ->  RUNNING

Done        ->   Dequeue              ->  STOPPED


Program start
           |
           v
        STOPPED -------> WAITING -------> RUNNING --------,
           ^    1.trig/          n.trig/          done/   |
           |                     Reset+Enqueue    Dequeue |
           |                                              |
           '----------------------------------------------'

\endverbatim
 */


/*! \enum DSPSTAT
 *
 *        Dispatch status values
 */
typedef   enum
{
  NOBREAK       = 0x0100,               //!< Dispatcher running (looping)
  STOPBREAK     = 0x0200,               //!< Break because of program stop
  SLEEPBREAK    = 0x0400,               //!< Break because of sleeping
  INSTRBREAK    = 0x0800,               //!< Break because of opcode break
  BUSYBREAK     = 0x1000,               //!< Break because of waiting for completion
  PRGBREAK      = 0x2000,               //!< Break because of program break
  USERBREAK     = 0x4000,               //!< Break because of user decision
  FAILBREAK     = 0x8000                //!< Break because of fail
}
DSPSTAT;

typedef   void      (*PRIM)(void);      //!< Prototype for all byte codes



/*! \page memorylayout Memory Layout
 *
 *  RAM layout
 *
 *-   GlobalVariables                   (aligned)
 *
 *-   ObjectPointerList                 (aligned)
 *
 *-   OBJ                               (aligned)
 *  -   Ip                              (4 bytes)
 *  -   Status                          (2 bytes)
 *  -   TriggerCount/CallerId           (2 bytes)
 *  -   Local                           (0..MAX Bytes)\n
 *
 */

/*! \struct OBJ
 *          Object data is used to hold the variables used for an object (allocated at image load time)
 */
typedef   struct                        // Object
{
  IP      Ip;                           //!< Object instruction pointer
  LP      pLocal;                       //!< Local variable pointer
#ifndef DISABLE_NEW_CALL_MUTEX
  UBYTE   ObjStatus;                    //!< Object status
  UBYTE   Blocked;
#else
  UWORD   ObjStatus;                    //!< Object status
#endif
  union                                 //!< Different meaning for SUBCALL and BLOCKS
  {
    OBJID   CallerId;                   //!< Caller id used for SUBCALL to save object id to return to
    TRIGGER TriggerCount;               //!< Trigger count used by BLOCK's trigger logic
  }u;
  VARDATA Local[];                      //!< Poll of bytes used for local variables
}
OBJ;


/*! \struct BRKP
 *          Breakpoint data hold information used for breakpoint
 */
typedef   struct
{
  IMINDEX Addr;                         //!< Offset to breakpoint address from image start
  OP      OpCode;                       //!< Saved substituted opcode
}
BRKP;

/*! \struct PRG
 *          Program data hold information about a program
 */
typedef   struct
{
  ULONG     InstrCnt;                   //!< Instruction counter used for performance analyses
  ULONG     InstrTime;                  //!< Instruction time used for performance analyses

  ULONG     StartTime;                  //!< Program start time [mS]
  ULONG     RunTime;                    //!< Program run time [uS]

  IP        pImage;                     //!< Pointer to start of image
  GP        pData;                      //!< Pointer to start of data
  GP        pGlobal;                    //!< Pointer to start of global bytes
  OBJHEAD*  pObjHead;                   //!< Pointer to start of object headers
  OBJ**     pObjList;                   //!< Pointer to object pointer list
  IP        ObjectIp;                   //!< Working object Ip
  LP        ObjectLocal;                //!< Working object locals

  OBJID     Objects;                    //!< No of objects in image
  OBJID     ObjectId;                   //!< Active object id

  OBJSTAT   Status;                     //!< Program status
  OBJSTAT   StatusChange;               //!< Program status change
  RESULT    Result;                     //!< Program result (OK, BUSY, FAIL)

  BRKP      Brkp[MAX_BREAKPOINTS];      //!< Storage for breakpoint logic

  LABEL     Label[MAX_LABELS];          //!< Storage for labels
  UWORD     Debug;                      //!< Debug flag

  DATA8     Name[FILENAME_SIZE];

}
PRG;


#define     TYPE_NAME_LENGTH    11
#define     SYMBOL_LENGTH       4       //!< Symbol leng th (not including zero)

/*! \struct TYPES
 *          Device type data
 */
typedef   struct // if data type changes - remember to change "cInputTypeDataInit" !
{
  SBYTE     Name[TYPE_NAME_LENGTH + 1]; //!< Device name
  DATA8     Type;                       //!< Device type
  DATA8     Connection;
  DATA8     Mode;                       //!< Device mode
  DATA8     DataSets;
  DATA8     Format;
  DATA8     Figures;
  DATA8     Decimals;
  DATA8     Views;
  DATAF     RawMin;                     //!< Raw minimum value      (e.c. 0.0)
  DATAF     RawMax;                     //!< Raw maximum value      (e.c. 1023.0)
  DATAF     PctMin;                     //!< Percent minimum value  (e.c. -100.0)
  DATAF     PctMax;                     //!< Percent maximum value  (e.c. 100.0)
  DATAF     SiMin;                      //!< SI unit minimum value  (e.c. -100.0)
  DATAF     SiMax;                      //!< SI unit maximum value  (e.c. 100.0)
  UWORD     InvalidTime;                //!< mS from type change to valid data
  UWORD     IdValue;                    //!< Device id value        (e.c. 0 ~ UART)
  DATA8     Pins;                       //!< Device pin setup
  SBYTE     Symbol[SYMBOL_LENGTH + 1];  //!< SI unit symbol
  UWORD     Align;
}
TYPES;

#define     TYPE_PARAMETERS         19  //!< Number of members in the structure above
#define     MAX_DEVICE_INFOLENGTH   54  //!< Number of bytes in the structure above (can not be changed)


/*!\enum  ERR
 *
 *        Describes error numbers
 */
typedef   enum
{
  TOO_MANY_ERRORS_TO_BUFFER,
  TYPEDATA_TABLE_FULL,
  TYPEDATA_FILE_NOT_FOUND,
  ANALOG_DEVICE_FILE_NOT_FOUND,
  ANALOG_SHARED_MEMORY,
  UART_DEVICE_FILE_NOT_FOUND,
  UART_SHARED_MEMORY,
  IIC_DEVICE_FILE_NOT_FOUND,
  IIC_SHARED_MEMORY,
  DISPLAY_SHARED_MEMORY,
  OUTPUT_SHARED_MEMORY,
  COM_COULD_NOT_OPEN_FILE,
  COM_NAME_TOO_SHORT,
  COM_NAME_TOO_LONG,
  COM_INTERNAL,
  VM_INTERNAL,
  VM_PROGRAM_VALIDATION,
  VM_PROGRAM_NOT_STARTED,
  VM_PROGRAM_FAIL_BREAK,
  VM_PROGRAM_INSTRUCTION_BREAK,
  VM_PROGRAM_NOT_FOUND,
  FILE_OPEN_ERROR,
  FILE_READ_ERROR,
  FILE_WRITE_ERROR,
  FILE_CLOSE_ERROR,
  FILE_GET_HANDLE_ERROR,
  FILE_NAME_ERROR,
  USB_SHARED_MEMORY,
  OUT_OF_MEMORY,
  ERRORS
}
ERR;

//        INTERFACE FOR SHARED LIBRARIES

extern    void      PrimParAdvance(void);                    // Dummy get parameter

extern    void*     PrimParPointer(void);                    // Get pointer to primitives and system calls parameters

extern    IP        GetImageStart(void);                     // Get pointer to start of image

extern    void      SetDispatchStatus(DSPSTAT Status);       // Set dispatch status (result from executing byte code)

extern    void      SetInstructions(ULONG Instructions);     // Set number of instructions before VMThread change

extern    PRGID     CurrentProgramId(void);                  // Get current program id

extern    OBJSTAT   ProgramStatus(PRGID PrgId);              // Get program status

extern    OBJSTAT   ProgramStatusChange(PRGID PrgId);        // Get program status change

extern    void      ProgramEnd(PRGID PrgId);

extern    OBJID     CallingObjectId(void);                   // Get calling objects id

extern    void      AdjustObjectIp(IMOFFS Value);            // Adjust IP

extern    IP        GetObjectIp(void);                       // Get IP

extern    void      SetObjectIp(IP Ip);                      // Set IP

extern    ULONG     GetTimeUS(void);                         // Get uS

extern    ULONG     GetTimeMS(void);                         // Get mS

extern    ULONG     GetTime(void);                           // Get actual program time

extern    ULONG     CurrentObjectIp(void);                   // Get current object ip

extern    void      VmPrint(char *pString);                  // print string

extern    void      SetTerminalEnable(DATA8 Value);          // Terminal enable/disable

extern    DATA8     GetTerminalEnable(void);                 // Get terminal enable state

extern    void      GetResourcePath(char *pString,DATA8 MaxLength);// Get resource path

extern    void*     VmMemoryResize(HANDLER Handle,DATA32 Elements);

extern    void      SetVolumePercent(DATA8 Volume);

extern    DATA8     GetVolumePercent(void);

extern    void      SetSleepMinutes(DATA8 Minutes);

extern    DATA8     GetSleepMinutes(void);

extern    DSPSTAT   ExecuteByteCode(IP pByteCode,GP pGlobals,LP pLocals); // Execute byte code stream (C-call)

extern    DATA8     CheckSdcard(DATA8 *pChanged,DATA32 *pTotal,DATA32 *pFree,DATA8 Force);

extern    DATA8     CheckUsbstick(DATA8 *pChanged,DATA32 *pTotal,DATA32 *pFree,DATA8 Force);

extern    void      SetUiUpdate(void);

extern    RESULT    ValidateChar(DATA8 *pChar,DATA8 Set);

extern    RESULT    ValidateString(DATA8 *pString,DATA8 Set);

extern    ERR       LogErrorGet(void);

#ifdef BUFPRINTSIZE
extern    void      BufPrint(char Cmd,char *pFormat, ...);
#endif



#define   ERR_STRING_SIZE         vmERR_STRING_SIZE          // Inclusive zero termination

extern    void      LogErrorNumber(ERR Error);               // Log error number
extern    DATA8     LogErrorNumberExists(ERR Error);


#ifndef   DISABLE_OLD_COLOR

#define   COLORS                        4
#define   CALPOINTS                     3


/*! \page NxtColorMemory
 *
 *  <b>     Shared Memory </b>
 *
 *  <hr size="1"/>
 *
 *
 *  \verbatim
 */

typedef   struct
{
  ULONG   Calibration[CALPOINTS][COLORS];
  UWORD   CalLimits[CALPOINTS - 1];
  UWORD   Crc;
  UWORD   ADRaw[COLORS];
  UWORD   SensorRaw[COLORS];
}
COLORSTRUCT;

/*\endverbatim
 *
 *  \n
 */

#endif


//        INTERFACE BETWEEN SHARED LIBRARIES AND MODULES

/*! \page AnalogModuleMemory
 *  <b>     Shared Memory </b>
 *
 *  <hr size="1"/>
 *
 *  It is possible to get a pointer to the raw analogue values for use in userspace
 *  this pointer will point to a struct and the layout is following:
 *
 *  \verbatim
 */

typedef   struct
{
  DATA16  InPin1[INPUTS];         //!< Analog value at input port connection 1
  DATA16  InPin6[INPUTS];         //!< Analog value at input port connection 6
  DATA16  OutPin5[OUTPUTS];       //!< Analog value at output port connection 5
  DATA16  BatteryTemp;            //!< Battery temperature
  DATA16  MotorCurrent;           //!< Current flowing to motors
  DATA16  BatteryCurrent;         //!< Current flowing from the battery
  DATA16  Cell123456;             //!< Voltage at battery cell 1, 2, 3,4, 5, and 6
#ifndef DISABLE_FAST_DATALOG_BUFFER
  DATA16  Pin1[INPUTS][DEVICE_LOGBUF_SIZE];      //!< Raw value from analog device
  DATA16  Pin6[INPUTS][DEVICE_LOGBUF_SIZE];      //!< Raw value from analog device
  UWORD   Actual[INPUTS];
  UWORD   LogIn[INPUTS];
  UWORD   LogOut[INPUTS];
#endif
#ifndef   DISABLE_OLD_COLOR
  COLORSTRUCT  NxtCol[INPUTS];
#endif
  DATA16  OutPin5Low[OUTPUTS];    //!< Analog value at output port connection 5 when connection 6 is low

  DATA8   Updated[INPUTS];

  DATA8   InDcm[INPUTS];          //!< Input port device types
  DATA8   InConn[INPUTS];

  DATA8   OutDcm[OUTPUTS];        //!< Output port device types
  DATA8   OutConn[OUTPUTS];
}
ANALOG;

/*\endverbatim
 *
 *  \n
 */


/*! \page UartModuleMemory
 *
 *  <b>     Shared Memory </b>
 *
 *  <hr size="1"/>
 *
 *  It is possible to get a pointer to the uart values for use in userspace
 *  this pointer will point to a struct and the layout is following:
 *
 *  \verbatim
 */

#define   UART_DATA_LENGTH        MAX_DEVICE_DATALENGTH
#define   UART_BUFFER_SIZE        64

typedef   struct
{
  TYPES   TypeData[INPUTS][MAX_DEVICE_MODES]; //!< TypeData

#ifndef DISABLE_FAST_DATALOG_BUFFER
  UWORD   Repeat[INPUTS][DEVICE_LOGBUF_SIZE];
  DATA8   Raw[INPUTS][DEVICE_LOGBUF_SIZE][UART_DATA_LENGTH];      //!< Raw value from UART device
  UWORD   Actual[INPUTS];
  UWORD   LogIn[INPUTS];
#else
  DATA8   Raw[INPUTS][UART_DATA_LENGTH];      //!< Raw value from UART device
#endif
  DATA8   Status[INPUTS];                     //!< Status
  DATA8   Output[INPUTS][UART_DATA_LENGTH];   //!< Bytes to UART device
  DATA8   OutputLength[INPUTS];
}
UART;

/*\endverbatim
 *
 *  \n
 */


#define   UART_PORT_CHANGED       0x01        //!< Input port changed
#define   UART_DATA_READY         0x08        //!< Data is ready
#define   UART_WRITE_REQUEST      0x10        //!< Write request


typedef   struct
{
  DATA8     Connection[INPUTS];
  DATA8     Type[INPUTS];
  DATA8     Mode[INPUTS];
}
DEVCON;


typedef   struct
{
  TYPES   TypeData;
  DATA8   Port;
  DATA8   Mode;
}
UARTCTL;

#define   UART_SET_CONN           _IOWR('u',0,DEVCON)
#define   UART_READ_MODE_INFO     _IOWR('u',1,UARTCTL)
#define   UART_NACK_MODE_INFO     _IOWR('u',2,UARTCTL)
#define   UART_CLEAR_CHANGED      _IOWR('u',3,UARTCTL)


/*! \page IicModuleMemory
 *
 *  <b>     Shared Memory </b>
 *
 *  <hr size="1"/>
 *
 *  It is possible to get a pointer to the iic values for use in userspace
 *  this pointer will point to a struct and the layout is following:
 *
 *  \verbatim
 */

#define   IIC_DATA_LENGTH         MAX_DEVICE_DATALENGTH
#define   IIC_NAME_LENGTH         8

typedef   struct
{
  TYPES   TypeData[INPUTS][MAX_DEVICE_MODES]; //!< TypeData

#ifndef DISABLE_FAST_DATALOG_BUFFER
  UWORD   Repeat[INPUTS][DEVICE_LOGBUF_SIZE];
  DATA8   Raw[INPUTS][DEVICE_LOGBUF_SIZE][IIC_DATA_LENGTH];      //!< Raw value from IIC device
  UWORD   Actual[INPUTS];
  UWORD   LogIn[INPUTS];
#else
  DATA8   Raw[INPUTS][IIC_DATA_LENGTH];      //!< Raw value from IIC device
#endif
  DATA8   Status[INPUTS];                     //!< Status
  DATA8   Changed[INPUTS];
  DATA8   Output[INPUTS][IIC_DATA_LENGTH];    //!< Bytes to IIC device
  DATA8   OutputLength[INPUTS];
}
IIC;

/*\endverbatim
 *
 *  \n
 */


#define   IIC_PORT_CHANGED       0x01         //!< Input port changed
#define   IIC_DATA_READY         0x08         //!< Data is ready
#define   IIC_WRITE_REQUEST      0x10         //!< Write request


typedef   struct
{
  TYPES   TypeData;
  DATA8   Port;
  DATA8   Mode;
}
IICCTL;


typedef   struct
{
  RESULT  Result;
  DATA8   Port;
  DATA8   Repeat;
  DATA16  Time;
  DATA8   WrLng;
  DATA8   WrData[IIC_DATA_LENGTH];
  DATA8   RdLng;
  DATA8   RdData[IIC_DATA_LENGTH];
}
IICDAT;


typedef   struct
{
  DATA8   Port;
  DATA16  Time;
  DATA8   Type;
  DATA8   Mode;
  DATA8   Manufacturer[IIC_NAME_LENGTH + 1];
  DATA8   SensorType[IIC_NAME_LENGTH + 1];
  DATA8   SetupLng;
  ULONG   SetupString;
  DATA8   PollLng;
  ULONG   PollString;
  DATA8   ReadLng;
}
IICSTR;


#define   IIC_SET_CONN            _IOWR('i',2,DEVCON)
#define   IIC_READ_TYPE_INFO      _IOWR('i',3,IICCTL)
#define   IIC_SETUP               _IOWR('i',5,IICDAT)
#define   IIC_SET                 _IOWR('i',6,IICSTR)
#define   IIC_READ_STATUS					_IOWR('i',7,IICDAT)
#define   IIC_READ_DATA						_IOWR('i',8,IICDAT)
#define   IIC_WRITE_DATA					_IOWR('i',9,IICDAT)




#define   TST_PIN_LENGTH    8

typedef   struct
{
  DATA8   Port;
  DATA8   Length;
  DATA8   String[TST_PIN_LENGTH + 1];
}
TSTPIN;

#define   TST_PIN_ON              _IOWR('t',1,TSTPIN)
#define   TST_PIN_OFF             _IOWR('t',2,TSTPIN)
#define   TST_PIN_READ            _IOWR('t',3,TSTPIN)
#define   TST_PIN_WRITE           _IOWR('t',4,TSTPIN)




#define   TST_UART_LENGTH         UART_BUFFER_SIZE

typedef   struct
{
  DATA32  Bitrate;
  DATA8   Port;
  DATA8   Length;
  DATA8   String[TST_UART_LENGTH];
}
TSTUART;

#define   TST_UART_ON             _IOWR('t',5,TSTUART)
#define   TST_UART_OFF            _IOWR('t',6,TSTUART)
#define   TST_UART_EN             _IOWR('t',7,TSTUART)
#define   TST_UART_DIS            _IOWR('t',8,TSTUART)
#define   TST_UART_READ           _IOWR('t',9,TSTUART)
#define   TST_UART_WRITE          _IOWR('t',10,TSTUART)

#define   LCD_BUFFER_SIZE (((LCD_WIDTH + 7) / 8) * LCD_HEIGHT)
#define   LCD_TOPLINE_SIZE (((LCD_WIDTH + 7) / 8) * (TOPLINE_HEIGHT + 1))


/*! \page DisplayModuleMemory
 *
 *  <b>     Shared Memory </b>
 *
 *  <hr size="1"/>
 *
 *  It is possible to get a pointer to the display memory for use in userspace
 *  this pointer will point to a frame buffer as follows:
 *
 *  \verbatim
 */

typedef   struct
{
  UBYTE   Lcd[LCD_BUFFER_SIZE];
}
LCD;

/*\endverbatim
 *
 *  \n
 */

/*! \page UsbModuleMemory
 *
 *  <b>     Shared Memory </b>
 *
 *  <hr size="1"/>
 *
 *  It is possible to get a pointer to the USB Speed (FULL or HIGH) for use in userspace
 *  this pointer will point to a struct and the layout is following:
 *
 *  \verbatim
 */

typedef enum {
    FULL_SPEED,
    HIGH_SPEED
} USB_SPEED;

/*\endverbatim
 *
 *  \n
 */

/*! \page VmNonvolatileMemory
 *
 *  <b>     VM non volatile Memory </b>
 *
 *  <hr size="1"/>
 *
 *  This struct will be loaded at start up and save when closing down VM
 *
 *  \verbatim
 */

typedef   struct
{
  DATA8   VolumePercent;                //!< System default volume [0..100%]
  DATA8   SleepMinutes;                 //!< System sleep          [0..120min] (0 = ~)
}
NONVOL;

/*\endverbatim
 *
 *  \n
 */


/*
 *                      Motor/OUTPUT Typedef
 */
typedef struct
{
  SLONG TachoCounts;
  SBYTE Speed;
  SLONG TachoSensor;
}MOTORDATA;

typedef struct
{
  DATA8   Cmd;
  DATA8   Nos;
  DATA8   Power;
  DATA32  Step1;
  DATA32  Step2;
  DATA32  Step3;
  DATA8   Brake;
} STEPPOWER;

typedef struct
{
  DATA8   Cmd;
  DATA8   Nos;
  DATA8   Power;
  DATA32  Time1;
  DATA32  Time2;
  DATA32  Time3;
  DATA8   Brake;
} TIMEPOWER;

typedef struct
{
  DATA8   Cmd;
  DATA8   Nos;
  DATA8   Speed;
  DATA32  Step1;
  DATA32  Step2;
  DATA32  Step3;
  DATA8   Brake;
} STEPSPEED;

typedef struct
{
  DATA8   Cmd;
  DATA8   Nos;
  DATA8   Speed;
  DATA32  Time1;
  DATA32  Time2;
  DATA32  Time3;
  DATA8   Brake;
} TIMESPEED;

typedef struct
{
  DATA8   Cmd;
  DATA8   Nos;
  DATA8   Speed;
  DATA16  Turn;
  DATA32  Step;
  DATA8   Brake;
} STEPSYNC;

typedef struct
{
  DATA8   Cmd;
  DATA8   Nos;
  DATA8   Speed;
  DATA16  Turn;
  DATA32  Time;
  DATA8   Brake;
} TIMESYNC;
/*
 *                 End of Motor/OUTPUT Typedef
 */


#define     PRINTBUFFERSIZE             160

typedef struct
{
  NONVOL    NonVol;
  DATA8     FirstProgram[MAX_FILENAME_SIZE];

  char      PrintBuffer[PRINTBUFFERSIZE + 1];
  DATA8     TerminalEnabled;

  PRGID     FavouritePrg;
  PRGID     ProgramId;                    //!< Program id running
  PRG       Program[MAX_PROGRAMS];        //!< Program[0] is the UI byte codes running


  ULONG     InstrCnt;                     //!< Instruction counter (performance test)
  IP        pImage;                       //!< Pointer to start of image
  GP        pGlobal;                      //!< Pointer to start of global bytes
  OBJHEAD*  pObjHead;                     //!< Pointer to start of object headers
  OBJ**     pObjList;                     //!< Pointer to object pointer list

  IP        ObjectIp;                     //!< Working object Ip
  LP        ObjectLocal;                  //!< Working object locals
  OBJID     Objects;                      //!< No of objects in image
  OBJID     ObjectId;                     //!< Active object id

  IP        ObjIpSave;
  GP        ObjGlobalSave;
  LP        ObjLocalSave;
  DSPSTAT   DispatchStatusSave;
  ULONG     PrioritySave;

  long      TimerDataSec;
  long      TimerDatanSec;

  UWORD     Debug;

  UWORD     Test;

  UWORD     RefCount;

  ULONG     TimeuS;

  ULONG     OldTime1;
  ULONG     OldTime2;
  ULONG     NewTime;
#ifdef ENABLE_PERFORMANCE_TEST
  ULONG     PerformTimer;
  DATAF     PerformTime;
#endif

  DSPSTAT   DispatchStatus;               //!< Dispatch status
  ULONG     Priority;                     //!< Object priority

  ULONG     Value;
  HANDLER   Handle;

  ERR       Errors[ERROR_BUFFER_SIZE];
  UBYTE     ErrorIn;
  UBYTE     ErrorOut;

  DATA32    MemorySize;
  DATA32    MemoryFree;
  ULONG     MemoryTimer;

#ifndef DISABLE_SDCARD_SUPPORT
  DATA32    SdcardSize;
  DATA32    SdcardFree;
  ULONG     SdcardTimer;
  DATA8     SdcardOk;
#endif

#ifndef DISABLE_USBSTICK_SUPPORT
  DATA32    UsbstickSize;
  DATA32    UsbstickFree;
  ULONG     UsbstickTimer;
  DATA8     UsbstickOk;
#endif

  struct udev *udev;                      //!< Global udev context

  LCD       LcdBuffer;                    //!< Copy of last LCD update
  DATA8     LcdUpdated;                   //!< LCD updated

  ANALOG    Analog;
  ANALOG    *pAnalog;
  int       AdcFile;

#ifdef ENABLE_STATUS_TEST
  DATA8     Status;
#endif

#ifdef ALLOW_DEBUG_PULSE
  DATA8     PulseShow;
  DATA8     Pulse;
#endif
} GLOBALS;

extern GLOBALS VMInstance;

#endif /* LMS2012_H_ */
