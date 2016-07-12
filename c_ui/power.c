/*
 * LEGO® MINDSTORMS EV3
 *
 * Copyright (C) 2010-2013 The LEGO Group
 * Copyright (C) 2016 David Lechner <david@lechnology.com>
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

#include "lms2012.h"
#include "c_ui.h"
#include "power.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CALL_INTERVAL   400   // [mS]

#define AVR_CIN         300
#define AVR_COUT        30
#define AVR_VIN         30

/**
 * @brief Read Value from a sysfs power_supply attribute.
 *
 * @param  fd An open file handle.
 * @return The floating point representation of the value.
 */
static DATAF cUiPowerReadAttr(int fd)
{
    char str[20 + 1] = { 0 };
    int value;

    lseek(fd, 0, SEEK_SET);
    read(fd, str, 20);
    value = atoi(str);

    // value is in μV/μA
    return (DATAF)value / 1000000;
}

/**
 * @brief Opens files for battery voltage and current.
 *
 * Searches for a Linux power_supply class device with scope of "System" and
 * type of "Battery". If found, this will be used for battery voltage and
 * current if available. Also check the "technology" attribute to determine
 * if this is a rechargeable battery or not.
 *
 * Initializes the following global variables:
 * - UiInstance.BatteryVoltageNowFile: open file handle or -1
 * - UiInstance.BatteryCurrentNowFile: open file handle or -1
 * - UiInstance.VinCnt: actual battery voltage or default
 * - UiInstance.CinCnt: actual battery voltage or 0
 * - UiInstance.Accu: true if battery is rechargeable
 */
void cUiPowerOpenBatteryFiles(void)
{
    struct udev_enumerate *enumerate;
    struct udev_list_entry *list;

    UiInstance.BatteryVoltageNowFile = -1;
    UiInstance.BatteryCurrentNowFile = -1;
    enumerate = udev_enumerate_new(VMInstance.udev);
    udev_enumerate_add_match_subsystem(enumerate, "power_supply");
    udev_enumerate_add_match_property(enumerate, "POWER_SUPPLY_SCOPE", "System");
    udev_enumerate_add_match_property(enumerate, "POWER_SUPPLY_TYPE", "Battery");
    udev_enumerate_scan_devices(enumerate);
    list = udev_enumerate_get_list_entry(enumerate);
    if (!list) {
        fprintf(stderr, "Failed to find system power supply\n");
    } else {
        // just taking the first match in the list
        const char *path = udev_list_entry_get_name(list);
        char attr_path[255 + 1] = { 0 };
        int fd;

        snprintf(attr_path, 255, "%s/voltage_now", path);
        fd = open(attr_path, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Could not open '%s': %s\n", attr_path,
                    strerror(errno));
            UiInstance.VinCnt = DEFAULT_BATTERY_VOLTAGE;
        } else {
            UiInstance.BatteryVoltageNowFile = fd;
            UiInstance.VinCnt = cUiPowerReadAttr(fd);
        }

        snprintf(attr_path, 255, "%s/current_now", path);
        fd = open(attr_path, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Could not open '%s': %s\n", attr_path,
                    strerror(errno));
        } else {
            UiInstance.BatteryCurrentNowFile = fd;
            UiInstance.CinCnt = cUiPowerReadAttr(fd);
        }

        // Check to see if this is a rechargeable battery
        snprintf(attr_path, 255, "%s/technology", path);
        fd = open(attr_path, O_RDONLY);
        if (fd == -1) {
            UiInstance.Accu = 0;
        } else {
            char technology[20 + 1] = { 0 };

            read(fd, technology, 20);
            close(fd);

            if (strstr(technology, "LION")) {
                UiInstance.Accu = 1;
            } else {
                UiInstance.Accu = 0;
            }
        }
    }
    udev_enumerate_unref(enumerate);
}

void cUiPowerUpdateCnt(void)
{
    UiInstance.VinCnt  *= AVR_VIN  - 1;
    UiInstance.CinCnt  *= AVR_CIN  - 1;
    UiInstance.CoutCnt *= AVR_COUT - 1;

    if (UiInstance.BatteryVoltageNowFile != -1) {
        UiInstance.VinCnt += cUiPowerReadAttr(UiInstance.BatteryVoltageNowFile);
    } else {
        UiInstance.VinCnt += DEFAULT_BATTERY_VOLTAGE;
    }
    if (UiInstance.BatteryCurrentNowFile != -1) {
        UiInstance.CinCnt += cUiPowerReadAttr(UiInstance.BatteryCurrentNowFile);
    }
    UiInstance.CoutCnt += 0;

    UiInstance.VinCnt  /= AVR_VIN;
    UiInstance.CinCnt  /= AVR_CIN;
    UiInstance.CoutCnt /= AVR_COUT;
}

#ifndef DISABLE_LOW_VOLTAGE

static void cUiUpdatePower(void)
{
#ifdef DEBUG_TEMP_SHUTDOWN
    UiInstance.Vbatt = 7.0;
    UiInstance.Ibatt = 5.0;
#else
    UiInstance.Vbatt = UiInstance.VinCnt;
    UiInstance.Ibatt = UiInstance.CinCnt;
#endif
    // No known hardware can read motor current
    UiInstance.Imotor = 0;
}

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

// 400mS
void cUiPowerCheckVoltage(void)
{
    cUiUpdatePower();

    if (UiInstance.Vbatt >= UiInstance.BattWarningHigh) {
        UiInstance.Warning &= ~WARNING_BATTLOW;
    }

    if (UiInstance.Vbatt <= UiInstance.BattWarningLow) {
        UiInstance.Warning |= WARNING_BATTLOW;
    }

    if (UiInstance.Vbatt >= UiInstance.BattShutdownHigh) {
        // Good
        UiInstance.Warning &= ~WARNING_VOLTAGE;
    }

    if (UiInstance.Vbatt < UiInstance.BattShutdownLow) {
        // Bad
        UiInstance.Warning |= WARNING_VOLTAGE;

        ProgramEnd(USER_SLOT);

        if ((UiInstance.MilliSeconds - UiInstance.VoltageTimer) >= LOW_VOLTAGE_SHUTDOWN_TIME) {
            // Realy bad
#ifdef DEBUG
            printf("Shutting down due to low battery\n");
#endif
            UiInstance.ShutDown = 1;
        }
    } else {
        UiInstance.VoltageTimer = UiInstance.MilliSeconds;
    }
}

#endif /* DISABLE_LOW_VOLTAGE */

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

// 400mS
void cUiCheckPower(UWORD Time)
{
    DATA16  X,Y;
    DATAF   I;
    DATAF   Slope;

    I = UiInstance.Ibatt + UiInstance.Imotor;

    if (I > LOAD_BREAK_EVEN) {
        Slope  =  LOAD_SLOPE_UP;
    } else {
        Slope  =  LOAD_SLOPE_DOWN;
    }

    UiInstance.Iintegrated +=  (I - LOAD_BREAK_EVEN) * (Slope * (DATAF)Time / 1000.0);

    if (UiInstance.Iintegrated < 0.0) {
        UiInstance.Iintegrated  =  0.0;
    }
    if (UiInstance.Iintegrated > LOAD_SHUTDOWN_FAIL) {
        UiInstance.Iintegrated = LOAD_SHUTDOWN_FAIL;
    }

    if ((UiInstance.Iintegrated >= LOAD_SHUTDOWN_HIGH) || (I >= LOAD_SHUTDOWN_FAIL)) {
        UiInstance.Warning |=  WARNING_CURRENT;
        UiInstance.PowerShutdown = 1;
    }
    if (UiInstance.Iintegrated <= LOAD_BREAK_EVEN) {
        UiInstance.Warning &= ~WARNING_CURRENT;
        UiInstance.PowerShutdown = 0;
    }

    if (UiInstance.PowerShutdown) {
        if (UiInstance.ScreenBlocked == 0) {
            if (ProgramStatus(USER_SLOT) != STOPPED) {
                UiInstance.PowerState = 1;
            }
        }
        ProgramEnd(USER_SLOT);
    }

    switch (UiInstance.PowerState) {
    case 0:
        if (UiInstance.PowerShutdown) {
            UiInstance.PowerState++;
        }
        break;

    case 1:
        if (!UiInstance.ScreenBusy) {
            if (!UiInstance.VoltShutdown) {
                UiInstance.ScreenBlocked = 1;
                UiInstance.PowerState++;
            }
        }
        break;

    case 2:
        LCDCopy(&UiInstance.LcdSafe,&UiInstance.LcdSave,sizeof(UiInstance.LcdSave));
        UiInstance.PowerState++;
        break;

    case 3:
        X = 16;
        Y = 52;

        dLcdDrawPicture((*UiInstance.pLcd).Lcd,FG_COLOR,X,Y,POP3_width,POP3_height,(UBYTE*)POP3_bits);

        dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,X + 48,Y + 10,LARGE_ICON,WARNSIGN);
        dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,X + 72,Y + 10,LARGE_ICON,WARN_POWER);
        dLcdDrawLine((*UiInstance.pLcd).Lcd,FG_COLOR,X + 5,Y + 39,X + 138,Y + 39);
        dLcdDrawIcon((*UiInstance.pLcd).Lcd,FG_COLOR,X + 56,Y + 40,LARGE_ICON,YES_SEL);
        dLcdUpdate(UiInstance.pLcd);
        cUiButtonClearAll();
        UiInstance.PowerState++;
        break;

    case 4:
        if (cUiButtonGetShortPress(ENTER_BUTTON)) {
            if (UiInstance.ScreenBlocked) {
                UiInstance.ScreenBlocked = 0;
            }
            LCDCopy(&UiInstance.LcdSave,&UiInstance.LcdSafe,sizeof(UiInstance.LcdSafe));
            dLcdUpdate(UiInstance.pLcd);
            UiInstance.PowerState++;
        }
        if (!UiInstance.PowerShutdown) {
            UiInstance.PowerState = 0;
        }
        break;

    case 5:
        if (!UiInstance.PowerShutdown) {
            UiInstance.PowerState = 0;
        }
        break;

    }
}
#endif /* ENABLE_HIGH_CURRENT */

#ifndef DISABLE_VIRTUAL_BATT_TEMP

//Defines that can turn on debug messages
//#define __dbg1
//#define __dbg2

/*************************** Model parameters *******************************/
//Approx. initial internal resistance of 6 Energizer industrial batteries :
static float R_bat_init = 0.63468;
//Bjarke's proposal for spring resistance :
//float spring_resistance = 0.42;
//Batteries' heat capacity :
static float heat_cap_bat = 136.6598;
//Newtonian cooling constant for electronics :
static float K_bat_loss_to_elec = -0.0003; //-0.000789767;
//Newtonian heating constant for electronics :
static float K_bat_gain_from_elec = 0.001242896; //0.001035746;
//Newtonian cooling constant for environment :
static float K_bat_to_room = -0.00012;
//Battery power Boost
static float battery_power_boost = 1.7;
//Battery R_bat negative gain
static float R_bat_neg_gain = 1.00;

//Slope of electronics lossless heating curve (linear!!!) [Deg.C / s] :
static float K_elec_heat_slope = 0.0123175;
//Newtonian cooling constant for battery packs :
static float K_elec_loss_to_bat = -0.004137487;
//Newtonian heating constant for battery packs :
static float K_elec_gain_from_bat = 0.002027574; //0.00152068;
//Newtonian cooling constant for environment :
static float K_elec_to_room = -0.001931431; //-0.001843639;

// Function for estimating new battery temperature based on measurements
// of battery voltage and battery power.
static float new_bat_temp(float V_bat, float I_bat)
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
    if (index > 0) {
        I_bat_mean = ((index) * I_bat_mean + I_bat) / (index + 1) ;
    } else {
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
    if ((V_bat > 7.5) && (has_passed_7v5_flag == 'N')) {
        R_bat = R_bat_init; //7.5 V not passed a first time
    } else {
        //Only update R_bat with positive outcomes: R_bat_model - R_bat_model_old
        //R_bat updated with the change in model R_bat is not equal value in the model!
        if ((R_bat_model - R_bat_model_old) > 0) {
            R_bat = R_bat + R_bat_model - R_bat_model_old;
        } else {// The negative outcome of R_bat_model added to only part of R_bat
            R_bat = R_bat + ( R_bat_neg_gain * (R_bat_model - R_bat_model_old));
        }
        //Make sure we initialize R_bat later
        has_passed_7v5_flag = 'Y';
    }

    //Save R_bat_model for use in the next function call
    R_bat_model_old = R_bat_model;

    //Debug code:
#ifdef __dbg1
    if (index < 500) {
        printf("%c %f %f %f %f %f %f\n", has_passed_7v5_flag, R_1A, R_2A,
               slope_A, intercept_b, R_bat_model - R_bat_model_old, R_bat);
    }
#endif

    /*****Calculate the 4 types of temperature change for the batteries******/

    //Calculate the batteries' own temperature change
    dT_bat_own = R_bat * I_bat * I_bat * sample_period  * battery_power_boost
                 / heat_cap_bat;

    //Calculate the batteries' heat loss to the electronics
    if (T_bat > T_elec) {
        dT_bat_loss_to_elec = K_bat_loss_to_elec * (T_bat - T_elec)
                              * sample_period;
    } else {
        dT_bat_loss_to_elec = 0.0;
    }

    //Calculate the batteries' heat gain from the electronics
    if (T_bat < T_elec) {
        dT_bat_gain_from_elec = K_bat_gain_from_elec * (T_elec - T_bat)
                                * sample_period;
    } else {
        dT_bat_gain_from_elec = 0.0;
    }

    //Calculate the batteries' heat loss to environment
    dT_bat_loss_to_room = K_bat_to_room * T_bat * sample_period;
    /************************************************************************/


    /*****Calculate the 4 types of temperature change for the electronics****/

    //Calculate the electronics' own temperature change
    dT_elec_own = K_elec_heat_slope * sample_period;

    //Calculate the electronics' heat loss to the batteries
    if (T_elec > T_bat) {
        dT_elec_loss_to_bat = K_elec_loss_to_bat * (T_elec - T_bat)
                              * sample_period;
    } else {
        dT_elec_loss_to_bat = 0.0;
    }

    //Calculate the electronics' heat gain from the batteries
    if (T_elec < T_bat) {
        dT_elec_gain_from_bat = K_elec_gain_from_bat * (T_bat - T_elec)
                                * sample_period;
    } else {
        dT_elec_gain_from_bat = 0.0;
    }

    //Calculate the electronics' heat loss to the environment
    dT_elec_loss_to_room = K_elec_to_room * T_elec * sample_period;

    /*****************************************************************************/
    //Debug code:
#ifdef __dbg2
    if (index < 500) {
        printf("%f %f %f %f %f <> %f %f %f %f %f\n",dT_bat_own, dT_bat_loss_to_elec,
               dT_bat_gain_from_elec, dT_bat_loss_to_room, T_bat,
               dT_elec_own, dT_elec_loss_to_bat, dT_elec_gain_from_bat,
               dT_elec_loss_to_room, T_elec);
    }
#endif

    //Refresh battery temperature
    T_bat = T_bat + dT_bat_own + dT_bat_loss_to_elec
          + dT_bat_gain_from_elec + dT_bat_loss_to_room;

    //Refresh electronics temperature
    T_elec = T_elec + dT_elec_own + dT_elec_loss_to_bat
           + dT_elec_gain_from_bat + dT_elec_loss_to_room;

    return T_bat;
}

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

void cUiPowerCheckTemperature(void)
{
    if ((UiInstance.MilliSeconds - UiInstance.TempTimer) >= CALL_INTERVAL) {
        UiInstance.TempTimer +=  CALL_INTERVAL;
        UiInstance.Tbatt      =  new_bat_temp(UiInstance.Vbatt,(UiInstance.Ibatt * (DATAF)1.1));
#ifdef DEBUG_TEMP_SHUTDOWN
        UiInstance.Tbatt     +=  35.0;
#endif
#ifdef DEBUG_VIRTUAL_BATT_TEMP
        char    Buffer[250];
        int     BufferSize;

        if (TempFile >= MIN_HANDLE) {
            BufferSize  =  snprintf(Buffer,250,"%8.1f,%9.6f,%9.6f,%11.6f\n",(float)UiInstance.MilliSeconds / (float)1000,UiInstance.Vbatt,UiInstance.Ibatt,UiInstance.Tbatt);
            write(TempFile,Buffer,BufferSize);
        }
#endif
    }

    if (UiInstance.Tbatt >= TEMP_SHUTDOWN_WARNING) {
        UiInstance.Warning |=  WARNING_TEMP;
    } else {
        UiInstance.Warning &= ~WARNING_TEMP;
    }


    if (UiInstance.Tbatt >= TEMP_SHUTDOWN_FAIL) {
        ProgramEnd(USER_SLOT);
        UiInstance.ShutDown = 1;
    }
}

void cUiPowerCheckTemperature(void);

#ifdef DEBUG_VIRTUAL_BATT_TEMP

static int TempFile = -1;

void cUiPowerInitTemperature(void)
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
    if (pFile) {
        do {
            Str           =  fgets(Buffer,250,pFile);
            Buffer[249]   =  0;
            if (Str) {
                if ((Buffer[0] != '/') && (Buffer[0] != '*') && (Buffer[0] != ' ')) {
                    Const[Tmp]  =  DATAF_NAN;
                    if (sscanf(Buffer,"%f",&Const[Tmp]) != 1) {
                        Const[Tmp]  =  DATAF_NAN;
                    }
                    Tmp++;
                }
            }
        } while (Str != NULL);
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
    if (TempFile >= MIN_HANDLE) {
        if (Tmp) {
            BufferSize  =  snprintf(Buffer,250,"* TempConst.rtf ************************\n");
        } else {
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
    cUiPowerCheckTemperature();
}

void cUiPowerExitTemperature(void)
{
    if (TempFile >= MIN_HANDLE) {
        close(TempFile);
        TempFile  =  -1;
    }
}
#endif /* DEBUG_VIRTUAL_BATT_TEMP */

#endif /* DISABLE_VIRTUAL_BATT_TEMP */
