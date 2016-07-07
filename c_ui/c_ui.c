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


/*! \page UserInterfaceLibrary User Interface Library
 *
 *- \subpage  UserInterLibraryDescription
 *- \subpage  UIdesign
 */


/*! \page UserInterLibraryDescription Description
 *
 *  <hr size="1"/>
 *
 *  The user interface library contains different functionality used to support a graphical user interface.
 *  Graphic files as fonts, icons and pictures are ".xbm" files containing monochrome bitmap that can also
 *  be opened with a normal text editor. These files are coming from the LINUX world.
 *
  \verbatim


    Fonts:      must be X axis aligned on 8 pixel boundaries (build into the firmware)

                Normal font   8  x 9    (W x H)         Used in browser
                Small font    8  x 8    (W x H)         Used on display top line
                Large font    16 x 19   (W x H)         Used in numbers


    Icons:      must be X axis aligned on 8 pixel boundaries (build into the firmware)

                Normal icon   16 x 12   (W x H)         Used in browser
                Small icon    16 x 8    (W x H)         Used on display top line
                Large icon    24 x 24   (W x H)         Used as keys


    Pictures:   no restrictions (files in the file system)

                Smallest       1 x 1    (W x H)
                Largest      178 x 128  (W x H)


  \endverbatim
 *
 */


/*! \page UIdesign Folder Structure
 *
 *  <hr size="1"/>
 *
 \verbatim

                            PROJECTS

    lms2012-------prjs-----,----xxxxxxxx------icon file       (icon.rgf)
                  |        |                  byte code file  (xxxxxxxx.rbf)
                  |        |                  sound files     (.rsf)
                  |        |                  graphics files  (.rgf)
                  |        |                  datalog files   (.rdf)
                  |        |
                  |        |----yyyyyyyy------icon file       (icon.rgf)
                  |        |                  byte code file  (yyyyyyyy.rbf)
                  |        |                  sound files     (.rsf)
                  |        |                  graphics files  (.rgf)
                  |        |                  datalog files   (.rdf)
                  |        |
                  |        |--BrkProg_SAVE----byte code files (.rbf)
                  |        |
                  |        |---BrkDL_SAVE-----datalog files   (.rdf)
                  |        |
                  |        |
                  |        '---SD_Card---,----vvvvvvvv------icon file       (icon.rgf)
                  |                      |                  byte code file  (xxxxxxxx.rbf)
                  |                      |                  sound files     (.rsf)
                  |                      |                  graphics files  (.rgf)
                  |                      |                  datalog files   (.rdf)
                  |                      |
                  |                      |----wwwwwwww------icon file       (icon.rgf)
                  |                      |                  byte code file  (yyyyyyyy.rbf)
                  |                      |                  sound files     (.rsf)
                  |                      |                  graphics files  (.rgf)
                  |                      |                  datalog files   (.rdf)
                  |                      |
                  |                      |--BrkProg_SAVE----byte code files (.rbf)
                  |                      |
                  |                      '---BrkDL_SAVE-----datalog files   (.rdf)
                  |
                  |
                  |         APPS
                  |
                  apps-----,----aaaaaaaa------icon file       (icon.rgf)
                  |        |                  byte code files (aaaaaaaa.rbf)
                  |        |                  sound files     (.rsf)
                  |        |                  graphics files  (.rgf)
                  |        |
                  |        '----bbbbbbbb------icon file       (icon.rgf)
                  |                           byte code files (bbbbbbbb.rbf)
                  |                           sound files     (.rsf)
                  |                           graphics files  (.rgf)
                  |
                  |         TOOLS
                  |
                  tools----,----cccccccc------icon file       (icon.rgf)
                  |        |                  byte code files (cccccccc.rbf)
                  |        |                  sound files     (.rsf)
                  |        |                  graphics files  (.rgf)
                  |        |
                  |        '----dddddddd------icon file       (icon.rgf)
                  |                           byte code files (dddddddd.rbf)
                  |                           sound files     (.rsf)
                  |                           graphics files  (.rgf)
                  |
                  |         SYSTEM
                  |
                  sys------,----ui------------byte code file  (ui.rbf)
                           |                  sound files     (.rsf)
                           |                  graphics files  (.rgf)
                           |
                           '----settings------config files    (.rcf, .rtf, .dat)
                                              typedata.rcf    (device type data)

  \endverbatim
 *
 */

#include "lms2012.h"
#include "button.h"
#include "graph.h"
#include "led.h"
#include "textbox.h"
#include "c_ui.h"
#include "d_terminal.h"
#include "c_memory.h"
#include "c_com.h"
#include "c_input.h"
#include <string.h>

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/sysinfo.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include "d_lcd.h"

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <math.h>
#include <sys/utsname.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <time.h>

#ifdef    DEBUG_C_UI
#define   DEBUG
#endif

// from <time.h>
// supposed to #define _XOPEN_SOURCE, but doing so messes up other stuff
extern char *strptime(const char *s, const char *format, struct tm *tm);

UI_GLOBALS UiInstance;

#ifndef DISABLE_VIRTUAL_BATT_TEMP

  //Defines der kan tænde for debug beskeder
  //#define __dbg1
  //#define __dbg2

  /*************************** Model parameters *******************************/
  //Approx. initial internal resistance of 6 Energizer industrial batteries :
  float R_bat_init = 0.63468;
  //Bjarke's proposal for spring resistance :
  //float spring_resistance = 0.42;
  //Batteries' heat capacity :
  float heat_cap_bat = 136.6598;
  //Newtonian cooling constant for electronics :
  float K_bat_loss_to_elec = -0.0003; //-0.000789767;
  //Newtonian heating constant for electronics :
  float K_bat_gain_from_elec = 0.001242896; //0.001035746;
  //Newtonian cooling constant for environment :
  float K_bat_to_room = -0.00012;
  //Battery power Boost
  float battery_power_boost = 1.7;
  //Battery R_bat negative gain
  float R_bat_neg_gain = 1.00;

  //Slope of electronics lossless heating curve (linear!!!) [Deg.C / s] :
  float K_elec_heat_slope = 0.0123175;
  //Newtonian cooling constant for battery packs :
  float K_elec_loss_to_bat = -0.004137487;
  //Newtonian heating constant for battery packs :
  float K_elec_gain_from_bat = 0.002027574; //0.00152068;
  //Newtonian cooling constant for environment :
  float K_elec_to_room = -0.001931431; //-0.001843639;

  // Function for estimating new battery temperature based on measurements
  // of battery voltage and battery power.
    float new_bat_temp (float V_bat, float I_bat)
    {

      static int   index       = 0; //Keeps track of sample index since power-on
      static float I_bat_mean  = 0; //Running mean current
      const float sample_period = 0.4; //Algorithm update period in seconds
      static float T_bat = 0; //Battery temperature
      static float T_elec = 0; //EV3 electronics temperature

      static float R_bat_model_old = 0;//Old internal resistance of the battery model
      float R_bat_model;    //Internal resistance of the battery model
      static float R_bat = 0; //Internal resistance of the batteries
      float slope_A;        //Slope obtained by linear interpolation
      float intercept_b;    //Offset obtained by linear interpolation
      const float I_1A = 0.05;      //Current carrying capacity at bottom of the curve
      const float I_2A = 2.0;      //Current carrying capacity at the top of the curve

      float R_1A = 0.0;          //Internal resistance of the batteries at 1A and V_bat
      float R_2A = 0.0;          //Internal resistance of the batteries at 2A and V_bat

      //Flag that prevents initialization of R_bat when the battery is charging
      static unsigned char has_passed_7v5_flag = 'N';

      float dT_bat_own = 0.0; //Batteries' own heat
      float dT_bat_loss_to_elec = 0.0; // Batteries' heat loss to electronics
      float dT_bat_gain_from_elec = 0.0; //Batteries' heat gain from electronics
      float dT_bat_loss_to_room = 0.0; //Batteries' cooling from environment

      float dT_elec_own = 0.0; //Electronics' own heat
      float dT_elec_loss_to_bat = 0.0;//Electronics' heat loss to the battery pack
      float dT_elec_gain_from_bat = 0.0;//Electronics' heat gain from battery packs
      float dT_elec_loss_to_room = 0.0; //Electronics' heat loss to the environment

      /***************************************************************************/

      //Update the average current: I_bat_mean
      if (index > 0)
        {
    I_bat_mean = ((index) * I_bat_mean + I_bat) / (index + 1) ;
        }
      else
        {
    I_bat_mean = I_bat;
        }

      index = index + 1;


      //Calculate R_1A as a function of V_bat (internal resistance at 1A continuous)
      R_1A  =   0.014071 * (V_bat * V_bat * V_bat * V_bat)
        - 0.335324 * (V_bat * V_bat * V_bat)
        + 2.933404 * (V_bat * V_bat)
        - 11.243047 * V_bat
        + 16.897461;

      //Calculate R_2A as a function of V_bat (internal resistance at 2A continuous)
      R_2A  =   0.014420 * (V_bat * V_bat * V_bat * V_bat)
        - 0.316728 * (V_bat * V_bat * V_bat)
        + 2.559347 * (V_bat * V_bat)
        - 9.084076 * V_bat
        + 12.794176;

      //Calculate the slope by linear interpolation between R_1A and R_2A
      slope_A  =  (R_1A - R_2A) / (I_1A - I_2A);

      //Calculate intercept by linear interpolation between R1_A and R2_A
      intercept_b  =  R_1A - slope_A * R_1A;

      //Reload R_bat_model:
      R_bat_model  =  slope_A * I_bat_mean + intercept_b;

      //Calculate batteries' internal resistance: R_bat
      if ((V_bat > 7.5) && (has_passed_7v5_flag == 'N'))
        {
    R_bat = R_bat_init; //7.5 V not passed a first time
        }
      else
        {
    //Only update R_bat with positive outcomes: R_bat_model - R_bat_model_old
    //R_bat updated with the change in model R_bat is not equal value in the model!
    if ((R_bat_model - R_bat_model_old) > 0)
      {
        R_bat = R_bat + R_bat_model - R_bat_model_old;
      }
      else // The negative outcome of R_bat_model added to only part of R_bat
        {
          R_bat = R_bat + ( R_bat_neg_gain * (R_bat_model - R_bat_model_old));
        }
    //Make sure we initialize R_bat later
    has_passed_7v5_flag = 'Y';
        }

      //Save R_bat_model for use in the next function call
      R_bat_model_old = R_bat_model;

      //Debug code:
  #ifdef __dbg1
      if (index < 500)
        {
    printf("%c %f %f %f %f %f %f\n", has_passed_7v5_flag, R_1A, R_2A,
           slope_A, intercept_b, R_bat_model - R_bat_model_old, R_bat);
        }
  #endif

      /*****Calculate the 4 types of temperature change for the batteries******/

      //Calculate the batteries' own temperature change
      dT_bat_own = R_bat * I_bat * I_bat * sample_period  * battery_power_boost
                   / heat_cap_bat;

      //Calculate the batteries' heat loss to the electronics
      if (T_bat > T_elec)
        {
    dT_bat_loss_to_elec = K_bat_loss_to_elec * (T_bat - T_elec)
                          * sample_period;
        }
      else
        {
    dT_bat_loss_to_elec = 0.0;
        }

      //Calculate the batteries' heat gain from the electronics
      if (T_bat < T_elec)
        {
    dT_bat_gain_from_elec = K_bat_gain_from_elec * (T_elec - T_bat)
      * sample_period;
        }
      else
        {
    dT_bat_gain_from_elec = 0.0;
        }

      //Calculate the batteries' heat loss to environment
      dT_bat_loss_to_room = K_bat_to_room * T_bat * sample_period;
      /************************************************************************/



      /*****Calculate the 4 types of temperature change for the electronics****/

      //Calculate the electronics' own temperature change
      dT_elec_own = K_elec_heat_slope * sample_period;

      //Calculate the electronics' heat loss to the batteries
      if (T_elec > T_bat)
        {
    dT_elec_loss_to_bat =   K_elec_loss_to_bat * (T_elec - T_bat)
      * sample_period;
        }
      else
        {
    dT_elec_loss_to_bat = 0.0;
        }

      //Calculate the electronics' heat gain from the batteries
      if (T_elec < T_bat)
        {
    dT_elec_gain_from_bat = K_elec_gain_from_bat * (T_bat - T_elec)
      * sample_period;
        }
      else
        {
    dT_elec_gain_from_bat = 0.0;
        }

      //Calculate the electronics' heat loss to the environment
      dT_elec_loss_to_room = K_elec_to_room * T_elec * sample_period;

      /*****************************************************************************/
      //Debug code:
  #ifdef __dbg2
      if (index < 500)
        {
    printf("%f %f %f %f %f <> %f %f %f %f %f\n",dT_bat_own, dT_bat_loss_to_elec,
           dT_bat_gain_from_elec, dT_bat_loss_to_room, T_bat,
           dT_elec_own, dT_elec_loss_to_bat, dT_elec_gain_from_bat,
           dT_elec_loss_to_room, T_elec);
        }
  #endif



      //Refresh battery temperature
      T_bat =  T_bat + dT_bat_own + dT_bat_loss_to_elec
        + dT_bat_gain_from_elec + dT_bat_loss_to_room;

      //Refresh electronics temperature
      T_elec =  T_elec + dT_elec_own + dT_elec_loss_to_bat
        + dT_elec_gain_from_bat + dT_elec_loss_to_room;

      return T_bat;

    }
#endif


#ifdef DEBUG_VIRTUAL_BATT_TEMP
static    int TempFile = -1;
#endif


#ifndef DISABLE_VIRTUAL_BATT_TEMP

void      cUiCheckTemp(void);

#define   CALL_INTERVAL     400   // [mS]

#ifdef DEBUG_VIRTUAL_BATT_TEMP

void      cUiInitTemp(void)
{
  char    Buffer[250];
  int     BufferSize;
  float   Const[11];
  int     Tmp;
  char    *Str;
  LFILE   *pFile;

  mkdir("../prjs/TempTest",DIRPERMISSIONS);
  chmod("../prjs/TempTest",DIRPERMISSIONS);
  sync();

  Tmp  =  0;
  pFile = fopen ("../prjs/Const/TempConst.rtf","r");
  if (pFile != NULL)
  {
    do
    {
      Str           =  fgets(Buffer,250,pFile);
      Buffer[249]   =  0;
      if (Str != NULL)
      {
        if ((Buffer[0] != '/') && (Buffer[0] != '*') && (Buffer[0] != ' '))
        {
          Const[Tmp]  =  DATAF_NAN;
          if (sscanf(Buffer,"%f",&Const[Tmp]) != 1)
          {
            Const[Tmp]  =  DATAF_NAN;
          }
          Tmp++;
        }
      }
    }
    while (Str != NULL);
    fclose (pFile);

    R_bat_init            =  Const[0];
    heat_cap_bat          =  Const[1];
    K_bat_loss_to_elec    =  Const[2];
    K_bat_gain_from_elec  =  Const[3];
    K_bat_to_room         =  Const[4];
    battery_power_boost   =  Const[5];
    R_bat_neg_gain        =  Const[6];
    K_elec_heat_slope     =  Const[7];
    K_elec_loss_to_bat    =  Const[8];
    K_elec_gain_from_bat  =  Const[9];
    K_elec_to_room        =  Const[10];

  }

  TempFile  =  open("../prjs/TempTest/TempFile.rtf",O_CREAT | O_WRONLY | O_APPEND | O_SYNC,FILEPERMISSIONS);
  chmod("../prjs/TempTest/TempFile.rtf",FILEPERMISSIONS);
  if (TempFile >= MIN_HANDLE)
  {
    if (Tmp)
    {
      BufferSize  =  snprintf(Buffer,250,"* TempConst.rtf ************************\n");
    }
    else
    {
      BufferSize  =  snprintf(Buffer,250,"* Build in *****************************\n");
    }
    write(TempFile,Buffer,BufferSize);
    BufferSize  =  snprintf(Buffer,250,"  R_bat_init           = %13.9f\n",R_bat_init);
    write(TempFile,Buffer,BufferSize);
    BufferSize  =  snprintf(Buffer,250,"  heat_cap_bat         = %13.9f\n",heat_cap_bat);
    write(TempFile,Buffer,BufferSize);
    BufferSize  =  snprintf(Buffer,250,"  K_bat_loss_to_elec   = %13.9f\n",K_bat_loss_to_elec);
    write(TempFile,Buffer,BufferSize);
    BufferSize  =  snprintf(Buffer,250,"  K_bat_gain_from_elec = %13.9f\n",K_bat_gain_from_elec);
    write(TempFile,Buffer,BufferSize);
    BufferSize  =  snprintf(Buffer,250,"  K_bat_to_room        = %13.9f\n",K_bat_to_room);
    write(TempFile,Buffer,BufferSize);
    BufferSize  =  snprintf(Buffer,250,"  battery_power_boost  = %13.9f\n",battery_power_boost);
    write(TempFile,Buffer,BufferSize);
    BufferSize  =  snprintf(Buffer,250,"  R_bat_neg_gain       = %13.9f\n",R_bat_neg_gain);
    write(TempFile,Buffer,BufferSize);
    BufferSize  =  snprintf(Buffer,250,"  K_elec_heat_slope    = %13.9f\n",K_elec_heat_slope);
    write(TempFile,Buffer,BufferSize);
    BufferSize  =  snprintf(Buffer,250,"  K_elec_loss_to_bat   = %13.9f\n",K_elec_loss_to_bat);
    write(TempFile,Buffer,BufferSize);
    BufferSize  =  snprintf(Buffer,250,"  K_elec_gain_from_bat = %13.9f\n",K_elec_gain_from_bat);
    write(TempFile,Buffer,BufferSize);
    BufferSize  =  snprintf(Buffer,250,"  K_elec_to_room       = %13.9f\n",K_elec_to_room);
    write(TempFile,Buffer,BufferSize);
    BufferSize  =  snprintf(Buffer,250,"****************************************\n");
    write(TempFile,Buffer,BufferSize);
  }
  UiInstance.TempTimer  =  (UiInstance.MilliSeconds - CALL_INTERVAL);
  cUiCheckTemp();
}


void      cUiExitTemp(void)
{
  if (TempFile >= MIN_HANDLE)
  {
    close(TempFile);
    TempFile  =  -1;
  }
}
#endif

#endif


static IMGDATA DownloadSuccessSound[] = {
  opINFO, LC0(GET_VOLUME), LV0(0),
  opSOUND, LC0(PLAY), LV0(0), LCS, 'u','i','/','D','o','w','n','l','o','a','d','S','u','c','c','e','s','s', 0,
  opSOUND_READY,
  opOBJECT_END
};

void      cUiDownloadSuccessSound(void)
{
  VARDATA Locals[1];

  ExecuteByteCode(DownloadSuccessSound,NULL,Locals);
}

void      cUiAlive(void)
{
  UiInstance.SleepTimer  =  0;
}

RESULT    cUiInit(void)
{
  RESULT  Result = OK;
  ANALOG  *pAdcTmp;
  UBYTE   Tmp;
  char    Buffer[32];
  char    OsBuf[2000];
  int     Lng;
  int     Start;
  int     Sid;
  struct  ifreq Sifr;
  struct  utsname *pOs;
  struct  tm tm;

  cUiAlive();

  UiInstance.ReadyForWarnings   =  0;
  UiInstance.UpdateState        =  0;
  UiInstance.RunLedEnabled      =  0;
  UiInstance.RunScreenEnabled   =  0;
  UiInstance.TopLineEnabled     =  0;
  UiInstance.BackButtonBlocked  =  0;
  UiInstance.Escape             =  0;
  UiInstance.KeyBufIn           =  0;
  UiInstance.Keys               =  0;
  UiInstance.UiWrBufferSize     =  0;

  UiInstance.ScreenBusy         =  0;
  UiInstance.ScreenBlocked      =  0;
  UiInstance.ScreenPrgId        =  -1;
  UiInstance.ScreenObjId        =  -1;

  UiInstance.PowerInitialized   =  0;
  UiInstance.ShutDown           =  0;

  UiInstance.PowerShutdown      =  0;
  UiInstance.PowerState         =  0;
  UiInstance.VoltShutdown       =  0;
  UiInstance.Warnlight          =  0;
  UiInstance.Warning            =  0;
  UiInstance.WarningShowed      =  0;
  UiInstance.WarningConfirmed   =  0;
  UiInstance.VoltageState       =  0;

  UiInstance.pLcd               =  &UiInstance.LcdSafe;
  UiInstance.pAnalog            =  &UiInstance.Analog;

  UiInstance.Browser.PrgId      =  0;
  UiInstance.Browser.ObjId      =  0;

  UiInstance.Tbatt              =  0.0;
  UiInstance.Vbatt              =  9.0;
  UiInstance.Ibatt              =  0.0;
  UiInstance.Imotor             =  0.0;
  UiInstance.Iintegrated        =  0.0;
#ifdef Linux_X86
  UiInstance.Ibatt              =  0.1;
  UiInstance.Imotor             =  0.0;
#endif

  Result          =  dTerminalInit();

  UiInstance.ButtonFile =  cUiButtonOpenFile();
  UiInstance.LedRightRedTriggerFile   = cUiLedOpenTriggerFile("ev3:right", "red");
  UiInstance.LedLeftRedTriggerFile    = cUiLedOpenTriggerFile("ev3:left",  "red");
  UiInstance.LedRightGreenTriggerFile = cUiLedOpenTriggerFile("ev3:right", "green");
  UiInstance.LedLeftGreenTriggerFile  = cUiLedOpenTriggerFile("ev3:left",  "green");
  UiInstance.PowerFile  =  open(POWER_DEVICE_NAME,O_RDWR);
  UiInstance.AdcFile    =  open(ANALOG_DEVICE_NAME,O_RDWR | O_SYNC);

  dLcdInit((*UiInstance.pLcd).Lcd);

  snprintf(UiInstance.HwVers,HWVERS_SIZE,"compat");
  UiInstance.Hw = 0;

  if (UiInstance.AdcFile >= MIN_HANDLE)
  {
    pAdcTmp  =  (ANALOG*)mmap(0, sizeof(ANALOG), PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, UiInstance.AdcFile, 0);

    if (pAdcTmp == MAP_FAILED)
    {
#ifndef Linux_X86
      LogErrorNumber(ANALOG_SHARED_MEMORY);
      Result  =  FAIL;
#endif
    }
    else
    {
      UiInstance.pAnalog  =  pAdcTmp;
    }
  }
  else
  {
#ifndef Linux_X86
    LogErrorNumber(ANALOG_DEVICE_FILE_NOT_FOUND);
    Result  =  FAIL;
#endif
  }

  if (SPECIALVERS < '0')
  {
    snprintf(UiInstance.FwVers,FWVERS_SIZE,"V%4.2f",VERS);
  }
  else
  {
    snprintf(UiInstance.FwVers,FWVERS_SIZE,"V%4.2f%c",VERS,SPECIALVERS);
  }


  snprintf(Buffer,32,"%s %s",__DATE__,__TIME__);
  strptime((const char*)Buffer,(const char*)"%B %d %Y %H:%M:%S",(struct tm*)&tm);
  strftime(UiInstance.FwBuild,FWBUILD_SIZE,"%y%m%d%H%M",&tm);

  pOs  =  (struct utsname*)OsBuf;
  uname(pOs);

  snprintf(UiInstance.OsVers,OSVERS_SIZE,"%s %s",(*pOs).sysname,(*pOs).release);

  sprintf((char*)UiInstance.OsBuild,"?");

  Lng  =  strlen((*pOs).version) - 9;
  if (Lng > 0)
  {
    (*pOs).version[Lng++]  =  ' ';
    (*pOs).version[Lng++]  =  ' ';
    (*pOs).version[Lng++]  =  ' ';
    (*pOs).version[Lng++]  =  ' ';

    Lng     =  strlen((*pOs).version);
    Tmp     =  0;
    Start   =  0;

    while ((Start < Lng) && (Tmp == 0))
    {
      if (strptime((const char*)&(*pOs).version[Start],(const char*)"%B %d %H:%M:%S %Y",(struct tm*)&tm) != NULL)
      {
        Tmp  =  1;
      }
      Start++;
    }
    if (Tmp)
    {
      strftime((char*)UiInstance.OsBuild,OSBUILD_SIZE,"%y%m%d%H%M",&tm);
    }
  }

  UiInstance.IpAddr[0]  =  0;
  Sid   =  socket(AF_INET,SOCK_DGRAM,0);
  Sifr.ifr_addr.sa_family   =  AF_INET;
  strncpy(Sifr.ifr_name,"eth0",IFNAMSIZ - 1);
  if (ioctl(Sid,SIOCGIFADDR,&Sifr) >= 0)
  {
    snprintf(UiInstance.IpAddr,IPADDR_SIZE,"%s",inet_ntoa(((struct sockaddr_in *)&Sifr.ifr_addr)->sin_addr));
  }
  close(Sid);

  cUiButtonClearAll();

  UiInstance.BattIndicatorHigh      =  BATT_INDICATOR_HIGH;
  UiInstance.BattIndicatorLow       =  BATT_INDICATOR_LOW;
  UiInstance.BattWarningHigh        =  BATT_WARNING_HIGH;
  UiInstance.BattWarningLow         =  BATT_WARNING_LOW;
  UiInstance.BattShutdownHigh       =  BATT_SHUTDOWN_HIGH;
  UiInstance.BattShutdownLow        =  BATT_SHUTDOWN_LOW;

  UiInstance.Accu  =  0;
  if (UiInstance.PowerFile >= MIN_HANDLE)
  {
    read(UiInstance.PowerFile,Buffer,2);
    if (Buffer[0] == '1')
    {
      UiInstance.Accu               =  1;
      UiInstance.BattIndicatorHigh  =  ACCU_INDICATOR_HIGH;
      UiInstance.BattIndicatorLow   =  ACCU_INDICATOR_LOW;
      UiInstance.BattWarningHigh    =  ACCU_WARNING_HIGH;
      UiInstance.BattWarningLow     =  ACCU_WARNING_LOW;
      UiInstance.BattShutdownHigh   =  ACCU_SHUTDOWN_HIGH;
      UiInstance.BattShutdownLow    =  ACCU_SHUTDOWN_LOW;
    }
  }

#ifdef DEBUG_VIRTUAL_BATT_TEMP
  cUiInitTemp();
#endif

  return (Result);
}


RESULT    cUiOpen(void)
{
  RESULT  Result = FAIL;

  // Save screen before run
  LCDCopy(&UiInstance.LcdSafe,&UiInstance.LcdPool[0],sizeof(LCD));

  cUiButtonClearAll();
  cUiLedSetState(LED_GREEN_PULSE);
  UiInstance.RunScreenEnabled   =  3;
  UiInstance.RunLedEnabled      =  1;
  UiInstance.TopLineEnabled     =  0;

  Result  =  OK;

  return (Result);
}


RESULT    cUiClose(void)
{
  RESULT  Result = FAIL;

  UiInstance.Warning           &= ~WARNING_BUSY;
  UiInstance.RunLedEnabled      =  0;
  UiInstance.RunScreenEnabled   =  0;
  UiInstance.TopLineEnabled     =  1;
  UiInstance.BackButtonBlocked  =  0;
  UiInstance.Browser.NeedUpdate =  1;
  cUiLedSetState(LED_GREEN);

  cUiButtonClearAll();

  Result  =  OK;

  return (Result);
}


RESULT    cUiExit(void)
{
  RESULT  Result = FAIL;

#ifdef DEBUG_VIRTUAL_BATT_TEMP
  cUiExitTemp();
#endif

  Result  =  dTerminalExit();

  if (UiInstance.ButtonFile >= MIN_HANDLE) {
    close(UiInstance.ButtonFile);
  }

  if (UiInstance.AdcFile >= MIN_HANDLE)
  {
    munmap(UiInstance.pAnalog,sizeof(ANALOG));
    close(UiInstance.AdcFile);
  }

  if (UiInstance.PowerFile >= MIN_HANDLE)
  {
    close(UiInstance.PowerFile);
  }

  Result  =  OK;

  return (Result);
}

RESULT    cUiUpdateInput(void)
{
  UBYTE   Key;

  if (GetTerminalEnable())
  {

    if (dTerminalRead(&Key) == OK)
    {
      switch (Key)
      {
        case ' ' :
        {
          UiInstance.Escape  =  Key;
        }
        break;

        case '<' :
        {
          UiInstance.Escape  =  Key;
        }
        break;

        case '\r' :
        case '\n' :
        {
          if (UiInstance.KeyBufIn)
          {
            UiInstance.Keys      =  UiInstance.KeyBufIn;
            UiInstance.KeyBufIn  =  0;
          }
        }
        break;

        default :
        {
          UiInstance.KeyBuffer[UiInstance.KeyBufIn]  =  Key;
          if (++UiInstance.KeyBufIn >= KEYBUF_SIZE)
          {
            UiInstance.KeyBufIn--;
          }
          UiInstance.KeyBuffer[UiInstance.KeyBufIn]  =  0;
        }
        break;

      }
    }
  }
  dLcdRead();

  return (OK);
}


DATA8     cUiEscape(void)
{
  DATA8   Result;

  Result              =  UiInstance.Escape;
  UiInstance.Escape   =  0;

  return (Result);
}


void      cUiTestpin(DATA8 State)
{
  DATA8   Data8;

  Data8  =  State;
  if (UiInstance.PowerFile >= MIN_HANDLE)
  {
    write(UiInstance.PowerFile,&Data8,1);
  }
}


UBYTE     AtoN(DATA8 Char)
{
  UBYTE   Tmp = 0;

  if ((Char >= '0') && (Char <= '9'))
  {
    Tmp  =  (UBYTE)(Char - '0');
  }
  else
  {
    Char  |=  0x20;

    if ((Char >= 'a') && (Char <= 'f'))
    {
      Tmp  =  (UBYTE)(Char - 'a') + 10;
    }
  }

  return (Tmp);
}


void      cUiFlushBuffer(void)
{
  if (UiInstance.UiWrBufferSize)
  {
    if (GetTerminalEnable())
    {
      dTerminalWrite((UBYTE*)UiInstance.UiWrBuffer,UiInstance.UiWrBufferSize);
    }
    UiInstance.UiWrBufferSize              =  0;
  }
}


void      cUiWriteString(DATA8 *pString)
{
  while (*pString)
  {
    UiInstance.UiWrBuffer[UiInstance.UiWrBufferSize] = *pString;
    if (++UiInstance.UiWrBufferSize >= UI_WR_BUFFER_SIZE)
    {
      cUiFlushBuffer();
    }
    pString++;
  }
}

#define   SHUNT_IN        0.11              //  [Ohm]
#define   AMP_CIN         22.0              //  [Times]

#define   EP2_SHUNT_IN    0.05              //  [Ohm]
#define   EP2_AMP_CIN     15.0              //  [Times]

#define   SHUNT_OUT       0.055             //  [Ohm]
#define   AMP_COUT        19.0              //  [Times]

#define   VCE             0.05              //  [V]
#define   AMP_VIN         0.5               //  [Times]

#define   AVR_CIN         300
#define   AVR_COUT        30
#define   AVR_VIN         30

#define   CNT_V(C)    (((DATAF)C * (DATAF)ADC_REF) / ((DATAF)ADC_RES * (DATAF)1000.0))


void      cUiUpdateCnt(void)
{
  if (UiInstance.PowerInitialized)
  {
    UiInstance.CinCnt  *= (DATAF)(AVR_CIN - 1);
    UiInstance.CoutCnt *= (DATAF)(AVR_COUT - 1);
    UiInstance.VinCnt  *= (DATAF)(AVR_VIN - 1);

    UiInstance.CinCnt  +=  (DATAF)(*UiInstance.pAnalog).BatteryCurrent;
    UiInstance.CoutCnt +=  (DATAF)(*UiInstance.pAnalog).MotorCurrent;
    UiInstance.VinCnt  +=  (DATAF)(*UiInstance.pAnalog).Cell123456;

    UiInstance.CinCnt  /= (DATAF)(AVR_CIN);
    UiInstance.CoutCnt /= (DATAF)(AVR_COUT);
    UiInstance.VinCnt  /= (DATAF)(AVR_VIN);
  }
  else
  {
    UiInstance.CinCnt   =  (DATAF)(*UiInstance.pAnalog).BatteryCurrent;
    UiInstance.CoutCnt  =  (DATAF)(*UiInstance.pAnalog).MotorCurrent;
    UiInstance.VinCnt   =  (DATAF)(*UiInstance.pAnalog).Cell123456;
    UiInstance.PowerInitialized  =  1;
  }
}


void      cUiUpdatePower(void)
{
#ifndef Linux_X86
  DATAF   CinV;
  DATAF   CoutV;

  if ((UiInstance.Hw == FINAL) || (UiInstance.Hw == FINALB))
  {
    CinV                =  CNT_V(UiInstance.CinCnt) / AMP_CIN;
    UiInstance.Vbatt    =  (CNT_V(UiInstance.VinCnt) / AMP_VIN) + CinV + VCE;

    UiInstance.Ibatt    =  CinV / SHUNT_IN;
    CoutV               =  CNT_V(UiInstance.CoutCnt) / AMP_COUT;
    UiInstance.Imotor   =  CoutV / SHUNT_OUT;

  }
  else
  {
    CinV                =  CNT_V(UiInstance.CinCnt) / EP2_AMP_CIN;
    UiInstance.Vbatt    =  (CNT_V(UiInstance.VinCnt) / AMP_VIN) + CinV + VCE;

    UiInstance.Ibatt    =  CinV / EP2_SHUNT_IN;
    UiInstance.Imotor   =  0;

  }

#endif
#ifdef DEBUG_TEMP_SHUTDOWN

  UiInstance.Vbatt  =  7.0;
  UiInstance.Ibatt  =  5.0;

#endif
}


//#define   TRACK_UPDATE


/*! \page pmbattind Battery Indicator
 *
 *  <hr size="1"/>
 *\n
 *  The battery indicator is located in the top right corner of the display and is updated when the display
 *  is updated and the "TopLine" is enabled (and at least every 400 mS).
 *  The range from BATT_INDICATOR_LOW to BATT_INDICATOR_HIGH is divided into three pictures and the appropriate
 *  one is shown: an empty battery when the voltage is below or equal to BATT_INDICATOR_LOW and a full
 *  battery when the voltage is above or equal to BATT_INDICATOR_HIGH.
 *  If a rechargeable accu is used the ranges ACCU_INDICATOR_LOW and ACCU_INDICATOR_HIGH is used.
\verbatim


                          V
                          ^
                          |
                          |
                          |
                          |    Full battery picture
  XXXX_INDICATOR_HIGH     -         -         -         -
                          |    Three/fourth full battery picture
                          -         -         -         -
                          |    Half full battery picture
                          -         -         -         -
                          |    One/fourth full battery picture
  XXXX_INDICATOR_LOW      -         -         -         -
                          |    Empty battery picture
                          |
                          |
                          |
                          |
                          |
                          |
                          |
                          '-----------------------------------

  XXXX = BATT/ACCU

 \endverbatim
 */


#define   TOP_BATT_ICONS     5
UBYTE     TopLineBattIconMap[TOP_BATT_ICONS] =
{
  SICON_BATT_0,           //  0
  SICON_BATT_1,           //  1
  SICON_BATT_2,           //  2
  SICON_BATT_3,           //  3
  SICON_BATT_4            //  4
};


#define   TOP_BT_ICONS       4
UBYTE     TopLineBtIconMap[TOP_BT_ICONS] =
{
  SICON_BT_ON,            //  001
  SICON_BT_VISIBLE,       //  011
  SICON_BT_CONNECTED,     //  101
  SICON_BT_CONNVISIB,     //  111
};


#define   TOP_WIFI_ICONS     4
UBYTE     TopLineWifiIconMap[TOP_WIFI_ICONS] =
{
  SICON_WIFI_3,           //  001
  SICON_WIFI_3,           //  011
  SICON_WIFI_CONNECTED,   //  101
  SICON_WIFI_CONNECTED,   //  111
};


void      cUiUpdateTopline(void)
{
  DATA16  X1,X2;
  DATA16  V;
  DATA8   BtStatus;
  DATA8   WifiStatus;
  DATA8   TmpStatus;
  DATA8   Name[NAME_LENGTH + 1];

#ifdef TRACK_UPDATE
  static  DATA16 Counter = 0;
  static  char   Buffer[10];
#endif
#ifdef DEBUG_VIRTUAL_BATT_TEMP
  char    TempBuf[10];
#endif
#ifdef ENABLE_MEMORY_TEST
  DATA32  Total;
  DATA32  Free;
#endif

  if (UiInstance.TopLineEnabled)
  {
    // Clear top line
    LCDClearTopline(UiInstance.pLcd);

#ifdef TRACK_UPDATE
    Counter++;
    sprintf(Buffer,"%d",Counter);
    dLcdDrawText((*UiInstance.pLcd).Lcd,FG_COLOR,16,1,SMALL_FONT,(DATA8*)Buffer);
#endif
    // Show BT status
    TmpStatus   =  0;
    BtStatus    =  cComGetBtStatus();
    if (BtStatus > 0)
    {
      TmpStatus   =  1;
      BtStatus >>= 1;
      if ((BtStatus >= 0 ) && (BtStatus < TOP_BT_ICONS))
      {
        dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,0,1,SMALL_ICON,TopLineBtIconMap[BtStatus]);
      }
    }
    if (UiInstance.BtOn != TmpStatus)
    {
      UiInstance.BtOn       =  TmpStatus;
      UiInstance.UiUpdate   =  1;
    }

    // Show WIFI status
    TmpStatus   =  0;
    WifiStatus  =  cComGetWifiStatus();
    if (WifiStatus > 0)
    {
      TmpStatus   =  1;
      WifiStatus >>= 1;
      if ((WifiStatus >= 0 ) && (WifiStatus < TOP_WIFI_ICONS))
      {
        dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,16,1,SMALL_ICON,TopLineWifiIconMap[WifiStatus]);
      }
    }
    if (UiInstance.WiFiOn != TmpStatus)
    {
      UiInstance.WiFiOn     =  TmpStatus;
      UiInstance.UiUpdate   =  1;
    }
#ifdef DEBUG_VIRTUAL_BATT_TEMP
    snprintf(TempBuf,10,"%4.1f",UiInstance.Tbatt);
    dLcdDrawText((*UiInstance.pLcd).Lcd,FG_COLOR,32,1,SMALL_FONT,(DATA8*)TempBuf);
#endif

    // Show brick name
    cComGetBrickName(NAME_LENGTH + 1,Name);

    X1  =  dLcdGetFontWidth(SMALL_FONT);
    X2  =  LCD_WIDTH / X1;
    X2 -=  strlen((char*)Name);
    X2 /=  2;
    X2 *=  X1;
    dLcdDrawText((*UiInstance.pLcd).Lcd,FG_COLOR,X2,1,SMALL_FONT,Name);

#ifdef ENABLE_PERFORMANCE_TEST
    X1  =  100;
    X2  =  (DATA16)VMInstance.PerformTime;
    X2  =  (X2 * 40) / 1000;
    if (X2 > 40)
    {
      X2  =  40;
    }
    dLcdRect((*UiInstance.pLcd).Lcd,FG_COLOR,X1,3,40,5);
    dLcdFillRect((*UiInstance.pLcd).Lcd,FG_COLOR,X1,3,X2,5);
#endif
#ifdef ENABLE_LOAD_TEST
    X1  =  100;
    X2  =  (DATA16)(UiInstance.Iintegrated * 1000.0);
    X2  =  (X2 * 40) / (DATA16)(LOAD_SHUTDOWN_HIGH * 1000.0);
    if (X2 > 40)
    {
      X2  =  40;
    }
    dLcdRect((*UiInstance.pLcd).Lcd,FG_COLOR,X1,3,40,5);
    dLcdFillRect((*UiInstance.pLcd).Lcd,FG_COLOR,X1,3,X2,5);
#endif
#ifdef ENABLE_MEMORY_TEST
    cMemoryGetUsage(&Total,&Free,0);

    X1  =  100;
    X2  =  (DATA16)((((Total - (DATA32)LOW_MEMORY) - Free) * (DATA32)40) / (Total - (DATA32)LOW_MEMORY));
    if (X2 > 40)
    {
      X2  =  40;
    }
    dLcdRect((*UiInstance.pLcd).Lcd,FG_COLOR,X1,3,40,5);
    dLcdFillRect((*UiInstance.pLcd).Lcd,FG_COLOR,X1,3,X2,5);
#endif

// 112,118,124,130,136,142,148
#ifdef ENABLE_STATUS_TEST
    X1  =  100;
    X2  =  102;
    for (V = 0;V < 7;V++)
    {
      dLcdDrawPixel((*UiInstance.pLcd).Lcd,FG_COLOR,X2 + (V * 6),5);
      dLcdDrawPixel((*UiInstance.pLcd).Lcd,FG_COLOR,X2 + (V * 6),4);

      if ((VMInstance.Status & (0x40 >> V)))
      {
        dLcdFillRect((*UiInstance.pLcd).Lcd,FG_COLOR,X1 + (V * 6),2,5,6);
      }
    }
#else
#ifdef ALLOW_DEBUG_PULSE
/*
  GUI SLOT running                        1... ....
  USER SLOT running                       .1.. ....
  CMD SLOT running                        ..1. ....
  TRM SLOT running                        ...1 ....
  DEBUG SLOT running                      .... 1...
  KEY active                              .... .1..
  BROWSER running                         .... ..1.
  UI running background                   .... ...1
*/
    if (VMInstance.PulseShow)
    {
      if (VMInstance.PulseShow == 1)
      {
        X1  =  100;
        X2  =  102;
        for (V = 0;V < 8;V++)
        {
          if ((VMInstance.Pulse & (0x80 >> V)))
          {
            dLcdFillRect((*UiInstance.pLcd).Lcd,FG_COLOR,X1 + (V * 5),2,5,6);
          }

          dLcdInversePixel((*UiInstance.pLcd).Lcd,X2 + (V * 5),5);
          dLcdInversePixel((*UiInstance.pLcd).Lcd,X2 + (V * 5),4);
        }
        VMInstance.Pulse  =  0;
        VMInstance.PulseShow  =  2;
      }
      else
      {
        VMInstance.PulseShow  =  1;
      }
    }
#endif
#endif

    // Calculate number of icons
    X1  =  dLcdGetIconWidth(SMALL_ICON);
    X2  =  (LCD_WIDTH - X1) / X1;

#ifndef Linux_X86
    // Show USB status
    if (cComGetUsbStatus())
    {
      dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,(X2 - 1) * X1,1,SMALL_ICON,SICON_USB);
    }
#endif

#ifdef DEBUG_BACK_BLOCKED
    if (UiInstance.BackButtonBlocked)
    {
      dLcdFillRect((*UiInstance.pLcd).Lcd,FG_COLOR,100,2,5,6);
    }
#endif

    // Show battery
    V   =  (DATA16)(UiInstance.Vbatt * 1000.0);
    V  -=  UiInstance.BattIndicatorLow;
    V   =  (V * (TOP_BATT_ICONS - 1)) / (UiInstance.BattIndicatorHigh - UiInstance.BattIndicatorLow);
    if (V > (TOP_BATT_ICONS - 1))
    {
      V  =  (TOP_BATT_ICONS - 1);
    }
    if (V < 0)
    {
      V  =  0;
    }
    dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,X2 * X1,1,SMALL_ICON,TopLineBattIconMap[V]);

#ifdef DEBUG_RECHARGEABLE
    if (UiInstance.Accu == 0)
    {
      dLcdDrawPixel((*UiInstance.pLcd).Lcd,FG_COLOR,176,4);
      dLcdDrawPixel((*UiInstance.pLcd).Lcd,FG_COLOR,176,5);
    }
#endif

    // Show bottom line
    dLcdDrawLine((*UiInstance.pLcd).Lcd,FG_COLOR,0,TOPLINE_HEIGHT,LCD_WIDTH,TOPLINE_HEIGHT);
  }
}


void      cUiUpdateLcd(void)
{
  UiInstance.Font  =  NORMAL_FONT;
  cUiUpdateTopline();
  dLcdUpdate(UiInstance.pLcd);
}


#include  "mindstorms.xbm"
#include  "Ani1x.xbm"
#include  "Ani2x.xbm"
#include  "Ani3x.xbm"
#include  "Ani4x.xbm"
#include  "Ani5x.xbm"
#include  "Ani6x.xbm"
#include  "POP3.xbm"


void      cUiRunScreen(void)
{ // 100mS

  if (UiInstance.ScreenBlocked  ==  0)
  {
    switch (UiInstance.RunScreenEnabled)
    {
      case 0 :
      {
      }
      break;

      case 1 :
      { // 100mS

        UiInstance.RunScreenEnabled++;
      }
      break;

      case 2 :
      { // 200mS

        UiInstance.RunScreenEnabled++;
      }
      break;

      case 3 :
      {
        // Init animation number
        UiInstance.RunScreenNumber  =  1;

        // Clear screen
        LCDClear((*UiInstance.pLcd).Lcd);
        cUiUpdateLcd();


        // Enable top line

        // Draw fixed image
        dLcdDrawPicture((*UiInstance.pLcd).Lcd,FG_COLOR,8,39,mindstorms_width,mindstorms_height,(UBYTE*)mindstorms_bits);
        cUiUpdateLcd();

        // Draw user program name
        dLcdDrawText((*UiInstance.pLcd).Lcd,FG_COLOR,8,79,1,(DATA8*)VMInstance.Program[USER_SLOT].Name);
        cUiUpdateLcd();

        UiInstance.RunScreenTimer     =  0;
        UiInstance.RunScreenCounter   =  0;

        UiInstance.RunScreenEnabled++;
      }
      // FALL THROUGH

      case 4 :
      { //   0mS -> Ani1

        if (UiInstance.RunLedEnabled)
        {
          cUiLedSetState(LED_GREEN_PULSE);
        }

        dLcdDrawPicture((*UiInstance.pLcd).Lcd,FG_COLOR,8,67,Ani1x_width,Ani1x_height,(UBYTE*)Ani1x_bits);
        cUiUpdateLcd();

        UiInstance.RunScreenTimer  =  UiInstance.MilliSeconds;
        UiInstance.RunScreenEnabled++;
      }
      break;

      case 5 :
      { // 100mS

        UiInstance.RunScreenEnabled++;
      }
      break;

      case 6 :
      { // 200mS

        UiInstance.RunScreenEnabled++;
      }
      break;

      case 7 :
      { // 300mS

        UiInstance.RunScreenEnabled++;
      }
      break;

      case 8 :
      { // 400mS

        UiInstance.RunScreenEnabled++;
      }
      break;

      case 9 :
      { // 500mS -> Ani2

        dLcdDrawPicture((*UiInstance.pLcd).Lcd,FG_COLOR,8,67,Ani2x_width,Ani2x_height,(UBYTE*)Ani2x_bits);
        cUiUpdateLcd();

        UiInstance.RunScreenEnabled++;
      }
      break;

      case 10 :
      { // 600mS -> Ani3

        dLcdDrawPicture((*UiInstance.pLcd).Lcd,FG_COLOR,8,67,Ani3x_width,Ani3x_height,(UBYTE*)Ani3x_bits);
        cUiUpdateLcd();

        UiInstance.RunScreenEnabled++;
      }
      break;

      case 11 :
      { // 700ms -> Ani4

        dLcdDrawPicture((*UiInstance.pLcd).Lcd,FG_COLOR,8,67,Ani4x_width,Ani4x_height,(UBYTE*)Ani4x_bits);
        cUiUpdateLcd();

        UiInstance.RunScreenEnabled++;
      }
      break;

      case 12 :
      { // 800ms -> Ani5

        dLcdDrawPicture((*UiInstance.pLcd).Lcd,FG_COLOR,8,67,Ani5x_width,Ani5x_height,(UBYTE*)Ani5x_bits);
        cUiUpdateLcd();

        UiInstance.RunScreenEnabled++;
      }
      break;

      default :
      { // 900ms -> Ani6

        dLcdDrawPicture((*UiInstance.pLcd).Lcd,FG_COLOR,8,67,Ani6x_width,Ani6x_height,(UBYTE*)Ani6x_bits);
        cUiUpdateLcd();

        UiInstance.RunScreenEnabled   =  4;
      }
      break;

    }
  }
}

#ifndef   DISABLE_LOW_VOLTAGE

/*! \page pmbattsd Low Voltage Shutdown
 *
 *  <hr size="1"/>
 *\n
 *  Low voltage shutdown is testing the battery voltage against BATT_SHUTDOWN_LOW level and when the voltage is
 *  below that a running user program will be stopped, running motors will stop, a popup window will appear on the screen
 *  and a timer will be started. If the voltage not rises over the BATT_SHUTDOWN_LOW level within LOW_VOLTAGE_SHUTDOWN_TIME
 *  the brick will save to flash and power down.
\verbatim


                          V
                          ^
                          |
                          |
                          |
                          |
  XXXX_WARNING_HIGH       -    Make LEDs normal
                          |
                          |
                          |    Keep LED colors
                          |
                          |
  XXXX_WARNING_LOW        -    Make LEDs ORANGE
                          |
  XXXX_SHUTDOWN_HIGH      -         -         -         -
                          |
                          |    Reset shutdown timer
                          |
  XXXX_SHUTDOWN_LOW       -         -         -         -
                          |
                          |    Show popup and stop user program immediately
                          |
                          |    Power down after LOW_VOLTAGE_SHUTDOWN_TIME
                          |
                          |
                          '-----------------------------------

  XXXX = BATT/ACCU

 \endverbatim
 */

void      cUiCheckVoltage(void)
{ // 400mS

  cUiUpdatePower();

  if (UiInstance.Vbatt >= UiInstance.BattWarningHigh)
  {
    UiInstance.Warning &= ~WARNING_BATTLOW;
  }

  if (UiInstance.Vbatt <= UiInstance.BattWarningLow)
  {
    UiInstance.Warning |=  WARNING_BATTLOW;
  }

  if (UiInstance.Vbatt >= UiInstance.BattShutdownHigh)
  { // Good

    UiInstance.Warning &= ~WARNING_VOLTAGE;
  }

  if (UiInstance.Vbatt < UiInstance.BattShutdownLow)
  { // Bad

    UiInstance.Warning |=  WARNING_VOLTAGE;

    ProgramEnd(USER_SLOT);

    if ((UiInstance.MilliSeconds - UiInstance.VoltageTimer) >= LOW_VOLTAGE_SHUTDOWN_TIME)
    { // Realy bad
      #ifdef DEBUG
      printf("Shutting down due to low battery\n");
      #endif
      UiInstance.ShutDown         =  1;
    }
  }
  else
  {
    UiInstance.VoltageTimer  =  UiInstance.MilliSeconds;
  }
}

#endif

#ifdef ENABLE_HIGH_CURRENT
/*! \page pmloadsd High Load Shutdown
 *
 *  <hr size="1"/>
 *\n
 *  High load shutdown is based on the total current "I" drawn from the battery. A virtual integrated current "Iint"
 *  simulates the load on and the temperature inside the battery and is maintained by integrating the draw current over time.
 *  LOAD_BREAK_EVEN is the level that defines if "Iint" integrates up or down.
 *  If or when "Iint" reaches the limit (LOAD_SHUTDOWN_HIGH or LOAD_SHUTDOWN_FAIL) the user program is stopped, motors are stopped
 *  and a popup screen appears on the display.
 *  The popup screen disappears when a button is pressed but no user program can be activated until the "Iint" has decreased below
 *  LOAD_BREAK_EVEN level.
 *
 \verbatim


                          I
                          ^
  LOAD_SHUTDOWN_FAIL     -|-         -         -         -         -         -|-
                          |                                                   |
                          |                                                   |
                          |                          Iint                     |
  LOAD_SHUTDOWN_HIGH     -|-         -         -    -/-                       |
                          |          ______         /.                        |
                          |         /      \       / .                        |
                          |        /        \_____/  .                        |
                          |   ,---/-,             ,--- I                      |
  LOAD_BREAK_EVEN        -|---'  /  '-----, ,-----'  .               I -------'
                          |     /         | |        .                        .
                          |____/          '-'        .                        .
                          |                          .                        .
                          '--------------------------|------------------------|------> T
                          | B |  A  |  B  |C|  B  |A|D|                      |E|



  A. When I is greater than LOAD_BREAK_EVEN, Iint increases with a slope given by the difference
     (I - LOAD_BREAK_EVEN) multiplied by LOAD_SLOPE_UP.

  B. When I is equal to LOAD_BREAK_EVEN, Iint stays at its level.

  C. When I is lower than LOAD_BREAK_EVEN, Iint decreases with a slope given by the difference
     (LOAD_BREAK_EVEN - I) multiplied by LOAD_SLOPE_DOWN.

  D. When Iint reaches the LOAD_SHUTDOWN_HIGH level a running user program is stopped
     and running motors are stopped.

  E. When I reaches the LOAD_SHUTDOWN_FAIL level a running user program is stopped
     and running motors are stopped.

 \endverbatim
 */


void      cUiCheckPower(UWORD Time)
{ // 400mS

  DATA16  X,Y;
  DATAF   I;
  DATAF   Slope;

  I  =  UiInstance.Ibatt + UiInstance.Imotor;

  if (I > LOAD_BREAK_EVEN)
  {
    Slope  =  LOAD_SLOPE_UP;
  }
  else
  {
    Slope  =  LOAD_SLOPE_DOWN;
  }

  UiInstance.Iintegrated +=  (I - LOAD_BREAK_EVEN) * (Slope * (DATAF)Time / 1000.0);

  if (UiInstance.Iintegrated < 0.0)
  {
    UiInstance.Iintegrated  =  0.0;
  }
  if (UiInstance.Iintegrated > LOAD_SHUTDOWN_FAIL)
  {
    UiInstance.Iintegrated  =  LOAD_SHUTDOWN_FAIL;
  }

  if ((UiInstance.Iintegrated >= LOAD_SHUTDOWN_HIGH) || (I >= LOAD_SHUTDOWN_FAIL))
  {
    UiInstance.Warning       |=  WARNING_CURRENT;
    UiInstance.PowerShutdown  =  1;
  }
  if (UiInstance.Iintegrated <= LOAD_BREAK_EVEN)
  {
    UiInstance.Warning       &= ~WARNING_CURRENT;
    UiInstance.PowerShutdown  =  0;
  }

  if (UiInstance.PowerShutdown)
  {
    if (UiInstance.ScreenBlocked == 0)
    {
      if (ProgramStatus(USER_SLOT) != STOPPED)
      {
        UiInstance.PowerState  =  1;
      }
    }
    ProgramEnd(USER_SLOT);
  }

  switch (UiInstance.PowerState)
  {
    case 0 :
    {
      if (UiInstance.PowerShutdown)
      {
        UiInstance.PowerState++;
      }
    }
    break;

    case 1 :
    {
      if (!UiInstance.ScreenBusy)
      {
        if ((!UiInstance.VoltShutdown))
        {
          UiInstance.ScreenBlocked  =  1;
          UiInstance.PowerState++;
        }
      }
    }
    break;

    case 2 :
    {
      LCDCopy(&UiInstance.LcdSafe,&UiInstance.LcdSave,sizeof(UiInstance.LcdSave));
      UiInstance.PowerState++;
    }
    break;

    case 3 :
    {
      X  =  16;
      Y  =  52;

      dLcdDrawPicture((*UiInstance.pLcd).Lcd,FG_COLOR,X,Y,POP3_width,POP3_height,(UBYTE*)POP3_bits);

      dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,X + 48,Y + 10,LARGE_ICON,WARNSIGN);
      dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,X + 72,Y + 10,LARGE_ICON,WARN_POWER);
      dLcdDrawLine((*UiInstance.pLcd).Lcd,FG_COLOR,X + 5,Y + 39,X + 138,Y + 39);
      dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,X + 56,Y + 40,LARGE_ICON,YES_SEL);
      dLcdUpdate(UiInstance.pLcd);
      cUiButtonClearAll();
      UiInstance.PowerState++;
    }
    break;

    case 4 :
    {
      if (cUiButtonGetShortPress(ENTER_BUTTON))
      {
        if (UiInstance.ScreenBlocked)
        {
          UiInstance.ScreenBlocked  =  0;
        }
        LCDCopy(&UiInstance.LcdSave,&UiInstance.LcdSafe,sizeof(UiInstance.LcdSafe));
        dLcdUpdate(UiInstance.pLcd);
        UiInstance.PowerState++;
      }
      if ((!UiInstance.PowerShutdown))
      {
        UiInstance.PowerState  =  0;
      }
    }
    break;

    case 5 :
    {
      if ((!UiInstance.PowerShutdown))
      {
        UiInstance.PowerState  =  0;
      }
    }
    break;

  }
}
#endif


#ifndef DISABLE_VIRTUAL_BATT_TEMP

/*! \page pmtempsd High Temperature Shutdown
 *
 *  <hr size="1"/>
 *\n
 *  High temperature shutdown is based on the total current drawn from the battery and the battery voltage. An estimated
 *  temperature rise is calculated from the battery voltage, current, internal resistance and time
 *
 *
 \verbatim





 \endverbatim
 */

void      cUiCheckTemp(void)
{
  if ((UiInstance.MilliSeconds - UiInstance.TempTimer) >= CALL_INTERVAL)
  {
    UiInstance.TempTimer +=  CALL_INTERVAL;
    UiInstance.Tbatt      =  new_bat_temp(UiInstance.Vbatt,(UiInstance.Ibatt * (DATAF)1.1));
#ifdef DEBUG_TEMP_SHUTDOWN
    UiInstance.Tbatt     +=  35.0;
#endif
#ifdef DEBUG_VIRTUAL_BATT_TEMP
    char    Buffer[250];
    int     BufferSize;

    if (TempFile >= MIN_HANDLE)
    {
      BufferSize  =  snprintf(Buffer,250,"%8.1f,%9.6f,%9.6f,%11.6f\n",(float)UiInstance.MilliSeconds / (float)1000,UiInstance.Vbatt,UiInstance.Ibatt,UiInstance.Tbatt);
      write(TempFile,Buffer,BufferSize);
    }
#endif
  }

  if (UiInstance.Tbatt >= TEMP_SHUTDOWN_WARNING)
  {
    UiInstance.Warning   |=  WARNING_TEMP;
  }
  else
  {
    UiInstance.Warning   &= ~WARNING_TEMP;
  }


  if (UiInstance.Tbatt >= TEMP_SHUTDOWN_FAIL)
  {
    ProgramEnd(USER_SLOT);
    UiInstance.ShutDown         =  1;
  }
}
#endif

#ifndef   DISABLE_LOW_MEMORY

void      cUiCheckMemory(void)
{ // 400mS

  DATA32  Total;
  DATA32  Free;

  cMemoryGetUsage(&Total,&Free,0);

  if (Free > LOW_MEMORY)
  { // Good

    UiInstance.Warning   &= ~WARNING_MEMORY;
  }
  else
  { // Bad

    UiInstance.Warning   |=  WARNING_MEMORY;
  }

}

#endif


void      cUiCheckAlive(UWORD Time)
{
  ULONG   Timeout;

  UiInstance.SleepTimer +=  Time;

  if (UiInstance.Activated & BUTTON_ACTIVATION_ALIVE)
  {
    UiInstance.Activated &= ~BUTTON_ACTIVATION_ALIVE;
    cUiAlive();
  }
  Timeout  =  (ULONG)GetSleepMinutes();

  if (Timeout)
  {
    if (UiInstance.SleepTimer >= (Timeout * 60000L))
    {
      UiInstance.ShutDown         =  1;
    }
  }
}


void      cUiUpdate(UWORD Time)
{
  DATA8   Warning;
  DATA8   Tmp;

  UiInstance.MilliSeconds += (ULONG)Time;

  cUiUpdateButtons(Time);
  cUiUpdateInput();
  cUiUpdateCnt();

#ifdef MAX_FRAMES_PER_SEC
  UiInstance.DisplayTimer += (ULONG)Time;
  if (UiInstance.DisplayTimer >= (1000 / MAX_FRAMES_PER_SEC))
  {
    UiInstance.AllowUpdate    =  1;
    if (UiInstance.ScreenBusy == 0)
    {
      dLcdAutoUpdate();
    }
  }
#endif


  if ((UiInstance.MilliSeconds - UiInstance.UpdateStateTimer) >= 50)
  {
    UiInstance.UpdateStateTimer  =  UiInstance.MilliSeconds;

    if (!UiInstance.Event)
    {
      UiInstance.Event  =  cComGetEvent();
    }

    switch (UiInstance.UpdateState++)
    {
      case 0 :
      { //  50 mS

#ifdef ENABLE_HIGH_CURRENT
        if (UiInstance.ReadyForWarnings)
        {
          cUiCheckPower(400);
        }
#endif
#ifndef DISABLE_VIRTUAL_BATT_TEMP
        if (UiInstance.ReadyForWarnings)
        {
          if (!UiInstance.Accu)
          {
            cUiCheckTemp();
          }
        }
#endif
      }
      break;

      case 1 :
      { // 100 mS

        cUiRunScreen();
      }
      break;

      case 2 :
      { // 150 mS

        cUiCheckAlive(400);
#ifndef DISABLE_LOW_MEMORY
        if (UiInstance.ReadyForWarnings)
        {
          cUiCheckMemory();
        }
#endif
      }
      break;

      case 3 :
      { // 200 mS

        cUiRunScreen();
      }
      break;

      case 4 :
      { // 250 mS

#ifndef DISABLE_LOW_VOLTAGE
        if (UiInstance.ReadyForWarnings)
        {
          cUiCheckVoltage();
        }
#endif
      }
      break;

      case 5 :
      { // 300 mS

        cUiRunScreen();
      }
      break;

      case 6 :
      { // 350 mS

        if (UiInstance.ScreenBusy == 0)
        {
          cUiUpdateTopline();
          dLcdUpdate(UiInstance.pLcd);
        }
      }
      break;

      default :
      { // 400 mS

        cUiRunScreen();
        UiInstance.UpdateState       =  0;
        UiInstance.ReadyForWarnings  = 1;
      }
      break;

    }

    if (UiInstance.Warning)
    { // Some warning present

      if (!UiInstance.Warnlight)
      { // If not on - turn orange light on

        UiInstance.Warnlight  =  1;
        cUiLedSetState(UiInstance.LedState);
      }
    }
    else
    { // No warning

      if (UiInstance.Warnlight)
      { // If orange light on - turn it off

        UiInstance.Warnlight  =  0;
        cUiLedSetState(UiInstance.LedState);
      }
    }

    // Get valid popup warnings
    Warning   =  UiInstance.Warning & WARNINGS;

    // Find warnings that has not been showed
    Tmp       =  Warning & ~UiInstance.WarningShowed;

    if (Tmp)
    { // Show popup

      if (!UiInstance.ScreenBusy)
      { // Wait on screen

        if (UiInstance.ScreenBlocked  ==  0)
        {
          LCDCopy(&UiInstance.LcdSafe,&UiInstance.LcdSave,sizeof(UiInstance.LcdSave));
          dLcdDrawPicture((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_X,vmPOP3_ABS_Y,POP3_width,POP3_height,(UBYTE*)POP3_bits);

          if (Tmp & WARNING_TEMP)
          {
            dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_ICON_X1,vmPOP3_ABS_WARN_ICON_Y,LARGE_ICON,WARNSIGN);
            dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_ICON_X2,vmPOP3_ABS_WARN_SPEC_ICON_Y,LARGE_ICON,WARN_POWER);
            dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_ICON_X3,vmPOP3_ABS_WARN_SPEC_ICON_Y,LARGE_ICON,TO_MANUAL);
            UiInstance.WarningShowed |=  WARNING_TEMP;
          }
          else
          {
            if (Tmp & WARNING_CURRENT)
            {
              dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_ICON_X1,vmPOP3_ABS_WARN_ICON_Y,LARGE_ICON,WARNSIGN);
              dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_ICON_X2,vmPOP3_ABS_WARN_SPEC_ICON_Y,LARGE_ICON,WARN_POWER);
              dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_ICON_X3,vmPOP3_ABS_WARN_SPEC_ICON_Y,LARGE_ICON,TO_MANUAL);
              UiInstance.WarningShowed |=  WARNING_CURRENT;
            }
            else
            {
              if (Tmp & WARNING_VOLTAGE)
              {
                dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_ICON_X,vmPOP3_ABS_WARN_ICON_Y,LARGE_ICON,WARNSIGN);
                dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_SPEC_ICON_X,vmPOP3_ABS_WARN_SPEC_ICON_Y,LARGE_ICON,WARN_BATT);
                UiInstance.WarningShowed |=  WARNING_VOLTAGE;
              }
              else
              {
                if (Tmp & WARNING_MEMORY)
                {
                  dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_ICON_X,vmPOP3_ABS_WARN_ICON_Y,LARGE_ICON,WARNSIGN);
                  dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_SPEC_ICON_X,vmPOP3_ABS_WARN_SPEC_ICON_Y,LARGE_ICON,WARN_MEMORY);
                  UiInstance.WarningShowed |=  WARNING_MEMORY;
                }
                else
                {
                  if (Tmp & WARNING_DSPSTAT)
                  {
                    dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_ICON_X,vmPOP3_ABS_WARN_ICON_Y,LARGE_ICON,WARNSIGN);
                    dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_SPEC_ICON_X,vmPOP3_ABS_WARN_SPEC_ICON_Y,LARGE_ICON,PROGRAM_ERROR);
                    UiInstance.WarningShowed |=  WARNING_DSPSTAT;
                  }
                  else
                  {
                    if (Tmp & WARNING_RAM)
                    {
                      dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_ICON_X,vmPOP3_ABS_WARN_ICON_Y,LARGE_ICON,WARNSIGN);
                      dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_SPEC_ICON_X,vmPOP3_ABS_WARN_SPEC_ICON_Y,LARGE_ICON,WARN_MEMORY);
                      UiInstance.WarningShowed |=  WARNING_RAM;
                    }
                    else
                    {
                    }
                  }
                }
              }
            }
          }
          dLcdDrawLine((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_LINE_X,vmPOP3_ABS_WARN_LINE_Y,vmPOP3_ABS_WARN_LINE_ENDX,vmPOP3_ABS_WARN_LINE_Y);
          dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,vmPOP3_ABS_WARN_YES_X,vmPOP3_ABS_WARN_YES_Y,LARGE_ICON,YES_SEL);
          dLcdUpdate(UiInstance.pLcd);
          cUiButtonClearAll();
          UiInstance.ScreenBlocked  =  1;
        }
      }
    }

    // Find warnings that have been showed but not confirmed
    Tmp       =  UiInstance.WarningShowed;
    Tmp      &= ~UiInstance.WarningConfirmed;

    if (Tmp)
    {
      if (cUiButtonGetShortPress(ENTER_BUTTON))
      {
        UiInstance.ScreenBlocked  =  0;
        LCDCopy(&UiInstance.LcdSave,&UiInstance.LcdSafe,sizeof(UiInstance.LcdSafe));
        dLcdUpdate(UiInstance.pLcd);
        if (Tmp & WARNING_TEMP)
        {
          UiInstance.WarningConfirmed |=  WARNING_TEMP;
        }
        else
        {
          if (Tmp & WARNING_CURRENT)
          {
            UiInstance.WarningConfirmed |=  WARNING_CURRENT;
          }
          else
          {
            if (Tmp & WARNING_VOLTAGE)
            {
              UiInstance.WarningConfirmed |=  WARNING_VOLTAGE;
            }
            else
            {
              if (Tmp & WARNING_MEMORY)
              {
                UiInstance.WarningConfirmed |=  WARNING_MEMORY;
              }
              else
              {
                if (Tmp & WARNING_DSPSTAT)
                {
                  UiInstance.WarningConfirmed |=  WARNING_DSPSTAT;
                  UiInstance.Warning          &= ~WARNING_DSPSTAT;
                }
                else
                {
                  if (Tmp & WARNING_RAM)
                  {
                    UiInstance.WarningConfirmed |=  WARNING_RAM;
                    UiInstance.Warning          &= ~WARNING_RAM;
                  }
                  else
                  {
                  }
                }
              }
            }
          }
        }
      }
    }

    // Find warnings that have been showed, confirmed and removed
    Tmp       = ~Warning;
    Tmp      &=  WARNINGS;
    Tmp      &=  UiInstance.WarningShowed;
    Tmp      &=  UiInstance.WarningConfirmed;

    UiInstance.WarningShowed     &= ~Tmp;
    UiInstance.WarningConfirmed  &= ~Tmp;
  }
}


//******* BYTE CODE SNIPPETS **************************************************


/*! \page cUi
 *  <hr size="1"/>
 *  <b>     opUI_FLUSH ()  </b>
 *
 *- User Interface flush buffers\n
 *- Dispatch status unchanged
 *
 */
/*! \brief  opUI_FLUSH byte code
 *
 */
void      cUiFlush(void)
{
  cUiFlushBuffer();
}


/*! \page cUi
 *  <hr size="1"/>
 *  <b>     opUI_READ (CMD, ....)  </b>
 *
 *- User Interface read\n
 *- Dispatch status can change to BUSYBREAK and FAILBREAK
 *
 *  \param  (DATA8)   CMD     - \ref uireadsubcode
 *
 *\n
 *  - CMD = GET_STRING
 *\n  Get string from terminal\n
 *    -  \param  (DATA8)   LENGTH       - Maximal length of string returned\n
 *    -  \return (DATA8)   DESTINATION  - String variable or handle to string\n
 *
 *\n
 *  - CMD = KEY
 *\n  Get key from terminal (used for debugging)\n
 *    -  \return (DATA8)   VALUE    - Key value from lms_cmdin (0 = no key)\n
 *
 *\n
 *  - CMD = GET_ADDRESS
 *\n  Get address from terminal (used for debugging)\n
 *    -  \return (DATA32)  VALUE    - Address from lms_cmdin\n
 *
 *\n
 *  - CMD = GET_CODE
 *\n  Get code snippet from terminal (used for debugging)\n
 *    -  \param  (DATA32)  LENGTH   - Maximal code stream length\n
 *    -  \return (DATA32)  *IMAGE   - Address of image\n
 *    -  \return (DATA32)  *GLOBAL  - Address of global variables\n
 *    -  \return (DATA8)   FLAG     - Flag tells if image is ready to execute [1=ready]\n
 *
 *\n
 *  - CMD = GET_HW_VERS
 *\n  Get hardware version string\n
 *    -  \param  (DATA8)   LENGTH       - Maximal length of string returned (-1 = no check)\n
 *    -  \return (DATA8)   DESTINATION  - String variable or handle to string\n
 *
 *\n
 *  - CMD = GET_FW_VERS
 *\n  Get firmware version string\n
 *    -  \param  (DATA8)   LENGTH       - Maximal length of string returned (-1 = no check)\n
 *    -  \return (DATA8)   DESTINATION  - String variable or handle to string\n
 *
 *
 *\n
 *  - CMD = GET_FW_BUILD
 *\n  Get firmware build string\n
 *    -  \param  (DATA8)   LENGTH       - Maximal length of string returned (-1 = no check)\n
 *    -  \return (DATA8)   DESTINATION  - String variable or handle to string\n
 *
 *\n
 *  - CMD = GET_OS_VERS
 *\n  Get os version string\n
 *    -  \param  (DATA8)   LENGTH       - Maximal length of string returned (-1 = no check)\n
 *    -  \return (DATA8)   DESTINATION  - String variable or handle to string\n
 *
 *\n
 *  - CMD = GET_OS_BUILD
 *\n  Get os build string\n
 *    -  \param  (DATA8)   LENGTH       - Maximal length of string returned (-1 = no check)\n
 *    -  \return (DATA8)   DESTINATION  - String variable or handle to string\n
 *
 *\n
 *  - CMD = GET_VERSION
 *\n  Get version string\n
 *    -  \param  (DATA8)   LENGTH       - Maximal length of string returned (-1 = no check)\n
 *    -  \return (DATA8)   DESTINATION  - String variable or handle to string\n
 *
 *\n
 *  - CMD = GET_IP
 *\n  Get IP address string\n
 *    -  \param  (DATA8)   LENGTH       - Maximal length of string returned (-1 = no check)\n
 *    -  \return (DATA8)   DESTINATION  - String variable or handle to string\n
 *
 *\n
 *  - CMD = GET_VBATT
 *    -  \return (DATAF)   VALUE    - Battery voltage [V]\n
 *
 *\n
 *  - CMD = GET_IBATT
 *    -  \return (DATAF)   VALUE    - Battery current [A]\n
 *
 *\n
 *  - CMD = GET_TBATT
 *    -  \return (DATAF)   VALUE    - Battery temperature rise [C]\n
 *
 *\n
 *  - CMD = GET_IMOTOR
 *    -  \return (DATAF)    VALUE    - Motor current [A]\n
 *
 *\n
 *  - CMD = GET_SDCARD
 *    -  \return (DATA8)    STATE    - SD card present [0..1]\n
 *    -  \return (DATA32)   TOTAL    - Kbytes in total\n
 *    -  \return (DATA32)   FREE     - Kbytes free\n
 *
 *\n
 *  - CMD = GET_USBSTICK
 *    -  \return (DATA8)    STATE    - USB stick present [0..1]\n
 *    -  \return (DATA32)   TOTAL    - Kbytes in total\n
 *    -  \return (DATA32)   FREE     - Kbytes free\n
 *
 *\n
 *  - CMD = GET_LBATT
 *\n  Get battery level in %\n
 *    -  \return (DATA8)    PCT      - Battery level [0..100]\n
 *
 *\n
 *  - CMD = GET_EVENT
 *\n  Get event (internal use)\n
 *    -  \return (DATA8)    EVENT    - Event [1,2 = Bluetooth events]\n
 *
 *\n
 *  - CMD = GET_SHUTDOWN
 *\n  Get and clear shutdown flag (internal use)\n
 *    -  \return (DATA8)    FLAG     - Flag [1=want to shutdown]\n
 *
 *\n
 *  - CMD = GET_WARNING
 *\n  Read warning bit field (internal use)\n
 *    -  \return (DATA8) \ref warnings - Bit field containing various warnings\n
 *
 *\n
 *  - CMD = TEXTBOX_READ
 *\n  Read line from text box\n
 *    -  \param  (DATA8)   TEXT        - First character in text box text (must be zero terminated)\n
 *    -  \param  (DATA32)  SIZE        - Maximal text size (including zero termination)\n
 *    -  \param  (DATA8)     \ref delimiters "DEL" - Delimiter code\n
 *    -  \param  (DATA8)   LENGTH      - Maximal length of string returned (-1 = no check)\n
 *    -  \param  (DATA16)  LINE        - Selected line number\n
 *    -  \return (DATA8)   DESTINATION - String variable or handle to string\n
 *
 *\n
 */
/*! \brief  opUI_READ byte code
 *
 */
void      cUiRead(void)
{
  IP      TmpIp;
  DATA8   Cmd;
  DATA8   Lng;
  DATA8   Data8;
  DATA8   *pSource;
  DATA8   *pDestination;
  DATA16  Data16;
  IMGDATA Tmp;
  DATA32  pImage;
  DATA32  Length;
  DATA32  Total;
  DATA32  Size;
  IMGHEAD *pImgHead;
  OBJHEAD *pObjHead;


  TmpIp   =  GetObjectIp();
  Cmd     =  *(DATA8*)PrimParPointer();

  switch (Cmd)
  { // Function

    case GET_STRING :
    {
      if (UiInstance.Keys)
      {
        Lng           = *(DATA8*)PrimParPointer();
        pDestination  =  (DATA8*)PrimParPointer();
        pSource       =  (DATA8*)UiInstance.KeyBuffer;

        while ((UiInstance.Keys) && (Lng))
        {
          *pDestination  =  *pSource;
          pDestination++;
          pSource++;
          UiInstance.Keys--;
          Lng--;
        }
        *pDestination  =  0;
        UiInstance.KeyBufIn       =  0;
      }
      else
      {
        SetObjectIp(TmpIp - 1);
        SetDispatchStatus(BUSYBREAK);
      }
    }
    break;

    case KEY :
    {
      if (UiInstance.KeyBufIn)
      {
        *(DATA8*)PrimParPointer()  =  (DATA8)UiInstance.KeyBuffer[0];
        UiInstance.KeyBufIn--;

        for (Lng = 0;Lng < UiInstance.KeyBufIn;Lng++)
        {
          UiInstance.KeyBuffer[Lng]  =  UiInstance.KeyBuffer[Lng + 1];
        }
#ifdef  DEBUG_TRACE_KEY
        printf("%s",(char*)UiInstance.KeyBuffer);
#endif
      }
      else
      {
        *(DATA8*)PrimParPointer()  =  (DATA8)0;
      }
    }
    break;

    case GET_SHUTDOWN :
    {
      *(DATA8*)PrimParPointer()   =  UiInstance.ShutDown;
      UiInstance.ShutDown         =  0;
    }
    break;

    case GET_WARNING :
    {
      *(DATA8*)PrimParPointer()   =  UiInstance.Warning;
    }
    break;

    case GET_LBATT :
    {
      Data16  =  (DATA16)(UiInstance.Vbatt * 1000.0);
      Data16  -=  UiInstance.BattIndicatorLow;
      Data16   =  (Data16 * 100) / (UiInstance.BattIndicatorHigh - UiInstance.BattIndicatorLow);
      if (Data16 > 100)
      {
        Data16  =  100;
      }
      if (Data16 < 0)
      {
        Data16  =  0;
      }
      *(DATA8*)PrimParPointer()   =  (DATA8)Data16;
    }
    break;

    case ADDRESS :
    {
      if (UiInstance.Keys)
      {
        *(DATA32*)PrimParPointer()  =  (DATA32)atol((const char*)UiInstance.KeyBuffer);
        UiInstance.Keys     =  0;
      }
      else
      {
        SetObjectIp(TmpIp - 1);
        SetDispatchStatus(BUSYBREAK);
      }
    }
    break;

    case CODE :
    {
      if (UiInstance.Keys)
      {
        Length        = *(DATA32*)PrimParPointer();
        pImage        = *(DATA32*)PrimParPointer();

        pImgHead      = (IMGHEAD*)pImage;
        pObjHead      = (OBJHEAD*)(pImage + sizeof(IMGHEAD));
        pDestination  = (DATA8*)(pImage + sizeof(IMGHEAD) + sizeof(OBJHEAD));

        if (Length > (sizeof(IMGHEAD) + sizeof(OBJHEAD)))
        {

          (*pImgHead).Sign[0]                =  'l';
          (*pImgHead).Sign[1]                =  'e';
          (*pImgHead).Sign[2]                =  'g';
          (*pImgHead).Sign[3]                =  'o';
          (*pImgHead).ImageSize              =  0;
          (*pImgHead).VersionInfo            =  (UWORD)(VERS * 100.0);
          (*pImgHead).NumberOfObjects        =  1;
          (*pImgHead).GlobalBytes            =  0;

          (*pObjHead).OffsetToInstructions   =  (IP)(sizeof(IMGHEAD) + sizeof(OBJHEAD));
          (*pObjHead).OwnerObjectId          =  0;
          (*pObjHead).TriggerCount           =  0;
          (*pObjHead).LocalBytes             =  MAX_COMMAND_LOCALS;

          pSource           =  (DATA8*)UiInstance.KeyBuffer;
          Size              =  sizeof(IMGHEAD) + sizeof(OBJHEAD);

          Length           -=  sizeof(IMGHEAD) + sizeof(OBJHEAD);
          Length--;
          while ((UiInstance.Keys) && (Length))
          {
            Tmp         =  (IMGDATA)(AtoN(*pSource) << 4);
            pSource++;
            UiInstance.Keys--;
            if (UiInstance.Keys)
            {
              Tmp      +=  (IMGDATA)(AtoN(*pSource));
              pSource++;
              UiInstance.Keys--;
            }
            else
            {
              Tmp       =  0;
            }
            *pDestination  =  Tmp;
            pDestination++;
            Length--;
            Size++;
          }
          *pDestination  =  opOBJECT_END;
          Size++;
          (*pImgHead).ImageSize         =  Size;
          memset(UiInstance.Globals,0,sizeof(UiInstance.Globals));

          *(DATA32*)PrimParPointer()    =  (DATA32)UiInstance.Globals;
          *(DATA8*)PrimParPointer()     =  1;
        }
      }
      else
      {
        SetObjectIp(TmpIp - 1);
        SetDispatchStatus(BUSYBREAK);
      }
    }
    break;

    case GET_HW_VERS :
    {
      Lng           = *(DATA8*)PrimParPointer();
      pDestination  =  (DATA8*)PrimParPointer();

      if (VMInstance.Handle >= 0)
      {
        Data8  =  (DATA8)strlen((char*)UiInstance.HwVers) + 1;
        if ((Lng > Data8) || (Lng == -1))
        {
          Lng  =  Data8;
        }
        pDestination  =  (DATA8*)VmMemoryResize(VMInstance.Handle,(DATA32)Lng);
      }
      if (pDestination != NULL)
      {
        snprintf((char*)pDestination,Lng,"%s",UiInstance.HwVers);
      }
    }
    break;

    case GET_FW_VERS :
    {
      Lng           = *(DATA8*)PrimParPointer();
      pDestination  =  (DATA8*)PrimParPointer();

      if (VMInstance.Handle >= 0)
      {
        Data8  =  (DATA8)strlen((char*)UiInstance.FwVers) + 1;
        if ((Lng > Data8) || (Lng == -1))
        {
          Lng  =  Data8;
        }
        pDestination  =  (DATA8*)VmMemoryResize(VMInstance.Handle,(DATA32)Lng);
      }
      if (pDestination != NULL)
      {
        snprintf((char*)pDestination,Lng,"%s",UiInstance.FwVers);
      }
    }
    break;

    case GET_FW_BUILD :
    {
      Lng           = *(DATA8*)PrimParPointer();
      pDestination  =  (DATA8*)PrimParPointer();

      if (VMInstance.Handle >= 0)
      {
        Data8  =  (DATA8)strlen((char*)UiInstance.FwBuild) + 1;
        if ((Lng > Data8) || (Lng == -1))
        {
          Lng  =  Data8;
        }
        pDestination  =  (DATA8*)VmMemoryResize(VMInstance.Handle,(DATA32)Lng);
      }
      if (pDestination != NULL)
      {
        snprintf((char*)pDestination,Lng,"%s",UiInstance.FwBuild);
      }
    }
    break;

    case GET_OS_VERS :
    {
      Lng           = *(DATA8*)PrimParPointer();
      pDestination  =  (DATA8*)PrimParPointer();

      if (VMInstance.Handle >= 0)
      {
        Data8  =  (DATA8)strlen((char*)UiInstance.OsVers) + 1;
        if ((Lng > Data8) || (Lng == -1))
        {
          Lng  =  Data8;
        }
        pDestination  =  (DATA8*)VmMemoryResize(VMInstance.Handle,(DATA32)Lng);
      }
      if (pDestination != NULL)
      {
        snprintf((char*)pDestination,Lng,"%s",UiInstance.OsVers);
      }
    }
    break;

    case GET_OS_BUILD :
    {
      Lng           = *(DATA8*)PrimParPointer();
      pDestination  =  (DATA8*)PrimParPointer();

      if (VMInstance.Handle >= 0)
      {
        Data8  =  (DATA8)strlen((char*)UiInstance.OsBuild) + 1;
        if ((Lng > Data8) || (Lng == -1))
        {
          Lng  =  Data8;
        }
        pDestination  =  (DATA8*)VmMemoryResize(VMInstance.Handle,(DATA32)Lng);
      }
      if (pDestination != NULL)
      {
        snprintf((char*)pDestination,Lng,"%s",UiInstance.OsBuild);
      }
    }
    break;

    case GET_VERSION :
    {
      snprintf((char*)UiInstance.ImageBuffer,IMAGEBUFFER_SIZE,"%s V%4.2f%c(%s %s)",PROJECT,VERS,SPECIALVERS,__DATE__,__TIME__);
      Lng           = *(DATA8*)PrimParPointer();
      pDestination  =  (DATA8*)PrimParPointer();
      pSource       =  (DATA8*)UiInstance.ImageBuffer;

      if (VMInstance.Handle >= 0)
      {
        Data8  =  (DATA8)strlen((char*)UiInstance.ImageBuffer) + 1;
        if ((Lng > Data8) || (Lng == -1))
        {
          Lng  =  Data8;
        }
        pDestination  =  (DATA8*)VmMemoryResize(VMInstance.Handle,(DATA32)Lng);
      }
      if (pDestination != NULL)
      {
        snprintf((char*)pDestination,Lng,"%s",UiInstance.ImageBuffer);
      }
    }
    break;

    case GET_IP :
    {
      Lng           = *(DATA8*)PrimParPointer();
      pDestination  =  (DATA8*)PrimParPointer();

      if (VMInstance.Handle >= 0)
      {
        Data8  =  IPADDR_SIZE;
        if ((Lng > Data8) || (Lng == -1))
        {
          Lng  =  Data8;
        }
        pDestination  =  (DATA8*)VmMemoryResize(VMInstance.Handle,(DATA32)Lng);
      }
      if (pDestination != NULL)
      {
        snprintf((char*)pDestination,Lng,"%s",UiInstance.IpAddr);
      }

    }
    break;

    case GET_POWER :
    {
      *(DATAF*)PrimParPointer()  =  UiInstance.Vbatt;
      *(DATAF*)PrimParPointer()  =  UiInstance.Ibatt;
      *(DATAF*)PrimParPointer()  =  UiInstance.Iintegrated;
      *(DATAF*)PrimParPointer()  =  UiInstance.Imotor;
    }
    break;

    case GET_VBATT :
    {
      *(DATAF*)PrimParPointer()  =  UiInstance.Vbatt;
    }
    break;

    case GET_IBATT :
    {
      *(DATAF*)PrimParPointer()  =  UiInstance.Ibatt;
    }
    break;

    case GET_IINT :
    {
      *(DATAF*)PrimParPointer()  =  UiInstance.Iintegrated;
    }
    break;

    case GET_IMOTOR :
    {
      *(DATAF*)PrimParPointer()  =  UiInstance.Imotor;
    }
    break;

    case GET_EVENT :
    {
      *(DATA8*)PrimParPointer()  =  UiInstance.Event;
      UiInstance.Event           =  0;
    }
    break;

    case GET_TBATT :
    {
      *(DATAF*)PrimParPointer()  =  UiInstance.Tbatt;
    }
    break;

    case TEXTBOX_READ :
    {
      pSource       =  (DATA8*)PrimParPointer();
      Size          = *(DATA32*)PrimParPointer();
      Data8         = *(DATA8*)PrimParPointer();
      Lng           = *(DATA8*)PrimParPointer();
      Data16        = *(DATA16*)PrimParPointer();
      pDestination  =  (DATA8*)PrimParPointer();

      cUiTextboxReadLine(pSource,Size,Data8,Lng,Data16,pDestination,&Data8);
    }
    break;

    case GET_SDCARD :
    {
      *(DATA8*)PrimParPointer()   =  CheckSdcard(&Data8,&Total,&Size,0);
      *(DATA32*)PrimParPointer()  =  Total;
      *(DATA32*)PrimParPointer()  =  Size;
    }
    break;

    case GET_USBSTICK :
    {
      *(DATA8*)PrimParPointer()   =  CheckUsbstick(&Data8,&Total,&Size,0);
      *(DATA32*)PrimParPointer()  =  Total;
      *(DATA32*)PrimParPointer()  =  Size;
    }
    break;

  }
}


/*! \page cUi
 *  <hr size="1"/>
 *  <b>     opUI_WRITE (CMD, ....)  </b>
 *
 *- UI write data\n
 *- Dispatch status can change to BUSYBREAK and FAILBREAK
 *
 *  \param  (DATA8)   CMD     - \ref uiwritesubcode
 *
 *\n
 *  - CMD = WRITE_FLUSH
 *
 *\n
 *  - CMD = FLOATVALUE
 *    -  \param  (DATAF)   VALUE    - Value to write\n
 *    -  \param  (DATA8)   FIGURES  - Total number of figures inclusive decimal point\n
 *    -  \param  (DATA8)   DECIMALS - Number of decimals\n
 *
 *\n
 *  - CMD = PUT_STRING
 *    -  \param  (DATA8)   STRING   - First character in string to write\n
 *
 *\n
 *  - CMD = CODE
 *    -  \param  (DATA8)   ARRAY    - First byte in byte array to write\n
 *    -  \param  (DATA32)  LENGTH   - Length of array\n
 *
 *\n
 *  - CMD = VALUE8
 *    -  \param  (DATA8)   VALUE    - Value to write\n
 *
 *\n
 *  - CMD = VALUE16
 *    -  \param  (DATA16)  VALUE    - Value to write\n
 *
 *\n
 *  - CMD = VALUE32
 *    -  \param  (DATA32)  VALUE    - Value to write\n
 *
 *\n
 *  - CMD = VALUEF
 *    -  \param  (DATAF)   VALUE    - Value to write\n
 *
 *\n
 *  - CMD = LED
 *    -  \param  (DATA8)   PATTERN  - \ref ledpatterns \n
 *
 *\n
 *  - CMD = SET_BUSY
 *    -  \param  (DATA8)   VALUE    - Value [0,1]\n
 *
 *\n
 *  - CMD = POWER
 *    -  \param  (DATA8)   VALUE    - Value [0,1]\n
 *
 *\n
 *  - CMD = TERMINAL
 *    -  \param  (DATA8)   STATE    - Value [0 = Off,1 = On]\n
 *
 *\n
 *  - CMD = SET_TESTPIN
 *    -  \param  (DATA8)   STATE    - Value [0 = low,1 = high]\n
 *
 *\n
 *  - CMD = INIT_RUN
 *\n  Start the "Mindstorms" "run" screen\n
 *
 *
 *\n
 *  - CMD = GRAPH_SAMPLE
 *\n  Update tick to scroll graph horizontally in memory when drawing graph in "scope" mode\n
 *
 *\n
 *  - CMD = DOWNLOAD_END
 *\n  Send to brick when file down load is completed (plays sound and updates the UI browser)\n
 *
 *\n
 *  - CMD = SCREEN_BLOCK
 *\n  Set or clear screen block status (if screen blocked - all graphical screen action are disabled)\n
 *    -  \param  (DATA8)   STATUS   - Value [0 = normal,1 = blocked]\n
 *
 *\n
 *  - CMD = TEXTBOX_APPEND
 *\n  Append line of text at the bottom of a text box\n
 *    -  \param  (DATA8)   TEXT        - First character in text box text (must be zero terminated)\n
 *    -  \param  (DATA32)  SIZE        - Maximal text size (including zero termination)\n
 *    -  \param  (DATA8)     \ref delimiters "DEL" - Delimiter code\n
 *    -  \param  (DATA8)   SOURCE      - String variable or handle to string to append\n
 *
 *\n
 *
 *\n
 *
 */
/*! \brief  opUI_WRITE byte code
 *
 */
void      cUiWrite(void)
{
  IP      TmpIp;
  DATA8   Cmd;
  DATA8   *pSource;
  DSPSTAT DspStat = BUSYBREAK;
  DATA8   Buffer[50];
  DATA8   Data8;
  DATA16  Data16;
  DATA32  Data32;
  DATA32  pGlobal;
  DATA32  Tmp;
  DATAF   DataF;
  DATA8   Figures;
  DATA8   Decimals;
  DATA8   No;
  DATA8   *pText;


  TmpIp   =  GetObjectIp();
  Cmd     =  *(DATA8*)PrimParPointer();

  switch (Cmd)
  { // Function

    case WRITE_FLUSH :
    {
      cUiFlush();
      DspStat  =  NOBREAK;
    }
    break;

    case FLOATVALUE :
    {
      DataF     =  *(DATAF*)PrimParPointer();
      Figures   =  *(DATA8*)PrimParPointer();
      Decimals  =  *(DATA8*)PrimParPointer();

      snprintf((char*)Buffer,32,"%*.*f",Figures,Decimals,DataF);
      cUiWriteString(Buffer);

      DspStat  =  NOBREAK;
    }
    break;

    case STAMP :
    { // write time, prgid, objid, ip

      pSource  =  (DATA8*)PrimParPointer();
      snprintf((char*)Buffer,50,"####[ %09u %01u %03u %06u %s]####\n",GetTime(),CurrentProgramId(),CallingObjectId(),CurrentObjectIp(),pSource);
      cUiWriteString(Buffer);
      cUiFlush();
      DspStat  =  NOBREAK;
    }
    break;

    case PUT_STRING :
    {
      pSource  =  (DATA8*)PrimParPointer();
      cUiWriteString(pSource);
      DspStat  =  NOBREAK;
    }
    break;

    case CODE :
    {
      pGlobal  =  *(DATA32*)PrimParPointer();
      Data32   = *(DATA32*)PrimParPointer();

      pSource  =  (DATA8*)pGlobal;

      cUiWriteString((DATA8*)"\r\n    ");
      for (Tmp = 0;Tmp < Data32;Tmp++)
      {
        snprintf((char*)Buffer,7,"%02X ",pSource[Tmp] & 0xFF);
        cUiWriteString(Buffer);
        if (((Tmp & 0x3) == 0x3) && ((Tmp & 0xF) != 0xF))
        {
          cUiWriteString((DATA8*)" ");
        }
        if (((Tmp & 0xF) == 0xF) && (Tmp < (Data32 - 1)))
        {
          cUiWriteString((DATA8*)"\r\n    ");
        }
      }
      cUiWriteString((DATA8*)"\r\n");
      DspStat  =  NOBREAK;
    }
    break;

    case TEXTBOX_APPEND :
    {
      pText     =  (DATA8*)PrimParPointer();
      Data32    =  *(DATA32*)PrimParPointer();
      Data8     =  *(DATA8*)PrimParPointer();
      pSource   =  (DATA8*)PrimParPointer();

      cUiTextboxAppendLine(pText,Data32,Data8,pSource,UiInstance.Font);

      DspStat  =  NOBREAK;
    }
    break;

    case SET_BUSY :
    {
      Data8     =  *(DATA8*)PrimParPointer();

      if (Data8)
      {
        UiInstance.Warning |=  WARNING_BUSY;
      }
      else
      {
        UiInstance.Warning &= ~WARNING_BUSY;
      }

      DspStat  =  NOBREAK;
    }
    break;

    case VALUE8 :
    {
      Data8  =  *(DATA8*)PrimParPointer();
      if (Data8 != DATA8_NAN)
      {
        snprintf((char*)Buffer,7,"%d",(int)Data8);
      }
      else
      {
        snprintf((char*)Buffer,7,"nan");
      }
      cUiWriteString(Buffer);

      DspStat  =  NOBREAK;
    }
    break;

    case VALUE16 :
    {
      Data16  =  *(DATA16*)PrimParPointer();
      if (Data16 != DATA16_NAN)
      {
        snprintf((char*)Buffer,9,"%d",Data16 & 0xFFFF);
      }
      else
      {
        snprintf((char*)Buffer,7,"nan");
      }
      cUiWriteString(Buffer);

      DspStat  =  NOBREAK;
    }
    break;

    case VALUE32 :
    {
      Data32  =  *(DATA32*)PrimParPointer();
      if (Data32 != DATA32_NAN)
      {
        snprintf((char*)Buffer,14,"%ld",(long unsigned int)(Data32 & 0xFFFFFFFF));
      }
      else
      {
        snprintf((char*)Buffer,7,"nan");
      }

      cUiWriteString(Buffer);

      DspStat  =  NOBREAK;
    }
    break;

    case VALUEF :
    {
      DataF  =  *(DATAF*)PrimParPointer();
      snprintf((char*)Buffer,24,"%f",DataF);
      cUiWriteString(Buffer);

      DspStat  =  NOBREAK;
    }
    break;

    case LED :
    {
      Data8     =  *(DATA8*)PrimParPointer();
      if (Data8 < 0)
      {
        Data8   =  0;
      }
      if (Data8 >= LEDPATTERNS)
      {
        Data8   =  LEDPATTERNS - 1;
      }
      cUiLedSetState(Data8);
      UiInstance.RunLedEnabled  =  0;

      DspStat  =  NOBREAK;
    }
    break;

    case POWER :
    {
      Data8  =  *(DATA8*)PrimParPointer();

      if (UiInstance.PowerFile >= 0)
      {
        ioctl(UiInstance.PowerFile,0,(size_t)&Data8);
      }

      DspStat  =  NOBREAK;
    }
    break;

    case TERMINAL :
    {
      No     =  *(DATA8*)PrimParPointer();

      if (No)
      {
        SetTerminalEnable(1);
      }
      else
      {
        SetTerminalEnable(0);
      }

      DspStat  =  NOBREAK;
    }
    break;

    case SET_TESTPIN :
    {
      Data8   =  *(DATA8*)PrimParPointer();
      cUiTestpin(Data8);
      DspStat  =  NOBREAK;
    }
    break;

    case INIT_RUN :
    {
      UiInstance.RunScreenEnabled  =  3;

      DspStat  =  NOBREAK;
    }
    break;

    case UPDATE_RUN :
    {
      DspStat  =  NOBREAK;
    }
    break;

    case GRAPH_SAMPLE :
    {
      cUiGraphSample();
      DspStat  =  NOBREAK;
    }
    break;

    case DOWNLOAD_END :
    {
      UiInstance.UiUpdate  =  1;
      cUiDownloadSuccessSound();
      DspStat  =  NOBREAK;
    }
    break;

    case SCREEN_BLOCK :
    {
      UiInstance.ScreenBlocked  =  *(DATA8*)PrimParPointer();
      DspStat  =  NOBREAK;
    }
    break;

    case ALLOW_PULSE :
    {
      Data8  =  *(DATA8*)PrimParPointer();
#ifdef ALLOW_DEBUG_PULSE
      VMInstance.PulseShow  =  Data8;
#endif
      DspStat  =  NOBREAK;
    }
    break;

    case SET_PULSE :
    {
      Data8  =  *(DATA8*)PrimParPointer();
#ifdef ALLOW_DEBUG_PULSE
      VMInstance.Pulse     |=  Data8;
#endif
      DspStat  =  NOBREAK;
    }
    break;

    default :
    {
      DspStat  =  FAILBREAK;
    }
    break;

  }

  if (DspStat == BUSYBREAK)
  { // Rewind IP

    SetObjectIp(TmpIp - 1);
  }
  SetDispatchStatus(DspStat);
}

/*! \page cUi
 *  \anchor KEEPALIVE
 *  <hr size="1"/>
 *  <b>     opKEEP_ALIVE (MINUTES)  </b>
 *
 *- Keep alive\n
 *- Dispatch status unchanged
 *
 *  \return (DATA8)   MINUTES     - Number of minutes before sleep
 *
 *\n
 */
/*! \brief  opKEEP_ALIVE byte code
 *
 */
void      cUiKeepAlive(void)
{
  cUiAlive();
  *(DATA8*)PrimParPointer()  =  GetSleepMinutes();
}


//*****************************************************************************
