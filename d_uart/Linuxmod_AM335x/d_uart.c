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


/*! \page UartModule UART Module
 *
 *  <hr size="1"/>
 *
 *  Manages communication with intelligent devices on an input port.\n
 *
 *-  \subpage UartDriver
 *-  \subpage UartProtocol
 *-  \subpage UartModuleMemory
 *-  \subpage UartResources
 *
 */


/*! \page UartDriver UART Device Controller
 *
 *  <hr size="1"/>
 *
 *  Manages the sequence necessary when adding a UART devices to an input port.\n
 *
 * \n
 *
 *  The UART Device Controller gets information from the DCM driver \ref DcmDriver "Device Detection Manager Driver"
 *  about the port state and when the DCM driver detects a UART device on the port the sequence below the following defines
 *  is started inside the UART Device Controller.
 *
 * \verbatim
*/

#define   LOWEST_BITRATE                2400  //  Lowest possible bit rate (always used for sync and info)  [b/S]
#define   MIDLE_BITRATE                57600  //  Highest bit rate allowed when adjusting clock             [b/S]
#define   HIGHEST_BITRATE             460800  //  Highest possible bit rate                                 [b/S]

/*
  SEQUENCE WHEN UART DEVICE IS DETECTED
  =====================================

        HOST                                                                                DEVICE
    ------------------------------------------------------------        ----------------------------------------------------------------------
                                                                                        - Reset                 <----------------------------,
                                                                                          - Set TX active (low)                              |
                                                                                          - Set RX floating                                  |
                                                                                          - Wait half a second 1st.time (next time >=10mS)   |
                                                                                                                                             |
    - Enable UART communication                                                         - Enable UART communication                          |
      - Setup UART for LOWEST_BITRATE                                                     - Setup UART for LOWEST_BITRATE                    |
      - Setup hardware buffers                                                                                                               |
                                                                                                                                             |
                                                                                                                                             |
    - Exchange informations                                                             - Exchange informations                              |
      - Receive command data                                         <-- CMD              - Send command data (type,modes,speed, etc)        |
      - Send command data (type,modes,speed, etc)           CMD  -->                      - Receive command data                             |
      - Receive info data                                            <-- INFO             - Send info data (name,scaling,data format, etc)   |
      - Send info data (name,scaling,data format, etc)      INFO -->                      - Receive info data                                |
      - Receive acknowledge                                          <-- ACK              - When finished info send acknowledge              |
                                                                                          - Timeout (80mS)      -----------------------------'
      - When finished info send acknowledge                 ACK  -->                      - Receive acknowledge
      - Switch to valid communication speed                                               - Switch to valid communication speed


    - Communication running                                                              - Communication running
      - Receive data                                                 <-- DATA             - Send data
      - Every 100mS, send watchdog service                  NACK -->                      - If more than one of six messages is correct send NACK
      - Receive data                                                 <-- DATA             - Send data
      - Receive data                                                 <-- DATA             - Send data
      - Receive data                                                 <-- DATA             - Send data
         --
      - Send command                                        CMD  -->                      - Receive command
      - Receive data                                                 <-- DATA             - Send data
      - Receive data                                                 <-- DATA             - Send data
         --
      - Receive data                                                 <-- DATA             - Send data
      - Every 100mS, send watchdog service                  NACK -->                      - If more than one of six messages is correct send NACK
      - Receive data                                                 <-- DATA             - Send data
      - Receive data                                                 <-- DATA             - Send data
      - Receive data                                                 <-- DATA             - Send data
         --


  DEVICES
  =======

  Devices should only send data when data changes at a maximum rate of 1mS and if the time elapsed since last change exceeds 100mS.
  Data should also be send after a watchdog service message has been received.
  If watchdog service (NACK) is not received within 1000 mS the device should reset.

  When placed wrong on an output port device TX should continue to transmit SYNC, INFO or DATA to ensure that the host detects
  that connection 6 is low
  \endverbatim
  \ref ExampleUartProtocol "Ex."
  \verbatim


  HOST
  ====

  The host sends a watchdog service message (NACK) every 100 mS if no error occours.
  The host should check every data message format against the info once sent from the device and leave out the watchdog service when the data
  is not valid within 6 messages.





\endverbatim
 *
 *  \n
 */


/*! \page UartProtocol UART Device Communication Protocol
 *
 *  <hr size="1"/>
 *
 *  Definition of protocol used when an UART communicating device is detected at an input port.\n
 *
 * \n
 *
 *  The communication used messages that consists of one or more bytes:
 *
 *  \verbatim

  FIRST BYTE IN MESSAGE MEANS
  ===========================

  bit 76543210
      ||
      00LLLCC0  SYS     - System message
      ||||||||
      ||000000  SYNC    - Synchronisation byte
      ||000010  NACK    - Not acknowledge byte
      ||000100  ACK     - Acknowledge byte
      ||LLL110  ESC     - Reserved for future use
      ||
      ||
      01LLLCCC  CMD     - Command message
      ||   |||
      ||   000  TYPE
      ||   001  MODES
      ||   010  SPEED
      ||   011  SELECT
      ||   100  WRITE
      ||   101
      ||   110
      ||   111
      ||
      ||
      10LLLMMM  INFO    - Info message (next byte is command)
      ||   |||
      ||   000  Mode 0 default   (must be last mode in INFO stream to select as default)
      ||   001  Mode 1
      ||   010  Mode 2
      ||   011  Mode 3
      ||   100  Mode 4
      ||   101  Mode 5
      ||   110  Mode 6
      ||   111  Mode 7
      ||
      ||
      11LLLMMM  DATA    - Data message
        |||
        000     Message pay load is 1 byte not including command byte and check byte
        001     Message pay load is 2 bytes not including command byte and check byte
        010     Message pay load is 4 bytes not including command byte and check byte
        011     Message pay load is 8 bytes not including command byte and check byte
        100     Message pay load is 16 bytes not including command byte and check byte
        101     Message pay load is 32 bytes not including command byte and check byte



  Messages From Device
  ====================

    Command messages:

      TYPE      01000000  tttttttt  cccccccc                                                                                    Device type

      MODES     01001001  00000iii  00000jjj  cccccccc                                                                          Number of modes

      SPEED     01010010  ssssssss  ssssssss  ssssssss  ssssssss  cccccccc                                                      Max communication speed

    Info messages:

      NAME      10LLLMMM  00000000  aaaaaaaa  ........  cccccccc                                                                Name of Device in mode MMM

      RAW       10011MMM  00000001  llllllll  llllllll  llllllll  llllllll  hhhhhhhh  hhhhhhhh  hhhhhhhh  hhhhhhhh  cccccccc    Raw value span in mode MMM

      PCT       10011MMM  00000010  llllllll  llllllll  llllllll  llllllll  hhhhhhhh  hhhhhhhh  hhhhhhhh  hhhhhhhh  cccccccc    Percentage span in mode MMM

      SI        10011MMM  00000011  llllllll  llllllll  llllllll  llllllll  hhhhhhhh  hhhhhhhh  hhhhhhhh  hhhhhhhh  cccccccc    SI unit value span in mode MMM

      SYMBOL    10011MMM  00000100  aaaaaaaa  aaaaaaaa  aaaaaaaa  aaaaaaaa  aaaaaaaa  aaaaaaaa  aaaaaaaa  aaaaaaaa  cccccccc    SI symbol

      FORMAT    10010MMM  10000000  00nnnnnn  000000ff  0000FFFF  0000DDDD  cccccccc                                            Format of data in mode MMM

    Data messages:

      DATA      11LLLMMM  dddddddd  ........  cccccccc                                                                          Data in format described under INFO MMM


    Messages from the device must follow the above sequence
    Devices with more modes can repeat "Info messages" once for every mode
    Highest "mode number" must be first
    NAME is the first in info sequence and is necessary to initiate a mode info
    FORMAT is last in info sequence and is necessary to complete the modes info
    Other info messages is optional and has a default value that will be used if not provided
    Delay 10 mS between modes (from FORMATx to NAMEy) to allow the informations to be saved in the brick


    # After ACK only DATA is allowed at the moment

    (Simplest device only needs to send: TYPE, NAME, FORMAT - if SPEED, RAW, PCT and SI are defaults)



  Messages To Device
  ==================

    Command messages:

      SPEED     01010010  ssssssss  ssssssss  ssssssss  ssssssss  cccccccc                                                      Max communication speed

      SELECT    01000011  00000mmm  cccccccc                                                                                    Select new mode

      WRITE     01LLL100  dddddddd  ........  cccccccc                                                                          Write 1-23 bytes to device

    # After ACK only SELECT and WRITE is allowed at the moment



  BIT Explanations
  ================

      LLL       = Message pay load bytes not including command byte and check byte
                  000   = 1
                  001   = 2
                  010   = 4
                  011   = 8
                  100   = 16
                  101   = 32

      CCC       = Command
                  000   = TYPE
                  001   = MODES
                  010   = SPEED
                  011   = SELECT

      MMM       = Mode
                  000   = Mode 0 default   (must be last mode in INFO stream to select as default)
                  001   = Mode 1
                  010   = Mode 2
                  011   = Mode 3
                  100   = Mode 4
                  101   = Mode 5
                  110   = Mode 6
                  111   = Mode 7

      iii       = Number of modes
                  000   = Only mode 0      (default if message not received)
                  001   = Mode 0,1
                  010   = Mode 0,1,2
                  011   = Mode 0,1,2,3
                  100   = Mode 0,1,2,3,4
                  101   = Mode 0,1,2,3,4,5
                  110   = Mode 0,1,2,3,4,5,6
                  111   = Mode 0,1,2,3,4,5,6,7

      jjj       = Number of modes in view and data log (default is iii if not received)
                  000   = Only mode 0
                  001   = Mode 0,1
                  010   = Mode 0,1,2
                  011   = Mode 0,1,2,3
                  100   = Mode 0,1,2,3,4
                  101   = Mode 0,1,2,3,4,5
                  110   = Mode 0,1,2,3,4,5,6
                  111   = Mode 0,1,2,3,4,5,6,7

      cccccccc  = Check byte (result of 0xFF exclusively or'ed with all preceding bytes)

      ssssssss  = Speed 4 bytes (ULONG with LSB first)

      tttttttt  = Device type (used together with mode to form VM device type)

      llllllll  = Floating point value for low (RAW/PCT/SI) value (used to scale values)
                  (Default if not received - RAW = 0.0, PCT = 0.0, SI = 0.0)

      hhhhhhhh  = Floating point value for high (RAW/PCT/SI) value (used to scale values)
                  (Default if not received - RAW = 1023.0, PCT = 100.0, SI = 1.0)

      nnnnn     = Number of data sets [1..32 DATA8] [1..16 DATA16] [1..8 DATA32] [1..8 DATAF]

      ff        = Data set format
                  00    = DATA8
                  01    = DATA16
                  10    = DATA32
                  11    = DATAF

      aaaaaaaa  = Device name in ACSII characters (fill with zero '\0' to get to LLL - no zero termination necessary)
                  (Default if not received - empty)

      dddddddd  = Binary data (LSB first)

      mmm       = Mode
                  000   = Mode 0
                  001   = Mode 1
                  010   = Mode 2
                  011   = Mode 3
                  100   = Mode 4
                  101   = Mode 5
                  110   = Mode 6
                  111   = Mode 7

      FFFF      = Total number of figures shown in view and datalog [0..15] (inclusive decimal point and decimals)
                  (Default if not received - 4)

      DDDD      = Number of decimals shown in view and datalog [0..15]
                  (Default if not received - 0)


  \endverbatim \anchor ExampleUartProtocol \verbatim

  Example with info coming from a device with two modes
  =====================================================


      TYPE      01000000  tttttttt  cccccccc                                                type

      MODES     01001001  00000001  00000001  cccccccc                                      Mode 0 and 1, both shown in view

      SPEED     01010010  00000000  11100001  00000000  00000000  cccccccc                  57600 bits/Second


      NAME      10011001  00000000  'L' 'i' 'g' 'h' 't' '\0' '\0' '\0'  cccccccc            "Light"

      RAW       10011001  00000001  0.0 1023.0  cccccccc                                    RAW 0-1023

      SI        10011001  00000011  0.0 1023.0  cccccccc                                    SI 0-1023

      SYMBOL    10011001  00000100  'l' 'x' '\0' '\0' '\0' '\0' '\0' '\0' cccccccc          "lx"

      FORMAT    10010001  10000000  00000001  00000001  00000100  00000000  cccccccc        1 * DATA16, 4 figures, 0 decimals


      NAME      10011000  00000000  'C' 'o' 'l' 'o' 'r' '\0' '\0' '\0'  cccccccc            "Color"

      RAW       10011000  00000001  0.0    6.0  cccccccc                                    RAW 0-6

      SI        10011000  00000011  0.0    6.0  cccccccc                                    SI 0-6

      FORMAT    10010000  10000000  00000001  00000001  00000001  00000000  cccccccc        1 * DATA16, 1 figure, 0 decimals


      ACK


  Error handling
  ==============

  Before ACK:

  IF ANYTHING GOES WRONG - HOST WILL NOT ACK AND DEVICE MUST RESET !


  After ACK

  IF MORE THAN 5 ERRORS - HOST WILL NOT SERVICE WATCHDOG



\endverbatim
 *
 *  \n
 */

// FIRST BYTE

#define   BYTE_SYNC                     0x00                            // Synchronisation byte
#define   BYTE_ACK                      0x04                            // Acknowledge byte
#define   BYTE_NACK                     0x02                            // Not acknowledge byte

#define   MESSAGE_SYS                   0x00                            // System message
#define   MESSAGE_CMD                   0x40                            // Command message
#define   MESSAGE_INFO                  0x80                            // Info message
#define   MESSAGE_DATA                  0xC0                            // Data message
#define   GET_MESSAGE_TYPE(B)           (B & 0xC0)                      // Get message type

#define   CMD_TYPE                      0x00                            // CMD command - TYPE     (device type for VM reference)
#define   CMD_MODES                     0x01                            // CMD command - MODES    (number of supported modes 0=1)
#define   CMD_SPEED                     0x02                            // CMD command - SPEED    (maximun communication speed)
#define   CMD_SELECT                    0x03                            // CMD command - SELECT   (select mode)
#define   CMD_WRITE                     0x04                            // CMD command - WRITE    (write to device)
#define   GET_CMD_COMMAND(B)            (B & 0x07)                      // Get CMD command

#define   GET_MODE(B)                   (B & 0x07)                      // Get mode

#define   CONVERT_LENGTH(C)             (1 << (C & 0x07))
#define   GET_MESSAGE_LENGTH(B)         (CONVERT_LENGTH(B >> 3))        // Get message length exclusive check byte

#define   MAKE_CMD_COMMAND(C,LC)        (MESSAGE_CMD + (C & 0x07) + ((LC & 0x07) << 3))


// SECOND INFO BYTE

#define   INFO_NAME                     0x00                            // INFO command - NAME    (device name)
#define   INFO_RAW                      0x01                            // INFO command - RAW     (device RAW value span)
#define   INFO_PCT                      0x02                            // INFO command - PCT     (device PCT value span)
#define   INFO_SI                       0x03                            // INFO command - SI      (device SI  value span)
#define   INFO_SYMBOL                   0x04                            // INFO command - SYMBOL  (device SI  unit symbol)
#define   INFO_FORMAT                   0x80                            // INFO command - FORMAT  (device data sets and format)
#define   GET_INFO_COMMAND(B)           (B)                             // Get INFO command


#include <asm/types.h>

#include "../../lms2012/source/lms2012.h"
#include "../../lms2012/source/am335x.h"


TYPES     TypeDefaultUart[] =
{
//   Name                    Type                   Connection        Mode  DataSets  Format  Figures   Decimals    Views   RawMin  RawMax  PctMin  PctMax  SiMin   SiMax   Time   IdValue  Pins  Symbol
  { ""                     , TYPE_UNKNOWN         , CONN_UNKNOWN    , 0   , 1       , 1     , 4     ,   0     ,     0   ,      0.0, 1023.0,    0.0,  100.0,    0.0, 1023.0,   10,     0,    '-',  ""    },
  { "TERMINAL"             , TYPE_TERMINAL        , CONN_INPUT_UART , 0   , 0       , 0     , 4     ,   0     ,     0   ,      0.0, 4095.0,    0.0,  100.0,    0.0, 1000.0,    0,     0,    '-',  ""    },
  { "\0" }
};




#define   MODULE_NAME 			"uart_module"
#define   DEVICE1_NAME                  UART_DEVICE
#define   DEVICE2_NAME                  TEST_UART_DEVICE


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

struct liu_test
{
	unsigned char rx_tx;
	char state;
	int bitrate;
	char data;
};

struct liu_test liu_data[1024];

static	int pliu = 0;

static volatile ULONG *CM_PER;
static volatile ULONG *CM;

static volatile ULONG *GPIOBANK0;
static volatile ULONG *GPIOBANK1;
static volatile ULONG *GPIOBANK2;
static volatile ULONG *GPIOBANK3;

int	Hw = 0x6;

#define   NO_OF_INPUT_PORTS             INPUTS

enum InputUartPins
{
	INPUT_UART_BUFFER,
	INPUT_UART_TXD,
	INPUT_UART_RXD,
	INPUT_UART_PIN5,
	INPUT_UART_PIN6,
	INPUT_UART_PINS
};



INPIN     InputUartPin[NO_OF_INPUT_PORTS][INPUT_UART_PINS];

#define   Uart1       0
#define   Uart2       1
#define   Uart3       2
#define   Uart4       3



unsigned long   UartBaseAddr[] =
{
	0x481A8000,   // Port 1, am335x_uart4
	0x48024000,   // Port 2, am335x_uart2
	0x48022000,   // Port 3, am335x_uart1
	0x481AA000    // Port 4, am335x_uart5
};


int	UartIntr[] =
{
	AM33XX_IRQ_UART4, // Port 1
	AM33XX_IRQ_UART2, // Port 2
	AM33XX_IRQ_UART1, // Port 3
	AM33XX_IRQ_UART5  // Port 4
};



INPIN     EP2_InputUartPin[][INPUT_UART_PINS] =
{
  { // Input UART 1
    {GP1_19 , NULL, 0 }, // Buffer disable
    {GP0_31 , NULL, 0 }, // TXD
    {GP0_30 , NULL, 0 }, // RXD
    {GP1_28 , NULL, 0 }, // Pin 5  - DIGIA0          - Digital input/output
    {GP1_18 , NULL, 0 }, // Pin 6  - DIGIA1          - Digital input/output
  },
  { // Input UART 2
    {GP0_4  , NULL, 0 }, // Buffer disable
    {GP0_3  , NULL, 0 }, // TXD
    {GP0_2  , NULL, 0 }, // RXD
    {GP1_16 , NULL, 0 }, // Pin 5  - DIGIB0          - Digital input/output
    {GP0_5  , NULL, 0 }, // Pin 6  - DIGIB1          - Digital input/output
  },
  { // Input UART 3
    {GP3_19 , NULL, 0 }, // Buffer disable
    {GP0_15 , NULL, 0 }, // TXD
    {GP0_14 , NULL, 0 }, // RXD
    {GP0_12 , NULL, 0 }, // Pin 5  - DIGIC0          - Digital input/output
    {GP0_13 , NULL, 0 }, // Pin 6  - DIGIC1          - Digital input/output
  },
  { // Input UART 4
    {GP3_15 , NULL, 0 }, // Buffer disable
    {GP2_14 , NULL, 0 }, // TXD
    {GP2_15 , NULL, 0 }, // RXD
    {GP3_21 , NULL, 0 }, // Pin 5  - DIGID0          - Digital input/output
    {GP1_17 , NULL, 0 }, // Pin 6  - DIGID1          - Digital input/output
  },
};



INPIN     *pInputUartPin[] =
{
  [EP2]       =   (INPIN*)&EP2_InputUartPin[0],      //  EP2     platform
};







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
	int Port;
	int Pin;

	memcpy(InputUartPin, pInputUartPin[Hw], sizeof(EP2_InputUartPin));
	if (memcmp((const void*)InputUartPin,(const void*)pInputUartPin[Hw],sizeof(EP2_InputUartPin)) != 0)
	{
		printk("%s InputUartPin table broken!\n",MODULE_NAME);
	}

	for (Port = 0; Port < INPUTS; Port++)
	{
		for (Pin = 0; Pin < INPUT_UART_PINS; Pin++)
		{
			if (InputUartPin[Port][Pin].Pin >= 0)
			{
				if((InputUartPin[Port][Pin].Pin) >= 0 && (InputUartPin[Port][Pin].Pin) < 128)
				{
					if ((InputUartPin[Port][Pin].Pin) < 32)
					{
						InputUartPin[Port][Pin].pGpio  =  GPIOBANK0;
					}
					else if ((InputUartPin[Port][Pin].Pin) < 64)
					{
						InputUartPin[Port][Pin].pGpio  =  GPIOBANK1;
					}
					else if ((InputUartPin[Port][Pin].Pin) < 96)
					{
						InputUartPin[Port][Pin].pGpio  =  GPIOBANK2;
					}
					else
					{
						InputUartPin[Port][Pin].pGpio  =  GPIOBANK3;
					}
				InputUartPin[Port][Pin].Mask   =  (1 << (InputUartPin[Port][Pin].Pin & 0x1F));
				SetGpio(InputUartPin[Port][Pin].Pin);
				}
      			}
    		}
  	}
}


#define   PUARTFloat(port,pin)          {\
                                          (InputUartPin[port][pin].pGpio)[GPIO_OE] |=  InputUartPin[port][pin].Mask;\
                                        }


#define   PUARTRead(port,pin)           ((InputUartPin[port][pin].pGpio)[GPIO_DATAIN] & InputUartPin[port][pin].Mask)


#define   PUARTHigh(port,pin)           {\
                                          (InputUartPin[port][pin].pGpio)[GPIO_SETDATAOUT]  =  InputUartPin[port][pin].Mask;\
                                          (InputUartPin[port][pin].pGpio)[GPIO_OE]      &= ~InputUartPin[port][pin].Mask;\
                                        }

#define   PUARTLow(port,pin)            {\
                                          (InputUartPin[port][pin].pGpio)[GPIO_CLEARDATAOUT]  =  InputUartPin[port][pin].Mask;\
                                          (InputUartPin[port][pin].pGpio)[GPIO_OE]      &= ~InputUartPin[port][pin].Mask;\
                                        }

volatile ULONG *Base[INPUTS];
UBYTE IntSuccess[INPUTS];

#define UART_CLOCK1  48000000
#define UART_CLOCK2  48000000
#define UART_CLOCK3  48000000
#define UART_CLOCK4  48000000

#define	  UART_RBR                      0	//RHR
#define   UART_THR                      0	//THR
#define   UART_DLL                      0
#define   UART_IER                      (0x4>>2)
#define   UART_DLH                      (0x4>>2)
#define   UART_EFR                      (0x8>>2)
#define   UART_IIR                      (0x8>>2)
#define   UART_FCR                   	(0x8>>2)
#define   UART_LCR                      (0xC>>2)
#define   UART_MCR                      (0x10>>2)
#define   UART_LSR                      (0x14>>2)
#define   UART_TCR                      (0x18>>2)
#define   UART_MSR                      (0x18>>2) 
#define   UART_TLR                      (0x1C>>2)
#define   UART_MDR1                     (0x20>>2)
#define   UART_MDR2                     (0x24>>2)
#define   UART_SCR                      (0x40>>2)

#define   UART_SYSC                     (0x54>>2)
#define   UART_SYSS                     (0x58>>2)

#define   UART_RECBUF_SIZE              256

#define   UARTBUFFERSIZE    250
static    char UartBuffer[UARTBUFFERSIZE];

static UBYTE UartPortSend(UBYTE Port,UBYTE Byte);

UWORD   ShowTimer[INPUTS];

#define   LOGPOOLSIZE       100000
ULONG     LogPointer = 0;
ULONG     LogOutPointer  =  0;
char      LogPool[LOGPOOLSIZE];

void      UartWrite(char *pString)
{
  ULONG   Tmp;

  while (*pString)
  {
    LogPool[LogPointer]  =  *pString;

    Tmp  =  LogPointer;
    Tmp++;
    if (Tmp >= LOGPOOLSIZE)
    {
      Tmp  =  0;
    }
    if (Tmp != LogOutPointer)
    {
      LogPointer  =  Tmp;
      pString++;

    }
    else
    {
//      if (UartPortSend(DEBUG_UART,*pString))
      {
        pString++;
      }
    }
  }
}

// UART 1 *********************************************************************

volatile  ULONG *Uart1Base;
static    char  Uart1Name[20];


static    UBYTE Uart1RecBuf[UART_RECBUF_SIZE];
static    UWORD Uart1RecBufIn;
static    UWORD Uart1RecBufOut;

static    UBYTE Uart1RecMesLng;
static    UBYTE Uart1RecMes[UART_BUFFER_SIZE];
static    UBYTE Uart1RecMesIn;


irqreturn_t Uart1Interrupt(int irq, void *dev_id)
{
	UBYTE IntrType;

	IntrType = (UBYTE)Uart1Base[UART_IIR] & 0x0F;
	
	while(!(IntrType & 1))
	{
		if(IntrType == 2)
		{
		}
		else
		{
			if(IntrType & 2)
			{
				Uart1RecBuf[Uart1RecBufIn] = (UBYTE)Uart1Base[UART_LSR];
			}
			Uart1RecBuf[Uart1RecBufIn] = (UBYTE)Uart1Base[UART_RBR];

			if(++Uart1RecBufIn >= UART_RECBUF_SIZE)
			{
				Uart1RecBufIn = 0;
			}
		}
		IntrType = (UBYTE)Uart1Base[UART_IIR] & 0x0F;
	}


	return (IRQ_HANDLED);

}



static    UBYTE Uart1Read(UBYTE *pByte)
{
  UBYTE   Result = 0;

  if (Uart1RecBufIn != Uart1RecBufOut)
  {
    *pByte  =  Uart1RecBuf[Uart1RecBufOut];

    if (++Uart1RecBufOut >= UART_RECBUF_SIZE)
    {
      Uart1RecBufOut  =  0;
    }
    Result    =  1;
  }

  return (Result);
}


static    void Uart1Flush(void)
{
  Uart1Base[UART_FCR]   =  0x07;
  Uart1RecBufIn         =  0;
  Uart1RecBufOut        =  0;
  Uart1RecMesIn         =  0;
}


static UBYTE Uart1ReadData(UBYTE *pCmd,UBYTE *pData,UBYTE *pCheck,UBYTE *pFail)
{
  UBYTE   Byte;
  UBYTE   Length;
  UBYTE   Collect;

  Length    =  0;
  *pFail    =  0xFF;
  Collect   =  1;

  while (Collect)
  {
    if (Uart1Read(&Byte))
    {
      if (Uart1RecMesIn == 0)
      { // Wait for data message start

        if (GET_MESSAGE_TYPE(Byte) == MESSAGE_DATA)
        {

          Uart1RecMesLng  =  GET_MESSAGE_LENGTH(Byte) + 2;

          if (Uart1RecMesLng <= UART_BUFFER_SIZE)
          { // Valid length

            Uart1RecMes[Uart1RecMesIn]  =  Byte;
            Uart1RecMesIn++;
          }
        }
      }
      else
      {
        Uart1RecMes[Uart1RecMesIn]  =  Byte;

        if (++Uart1RecMesIn >= Uart1RecMesLng)
        { // Message ready

          *pCmd    =  Uart1RecMes[0];
          *pFail  ^=  *pCmd;

          while (Length < (Uart1RecMesLng - 2))
          {
            pData[Length]  = Uart1RecMes[Length + 1];
            *pFail       ^=  pData[Length];
            Length++;
          }
          *pCheck  =  Uart1RecMes[Length + 1];
          *pFail  ^=  *pCheck;

          Uart1RecMesIn  =  0;
          Collect  =  0;
        }
      }
    }
    else
    {
      Collect  =  0;
    }
  }

  return (Length);
}


static UBYTE Uart1Write(UBYTE Byte)
{
  UBYTE   Result = 0;

  if (Uart1Base[UART_LSR] & 0x20)
  {
    Uart1Base[UART_THR]   =  Byte;
    Result                =  1;
  }

  return (Result);
}


static void Uart1Setup(ULONG BitRate)
{
	ULONG   Divisor;

	Divisor = (ULONG)UART_CLOCK1 / (BitRate * (ULONG)16);

	
	Uart1Base[UART_LCR] = 0xBF;
	Uart1Base[UART_EFR] |= 0x10;	//Enables access to MCR
		
	Uart1Base[UART_LCR] = 0x80;
	Uart1Base[UART_MCR] |= 0x40;	//Enables access to TCR and TLR

	Uart1Base[UART_LCR] = 0xBF;
	Uart1Base[UART_SCR] |= 0x80;	//Enable granularity of 1 for trigger RX level
	Uart1Base[UART_SCR] &= ~(0x40);	//Disable granularity of 1 for trigger TX level
	Uart1Base[UART_TLR] = 0x0;

	Uart1Base[UART_SCR] |= 0x1;	//DMAMODE is set with SCR[2:1]
	Uart1Base[UART_SCR] &= ~(0x6);	//no DMA


	Uart1Base[UART_LCR] = 0x7F;
	Uart1Base[UART_IER] &= ~(0x10); //Disables sleep mode
	

	Uart1Base[UART_LCR] = 0xBF;

	Uart1Base[UART_MDR1] |= 0x7;	//Disable uart
	Uart1Base[UART_DLL] = 0x0;
	Uart1Base[UART_DLH] = 0x0;
	//Uart1Base[UART_MDR1] &= ~(0x7);	//uart 16x mode


	Uart1Base[UART_LCR] = 0x80;
	Uart1Base[UART_FCR] = (1 << 6) | 0x7;



	Uart1Base[UART_LCR] = 0xBF;

	//Uart1Base[UART_MDR1] |= 0x7;	//Disable uart
	Uart1Base[UART_DLL] = Divisor & 0xFF;
	Uart1Base[UART_DLH] = (Divisor & 0x3F00) >> 8;
	Uart1Base[UART_MDR1] &= ~(0x7);	//uart 16x mode

	
	Uart1Base[UART_LCR] = 0x3;
	Uart1Base[UART_IER] = 0x1;
}









static int Uart1Init(void)
{
	int Result = -1;

	Uart1Base = NULL;
	IntSuccess[Uart1] = 0;

	if(UartBaseAddr[Uart1])
	{
		snprintf(Uart1Name,20,"%s.port%d",DEVICE1_NAME,Uart1 + 1);
		GetPeriphealBasePtr(UartBaseAddr[Uart1], 0x88, (ULONG **)&Uart1Base);
		Base[Uart1] = Uart1Base;
		if(Uart1Base != NULL)
		{
			Result = 0;
			
			Uart1Base[UART_SYSC] |= 0x2;
			while(!(Uart1Base[UART_SYSS] & 0x1))
				;

			if(request_irq(UartIntr[Uart1] + 16, &Uart1Interrupt, IRQF_SHARED, Uart1Name, &Uart1Name) >=0)
			{
				IntSuccess[Uart1] = 1;

			}
			else
			{
				Result = -EBUSY;
			}
		}

	}
	return (Result);
}





void      Uart1Exit(void)
{
  Uart1Base[UART_IER]   =  0x00;

  if (Uart1Base != NULL)
  {
    if (IntSuccess[Uart1])
    {
      free_irq(UartIntr[Uart1] + 16,Uart1Name);
    }
    iounmap(Uart1Base);
  }
}


// UART 2 *********************************************************************

typedef struct
{
  ULONG   InfoData;
  ULONG   BitRate;
  ULONG   BitRateMax;
  UWORD   Timer;
  UWORD   WatchDog;
  UWORD   BreakTimer;
  UBYTE   Initialised;
  UBYTE   ChangeMode;
  UBYTE   State;
  UBYTE   OldState;
  UBYTE   SubState;
  UBYTE   Cmd;
  UBYTE   InfoCmd;
  UBYTE   Check;
  UBYTE   Types;
  UBYTE   Views;
  UBYTE   Mode;
  UBYTE   Type;
  UBYTE   DataOk;
  UBYTE   DataErrors;
  SBYTE   Name[TYPE_NAME_LENGTH + 1];
  UBYTE   InLength;
  UBYTE   InPointer;
  UBYTE   OutLength;
  UBYTE   OutPointer;
  UBYTE   InBuffer[UART_BUFFER_SIZE];
  UBYTE   OutBuffer[UART_BUFFER_SIZE];
}
UARTPORT;


UARTPORT  UartPort[INPUTS];

volatile  ULONG *Uart2Base;
static    char  Uart2Name[20];

static    UBYTE Uart2RecBuf[UART_RECBUF_SIZE];
static    UWORD Uart2RecBufIn;
static    UWORD Uart2RecBufOut;

static    UBYTE Uart2RecMesLng;
static    UBYTE Uart2RecMes[UART_BUFFER_SIZE];
static    UBYTE Uart2RecMesIn;


irqreturn_t Uart2Interrupt(int irq, void *dev_id)
{
	static int liu_count = 0;
	int i;
	
	UBYTE IntrType;

	IntrType = (UBYTE)Uart2Base[UART_IIR] & 0x0F;
	
	while(!(IntrType & 1))
	{
		if(IntrType == 2)
		{
		}
		else
		{
			if(IntrType & 2)
			{
				Uart2RecBuf[Uart2RecBufIn] = (UBYTE)Uart2Base[UART_LSR];
			}
			Uart2RecBuf[Uart2RecBufIn] = (UBYTE)Uart2Base[UART_RBR];
			
			liu_data[pliu].rx_tx = 'r';
			liu_data[pliu].state = UartPort[1].State;
			liu_data[pliu].bitrate = UartPort[1].BitRate;
			liu_data[pliu].data = Uart2RecBuf[Uart2RecBufIn];
			if(pliu++ > 1000)
				pliu = 1000;

			if(++Uart2RecBufIn >= UART_RECBUF_SIZE)
			{
				Uart2RecBufIn = 0;
			}
		}
		IntrType = (UBYTE)Uart2Base[UART_IIR] & 0x0F;
	}

//	if(UartPort[1].State == 10)
	{
//		liu_count++;
//		if(liu_count == 100)
		if(liu_data[240].rx_tx != 0 && liu_count == 0)
		{
			liu_count = 1;
			for(i = 0; i < 250; i++)
			{
				printk("\nliu-----%d\trx_tx = %c\tstate = %d\tbit = %d\tdata = 0x%x\t%d",\
				i, liu_data[i].rx_tx, liu_data[i].state, liu_data[i].bitrate, liu_data[i].data, liu_data[i].data);
			}

		}
	}

	return (IRQ_HANDLED);

}



static    UBYTE Uart2Read(UBYTE *pByte)
{
  UBYTE   Result = 0;

  if (Uart2RecBufIn != Uart2RecBufOut)
  {
    *pByte  =  Uart2RecBuf[Uart2RecBufOut];

    if (++Uart2RecBufOut >= UART_RECBUF_SIZE)
    {
      Uart2RecBufOut  =  0;
    }
    Result    =  1;
  }

  return (Result);
}


static    void Uart2Flush(void)
{
  Uart2Base[UART_FCR]   |=  0x06;
  Uart2RecBufIn         =  0;
  Uart2RecBufOut        =  0;
  Uart2RecMesIn         =  0;
}


static UBYTE Uart2ReadData(UBYTE *pCmd,UBYTE *pData,UBYTE *pCheck,UBYTE *pFail)
{
  UBYTE   Byte;
  UBYTE   Length;
  UBYTE   Collect;

  Length    =  0;
  *pFail    =  0xFF;
  Collect   =  1;

  while (Collect)
  {

    if (Uart2Read(&Byte))
    {
      if (Uart2RecMesIn == 0)
      { // Wait for data message start

        if (GET_MESSAGE_TYPE(Byte) == MESSAGE_DATA)
        {

          Uart2RecMesLng  =  GET_MESSAGE_LENGTH(Byte) + 2;

          if (Uart2RecMesLng <= UART_BUFFER_SIZE)
          { // Valid length

            Uart2RecMes[Uart2RecMesIn]  =  Byte;
            Uart2RecMesIn++;
          }
        }
      }
      else
      {
        Uart2RecMes[Uart2RecMesIn]  =  Byte;

        if (++Uart2RecMesIn >= Uart2RecMesLng)
        { // Message ready

          *pCmd    =  Uart2RecMes[0];
          *pFail  ^=  *pCmd;

          while (Length < (Uart2RecMesLng - 2))
          {
            pData[Length]  = Uart2RecMes[Length + 1];
            *pFail       ^=  pData[Length];
            Length++;
          }
          *pCheck  =  Uart2RecMes[Length + 1];
          *pFail  ^=  *pCheck;

          Uart2RecMesIn  =  0;
          Collect        =  0;
        }
      }
    }
    else
    {
      Collect  =  0;
    }
  }

  return (Length);
}


static UBYTE Uart2Write(UBYTE Byte)
{
  UBYTE   Result = 0;

  if (Uart2Base[UART_LSR] & 0x20)
  {
    Uart2Base[UART_THR]   =  Byte;
    Result                =  1;
			liu_data[pliu].rx_tx = 't';
			liu_data[pliu].state = UartPort[1].State;
			liu_data[pliu].bitrate = UartPort[1].BitRate;
			liu_data[pliu].data = Byte;
			if(pliu++ > 1000)
				pliu = 1000;
  }

  return (Result);
}


static void Uart2Setup(ULONG BitRate)
{
	ULONG   Divisor;

	Divisor = (ULONG)UART_CLOCK2 / (BitRate * (ULONG)16);

	
	Uart2Base[UART_LCR] = 0xBF;
	Uart2Base[UART_EFR] |= 0x10;	//Enables access to MCR
		
	Uart2Base[UART_LCR] = 0x80;
	Uart2Base[UART_MCR] |= 0x40;	//Enables access to TCR and TLR

	Uart2Base[UART_LCR] = 0xBF;
	Uart2Base[UART_SCR] |= 0x80;	//Enable granularity of 1 for trigger RX level
	Uart2Base[UART_SCR] &= ~(0x40);	//Disable granularity of 1 for trigger TX level
	Uart2Base[UART_TLR] = 0x0;

	Uart2Base[UART_SCR] |= 0x1;	//DMAMODE is set with SCR[2:1]
	Uart2Base[UART_SCR] &= ~(0x6);	//no DMA


	Uart2Base[UART_LCR] = 0x7F;
	Uart2Base[UART_IER] &= ~(0x10); //Disables sleep mode
	

	Uart2Base[UART_LCR] = 0xBF;

	Uart2Base[UART_MDR1] |= 0x7;	//Disable uart
	Uart2Base[UART_DLL] = 0x0;
	Uart2Base[UART_DLH] = 0x0;
	//Uart2Base[UART_MDR1] &= ~(0x7);	//uart 16x mode


	Uart2Base[UART_LCR] = 0x80;
	Uart2Base[UART_FCR] = (1 << 6) | 0x7;



	Uart2Base[UART_LCR] = 0xBF;

	//Uart2Base[UART_MDR1] |= 0x7;	//Disable uart
	Uart2Base[UART_DLL] = Divisor & 0xFF;
	Uart2Base[UART_DLH] = (Divisor & 0x3F00) >> 8;
	Uart2Base[UART_MDR1] &= ~(0x7);	//uart 16x mode

	
	Uart2Base[UART_LCR] = 0x3;
	Uart2Base[UART_IER] = 0x1;
}


static int Uart2Init(void)
{
	int Result = -1;

	Uart2Base = NULL;
	IntSuccess[Uart2] = 0;

	if(UartBaseAddr[Uart2])
	{
		snprintf(Uart2Name,20,"%s.port%d",DEVICE1_NAME,Uart2 + 1);
		GetPeriphealBasePtr(UartBaseAddr[Uart2], 0x88, (ULONG **)&Uart2Base);
		Base[Uart2] = Uart2Base;
		if(Uart2Base != NULL)
		{
			Result = 0;

			Uart2Base[UART_SYSC] |= 0x2;
			while(!(Uart2Base[UART_SYSS] & 0x1))
				;
			
			if(request_irq(UartIntr[Uart2] + 16, &Uart2Interrupt, IRQF_SHARED, Uart2Name, &Uart2Name) >= 0)
			{
				IntSuccess[Uart2] = 1;
			}
			else
			{
				Result = -EBUSY;
			}
		}


	}
	return (Result);
}





void      Uart2Exit(void)
{
  Uart2Base[UART_IER]   =  0x00;
  if (Uart2Base != NULL)
  {
    if (IntSuccess[Uart2])
    {
      free_irq(UartIntr[Uart2] + 16,Uart2Name);
    }
    iounmap(Uart2Base);
  }
}






//Uart 3 **************************************************************************************

volatile ULONG *Uart3Base;
static char Uart3Name[20];

static    UBYTE Uart3RecBuf[UART_RECBUF_SIZE];
static    UWORD Uart3RecBufIn;
static    UWORD Uart3RecBufOut;

static    UBYTE Uart3RecMesLng;
static    UBYTE Uart3RecMes[UART_BUFFER_SIZE];
static    UBYTE Uart3RecMesIn;


irqreturn_t Uart3Interrupt(int irq, void *dev_id)
{
	UBYTE IntrType;

	IntrType = (UBYTE)Uart3Base[UART_IIR] & 0x0F;
	
	while(!(IntrType & 1))
	{
		if(IntrType == 2)
		{
		}
		else
		{
			if(IntrType & 2)
			{
				Uart3RecBuf[Uart3RecBufIn] = (UBYTE)Uart3Base[UART_LSR];
			}
			Uart3RecBuf[Uart3RecBufIn] = (UBYTE)Uart3Base[UART_RBR];

			if(++Uart3RecBufIn >= UART_RECBUF_SIZE)
			{
				Uart3RecBufIn = 0;
			}
		}
		IntrType = (UBYTE)Uart3Base[UART_IIR] & 0x0F;
	}

	return (IRQ_HANDLED);

}



static    UBYTE Uart3Read(UBYTE *pByte)
{
  UBYTE   Result = 0;

  if (Uart3RecBufIn != Uart3RecBufOut)
  {
    *pByte  =  Uart3RecBuf[Uart3RecBufOut];

    if (++Uart3RecBufOut >= UART_RECBUF_SIZE)
    {
      Uart3RecBufOut  =  0;
    }
    Result    =  1;
  }

  return (Result);
}




static    void Uart3Flush(void)
{
  Uart3Base[UART_FCR]   =  0x07;
  Uart3RecBufIn         =  0;
  Uart3RecBufOut        =  0;
  Uart3RecMesIn         =  0;
}






static UBYTE Uart3ReadData(UBYTE *pCmd,UBYTE *pData,UBYTE *pCheck,UBYTE *pFail)
{
  UBYTE   Byte;
  UBYTE   Length;
  UBYTE   Collect;

  Length    =  0;
  *pFail    =  0xFF;
  Collect   =  1;

  while (Collect)
  {
    if (Uart3Read(&Byte))
    {
      if (Uart3RecMesIn == 0)
      { // Wait for data message start

        if (GET_MESSAGE_TYPE(Byte) == MESSAGE_DATA)
        {

          Uart3RecMesLng  =  GET_MESSAGE_LENGTH(Byte) + 2;

          if (Uart3RecMesLng <= UART_BUFFER_SIZE)
          { // Valid length

            Uart3RecMes[Uart3RecMesIn]  =  Byte;
            Uart3RecMesIn++;
          }
        }
      }
      else
      {
        Uart3RecMes[Uart3RecMesIn]  =  Byte;

        if (++Uart3RecMesIn >= Uart3RecMesLng)
        { // Message ready

          *pCmd    =  Uart3RecMes[0];
          *pFail  ^=  *pCmd;

          while (Length < (Uart3RecMesLng - 2))
          {
            pData[Length]  = Uart3RecMes[Length + 1];
            *pFail       ^=  pData[Length];
            Length++;
          }
          *pCheck  =  Uart3RecMes[Length + 1];
          *pFail  ^=  *pCheck;

          Uart3RecMesIn  =  0;
          Collect  =  0;
        }
      }
    }
    else
    {
      Collect  =  0;
    }
  }

  return (Length);
}


static UBYTE Uart3Write(UBYTE Byte)
{
  UBYTE   Result = 0;

  if (Uart3Base[UART_LSR] & 0x20)
  {
    Uart3Base[UART_THR]   =  Byte;
    Result                =  1;
  }

  return (Result);
}


static void Uart3Setup(ULONG BitRate)
{
	ULONG   Divisor;

	Divisor = (ULONG)UART_CLOCK2 / (BitRate * (ULONG)16);

	
	Uart3Base[UART_LCR] = 0xBF;
	Uart3Base[UART_EFR] |= 0x10;	//Enables access to MCR
		
	Uart3Base[UART_LCR] = 0x80;
	Uart3Base[UART_MCR] |= 0x40;	//Enables access to TCR and TLR

	Uart3Base[UART_LCR] = 0xBF;
	Uart3Base[UART_SCR] |= 0x80;	//Enable granularity of 1 for trigger RX level
	Uart3Base[UART_SCR] &= ~(0x40);	//Disable granularity of 1 for trigger TX level
	Uart3Base[UART_TLR] = 0x0;

	Uart3Base[UART_SCR] |= 0x1;	//DMAMODE is set with SCR[2:1]
	Uart3Base[UART_SCR] &= ~(0x6);	//no DMA


	Uart3Base[UART_LCR] = 0x7F;
	Uart3Base[UART_IER] &= ~(0x10); //Disables sleep mode
	

	Uart3Base[UART_LCR] = 0xBF;

	Uart3Base[UART_MDR1] |= 0x7;	//Disable uart
	Uart3Base[UART_DLL] = 0x0;
	Uart3Base[UART_DLH] = 0x0;
	//Uart3Base[UART_MDR1] &= ~(0x7);	//uart 16x mode


	Uart3Base[UART_LCR] = 0x80;
	Uart3Base[UART_FCR] = (1 << 6) | 0x7;



	Uart3Base[UART_LCR] = 0xBF;

	//Uart3Base[UART_MDR1] |= 0x7;	//Disable uart
	Uart3Base[UART_DLL] = Divisor & 0xFF;
	Uart3Base[UART_DLH] = (Divisor & 0x3F00) >> 8;
	Uart3Base[UART_MDR1] &= ~(0x7);	//uart 16x mode

	
	Uart3Base[UART_LCR] = 0x3;
	Uart3Base[UART_IER] = 0x1;

}


static int Uart3Init(void)
{
	int Result = -1;

	Uart3Base = NULL;
	IntSuccess[Uart3] = 0;

	if(UartBaseAddr[Uart3])
	{
		snprintf(Uart3Name,20,"%s.port%d",DEVICE1_NAME,Uart3 + 1);
		GetPeriphealBasePtr(UartBaseAddr[Uart3], 0x88, (ULONG **)&Uart3Base);
		Base[Uart3] = Uart3Base;
		if(Uart3Base != NULL)
		{
			Result = 0;
			
			Uart3Base[UART_SYSC] |= 0x2;
			while(!(Uart3Base[UART_SYSS] & 0x1))
				;

			if(request_irq(UartIntr[Uart3] + 16, &Uart3Interrupt, IRQF_SHARED, Uart3Name, &Uart3Name) >=0)
			{
				IntSuccess[Uart3] = 1;

			}
			else
			{
				Result = -EBUSY;
			}
		}

	}
	return (Result);
}


void      Uart3Exit(void)
{
  Uart3Base[UART_IER]   =  0x00;

  if (Uart3Base != NULL)
  {
    if (IntSuccess[Uart3])
    {
      free_irq(UartIntr[Uart3] + 16,Uart3Name);
    }
    iounmap(Uart3Base);
  }
}


// UART 4 *********************************************************************

volatile ULONG *Uart4Base;
static char Uart4Name[20];

static    UBYTE Uart4RecBuf[UART_RECBUF_SIZE];
static    UWORD Uart4RecBufIn;
static    UWORD Uart4RecBufOut;

static    UBYTE Uart4RecMesLng;
static    UBYTE Uart4RecMes[UART_BUFFER_SIZE];
static    UBYTE Uart4RecMesIn;


irqreturn_t Uart4Interrupt(int irq, void *dev_id)
{
	UBYTE IntrType;

	IntrType = (UBYTE)Uart4Base[UART_IIR] & 0x0F;
	
	while(!(IntrType & 1))
	{
		if(IntrType == 2)
		{
		}
		else
		{
			if(IntrType & 2)
			{
				Uart4RecBuf[Uart4RecBufIn] = (UBYTE)Uart4Base[UART_LSR];
			}
			Uart4RecBuf[Uart4RecBufIn] = (UBYTE)Uart4Base[UART_RBR];

			if(++Uart4RecBufIn >= UART_RECBUF_SIZE)
			{
				Uart4RecBufIn = 0;
			}
		}
		IntrType = (UBYTE)Uart4Base[UART_IIR] & 0x0F;
	}

	return (IRQ_HANDLED);

}



static    UBYTE Uart4Read(UBYTE *pByte)
{
  UBYTE   Result = 0;

  if (Uart4RecBufIn != Uart4RecBufOut)
  {
    *pByte  =  Uart4RecBuf[Uart4RecBufOut];

    if (++Uart4RecBufOut >= UART_RECBUF_SIZE)
    {
      Uart4RecBufOut  =  0;
    }
    Result    =  1;
  }

  return (Result);
}




static    void Uart4Flush(void)
{
  Uart4Base[UART_FCR]   =  0x07;
  Uart4RecBufIn         =  0;
  Uart4RecBufOut        =  0;
  Uart4RecMesIn         =  0;
}


static UBYTE Uart4ReadData(UBYTE *pCmd,UBYTE *pData,UBYTE *pCheck,UBYTE *pFail)
{
  UBYTE   Byte;
  UBYTE   Length;
  UBYTE   Collect;

  Length    =  0;
  *pFail    =  0xFF;
  Collect   =  1;

  while (Collect)
  {

    if (Uart4Read(&Byte))
    {
      if (Uart4RecMesIn == 0)
      { // Wait for data message start

        if (GET_MESSAGE_TYPE(Byte) == MESSAGE_DATA)
        {

          Uart4RecMesLng  =  GET_MESSAGE_LENGTH(Byte) + 2;

          if (Uart4RecMesLng <= UART_BUFFER_SIZE)
          { // Valid length

            Uart4RecMes[Uart4RecMesIn]  =  Byte;
            Uart4RecMesIn++;
          }
        }
      }
      else
      {
        Uart4RecMes[Uart4RecMesIn]  =  Byte;

        if (++Uart4RecMesIn >= Uart4RecMesLng)
        { // Message ready

          *pCmd    =  Uart4RecMes[0];
          *pFail  ^=  *pCmd;

          while (Length < (Uart4RecMesLng - 2))
          {
            pData[Length]  = Uart4RecMes[Length + 1];
            *pFail       ^=  pData[Length];
            Length++;
          }
          *pCheck  =  Uart4RecMes[Length + 1];
          *pFail  ^=  *pCheck;

          Uart4RecMesIn  =  0;
          Collect        =  0;
        }
      }
    }
    else
    {
      Collect  =  0;
    }
  }

  return (Length);
}


static UBYTE Uart4Write(UBYTE Byte)
{
  UBYTE   Result = 0;

  if (Uart4Base[UART_LSR] & 0x20)
  {
    Uart4Base[UART_THR]   =  Byte;
    Result                =  1;
  }

  return (Result);
}


static void Uart4Setup(ULONG BitRate)
{
	ULONG   Divisor;

	Divisor = (ULONG)UART_CLOCK2 / (BitRate * (ULONG)16);

	
	Uart4Base[UART_LCR] = 0xBF;
	Uart4Base[UART_EFR] |= 0x10;	//Enables access to MCR
		
	Uart4Base[UART_LCR] = 0x80;
	Uart4Base[UART_MCR] |= 0x40;	//Enables access to TCR and TLR

	Uart4Base[UART_LCR] = 0xBF;
	Uart4Base[UART_SCR] |= 0x80;	//Enable granularity of 1 for trigger RX level
	Uart4Base[UART_SCR] &= ~(0x40);	//Disable granularity of 1 for trigger TX level
	Uart4Base[UART_TLR] = 0x0;

	Uart4Base[UART_SCR] |= 0x1;	//DMAMODE is set with SCR[2:1]
	Uart4Base[UART_SCR] &= ~(0x6);	//no DMA


	Uart4Base[UART_LCR] = 0x7F;
	Uart4Base[UART_IER] &= ~(0x10); //Disables sleep mode
	

	Uart4Base[UART_LCR] = 0xBF;

	Uart4Base[UART_MDR1] |= 0x7;	//Disable uart
	Uart4Base[UART_DLL] = 0x0;
	Uart4Base[UART_DLH] = 0x0;
	//Uart4Base[UART_MDR1] &= ~(0x7);	//uart 16x mode


	Uart4Base[UART_LCR] = 0x80;
	Uart4Base[UART_FCR] = (1 << 6) | 0x7;



	Uart4Base[UART_LCR] = 0xBF;

	//Uart4Base[UART_MDR1] |= 0x7;	//Disable uart
	Uart4Base[UART_DLL] = Divisor & 0xFF;
	Uart4Base[UART_DLH] = (Divisor & 0x3F00) >> 8;
	Uart4Base[UART_MDR1] &= ~(0x7);	//uart 16x mode

	
	Uart4Base[UART_LCR] = 0x3;
	Uart4Base[UART_IER] = 0x1;

}


static int Uart4Init(void)
{
	int Result = -1;

	Uart4Base = NULL;
	IntSuccess[Uart4] = 0;

	if(UartBaseAddr[Uart4])
	{
		snprintf(Uart4Name,20,"%s.port%d",DEVICE1_NAME,Uart4 + 1);
		GetPeriphealBasePtr(UartBaseAddr[Uart4], 0x88, (ULONG **)&Uart4Base);
		Base[Uart4] = Uart4Base;
		if(Uart4Base != NULL)
		{
			Result = 0;

			Uart4Base[UART_SYSC] |= 0x2;
			while(!(Uart4Base[UART_SYSS] & 0x1))
				;

			if(request_irq(UartIntr[Uart4] + 16, &Uart4Interrupt, IRQF_SHARED, Uart4Name, &Uart4Name) >=0)
			{
				IntSuccess[Uart4] = 1;

			}
			else
			{
				Result = -EBUSY;
			}
		}

	}
	return (Result);
}


void      Uart4Exit(void)
{
  Uart4Base[UART_IER]   =  0x00;

  if (Uart4Base != NULL)
  {
    if (IntSuccess[Uart4])
    {
      free_irq(UartIntr[Uart4] + 16,Uart4Name);
    }
    iounmap(Uart4Base);
  }
}



// DEVICE1 ********************************************************************


#define   INFODATA_INIT                 0x00000000L
#define   INFODATA_CMD_TYPE             0x00000001L
#define   INFODATA_CMD_MODES            0x00000002L
#define   INFODATA_CMD_SPEED            0x00000004L

#define   INFODATA_INFO_NAME            0x00000100L
#define   INFODATA_INFO_RAW             0x00000200L
#define   INFODATA_INFO_PCT             0x00000400L
#define   INFODATA_INFO_SI              0x00000800L
#define   INFODATA_INFO_SYMBOL          0x00001000L
#define   INFODATA_INFO_FORMAT          0x00002000L

#define   INFODATA_CLEAR                (~(INFODATA_INFO_NAME | INFODATA_INFO_RAW | INFODATA_INFO_PCT | INFODATA_INFO_SI | INFODATA_INFO_SYMBOL | INFODATA_INFO_FORMAT))

#define   INFODATA_NEEDED               (INFODATA_CMD_TYPE | INFODATA_CMD_MODES | INFODATA_INFO_NAME | INFODATA_INFO_FORMAT)


enum      UART_STATE
{
  UART_IDLE,
  UART_INIT,
  UART_RESTART,
  UART_ENABLE,
  UART_FLUSH,
  UART_SYNC,
  UART_MESSAGE_START,
  UART_ESCAPE,
  UART_CMD,
  UART_INFO,
  UART_DATA,
  UART_DATA_COPY,
  UART_ACK_WAIT,
  UART_ACK_INFO,
  UART_CMD_ERROR,
  UART_INFO_ERROR,
  UART_TERMINAL,
  UART_DATA_ERROR,
  UART_ERROR,
  UART_EXIT,
  UART_STATES
};


char      UartStateText[UART_STATES][50] =
{
  "IDLE\n",
  "INIT",
  "UART_RESTART",
  "ENABLE",
  "FLUSH",
  "SYNC",
  "MESSAGE_START",
  "ESCAPE",
  "CMD",
  "INFO",
  "DATA",
  "DATA_COPY",
  "ACK_WAIT",
  "ACK_INFO",
  "CMD_ERROR",
  "INFO_ERROR",
  "TERMINAL",
  "DATA_ERROR",
  "ERROR",
  "EXIT"
};

UARTPORT  UartPortDefault =
{
  INFODATA_INIT,            // InfoData
  (ULONG)LOWEST_BITRATE,    // BitRate
  (ULONG)LOWEST_BITRATE,    // BitRateMax
  0,                        // Timer
  0,                        // WatchDog
  0,                        // BreakTimer
  0,                        // Initialised
  0,                        // ChangeMode
  UART_IDLE,                // State
  -1,                       // OldState
  0,                        // SubState
  0,                        // Cmd
  0,                        // InfoCmd
  0,                        // Check
  0,                        // Types
  0,                        // Views
  0,                        // Mode
  TYPE_UNKNOWN,             // Type
  0,                        // DataOk
  0,                        // DataErrors
  "",                       // Name
  0,                        // InLength
  0,                        // InPointer
  0,                        // OutLength
  0,                        // OutPointer
};

TYPES     TypeData[INPUTS][MAX_DEVICE_MODES]; //!< TypeData
DATA8     Changed[INPUTS][MAX_DEVICE_MODES];


#define   UART_TIMER_RESOLUTION         10                // [100uS]

#define   UART_BREAK_TIME               1000              // [100uS]
#define   UART_TERMINAL_DELAY           20000             // [100uS]
#define   UART_CHANGE_BITRATE_DELAY     100               // [100uS]
#define   UART_ACK_DELAY                100               // [100uS]
#define   UART_SHOW_TIME                2500              // [100uS]

#define   UART_WATCHDOG_TIME            1000              // [100uS]

#define   UART_ALLOWABLE_DATA_ERRORS    6

UBYTE     UartConfigured[INPUTS];
//UARTPORT  UartPort[INPUTS];

static    UART UartDefault;
static    UART *pUart = &UartDefault;

static    struct hrtimer Device1Timer;
static    ktime_t        Device1Time;

static    UBYTE TestMode = 0;

static void UartPortDisable(UBYTE Port)
{
  switch (Port)
  {
    case Uart1 :
    {
    }
    break;

    case Uart2 :
    {
    }
    break;

    case Uart3 :
    {
    }
    break;

    case Uart4 :
    {
    }
    break;

  }
  PUARTHigh(Port,INPUT_UART_BUFFER);
}


static void UartPortFlush(UBYTE Port)
{
  switch (Port)
  {
    case Uart1 :
    {
      Uart1Flush();
    }
    break;

    case Uart2 :
    {
      Uart2Flush();
    }
    break;

    case Uart3 :
    {
      Uart3Flush();
    }
    break;

    case Uart4 :
    {
     Uart4Flush();
    }
    break;

  }
}


static void UartPortEnable(UBYTE Port)
{
  SetGpio(InputUartPin[Port][INPUT_UART_TXD].Pin);
  PUARTLow(Port,INPUT_UART_BUFFER);

  switch (Port)
  {
    case Uart1 :
    {
      Uart1Flush();
    }
    break;

    case Uart2 :
    {
      Uart2Flush();
    }
    break;

    case Uart3 :
    {
      Uart3Flush();
    }
    break;

    case Uart4 :
    {
      Uart4Flush();
    }
    break;

  }
}


static UBYTE UartPortSend(UBYTE Port,UBYTE Byte)
{
  UBYTE   Result = 1;

  switch (Port)
  {
    case Uart1 :
    {
      Result  =  Uart1Write(Byte);
    }
    break;

    case Uart2 :
    {
      Result  =  Uart2Write(Byte);
    }
    break;

    case Uart3 :
    {
      Result  =  Uart3Write(Byte);
    }
    break;

    case Uart4 :
    {
      Result  =  Uart4Write(Byte);
    }
    break;


  }

  return (Result);
}


static UBYTE UartPortReceive(UBYTE Port,UBYTE *pByte)
{
  UBYTE   Result = 0;

  switch (Port)
  {
    case Uart1 :
    {
      Result  =  Uart1Read(pByte);
    }
    break;

    case Uart2 :
    {
      Result  =  Uart2Read(pByte);
    }
    break;

    case Uart3 :
    {
      Result  =  Uart3Read(pByte);
    }
    break;

    case Uart4 :
    {
      Result  =  Uart4Read(pByte);
    }
    break;

  }

  return (Result);
}


static UBYTE UartPortReadData(UBYTE Port,UBYTE *pCmd,UBYTE *pData,UBYTE *pCheck,UBYTE *pFail)
{
  UBYTE   Result = 0;

  switch (Port)
  {
    case Uart1 :
    {
      Result  =  Uart1ReadData(pCmd,pData,pCheck,pFail);
    }
    break;

    case Uart2 :
    {
      Result  =  Uart2ReadData(pCmd,pData,pCheck,pFail);
    }
    break;

    case Uart3 :
    {
      Result  =  Uart3ReadData(pCmd,pData,pCheck,pFail);
    }
    break;

    case Uart4 :
    {
      Result  =  Uart4ReadData(pCmd,pData,pCheck,pFail);
    }
    break;

  }

  return (Result);
}


static void UartPortSetup(UBYTE Port,ULONG BitRate)
{

  switch (Port)
  {
    case Uart1 :
    {
      Uart1Setup(BitRate);
    }
    break;

    case Uart2 :
    {
      Uart2Setup(BitRate);
    }
    break;

    case Uart3 :
    {
      Uart3Setup(BitRate);
    }
    break;

    case Uart4 :
    {
      Uart4Setup(BitRate);
    }
    break;

  }
}


UBYTE     WriteRequest[INPUTS];


static enum hrtimer_restart Device1TimerInterrupt1(struct hrtimer *pTimer)
{
  UBYTE   Port;
  UBYTE   Byte;
  UBYTE   CrcError = 0;
  UBYTE   Tmp = 0;
  ULONG   TmpL;
  UBYTE   Chksum;
  UBYTE   Pointer;
  UBYTE   Mode;
  UBYTE   TmpBuffer[UART_DATA_LENGTH];
#ifdef  DEBUG_TRACE_US
  UWORD   In;
#endif
#ifdef  DEBUG_TRACE_ANGLE
  UWORD   Angle;
#endif

  hrtimer_forward_now(pTimer,Device1Time);

  for (Port = 0;Port < NO_OF_INPUT_PORTS;Port++)
  { // look at one port at a time

    if ((UartPort[Port].State > UART_ENABLE) && (!TestMode))
    { // If port active

      if (++UartPort[Port].BreakTimer >= (UART_BREAK_TIME / UART_TIMER_RESOLUTION))
      { // Reset state machine if break received

        if (PUARTRead(Port,INPUT_UART_PIN6))
        {

#ifdef DEBUG_D_UART_ERROR
          snprintf(UartBuffer,UARTBUFFERSIZE,"    %d BREAK\n",Port);
          UartWrite(UartBuffer);
#endif

          UartPortDisable(Port);
          UartPort[Port]                          =  UartPortDefault;
          UartPortEnable(Port);
          UartPortSetup(Port,UartPort[Port].BitRate);
          for (Tmp = 0;Tmp < MAX_DEVICE_MODES;Tmp++)
          {
            TypeData[Port][Tmp]           =  TypeDefaultUart[0];
            Changed[Port][Tmp]            =  0;
          }
#ifndef DISABLE_FAST_DATALOG_BUFFER
          (*pUart).Actual[Port]           =  0;
          (*pUart).LogIn[Port]            =  0;
#endif
          (*pUart).Status[Port]           =  0;
          UartPortFlush(Port);
          UartPort[Port].State            =  UART_SYNC;
        }
      }
      if (PUARTRead(Port,INPUT_UART_PIN6))
      {
        UartPort[Port].BreakTimer  =  0;
      }
    }

    if (Port != DEBUG_UART)
    { // If port not used for debug

  #ifdef DEBUG
      ShowTimer[Port]++;
  #endif

      if (!TestMode)
      {

        switch (UartPort[Port].State)
        { // Main state machine

          case UART_IDLE :
          { // Port inactive

            (*pUart).Status[Port] &= ~UART_WRITE_REQUEST;
            (*pUart).Status[Port] &= ~UART_DATA_READY;
            WriteRequest[Port]     =  0;
          }
          break;

          case UART_INIT :
          { // Initialise port hardware

            PUARTFloat(Port,INPUT_UART_PIN5);
            PUARTFloat(Port,INPUT_UART_PIN6);
            UartPort[Port].State        =  UART_ENABLE;
          }
          break;

          case UART_RESTART :
          {
            UartPortDisable(Port);
            UartPort[Port].State        =  UART_ENABLE;
          }
          break;

          case UART_ENABLE :
          { // Initialise port variables

            UartPort[Port]                          =  UartPortDefault;
            UartPortEnable(Port);
            UartPortSetup(Port,UartPort[Port].BitRate);
            for (Tmp = 0;Tmp < MAX_DEVICE_MODES;Tmp++)
            {
              TypeData[Port][Tmp]           =  TypeDefaultUart[0];
              Changed[Port][Tmp]            =  0;
            }
  #ifndef DISABLE_FAST_DATALOG_BUFFER
            (*pUart).Actual[Port]           =  0;
            (*pUart).LogIn[Port]            =  0;
  #endif
            (*pUart).Status[Port]           =  0;
            UartPortFlush(Port);
            UartPort[Port].State            =  UART_SYNC;
          }
          break;

          case UART_SYNC :
          { // Look for UART_CMD, TYPE in rolling buffer window

            if (UartPortReceive(Port,&Byte))
            {
              if (UartPort[Port].InPointer < 3)
              { // 0,1,2
                UartPort[Port].InBuffer[UartPort[Port].InPointer]  =  Byte;
                UartPort[Port].InPointer++;
              }
              if (UartPort[Port].InPointer >= 3)
              {
                // Validate
                UartPort[Port].Check      =  0xFF;
                for (Tmp = 0;Tmp < 2;Tmp++)
                {
                  UartPort[Port].Check   ^=  UartPort[Port].InBuffer[Tmp];
                }
                if ((UartPort[Port].Check == UartPort[Port].InBuffer[2]) && (UartPort[Port].InBuffer[0] == 0x40) && (UartPort[Port].InBuffer[1] > 0) && (UartPort[Port].InBuffer[1] <= MAX_VALID_TYPE))
                {
                  UartPort[Port].Type       =  UartPort[Port].InBuffer[1];
                  UartPort[Port].InfoData  |=  INFODATA_CMD_TYPE;
  #ifdef HIGHDEBUG
                  snprintf(UartBuffer,UARTBUFFERSIZE,"    %d TYPE   = %-3u\n",Port,(UWORD)UartPort[Port].Type & 0xFF);
                  UartWrite(UartBuffer);
  #endif
                  UartPort[Port].State      =  UART_MESSAGE_START;
                }
                else
                {
  #ifdef DEBUG_D_UART_ERROR
  //                snprintf(UartBuffer,UARTBUFFERSIZE,"[%02X]",Byte);
  //                UartWrite(UartBuffer);
  //                snprintf(UartBuffer,UARTBUFFERSIZE,"    %d No sync %02X %02X %02X\n",Port,UartPort[Port].InBuffer[0],UartPort[Port].InBuffer[1],UartPort[Port].InBuffer[2]);
  //                UartWrite(UartBuffer);
  #endif
                  for (Tmp = 0;Tmp < 2;Tmp++)
                  {
                    UartPort[Port].InBuffer[Tmp]  =  UartPort[Port].InBuffer[Tmp + 1];
                  }

                  UartPort[Port].InPointer--;
                }
              }
            }
            if ((++(UartPort[Port].Timer) >= (UART_TERMINAL_DELAY / UART_TIMER_RESOLUTION)))
            {
              UartPort[Port].BitRate      =  115200;
              UartPortSetup(Port,UartPort[Port].BitRate);
              TypeData[Port][0]           =  TypeDefaultUart[1];
              UartPort[Port].State        =  UART_TERMINAL;
              Changed[Port][0]            =  1;
              (*pUart).Status[Port]      |=  UART_PORT_CHANGED;
            }
          }
          break;

          default :
          { // Get sensor informations

            if (UartPortReceive(Port,&Byte))
            {

              switch (UartPort[Port].State)
              {

  //** INTERPRETER **************************************************************

                case UART_MESSAGE_START :
                {
                  UartPort[Port].InPointer  =  0;
                  UartPort[Port].SubState   =  0;
                  UartPort[Port].Check      =  0xFF;
                  UartPort[Port].Cmd        =  Byte;

                  switch (GET_MESSAGE_TYPE(Byte))
                  {
                    case MESSAGE_CMD :
                    {
                      UartPort[Port].InLength   =  GET_MESSAGE_LENGTH(Byte);
                      UartPort[Port].State      =  UART_CMD;
                    }
                    break;

                    case MESSAGE_INFO :
                    {
                      UartPort[Port].InLength   =  GET_MESSAGE_LENGTH(Byte);
                      UartPort[Port].State      =  UART_INFO;
                    }
                    break;

                    case MESSAGE_DATA :
                    {
                    }
                    break;

                    default :
                    {
                      switch (Byte)
                      {
                        case BYTE_ACK :
                        {
    #ifdef HIGHDEBUG
                          snprintf(UartBuffer,UARTBUFFERSIZE,"    %d ACK RECEIVED\n",Port);
                          UartWrite(UartBuffer);
    #endif
                          if (UartPort[Port].Types == 0)
                          {
                            if ((UartPort[Port].InfoData & INFODATA_NEEDED) == INFODATA_NEEDED)
                            {
                              UartPort[Port].Timer    =  0;
                              UartPort[Port].State    =  UART_ACK_WAIT;
                            }
                            else
                            {
                              UartPort[Port].State  =  UART_INFO_ERROR;
                            }
                          }
                          else
                          {
                            UartPort[Port].State  =  UART_INFO_ERROR;
                          }
                        }
                        break;

                        case BYTE_NACK :
                        {
                        }
                        break;

                        case BYTE_SYNC :
                        {
                        }
                        break;

                        default :
                        {
                          UartPort[Port].InLength   =  GET_MESSAGE_LENGTH(Byte);
                          UartPort[Port].State      =  UART_ESCAPE;
                        }
                        break;

                      }

                    }
                    break;

                  }
                }
                break;

                case UART_ESCAPE :
                {
                  if (UartPort[Port].InPointer < UartPort[Port].InLength)
                  {
                    UartPort[Port].InBuffer[UartPort[Port].InPointer]  =  Byte;
                    UartPort[Port].InPointer++;
                  }
                  else
                  { // Message complete

                    UartPort[Port].InBuffer[UartPort[Port].InPointer]  =  0;
                    UartPort[Port].State  =  UART_MESSAGE_START;
                  }
                }
                break;

  //** CMD **********************************************************************

                case UART_CMD :
                { // Command message in progress

                  if (UartPort[Port].InPointer < UartPort[Port].InLength)
                  {
                    UartPort[Port].InBuffer[UartPort[Port].InPointer]  =  Byte;
                    UartPort[Port].InPointer++;
                  }
                  else
                  { // Message complete

                    UartPort[Port].InBuffer[UartPort[Port].InPointer]  =  0;
                    UartPort[Port].State  =  UART_MESSAGE_START;

                    if (UartPort[Port].Check !=  Byte)
                    { // Check not correct
  #ifdef DEBUG_D_UART_ERROR
                      snprintf(UartBuffer,UARTBUFFERSIZE," c  %d %02X[",Port,UartPort[Port].Cmd & 0xFF);
                      UartWrite(UartBuffer);
                      for (Tmp = 0;Tmp < UartPort[Port].InLength;Tmp++)
                      {
                        snprintf(UartBuffer,UARTBUFFERSIZE,"%02X",UartPort[Port].InBuffer[Tmp] & 0xFF);
                        UartWrite(UartBuffer);
                      }
                      snprintf(UartBuffer,UARTBUFFERSIZE,"]%02X\n",UartPort[Port].Check & 0xFF);
                      UartWrite(UartBuffer);
  #endif
                      UartPort[Port].State  =  UART_CMD_ERROR;
                    }
                    else
                    { // Command message valid
  #ifdef HIGHDEBUG
                      snprintf(UartBuffer,UARTBUFFERSIZE,"    %d %02X[",Port,UartPort[Port].Cmd & 0xFF);
                      UartWrite(UartBuffer);
                      for (Tmp = 0;Tmp < UartPort[Port].InLength;Tmp++)
                      {
                        snprintf(UartBuffer,UARTBUFFERSIZE,"%02X",UartPort[Port].InBuffer[Tmp] & 0xFF);
                        UartWrite(UartBuffer);
                      }
                      snprintf(UartBuffer,UARTBUFFERSIZE,"]%02X\n",UartPort[Port].Check & 0xFF);
                      UartWrite(UartBuffer);
  #endif
                      switch (GET_CMD_COMMAND(UartPort[Port].Cmd))
                      { // Command message type

                        case CMD_MODES :
                        { // Number of modes

                          if ((UartPort[Port].InBuffer[0] >= 0) && (UartPort[Port].InBuffer[0] < MAX_DEVICE_MODES))
                          { // Number of modes valid

                            if ((UartPort[Port].InfoData & INFODATA_CMD_MODES))
                            { // Modes already given

  #ifdef DEBUG_D_UART_ERROR
                              snprintf(UartBuffer,UARTBUFFERSIZE," ## %d MODES ALREADY GIVEN\n",Port);
                              UartWrite(UartBuffer);
  #endif
                              UartPort[Port].State  =  UART_CMD_ERROR;
                            }
                            else
                            {

                              UartPort[Port].Types  =  UartPort[Port].InBuffer[0] + 1;
                              if (UartPort[Port].InLength >= 2)
                              { // Both modes and views present

                                UartPort[Port].Views  =  UartPort[Port].InBuffer[1] + 1;
  #ifdef HIGHDEBUG
                                snprintf(UartBuffer,UARTBUFFERSIZE,"    %d MODES  = %u  VIEWS  = %u\n",Port,(UWORD)UartPort[Port].Types & 0xFF,(UWORD)UartPort[Port].Views & 0xFF);
                                UartWrite(UartBuffer);
  #endif
                              }
                              else
                              { // Only modes present

                                UartPort[Port].Views  =  UartPort[Port].Types;
  #ifdef HIGHDEBUG
                                snprintf(UartBuffer,UARTBUFFERSIZE,"    %d MODES  = %u = VIEWS\n",Port,(UWORD)UartPort[Port].Types & 0xFF);
                                UartWrite(UartBuffer);
  #endif
                              }
                              UartPort[Port].InfoData |=  INFODATA_CMD_MODES;
                            }
                          }
                          else
                          { // Number of modes invalid
  #ifdef DEBUG_D_UART_ERROR
                            snprintf(UartBuffer,UARTBUFFERSIZE," ## %d MODES ERROR  %u\n",Port,(UWORD)UartPort[Port].Types & 0xFF);
                            UartWrite(UartBuffer);
  #endif
                            UartPort[Port].State  =  UART_CMD_ERROR;
                          }
                        }
                        break;

                        case CMD_SPEED :
                        { // Highest communication speed

                          TmpL  =  0;
                          for (Tmp = 0;Tmp < UartPort[Port].InLength;Tmp++)
                          {
                            TmpL |=  (ULONG)UartPort[Port].InBuffer[Tmp] << (8 * Tmp);
                          }

                          if ((TmpL >= LOWEST_BITRATE) && (TmpL <= HIGHEST_BITRATE))
                          { // Speed valid

                            if ((UartPort[Port].InfoData & INFODATA_CMD_SPEED))
                            { // Speed already given

  #ifdef DEBUG_D_UART_ERROR
                              snprintf(UartBuffer,UARTBUFFERSIZE," ## %d SPEED ALREADY GIVEN\n",Port);
                              UartWrite(UartBuffer);
  #endif
                              UartPort[Port].State  =  UART_CMD_ERROR;
                            }
                            else
                            {
                              if ((UartPort[Port].BitRate != LOWEST_BITRATE) && (TmpL <= MIDLE_BITRATE))
                              { // allow bit rate adjust
  #ifdef HIGHDEBUG
                                snprintf(UartBuffer,UARTBUFFERSIZE,"    %d SPEED ADJUST\n",Port);
                                UartWrite(UartBuffer);
  #endif
                                UartPort[Port].BitRateMax  =  (UartPort[Port].BitRate * TmpL) / LOWEST_BITRATE;
                              }
                              else
                              {

                                UartPort[Port].BitRateMax  =  TmpL;
                              }
  #ifdef HIGHDEBUG
                              snprintf(UartBuffer,UARTBUFFERSIZE,"    %d SPEED  = %lu\n",Port,(unsigned long)UartPort[Port].BitRateMax);
                              UartWrite(UartBuffer);
  #endif
                              UartPort[Port].InfoData |=  INFODATA_CMD_SPEED;
                            }
                          }
                          else
                          { // Speed invalid
  #ifdef DEBUG_D_UART_ERROR
                            snprintf(UartBuffer,UARTBUFFERSIZE," ## %d SPEED ERROR\n",Port);
                            UartWrite(UartBuffer);
  #endif
                            UartPort[Port].State  =  UART_CMD_ERROR;
                          }
                        }
                        break;

                      }
                    }
                  }
                }
                break;

  //** INFO *********************************************************************

                case UART_INFO :
                { // Info messages in progress

                  switch (UartPort[Port].SubState)
                  {
                    case 0 :
                    {
                      UartPort[Port].InfoCmd      =  Byte;

                      // validate length

                      switch (GET_INFO_COMMAND(UartPort[Port].InfoCmd))
                      {

                        case INFO_FORMAT :
                        {
                          if (UartPort[Port].InLength < 2)
                          {
  #ifdef DEBUG_D_UART_ERROR
                            snprintf(UartBuffer,UARTBUFFERSIZE," ## %d FORMAT ERROR\n",Port);
                            UartWrite(UartBuffer);
  #endif
                            UartPort[Port].State  =  UART_INFO_ERROR;
                          }
                        }
                        break;

                      }
                      UartPort[Port].SubState++;
                    }
                    break;

                    default :
                    {
                      if (UartPort[Port].InPointer < UartPort[Port].InLength)
                      { // Info message in progress

                        UartPort[Port].InBuffer[UartPort[Port].InPointer]  =  Byte;
                        UartPort[Port].InPointer++;
                      }
                      else
                      { // Message complete

                        UartPort[Port].InBuffer[UartPort[Port].InPointer]  =  0;
                        UartPort[Port].State  =  UART_MESSAGE_START;

                        if (UartPort[Port].Check !=  Byte)
                        {
  #ifdef DEBUG_D_UART_ERROR
                          snprintf(UartBuffer,UARTBUFFERSIZE," c  %d %02X ",Port,UartPort[Port].Cmd & 0xFF);
                          UartWrite(UartBuffer);
                          snprintf(UartBuffer,UARTBUFFERSIZE,"%02X[",UartPort[Port].InfoCmd & 0xFF);
                          UartWrite(UartBuffer);
                          for (Tmp = 0;Tmp < UartPort[Port].InLength;Tmp++)
                          {
                            snprintf(UartBuffer,UARTBUFFERSIZE,"%02X",UartPort[Port].InBuffer[Tmp] & 0xFF);
                            UartWrite(UartBuffer);
                          }
                          snprintf(UartBuffer,UARTBUFFERSIZE,"]%02X\n",UartPort[Port].Check & 0xFF);
                          UartWrite(UartBuffer);
  #endif
                          UartPort[Port].State  =  UART_INFO_ERROR;
                        }
                        else
                        {
  #ifdef HIGHDEBUG
                          snprintf(UartBuffer,UARTBUFFERSIZE,"    %d %02X ",Port,UartPort[Port].Cmd & 0xFF);
                          UartWrite(UartBuffer);
                          snprintf(UartBuffer,UARTBUFFERSIZE,"%02X[",UartPort[Port].InfoCmd & 0xFF);
                          UartWrite(UartBuffer);
                          for (Tmp = 0;Tmp < UartPort[Port].InLength;Tmp++)
                          {
                            snprintf(UartBuffer,UARTBUFFERSIZE,"%02X",UartPort[Port].InBuffer[Tmp] & 0xFF);
                            UartWrite(UartBuffer);
                          }
                          snprintf(UartBuffer,UARTBUFFERSIZE,"]%02X\n",UartPort[Port].Check & 0xFF);
                          UartWrite(UartBuffer);
  #endif

                          Mode  =  GET_MODE(UartPort[Port].Cmd);

                          switch (GET_INFO_COMMAND(UartPort[Port].InfoCmd))
                          { // Info mesage type

                            case INFO_NAME :
                            { // Device name

                              UartPort[Port].InfoData &=  INFODATA_CLEAR;
                              if ((UartPort[Port].InBuffer[0] >= 'A') && (UartPort[Port].InBuffer[0] <= 'z') && (strlen(UartPort[Port].InBuffer) <= TYPE_NAME_LENGTH))
                              {
                                snprintf((char*)UartPort[Port].Name,TYPE_NAME_LENGTH + 1,"%s",(char*)UartPort[Port].InBuffer);
  #ifdef HIGHDEBUG
                                snprintf(UartBuffer,UARTBUFFERSIZE,"    %d NAME   = %s\n",Port,UartPort[Port].Name);
                                UartWrite(UartBuffer);
  #endif
                                TypeData[Port][Mode].Mode   =  Mode;
                                UartPort[Port].InfoData    |=  INFODATA_INFO_NAME;
                              }
                              else
                              {
  #ifdef DEBUG_D_UART_ERROR
                                UartPort[Port].InBuffer[TYPE_NAME_LENGTH]  =  0;
                                snprintf(UartBuffer,UARTBUFFERSIZE," f  %d NAME = %s\n",Port,UartPort[Port].InBuffer);
                                UartWrite(UartBuffer);
  #endif
                                UartPort[Port].State  =  UART_INFO_ERROR;
                              }
                            }
                            break;

                            case INFO_RAW :
                            { // Raw scaling values

                              TmpL  =  0;
                              for (Tmp = 0;(Tmp < UartPort[Port].InLength) && (Tmp < 4);Tmp++)
                              {
                                TmpL |=  (ULONG)UartPort[Port].InBuffer[Tmp] << (8 * Tmp);
                              }
                              *((ULONG*)&TypeData[Port][Mode].RawMin)  =  TmpL;
                              TmpL  =  0;
                              for (Tmp = 0;(Tmp < (UartPort[Port].InLength - 4)) && (Tmp < 4);Tmp++)
                              {
                                TmpL |=  (ULONG)UartPort[Port].InBuffer[Tmp + 4] << (8 * Tmp);
                              }
                              *((ULONG*)&TypeData[Port][Mode].RawMax)  =  TmpL;

                              if (TypeData[Port][Mode].Mode == GET_MODE(UartPort[Port].Cmd))
                              {
                                if ((UartPort[Port].InfoData & INFODATA_INFO_RAW))
                                { // Raw already given

  #ifdef DEBUG_D_UART_ERROR
                                  snprintf(UartBuffer,UARTBUFFERSIZE," ## %d RAW ALREADY GIVEN\n",Port);
                                  UartWrite(UartBuffer);
  #endif
                                  UartPort[Port].State  =  UART_INFO_ERROR;
                                }
                                else
                                {
  #ifdef HIGHDEBUG
                                  snprintf(UartBuffer,UARTBUFFERSIZE,"    %d RAW = Min..Max\n",Port);
                                  UartWrite(UartBuffer);
  #endif
                                  UartPort[Port].InfoData |=  INFODATA_INFO_RAW;
                                }
                              }
                              else
                              {
  #ifdef DEBUG_D_UART_ERROR
                                snprintf(UartBuffer,UARTBUFFERSIZE," f  %d RAW = Min..Max\n",Port);
                                UartWrite(UartBuffer);
  #endif
                                UartPort[Port].State  =  UART_INFO_ERROR;
                              }
                            }
                            break;

                            case INFO_PCT :
                            { // Pct scaling values

                              TmpL  =  0;
                              for (Tmp = 0;(Tmp < UartPort[Port].InLength) && (Tmp < 4);Tmp++)
                              {
                                TmpL |=  (ULONG)UartPort[Port].InBuffer[Tmp] << (8 * Tmp);
                              }
                              *((ULONG*)&TypeData[Port][Mode].PctMin)  =  TmpL;
                              TmpL  =  0;
                              for (Tmp = 0;(Tmp < (UartPort[Port].InLength - 4)) && (Tmp < 4);Tmp++)
                              {
                                TmpL |=  (ULONG)UartPort[Port].InBuffer[Tmp + 4] << (8 * Tmp);
                              }
                              *((ULONG*)&TypeData[Port][Mode].PctMax)  =  TmpL;

                              if (TypeData[Port][Mode].Mode == GET_MODE(UartPort[Port].Cmd))
                              { // Mode valid

                                if ((UartPort[Port].InfoData & INFODATA_INFO_PCT))
                                { // Pct already given

  #ifdef DEBUG_D_UART_ERROR
                                  snprintf(UartBuffer,UARTBUFFERSIZE," ## %d PCT ALREADY GIVEN\n",Port);
                                  UartWrite(UartBuffer);
  #endif
                                  UartPort[Port].State  =  UART_INFO_ERROR;
                                }
                                else
                                {
  #ifdef HIGHDEBUG
                                  snprintf(UartBuffer,UARTBUFFERSIZE,"    %d PCT = Min..Max\n",Port);
                                  UartWrite(UartBuffer);
  #endif
                                  UartPort[Port].InfoData |=  INFODATA_INFO_PCT;
                                }
                              }
                              else
                              { // Mode invalid
  #ifdef DEBUG_D_UART_ERROR
                                snprintf(UartBuffer,UARTBUFFERSIZE," f  %d PCT = Min..Max\n",Port);
                                UartWrite(UartBuffer);
  #endif
                                UartPort[Port].State  =  UART_INFO_ERROR;
                              }
                            }
                            break;

                            case INFO_SI :
                            { // SI unit scaling values

                              TmpL  =  0;
                              for (Tmp = 0;(Tmp < UartPort[Port].InLength) && (Tmp < 4);Tmp++)
                              {
                                TmpL |=  (ULONG)UartPort[Port].InBuffer[Tmp] << (8 * Tmp);
                              }
                              *((ULONG*)&TypeData[Port][Mode].SiMin)  =  TmpL;
                              TmpL  =  0;
                              for (Tmp = 0;(Tmp < (UartPort[Port].InLength - 4)) && (Tmp < 4);Tmp++)
                              {
                                TmpL |=  (ULONG)UartPort[Port].InBuffer[Tmp + 4] << (8 * Tmp);
                              }
                              *((ULONG*)&TypeData[Port][Mode].SiMax)  =  TmpL;

                              if (TypeData[Port][Mode].Mode == GET_MODE(UartPort[Port].Cmd))
                              { // Mode valid

                                if ((UartPort[Port].InfoData & INFODATA_INFO_SI))
                                { // Si already given

  #ifdef DEBUG_D_UART_ERROR
                                  snprintf(UartBuffer,UARTBUFFERSIZE," ## %d SI ALREADY GIVEN\n",Port);
                                  UartWrite(UartBuffer);
  #endif
                                  UartPort[Port].State  =  UART_INFO_ERROR;
                                }
                                else
                                {
  #ifdef HIGHDEBUG
                                  snprintf(UartBuffer,UARTBUFFERSIZE,"    %d SI  = Min..Max\n",Port);
                                  UartWrite(UartBuffer);
  #endif
                                  UartPort[Port].InfoData |=  INFODATA_INFO_SI;
                                }
                              }
                              else
                              { // Mode invalid
  #ifdef DEBUG_D_UART_ERROR
                                snprintf(UartBuffer,UARTBUFFERSIZE," f  %d SI  = Min..Max\n",Port);
                                UartWrite(UartBuffer);
  #endif
                                UartPort[Port].State  =  UART_INFO_ERROR;
                              }
                            }
                            break;

                            case INFO_SYMBOL :
                            { // Presentation format

                              if ((UartPort[Port].InfoData & INFODATA_INFO_SYMBOL))
                              { // Symbol already given

  #ifdef DEBUG_D_UART_ERROR
                                snprintf(UartBuffer,UARTBUFFERSIZE," ## %d SYMBOL ALREADY GIVEN\n",Port);
                                UartWrite(UartBuffer);
  #endif
                                UartPort[Port].State  =  UART_INFO_ERROR;
                              }
                              else
                              {
                                snprintf((char*)TypeData[Port][Mode].Symbol,SYMBOL_LENGTH + 1,"%s",(char*)UartPort[Port].InBuffer);
  #ifdef HIGHDEBUG
                                snprintf(UartBuffer,UARTBUFFERSIZE,"    %d SYMBOL = %s\n",Port,TypeData[Port][Mode].Symbol);
                                UartWrite(UartBuffer);
  #endif
                                UartPort[Port].InfoData |=  INFODATA_INFO_SYMBOL;
                              }
                            }
                            break;

                            case INFO_FORMAT :
                            { // Data sets and format

                              if ((UartPort[Port].InfoData & INFODATA_INFO_FORMAT))
                              { // Format already given

  #ifdef DEBUG_D_UART_ERROR
                                snprintf(UartBuffer,UARTBUFFERSIZE," ## %d FORMAT ALREADY GIVEN\n",Port);
                                UartWrite(UartBuffer);
  #endif
                                UartPort[Port].State  =  UART_INFO_ERROR;
                              }
                              else
                              {
                                TypeData[Port][Mode].DataSets  =  UartPort[Port].InBuffer[0];
                                TypeData[Port][Mode].Format    =  UartPort[Port].InBuffer[1];


                                if (TypeData[Port][Mode].DataSets > 0)
                                { // Data sets valid

                                  if (UartPort[Port].Types)
                                  { // Modes left

                                    UartPort[Port].Types--;
                                    if (TypeData[Port][Mode].Mode == GET_MODE(UartPort[Port].Cmd))
                                    { // Mode valid

                                      if (UartPort[Port].InLength >= 4)
                                      { // Figures and decimals present

                                        UartPort[Port].InfoData |=  INFODATA_INFO_FORMAT;

                                        if ((UartPort[Port].InfoData & INFODATA_NEEDED) == INFODATA_NEEDED)
                                        {
                                          snprintf((char*)TypeData[Port][Mode].Name,TYPE_NAME_LENGTH + 1,"%s",(char*)UartPort[Port].Name);

                                          TypeData[Port][Mode].Type        =  UartPort[Port].Type;
                                          TypeData[Port][Mode].Connection  =  CONN_INPUT_UART;
                                          TypeData[Port][Mode].Views       =  UartPort[Port].Views;

                                          TypeData[Port][Mode].Figures     =  UartPort[Port].InBuffer[2];
                                          TypeData[Port][Mode].Decimals    =  UartPort[Port].InBuffer[3];

//!<  \todo IR seeker hack
                                          if (TypeData[Port][Mode].Type == TYPE_IR)
                                          {
                                            TypeData[Port][Mode].InvalidTime  =  1100;
                                          }

                                          Changed[Port][Mode]              =  1;
  #ifdef HIGHDEBUG
                                          snprintf(UartBuffer,UARTBUFFERSIZE,"    %d FORMAT = %u * %u  %u.%u\n",Port,TypeData[Port][Mode].DataSets,TypeData[Port][Mode].Format,TypeData[Port][Mode].Figures,TypeData[Port][Mode].Decimals);
                                          UartWrite(UartBuffer);
  #endif
                                        }
                                        else
                                        { // Not enough info data given
  #ifdef DEBUG_D_UART_ERROR
                                          snprintf(UartBuffer,UARTBUFFERSIZE," ## %d NOT ENOUGH INFO GIVEN\n",Port);
                                          UartWrite(UartBuffer);
  #endif
                                          UartPort[Port].State  =  UART_INFO_ERROR;

                                        }
                                      }
                                      else
                                      { // Format invalid
  #ifdef DEBUG_D_UART_ERROR
                                        snprintf(UartBuffer,UARTBUFFERSIZE," ## %d FORMAT ERROR\n",Port);
                                        UartWrite(UartBuffer);
  #endif
                                        UartPort[Port].State  =  UART_INFO_ERROR;

                                      }
                                    }
                                    else
                                    { // Mode invalid
  #ifdef DEBUG_D_UART_ERROR
                                      snprintf(UartBuffer,UARTBUFFERSIZE," f  %d FORMAT = %u * %u  %u.%u\n",Port,TypeData[Port][Mode].DataSets,TypeData[Port][Mode].Format,TypeData[Port][Mode].Figures,TypeData[Port][Mode].Decimals);
                                      UartWrite(UartBuffer);
  #endif
                                      UartPort[Port].State  =  UART_INFO_ERROR;
                                    }
                                  }
                                  else
                                  { // No more modes left
  #ifdef DEBUG_D_UART_ERROR
                                    snprintf(UartBuffer,UARTBUFFERSIZE," ## %d TYPES ERROR\n",Port);
                                    UartWrite(UartBuffer);
  #endif
                                    UartPort[Port].State  =  UART_INFO_ERROR;

                                  }
                                }
                                else
                                { // Data sets invalid
  #ifdef DEBUG_D_UART_ERROR
                                  snprintf(UartBuffer,UARTBUFFERSIZE," f  %d FORMAT = %u * %u  %u.%u\n",Port,TypeData[Port][Mode].DataSets,TypeData[Port][Mode].Format,TypeData[Port][Mode].Figures,TypeData[Port][Mode].Decimals);
                                  UartWrite(UartBuffer);
  #endif
                                  UartPort[Port].State  =  UART_INFO_ERROR;
                                }
                              }
                            }
                            break;

                          }
                        }
                        break;

                      }
                    }
                  }

                  if (UartPort[Port].Type == UartPortDefault.Type)
                  {
  #ifdef DEBUG_D_UART_ERROR
                    snprintf(UartBuffer,UARTBUFFERSIZE," ## %d TYPE ERROR\n",Port);
                    UartWrite(UartBuffer);
  #endif
                    UartPort[Port].State  =  UART_INFO_ERROR;
                  }
                }
                break;

  //** ERRORS *******************************************************************

                case UART_CMD_ERROR :
                {
                  UartPort[Port].State  =  UART_ERROR;
                }
                break;

                case UART_INFO_ERROR :
                {
                  UartPort[Port].State  =  UART_ERROR;
                }
                break;

                default :
                {
                  UartPort[Port].State  =  UART_MESSAGE_START;
                }
                break;

              }

  //** END OF INFORMATIONS ******************************************************

              UartPort[Port].Check ^=  Byte;
            }
          }
          break;

  //** DATA *********************************************************************

          case UART_DATA :
          { // Get device data

            UartPort[Port].InLength   =  UartPortReadData(Port,&UartPort[Port].Cmd,TmpBuffer,&UartPort[Port].Check,&CrcError);

            if (UartPort[Port].InLength)
            {
//!<  \todo Color sensor hack (wrong checksum in mode 4 data)
              if ((UartPort[Port].Type == TYPE_COLOR) && (GET_MODE(UartPort[Port].Cmd) == 4))
              {
                CrcError  =  0;
              }

              if (!CrcError)
              {
                if (UartPort[Port].Initialised == 0)
                {
                  UartPort[Port].Initialised  =  1;
                }
                if (!((*pUart).Status[Port]  &  UART_PORT_CHANGED))
                {
                  if (UartPort[Port].Mode == GET_MODE(UartPort[Port].Cmd))
                  {
                    if (!((*pUart).Status[Port] & UART_DATA_READY))
                    {
                      (*pUart).Status[Port] |=  UART_DATA_READY;

  #ifdef DEBUG_TRACE_MODE_CHANGE
                      snprintf(UartBuffer,UARTBUFFERSIZE,"d_uart %d   State machine: mode changed to  %d\n",Port,UartPort[Port].Mode);
                      UartWrite(UartBuffer);
  #endif
                    }

#ifdef DEBUG_TRACE_US
                    if (Port == 1)
                    {
                      if (GET_MODE(UartPort[Port].Cmd) == 1)
                      {
                        In  =  (UWORD)TmpBuffer[0];
                        In |=  (UWORD)TmpBuffer[1] << 8;

                        if (In > 1000)
                        {
                          if (CrcError)
                          {
                            snprintf(UartBuffer,UARTBUFFERSIZE," c  %d %02X[",Port,UartPort[Port].Cmd & 0xFF);
                          }
                          else
                          {
                            snprintf(UartBuffer,UARTBUFFERSIZE,"    %d %02X[",Port,UartPort[Port].Cmd & 0xFF);
                          }
                          printk(UartBuffer);

                          for (Tmp = 0;Tmp < UartPort[Port].InLength;Tmp++)
                          {
                            snprintf(UartBuffer,UARTBUFFERSIZE,"%02X",TmpBuffer[Tmp] & 0xFF);
                            printk(UartBuffer);
                          }
                          snprintf(UartBuffer,UARTBUFFERSIZE,"]%02X\n",UartPort[Port].Check & 0xFF);
                          printk(UartBuffer);
                        }
                      }
                    }
#endif

  #ifndef DISABLE_FAST_DATALOG_BUFFER


                    memcpy((void*)(*pUart).Raw[Port][(*pUart).LogIn[Port]],(void*)TmpBuffer,UART_DATA_LENGTH);

                    (*pUart).Actual[Port]  =  (*pUart).LogIn[Port];
                    (*pUart).Repeat[Port][(*pUart).Actual[Port]]  =  0;

                    if (++((*pUart).LogIn[Port]) >= DEVICE_LOGBUF_SIZE)
                    {
                      (*pUart).LogIn[Port]      =  0;
                    }
  #else
                    memcpy((void*)(*pUart).Raw[Port],(void*)TmpBuffer,UART_DATA_LENGTH);
  #endif
                    if (UartPort[Port].DataErrors)
                    {
                      UartPort[Port].DataErrors--;
                    }
                    UartPort[Port].DataOk       =  1;
                  }
                  else
                  {
                    UartPort[Port].ChangeMode   =  1;
                  }
                }
              }
              else
              {
  #ifdef DEBUG_D_UART_ERROR
                snprintf(UartBuffer,UARTBUFFERSIZE," c  %d %02X[",Port,UartPort[Port].Cmd & 0xFF);
                UartWrite(UartBuffer);

                for (Tmp = 0;Tmp < UartPort[Port].InLength;Tmp++)
                {
                  snprintf(UartBuffer,UARTBUFFERSIZE,"%02X",TmpBuffer[Tmp] & 0xFF);
                  UartWrite(UartBuffer);
                }
                snprintf(UartBuffer,UARTBUFFERSIZE,"]%02X\n",UartPort[Port].Check & 0xFF);
                UartWrite(UartBuffer);
  #endif
  #ifndef DISABLE_UART_DATA_ERROR
                if (++UartPort[Port].DataErrors >= UART_ALLOWABLE_DATA_ERRORS)
                {
  #ifdef DEBUG_D_UART_ERROR
                  snprintf(UartBuffer,UARTBUFFERSIZE," ## %d No valid data in %d messages\n",Port,UartPort[Port].DataErrors);
                  UartWrite(UartBuffer);
  #endif
                  UartPort[Port].State      =  UART_DATA_ERROR;
                }
  #endif
              }
  #ifdef DEBUG_TRACE_ANGLE
              if (Port == 1)
              {
                Angle  =  (UWORD)(*pUart).Raw[Port][0];
                Angle |=  (UWORD)(*pUart).Raw[Port][1] << 8;

                if (Angle > 50)
                {
                  printk("Angle = %u\r",Angle);
                  if (CrcError)
                  {
                    snprintf(UartBuffer,UARTBUFFERSIZE," c  %d %02X[",Port,UartPort[Port].Cmd & 0xFF);
                  }
                  else
                  {
                    snprintf(UartBuffer,UARTBUFFERSIZE,"    %d %02X[",Port,UartPort[Port].Cmd & 0xFF);
                  }
                  printk(UartBuffer);

                  for (Tmp = 0;Tmp < UartPort[Port].InLength;Tmp++)
                  {
                    snprintf(UartBuffer,UARTBUFFERSIZE,"%02X",TmpBuffer[Tmp] & 0xFF);
                    printk(UartBuffer);
                  }
                  snprintf(UartBuffer,UARTBUFFERSIZE,"]%02X\n",UartPort[Port].Check & 0xFF);
                  printk(UartBuffer);
                }
              }
  #endif

  #ifdef DEBUG
              if (ShowTimer[Port] >= (UART_SHOW_TIME / UART_TIMER_RESOLUTION))
              {
                ShowTimer[Port]  =  0;
                if (CrcError)
                {
                  snprintf(UartBuffer,UARTBUFFERSIZE," c  %d %02X[",Port,UartPort[Port].Cmd & 0xFF);
                }
                else
                {
                  snprintf(UartBuffer,UARTBUFFERSIZE,"    %d %02X[",Port,UartPort[Port].Cmd & 0xFF);
                }
                UartWrite(UartBuffer);

                for (Tmp = 0;Tmp < UartPort[Port].InLength;Tmp++)
                {
                  snprintf(UartBuffer,UARTBUFFERSIZE,"%02X",TmpBuffer[Tmp] & 0xFF);
                  UartWrite(UartBuffer);
                }
                snprintf(UartBuffer,UARTBUFFERSIZE,"]%02X\n",UartPort[Port].Check & 0xFF);
                UartWrite(UartBuffer);
              }
  #endif
            }

            if (UartPort[Port].ChangeMode)
            { // Try to change mode

              if (UartPort[Port].OutPointer >= UartPort[Port].OutLength)
              { // Transmitter ready

  #ifdef DEBUG
                ShowTimer[Port]  =  0;
                if (CrcError)
                {
                  snprintf(UartBuffer,UARTBUFFERSIZE," c  %d %02X[",Port,UartPort[Port].Cmd & 0xFF);
                }
                else
                {
                  snprintf(UartBuffer,UARTBUFFERSIZE,"    %d %02X[",Port,UartPort[Port].Cmd & 0xFF);
                }
                UartWrite(UartBuffer);

                for (Tmp = 0;Tmp < UartPort[Port].InLength;Tmp++)
                {
                  snprintf(UartBuffer,UARTBUFFERSIZE,"%02X",TmpBuffer[Tmp] & 0xFF);
                  UartWrite(UartBuffer);
                }
                snprintf(UartBuffer,UARTBUFFERSIZE,"]%02X\n",UartPort[Port].Check & 0xFF);
                UartWrite(UartBuffer);
  #endif
                UartPort[Port].Cmd            =  UartPort[Port].Mode;
                UartPort[Port].OutBuffer[0]   =  MAKE_CMD_COMMAND(CMD_SELECT,0);
                UartPort[Port].OutBuffer[1]   =  UartPort[Port].Mode;
                UartPort[Port].OutBuffer[2]   =  0xFF ^ UartPort[Port].OutBuffer[0] ^ UartPort[Port].OutBuffer[1];
                UartPort[Port].OutPointer     =  0;
                UartPort[Port].OutLength      =  3;

                UartPort[Port].ChangeMode  =  0;

  #ifdef DEBUG_TRACE_MODE_CHANGE
                snprintf(UartBuffer,UARTBUFFERSIZE," WR %d %02X[",Port,UartPort[Port].OutBuffer[0]);
                UartWrite(UartBuffer);

                for (Tmp = 1;Tmp < (UartPort[Port].OutLength - 1);Tmp++)
                {
                  snprintf(UartBuffer,UARTBUFFERSIZE,"%02X",UartPort[Port].OutBuffer[Tmp] & 0xFF);
                  UartWrite(UartBuffer);
                }
                snprintf(UartBuffer,UARTBUFFERSIZE,"]%02X\n",UartPort[Port].OutBuffer[Tmp] & 0xFF);
                UartWrite(UartBuffer);
  #endif
              }
              (*pUart).Status[Port] &= ~UART_DATA_READY;
            }
            if (++UartPort[Port].WatchDog >= (UART_WATCHDOG_TIME / UART_TIMER_RESOLUTION))
            { // Try to service watch dog

              if (UartPort[Port].OutPointer >= UartPort[Port].OutLength)
              { // Transmitter ready

                UartPort[Port].WatchDog       =  0;

                if (!UartPort[Port].DataOk)
                { // No ok data since last watch dog service

  #ifndef DISABLE_UART_DATA_ERROR
                  if (++UartPort[Port].DataErrors >= UART_ALLOWABLE_DATA_ERRORS)
                  {
  #ifdef DEBUG_D_UART_ERROR
                    snprintf(UartBuffer,UARTBUFFERSIZE," ## %d No valid data in %d services\n",Port,UART_ALLOWABLE_DATA_ERRORS);
                    UartWrite(UartBuffer);
  #endif
                    UartPort[Port].State        =  UART_DATA_ERROR;
                  }
                  else
                  {
                    UartPort[Port].DataOk       =  1;
                  }
  #else
                  UartPort[Port].DataOk         =  1;
  #endif
                }
                if (UartPort[Port].DataOk)
                {
                  UartPort[Port].DataOk         =  0;

                  UartPort[Port].OutBuffer[0]   =  BYTE_NACK;
                  UartPort[Port].OutPointer     =  0;
                  UartPort[Port].OutLength      =  1;
  #ifdef DEBUG
                  snprintf(UartBuffer,UARTBUFFERSIZE," WD %d %02X\n",Port,UartPort[Port].OutBuffer[0]);
                  UartWrite(UartBuffer);
  #endif
                }
              }
            }

            if (WriteRequest[Port])
            { // Try to write message

              if (UartPort[Port].OutPointer >= UartPort[Port].OutLength)
              { // Transmitter ready

                // convert length to length code
                Byte  =  0;
                Tmp  =  CONVERT_LENGTH(Byte);
                while ((Tmp < UART_DATA_LENGTH) && (Tmp < (*pUart).OutputLength[Port]))
                {
                  Byte++;
                  Tmp  =  CONVERT_LENGTH(Byte);
                }

                Chksum    =  0xFF;

                UartPort[Port].OutBuffer[0]  =  MAKE_CMD_COMMAND(CMD_WRITE,Byte);
                Chksum                      ^=  UartPort[Port].OutBuffer[0];

                Pointer   =  0;
                while (Pointer < Tmp)
                {
                  if (Pointer < (*pUart).OutputLength[Port])
                  {
                    UartPort[Port].OutBuffer[1 + Pointer]  =  (*pUart).Output[Port][Pointer];
                  }
                  else
                  {
                    UartPort[Port].OutBuffer[1 + Pointer]  =  0;
                  }
                  Chksum ^=  UartPort[Port].OutBuffer[1 + Pointer];
                  Pointer++;

                }
                UartPort[Port].OutBuffer[1 + Pointer]   =  Chksum;
                UartPort[Port].OutPointer               =  0;
                UartPort[Port].OutLength                =  Tmp + 2;

                WriteRequest[Port]                      =  0;
                (*pUart).Status[Port]                  &= ~UART_WRITE_REQUEST;
  #ifdef DEBUG
                snprintf(UartBuffer,UARTBUFFERSIZE," WR %d %02X[",Port,UartPort[Port].OutBuffer[0]);
                UartWrite(UartBuffer);

                for (Tmp = 1;Tmp < (UartPort[Port].OutLength - 1);Tmp++)
                {
                  snprintf(UartBuffer,UARTBUFFERSIZE,"%02X",UartPort[Port].OutBuffer[Tmp] & 0xFF);
                  UartWrite(UartBuffer);
                }
                snprintf(UartBuffer,UARTBUFFERSIZE,"]%02X\n",UartPort[Port].OutBuffer[Tmp] & 0xFF);
                UartWrite(UartBuffer);
  #endif
              }
            }

  #ifndef DISABLE_FAST_DATALOG_BUFFER
            ((*pUart).Repeat[Port][(*pUart).Actual[Port]])++;
  #endif
          }
          break;

          case UART_ACK_WAIT :
          {
            if (++(UartPort[Port].Timer) >= (UART_ACK_DELAY / UART_TIMER_RESOLUTION))
            {
              (*pUart).Status[Port]  |=  UART_PORT_CHANGED;
              UartPortSend(Port,BYTE_ACK);
              UartPort[Port].Timer    =  0;
              UartPort[Port].State    =  UART_ACK_INFO;
#ifdef DEBUG_D_UART_ERROR
              snprintf(UartBuffer,UARTBUFFERSIZE,"    %d Type %-3d has changed modes: ",Port,TypeData[Port][0].Type);
              UartWrite(UartBuffer);
              for (Mode = 0;Mode < MAX_DEVICE_MODES;Mode++)
              {
                UartBuffer[Mode]  =  Changed[Port][Mode] + '0';
              }
              UartBuffer[Mode++]  =  '\r';
              UartBuffer[Mode++]  =  '\n';
              UartBuffer[Mode]    =  0;
              UartWrite(UartBuffer);
#endif
            }
          }
          break;

          case UART_ACK_INFO :
          {
            if (++(UartPort[Port].Timer) >= (UART_CHANGE_BITRATE_DELAY / UART_TIMER_RESOLUTION))
            {
              UartPort[Port].DataOk       =  1;
              UartPort[Port].DataErrors   =  0;
              UartPort[Port].Mode         =  0;
              UartPort[Port].BitRate      =  UartPort[Port].BitRateMax;
              UartPortSetup(Port,UartPort[Port].BitRate);
              UartPort[Port].WatchDog     =  (UART_WATCHDOG_TIME / UART_TIMER_RESOLUTION);
              UartPort[Port].State        =  UART_DATA;
            }
          }
          break;

          case UART_TERMINAL :
          {
          }
          break;

          case UART_DATA_ERROR :
          {
            UartPort[Port].State  =  UART_ERROR;
          }
          break;

          case UART_ERROR :
          {
          }
          break;

          case UART_EXIT :
          {
            UartPortDisable(Port);
            UartPort[Port]                          =  UartPortDefault;


            for (Tmp = 0;Tmp < MAX_DEVICE_MODES;Tmp++)
            {
              TypeData[Port][Tmp].Name[0]           =  0;
              TypeData[Port][Tmp].Type              =  0;
              Changed[Port][Tmp]                    =  0;
            }
            (*pUart).Status[Port]                   =  0;

            UartPort[Port].State                    =  UART_IDLE;
          }
          break;

        }
        if (UartPort[Port].OutPointer < UartPort[Port].OutLength)
        {
          if (UartPortSend(Port,UartPort[Port].OutBuffer[UartPort[Port].OutPointer]))
          {
            UartPort[Port].OutPointer++;
          }
        }

      }
      else
      {
        switch (UartPort[Port].State)
        { // Test state machine

          case UART_IDLE :
          { // Port inactive

          }
          break;

          case UART_INIT :
          { // Initialise port hardware

            UartPortDisable(Port);
            PUARTFloat(Port,INPUT_UART_PIN5);
            PUARTFloat(Port,INPUT_UART_PIN6);
            UartPort[Port].State        =  UART_ENABLE;
          }
          break;

          case UART_ENABLE :
          { // Initialise port variables

            UartPortEnable(Port);
            UartPortSetup(Port,UartPort[Port].BitRate);
            (*pUart).Status[Port]           =  0;
            UartPortFlush(Port);
            UartPort[Port].InPointer        =  0;
            UartPort[Port].State            =  UART_MESSAGE_START;
          }
          break;

          case UART_MESSAGE_START :
          {
            if (UartPort[Port].OutPointer < UartPort[Port].OutLength)
            {
              if (UartPortSend(Port,UartPort[Port].OutBuffer[UartPort[Port].OutPointer]))
              {
                UartPort[Port].OutPointer++;
              }
            }
            if (UartPortReceive(Port,&Byte))
            {
#ifdef HIGHDEBUG
              snprintf(UartBuffer,UARTBUFFERSIZE,"[0x%02X]\n",Byte);
              UartWrite(UartBuffer);
#endif
              if (UartPort[Port].InPointer < UART_BUFFER_SIZE)
              {
                UartPort[Port].InBuffer[UartPort[Port].InPointer]  =  Byte;
                UartPort[Port].InPointer++;
              }
            }
          }
          break;

        }

      }

#ifdef HIGHDEBUG
      if (UartPort[Port].OldState != UartPort[Port].State)
      {
        UartPort[Port].OldState    =  UartPort[Port].State;
        if (UartPort[Port].State  != UART_ENABLE)
        {
          snprintf(UartBuffer,UARTBUFFERSIZE,"    %d %s\n",Port,UartStateText[UartPort[Port].State]);
        }
        else
        {
          snprintf(UartBuffer,UARTBUFFERSIZE,"*** %d %s ***\n",Port,UartStateText[UartPort[Port].State]);
        }
        UartWrite(UartBuffer);
      }
#endif
    }
  }

  return (HRTIMER_RESTART);
}


static long Device1Ioctl(struct file *File, unsigned int Request, unsigned long Pointer)
{
  long     Result = 0;
  UARTCTL *pUartCtl;
  DEVCON  DevCon;
  DATA8   Port = 0;
  DATA8   Mode;

  switch (Request)
  {

    case UART_SET_CONN :
    {

      copy_from_user((void*)&DevCon,(void*)Pointer,sizeof(DEVCON));



	      for (Port = 0;Port < INPUTS;Port++)
	      {
	      	
		        if (DevCon.Connection[Port] == CONN_INPUT_UART)
		        {
			          if (UartConfigured[Port] == 0)
			          {
			            UartConfigured[Port]        =  1;
			            UartPort[Port].State        =  UART_INIT;
			          }
			          else
			          {
				            if (UartPort[Port].Initialised)
				            {
				              if (UartPort[Port].Mode != DevCon.Mode[Port])
				              {
						#ifdef DEBUG_TRACE_MODE_CHANGE
					                snprintf(UartBuffer,UARTBUFFERSIZE,"d_uart %d   Device1Ioctl: Changing to    %c\n",Port,DevCon.Mode[Port] + '0');
					                UartWrite(UartBuffer);
						#endif
					                UartPort[Port].Mode         =  DevCon.Mode[Port];
					                UartPort[Port].ChangeMode   =  1;
					                (*pUart).Status[Port]      &= ~UART_DATA_READY;
			              		}
			            }
			          }
		        }


			
		        else
		        {
			          (*pUart).Status[Port] &= ~UART_DATA_READY;
			          if (UartConfigured[Port])
			          {
			            UartConfigured[Port]        =  0;
			            UartPort[Port].State        =  UART_EXIT;
		         	          }
		        }
	      }

	    }
    break;

    case UART_READ_MODE_INFO :
    {
      pUartCtl  =  (UARTCTL*)Pointer;
      Port      =  (*pUartCtl).Port;
      Mode      =  (*pUartCtl).Mode;

#ifdef DEBUG
      if (TypeData[Port][Mode].Name[0])
      {
        snprintf(UartBuffer,UARTBUFFERSIZE,"d_uart %d   Device1Ioctl: READ    Type=%d Mode=%d\n",Port,TypeData[Port][Mode].Type,Mode);
        UartWrite(UartBuffer);
      }
#endif
      if ((Mode < MAX_DEVICE_MODES) && (Port < INPUTS))
      {
        copy_to_user((void*)&(*pUartCtl).TypeData,(void*)&TypeData[Port][Mode],sizeof(TYPES));
        if (Changed[Port][Mode] == 0)
        {
          (*pUartCtl).TypeData.Name[0]  =  0;
        }
        Changed[Port][Mode]  =  0;
      }
    }
    break;

    case UART_NACK_MODE_INFO :
    {
      pUartCtl  =  (UARTCTL*)Pointer;
      Port      =  (*pUartCtl).Port;
      Mode      =  (*pUartCtl).Mode;

#ifdef DEBUG
      snprintf(UartBuffer,UARTBUFFERSIZE,"d_uart %d   Device1Ioctl: NACK    Type=%d Mode=%d\n",Port,TypeData[Port][Mode].Type,Mode);
      UartWrite(UartBuffer);
#endif
      if ((Mode < MAX_DEVICE_MODES) && (Port < INPUTS))
      {
        Changed[Port][Mode]  =  1;
      }
    }
    break;

    case UART_CLEAR_CHANGED :
    {
      pUartCtl  =  (UARTCTL*)Pointer;
      Port      =  (*pUartCtl).Port;

      (*pUart).Status[Port] &= ~UART_PORT_CHANGED;
    }
    break;

  }
  return (Result);

}


static ssize_t Device1Write(struct file *File,const char *Buffer,size_t Count,loff_t *Data)
{
  char    Buf[UART_DATA_LENGTH + 1];
  DATA8   Port;
  int     Lng     = 0;

  if (Count <= (UART_DATA_LENGTH + 1))
  {

	
    	copy_from_user(Buf,Buffer,Count);

    	Port  =  Buf[0];
	
    	if (Port < INPUTS)
    	{
      		Lng  =  1;

      		while (Lng < Count)
      		{
        			(*pUart).Output[Port][Lng - 1]  =  Buf[Lng];
        			Lng++;
      		}
      		(*pUart).OutputLength[Port]   =  Lng - 1;
      		(*pUart).Status[Port]        |=  UART_WRITE_REQUEST;
      		WriteRequest[Port]            =  1;
    	}
  }
  return (Lng);
}


static ssize_t Device1Read(struct file *File,char *Buffer,size_t Count,loff_t *Offset)
{
  int     Lng     = 0;

#ifdef DEBUG_UART_WRITE
  if (LogOutPointer != LogPointer)
  {
    while ((Count--) && (LogOutPointer != LogPointer))
    {
      Buffer[Lng++]  =  LogPool[LogOutPointer];
      Buffer[Lng]    =  0;

      LogOutPointer++;
      if (LogOutPointer >= LOGPOOLSIZE)
      {
        LogOutPointer  =  0;
      }
    }
  }
  if (Lng == 0)
  {
    UartBuffer[0]  =  0x1B;
    UartBuffer[1]  =  '[';
    UartBuffer[2]  =  '2';
    UartBuffer[3]  =  'J';
    UartBuffer[4]  =  0x1B;
    UartBuffer[5]  =  '[';
    UartBuffer[6]  =  'H';
    UartBuffer[7]  =  0;
    UartWrite(UartBuffer);
    UartWrite(UartBuffer);
    snprintf(UartBuffer,UARTBUFFERSIZE,"-----------------------------------------------------------------\n");
    UartWrite(UartBuffer);
    snprintf(UartBuffer,UARTBUFFERSIZE,"    UART DUMP\n");
    UartWrite(UartBuffer);
    snprintf(UartBuffer,UARTBUFFERSIZE,"-----------------------------------------------------------------\n");
    UartWrite(UartBuffer);
  }
#else
  int     Tmp;
  int     Port;

  Port   =  0;
  Tmp    =  5;
  while ((Count > Tmp) && (Port < INPUTS))
  {
    if (Port != (INPUTS - 1))
    {
      Tmp    =  snprintf(&Buffer[Lng],4,"%2u ",(UWORD)UartPort[Port].State);
    }
    else
    {
      Tmp    =  snprintf(&Buffer[Lng],5,"%2u\r",(UWORD)UartPort[Port].State);
    }
    Lng   +=  Tmp;
    Count -=  Tmp;
    Port++;
  }
#endif

  return (Lng);
}


#define     SHM_LENGTH    (sizeof(UartDefault))
#define     NPAGES        ((SHM_LENGTH + PAGE_SIZE - 1) / PAGE_SIZE)
static void *kmalloc_ptr;

static int Device1Mmap(struct file *filp, struct vm_area_struct *vma)
{
   int ret;

   ret = remap_pfn_range(vma,vma->vm_start,virt_to_phys((void*)((unsigned long)pUart)) >> PAGE_SHIFT,vma->vm_end-vma->vm_start,PAGE_SHARED);

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
  .unlocked_ioctl        = Device1Ioctl,
};


static    struct miscdevice Device1 =
{
  MISC_DYNAMIC_MINOR,
  DEVICE1_NAME,
  &Device1Entries
};





static void Device1Init(void)
{
	int Result = 0;
	UWORD *pTmp;
	int i;
	int Tmp;

	UBYTE liu_test_byte;
	int liu = 0;

	Result = misc_register(&Device1);


	if(Result)
	{
		printk("%s device register failed\n", DEVICE1_NAME);
	}
	else
	{
		// allocate kernel shared memory for uart values (pUart)
		if ((kmalloc_ptr = kmalloc((NPAGES + 2) * PAGE_SIZE, GFP_KERNEL)) != NULL)
		{
			pTmp = (UWORD*)((((unsigned long)kmalloc_ptr) + PAGE_SIZE - 1) & PAGE_MASK);
			for (i = 0; i < NPAGES * PAGE_SIZE; i += PAGE_SIZE)
			{
				SetPageReserved(virt_to_page(((unsigned long)pTmp) + i));
			}
	     		pUart      =  (UART*)pTmp;
			memset(pUart, 0, sizeof(UART));

			Result += Uart1Init();
			Result += Uart2Init();
			Result += Uart3Init();
			Result += Uart4Init();





      			for (Tmp = 0;Tmp < INPUTS;Tmp++)
      			{
        			UartPort[Tmp]         =  UartPortDefault;
        			UartConfigured[Tmp]   =  0;

        			if (Tmp == DEBUG_UART)
        			{
          				UartPort[Tmp].BitRate   =  115200;
          				UartPortSetup(Tmp,UartPort[Tmp].BitRate);
          				TypeData[Tmp][0]        =  TypeDefaultUart[1];
          				(*pUart).Status[Tmp]   |=  UART_PORT_CHANGED;
          				UartPortEnable(Tmp);
					#ifdef DEBUG
          					snprintf(UartBuffer,UARTBUFFERSIZE,"  %s debug uart init test\n",DEVICE1_NAME);
          					UartWrite(UartBuffer);
					#endif
        			}
        			else
        			{
          				UartPortDisable(Tmp);
        			}
      			}

      			Device1Time  =  ktime_set(0,UART_TIMER_RESOLUTION * 100000);
      			hrtimer_init(&Device1Timer,CLOCK_MONOTONIC,HRTIMER_MODE_REL);
      			Device1Timer.function  =  Device1TimerInterrupt1;
      			hrtimer_start(&Device1Timer,Device1Time,HRTIMER_MODE_REL);

		}



	}


}


static void Device1Exit(void)
{
	  int     Tmp;
	  UWORD   *pTmp;
	  int     i;


	  hrtimer_cancel(&Device1Timer);

	  Uart4Exit();
	  Uart3Exit();
	  Uart2Exit();
	  Uart1Exit();

	  for (Tmp = 0; Tmp < INPUTS; Tmp++)
	  {
	  	  UartPort[Tmp] = UartPortDefault;

	  	  if (Tmp == DEBUG_UART)
		  {
		  	  UartPortEnable(Tmp);
		  }
		  else
		  {
		  	  UartPortDisable(Tmp);
		  }
	  }


	  if (Tmp == DEBUG_UART)
	  {
	  	  UartPortEnable(0);
	  }
	  // free shared memory
	  pTmp   =  (UWORD*)pUart;
	  pUart  =  &UartDefault;

	  for (i = 0; i < NPAGES * PAGE_SIZE; i+= PAGE_SIZE)
	  {
	    ClearPageReserved(virt_to_page(((unsigned long)pTmp) + i));
	  }
	  kfree(kmalloc_ptr);


	  misc_deregister(&Device1);


	 for (Tmp = 0; Tmp < INPUTS; Tmp++)
	 {
	 	 if(Base[Tmp] != NULL)
		 {
		 	 iounmap(Base[Tmp]);
		 }
	 }
	  printk("%s exit\n", DEVICE1_NAME);
}


// DEVICE2 ********************************************************************

#define   BUFFER_LNG      16

static long Device2Ioctl(struct file *File, unsigned int Request, unsigned long Pointer)
{
  long     Result = 0;
  TSTUART Tstuart;
  DATA8   Poi;
  UBYTE   Port;

  copy_from_user((void*)&Tstuart,(void*)Pointer,sizeof(TSTUART));
  Port  =  Tstuart.Port;

  switch (Request)
  {
    case TST_UART_OFF :
    { // Normal mode

      for (Port = 0;Port < INPUTS;Port++)
      {
        (*pUart).Status[Port] &= ~UART_DATA_READY;
        UartPort[Port].State      =  UART_EXIT;
      }
      TestMode      =  0;
    }
    break;

    case TST_UART_ON :
    { // Test mode

      TestMode      =  1;
      for (Port = 0;Port < INPUTS;Port++)
      {
        UartPortDisable(Port);
        UartPort[Port]            =  UartPortDefault;
        (*pUart).Status[Port]     =  0;
        UartPort[Port].State      =  UART_IDLE;
      }
    }
    break;

    case TST_UART_EN :
    {
      for (Port = 0;Port < INPUTS;Port++)
      {
        UartPort[Port]              =  UartPortDefault;
        UartPort[Port].BitRate      =  Tstuart.Bitrate;
        UartPort[Port].State        =  UART_INIT;
      }
    }
    break;

    case TST_UART_DIS :
    {
      for (Port = 0;Port < INPUTS;Port++)
      {
        (*pUart).Status[Port] &= ~UART_DATA_READY;
        UartPort[Port].State        =  UART_EXIT;
      }
    }
    break;

    case TST_UART_WRITE :
    { // Write data to uart

      for (Poi = 0;(Poi < Tstuart.Length) && (Poi < UART_BUFFER_SIZE);Poi++)
      {
        UartPort[Port].OutBuffer[Poi]  =  Tstuart.String[Poi];
      }
      UartPort[Port].OutPointer   =  0;
      UartPort[Port].OutLength    =  (DATA8)Poi;
    }
    break;

    case TST_UART_READ :
    { // Read data from uart

      for (Poi = 0;(Poi < UartPort[Port].InPointer) && (Poi < Tstuart.Length) && (Poi < UART_BUFFER_SIZE);Poi++)
      {
        Tstuart.String[Poi]  =  UartPort[Port].InBuffer[Poi];
      }

      copy_to_user((void*)Pointer,(void*)&Tstuart,sizeof(TSTUART));

      UartPort[Port].InPointer  =  0;
    }
    break;

  }
  return (Result);
}


static ssize_t Device2Write(struct file *File,const char *Buffer,size_t Count,loff_t *Data)
{
  int     Lng     = 0;

  return (Lng);
}


static ssize_t Device2Read(struct file *File,char *Buffer,size_t Count,loff_t *Offset)
{
  int     Lng = 0;

  return (Lng);
}


static    const struct file_operations Device2Entries =
{
  .owner        = THIS_MODULE,
  .read         = Device2Read,
  .write        = Device2Write,
  .unlocked_ioctl        = Device2Ioctl
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

	CM_PER[0x6C >> 2] |= 0x2;	//CM_PER_UART1_CLKCTRL
	while(0x2 != (ioread32(&CM_PER[0x6C >> 2]) & 0x2))
		;

	CM_PER[0x70 >> 2] |= 0x2;	//CM_PER_UART2_CLKCTRL
	while(0x2 != (ioread32(&CM_PER[0x70 >> 2]) & 0x2))
		;

	CM_PER[0x78 >> 2] |= 0x2;	//CM_PER_UART4_CLKCTRL
	while(0x2 != (ioread32(&CM_PER[0x78 >> 2]) & 0x2))
		;

	CM_PER[0x38 >> 2] |= 0x2;	//CM_PER_UART5_CLKCTRL
	while(0x2 != (ioread32(&CM_PER[0x38 >> 2]) & 0x2))
		;



	InitGpio();

	Device1Init();
	Device2Init();
	
	return (0);
}


static void ModuleExit(void)
{

	Device2Exit();
	Device1Exit();
	
	iounmap(CM);
	iounmap(CM_PER);
	iounmap(GPIOBANK0);
	iounmap(GPIOBANK1);
	iounmap(GPIOBANK2);
	iounmap(GPIOBANK3);

}


