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

#include "lms2012.h"
#include "browser.h"
#include "c_memory.h"
#include "c_ui.h"
#include "d_lcd.h"
#include "dialog.h"
#include "graph.h"
#include "keyboard.h"
#include "textbox.h"

#include <math.h>
#include <stdio.h>
#include <string.h>


void cUiDrawBar(DATA8 Color, DATA16 X, DATA16 Y, DATA16 X1, DATA16 Y1,
                DATA16 Min, DATA16 Max, DATA16 Act)
{
    DATA16  Tmp;
    DATA16  Items;
    DATA16  KnobHeight = 7;

    Items       =  Max - Min;

    switch (X1)
    {
        case 5 :
        {
            KnobHeight    =  7;
        }
        break;

        case 6 :
        {
            KnobHeight    =  9;
        }
        break;

        default :
        {
            if (Items > 0)
            {
                KnobHeight  =  Y1 / Items;
            }
        }
        break;

    }

    if (Act < Min)
    {
        Act  =  Min;
    }
    if (Act > Max)
    {
        Act  =  Max;
    }

    // Draw scroll bar
    dLcdRect(UiInstance.pLcd->Lcd,Color,X,Y,X1,Y1);

    // Draw nob
    Tmp  =  Y;
    if ((Items > 1) && (Act > 0))
    {
        Tmp        +=  ((Y1 - KnobHeight) * (Act - 1)) / (Items - 1);
    }

    switch (X1)
    {
        case 5 :
        {
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 1,Tmp);
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 2,Tmp);
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 3,Tmp);
            Tmp++;
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 1,Tmp);
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 3,Tmp);
            Tmp++;
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 2,Tmp);
            Tmp++;
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 1,Tmp);
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 3,Tmp);
            Tmp++;
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 2,Tmp);
            Tmp++;
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 1,Tmp);
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 3,Tmp);
            Tmp++;
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 1,Tmp);
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 2,Tmp);
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 3,Tmp);
        }
        break;

        case 6 :
        {
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 2,Tmp);
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 3,Tmp);
            Tmp++;
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 1,Tmp);
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 4,Tmp);
            Tmp++;
            Tmp++;
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 2,Tmp);
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 3,Tmp);
            Tmp++;
            Tmp++;
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 2,Tmp);
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 3,Tmp);
            Tmp++;
            Tmp++;
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 1,Tmp);
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 4,Tmp);
            Tmp++;
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 2,Tmp);
            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X + 3,Tmp);
        }
        break;

        default :
        {
            dLcdFillRect(UiInstance.pLcd->Lcd,Color,X,Tmp,X1,KnobHeight);
        }
        break;
    }
}

/*! \page cUi User Interface
 *  <hr size="1"/>
 *  <b>     opUI_DRAW (CMD, ....)  </b>
 *
 *- UI draw\n
 *- Dispatch status can change to BUSYBREAK
 *
 *  \param  (DATA8)   CMD           - \ref uidrawsubcode
 *
 *  - CMD = UPDATE\n
 *
 *\n
 *  - CMD = FILLRECT
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA16)  X1       - X size [0..LCD_WIDTH - X0]\n
 *    -  \param  (DATA16)  Y1       - Y size [0..LCD_HEIGHT - Y0]\n
 *
 *\n
 *  - CMD = INVERSERECT
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA16)  X1       - X size [0..LCD_WIDTH - X0]\n
 *    -  \param  (DATA16)  Y1       - Y size [0..LCD_HEIGHT - Y0]\n
 *
 *\n
 *  - CMD = RECTANGLE
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA16)  X1       - X size [0..LCD_WIDTH - X0]\n
 *    -  \param  (DATA16)  Y1       - Y size [0..LCD_HEIGHT - Y0]\n
 *
 *\n
 *  - CMD = PIXEL
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X        - X cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y        - Y cord [0..LCD_HEIGHT]\n
 *
 *\n
 *  - CMD = LINE
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA16)  X1       - X end [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y1       - Y end [0..LCD_HEIGHT]\n
 *
 *\n
 *  - CMD = DOTLINE
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA16)  X1       - X end [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y1       - Y end [0..LCD_HEIGHT]\n
 *    -  \param  (DATA16)  ON       - On pixels\n
 *    -  \param  (DATA16)  OFF      - Off pixels\n
 *
 *\n
 *  - CMD = CIRCLE
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA16)  R        - Radius\n
 *
 *\n
 *  - CMD = FILLCIRCLE
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA16)  R        - Radius\n
 *
 *\n
 *  - CMD = TEXT
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X        - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y        - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA8)   STRING   - First character in string to draw\n
 *
 *\n
 *  - CMD = ICON
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X        - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y        - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA8)   TYPE     - Icon type (pool)\n
 *    -  \param  (DATA8)   NO       - Icon no\n
 *
 *\n
 *  - CMD = BMPFILE
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X        - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y        - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA8)   NAME     - First character in filename (character string)\n
 *
 *\n
 *  - CMD = PICTURE
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X        - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y        - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA32)  *IP      - Address of picture\n
 *
 *\n
 *  - CMD = VALUE
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X        - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y        - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATAF)   VALUE    - Value to write\n
 *    -  \param  (DATA8)   FIGURES  - Total number of figures inclusive decimal point\n
 *    -  \param  (DATA8)   DECIMALS - Number of decimals\n
 *
 *\n
 *  - CMD = VIEW_VALUE
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X        - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y        - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATAF)   VALUE    - Value to write\n
 *    -  \param  (DATA8)   FIGURES  - Total number of figures inclusive decimal point\n
 *    -  \param  (DATA8)   DECIMALS - Number of decimals\n
 *
 *\n
 *  - CMD = VIEW_UNIT
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X        - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y        - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATAF)   VALUE    - Value to write\n
 *    -  \param  (DATA8)   FIGURES  - Total number of figures inclusive decimal point\n
 *    -  \param  (DATA8)   DECIMALS - Number of decimals\n
 *    -  \param  (DATA8)   LENGTH   - Maximal string length\n
 *    -  \param  (DATA8)   STRING   - First character in string to draw\n
 *
 *\n
 *  - CMD = NOTIFICATION
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA8)   ICON1    - First icon\n
 *    -  \param  (DATA8)   ICON2    - Second icon\n
 *    -  \param  (DATA8)   ICON2    - Third icon\n
 *    -  \param  (DATA8)   STRING   - First character in notification string\n
 *    -  \param  (DATA8)   *STATE   - State 0 = INIT\n
 *
 *\n
 *  - CMD = QUESTION
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA8)   ICON1    - First icon\n
 *    -  \param  (DATA8)   ICON2    - Second icon\n
 *    -  \param  (DATA8)   STRING   - First character in question string\n
 *    -  \param  (DATA8)   *STATE   - State 0 = NO, 1 = OK\n
 *    -  \return (DATA8)   OK       - Answer 0 = NO, 1 = OK, -1 = SKIP\n
 *
 *\n
 *  - CMD = ICON_QUESTION
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA8)   *STATE   - State 0 = INIT\n
 *    -  \param  (DATA32)  ICONS    - bitfield with icons\n
 *
 *\n
 *  - CMD = KEYBOARD
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA8)   LENGTH   - Maximal string length\n
 *    -  \param  (DATA8)   DEFAULT  - Default string (0 = none)\n
 *    -  \param  (DATA8)   *CHARSET - Internal use (must be a variable initialised by a "valid character set")\n
 *    -  \return (DATA8)   STRING   - First character in string receiving keyboard input\n
 *
 *\n
 *  - CMD = BROWSE
 *    -  \param  (DATA8)   TYPE     - \ref browsers
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA16)  X1       - X size [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y1       - Y size [0..LCD_HEIGHT]\n
 *    -  \param  (DATA8)   LENGTH   - Maximal string length\n
 *    -  \return (DATA8)   TYPE     - Item type (folder, byte code file, sound file, ...)(must be a zero initialised variable)\n
 *    -  \return (DATA8)   STRING   - First character in string receiving selected item name\n
 *
 *\n
 *  - CMD = VERTBAR
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR]\n
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA16)  X1       - X size [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y1       - Y size [0..LCD_HEIGHT]\n
 *    -  \param  (DATA16)  MIN      - Minimum value\n
 *    -  \param  (DATA16)  MAX      - Maximum value\n
 *    -  \param  (DATA16)  ACT      - Actual value\n
 *
 *\n
 *  - CMD = SELECT_FONT
 *    -  \param  (DATA8)   TYPE     - Font type [0..2] font will change to 0 when UPDATE is called\n
 *
 *\n
 *  - CMD = TOPLINE
 *    -  \param  (DATA8)   ENABLE   - Enable top status line (0 = disabled, 1 = enabled)\n
 *
 *\n
 *  - CMD = FILLWINDOW
 *    -  \param  (DATA8)   COLOR    - Color [BG_COLOR..FG_COLOR] (Color != BG_COLOR and FG_COLOR -> test pattern)\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA16)  Y1       - Y size [0..LCD_HEIGHT]\n
 *
 *\n
 *  - CMD = STORE
 *    -  \param  (DATA8)   NO       - Level number\n
 *
 *\n
 *  - CMD = RESTORE
 *    -  \param  (DATA8)   NO       - Level number (N=0 -> Saved screen just before run)\n
 *
 *\n
 *  - CMD = GRAPH_SETUP
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  X1       - X size [0..(LCD_WIDTH - X0)]\n
 *    -  \param  (DATA8)   ITEMS    - Number of datasets in arrayes\n
 *    -  \param  (DATA16)  OFFSET   - DATA16 array (handle) containing Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA16)  SPAN     - DATA16 array (handle) containing Y size [0..(LCD_HEIGHT - hOFFSET[])]\n
 *    -  \param  (DATAF)   MIN      - DATAF array (handle) containing min values\n
 *    -  \param  (DATAF)   MAX      - DATAF array (handle) containing max values\n
 *    -  \param  (DATAF)   SAMPLE   - DATAF array (handle) containing sample values\n
 *
 *\n
 *  - CMD = GRAPH_DRAW
 *    -  \param  (DATA8)   VIEW     - Dataset number to view (0=all)\n
 *
 *\n
 *  - CMD = TEXTBOX
 *\n  Draws and controls a text box (one long string containing characters and line delimiters) on the screen\n
 *    -  \param  (DATA16)  X0       - X start cord [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y0       - Y start cord [0..LCD_HEIGHT]\n
 *    -  \param  (DATA16)  X1       - X size [0..LCD_WIDTH]\n
 *    -  \param  (DATA16)  Y1       - Y size [0..LCD_HEIGHT]\n
 *    -  \param  (DATA8)   TEXT     - First character in text box text (must be zero terminated)\n
 *    -  \param  (DATA32)  SIZE     - Maximal text size (including zero termination)\n
 *    -  \param  (DATA8)     \ref delimiters "DEL" - Delimiter code\n
 *    -  \return (DATA16)  LINE     - Selected line number\n
 *
 *\n
 *
 */
/*! \brief  opUI_DRAW byte code
 *
 */
void cUiDraw(void)
{
    PRGID   TmpPrgId;
    OBJID   TmpObjId;
    IP      TmpIp;
    DATA8   GBuffer[25];
    UBYTE   pBmp[LCD_BUFFER_SIZE];
    DATA8   Cmd;
    DATA8   Color;
    DATA16  X;
    DATA16  Y;
    DATA16  X1;
    DATA16  Y1;
    DATA16  Y2;
    DATA16  Y3;
    DATA32  Size;
    DATA16  R;
    DATA8   *pText;
    DATA8   No;
    DATAF   DataF;
    DATA8   Figures;
    DATA8   Decimals;
    IP      pI;
    DATA8   *pState;
    DATA8   *pAnswer;
    DATA8   Lng;
    DATA8   SelectedChar;
    DATA8   *pType;
    DATA8   Type;
    DATA16  On;
    DATA16  Off;
    DATA16  CharWidth;
    DATA16  CharHeight;
    DATA8   TmpColor;
    DATA16  Tmp;
    DATA8   Length;
    DATA8   *pUnit;
    DATA32  *pIcons;
    DATA8   Items;
    DATA8   View;
    DATA16  *pOffset;
    DATA16  *pSpan;
    DATAF   *pMin;
    DATAF   *pMax;
    DATAF   *pVal;
    DATA16  Min;
    DATA16  Max;
    DATA16  Act;
    DATAF   Actual;
    DATAF   Lowest;
    DATAF   Highest;
    DATAF   Average;
    DATA8   Icon1;
    DATA8   Icon2;
    DATA8   Icon3;
    DATA8   Blocked;
    DATA8   Open;
    DATA8   Del;
    DATA8   *pCharSet;
    DATA16  *pLine;


    TmpPrgId      =  CurrentProgramId();

    if ((TmpPrgId != GUI_SLOT) && (TmpPrgId != DEBUG_SLOT))
    {
        UiInstance.RunScreenEnabled   =  0;
    }
    if (UiInstance.ScreenBlocked == 0)
    {
        Blocked  =  0;
    }
    else
    {
        TmpObjId      =  CallingObjectId();
        if ((TmpPrgId == UiInstance.ScreenPrgId) && (TmpObjId == UiInstance.ScreenObjId))
        {
            Blocked  =  0;
        }
        else
        {
            Blocked  =  1;
        }
    }

    TmpIp   =  GetObjectIp();
    Cmd     =  *(DATA8*)PrimParPointer();

    switch (Cmd)
    { // Function

        case UPDATE :
        {
            if (Blocked == 0)
            {
                cUiUpdateLcd();
                UiInstance.ScreenBusy  =  0;
            }
        }
        break;

        case CLEAN :
        {
            if (Blocked == 0)
            {
                UiInstance.Font  =  NORMAL_FONT;

                Color  =  BG_COLOR;
                if (Color)
                {
                    Color   =  -1;
                }
                memset(&(UiInstance.pLcd->Lcd[0]),Color,LCD_BUFFER_SIZE);

                UiInstance.ScreenBusy  =  1;
            }
        }
        break;

        case TEXTBOX :
        {
            X         =  *(DATA16*)PrimParPointer();  // start x
            Y         =  *(DATA16*)PrimParPointer();  // start y
            X1        =  *(DATA16*)PrimParPointer();  // size x
            Y1        =  *(DATA16*)PrimParPointer();  // size y
            pText     =  (DATA8*)PrimParPointer();    // textbox
            Size      =  *(DATA32*)PrimParPointer();  // textbox size
            Del       =  *(DATA8*)PrimParPointer();   // delimitter
            pLine     =  (DATA16*)PrimParPointer();   // line

            if (Blocked == 0)
            {
                if (cUiTextbox(X,Y,X1,Y1,pText,Size,Del,pLine) == BUSY)
                {
                    SetObjectIp(TmpIp - 1);
                    SetDispatchStatus(BUSYBREAK);
                }
            }
            else
            {
                SetObjectIp(TmpIp - 1);
                SetDispatchStatus(BUSYBREAK);
            }
        }
        break;

        case FILLRECT :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();
            Y         =  *(DATA16*)PrimParPointer();
            X1        =  *(DATA16*)PrimParPointer();
            Y1        =  *(DATA16*)PrimParPointer();
            if (Blocked == 0)
            {
                dLcdFillRect(UiInstance.pLcd->Lcd,Color,X,Y,X1,Y1);
                UiInstance.ScreenBusy  =  1;
            }
        }
        break;

        case INVERSERECT :
        {
            X         =  *(DATA16*)PrimParPointer();
            Y         =  *(DATA16*)PrimParPointer();
            X1        =  *(DATA16*)PrimParPointer();
            Y1        =  *(DATA16*)PrimParPointer();
            if (Blocked == 0)
            {
                dLcdInverseRect(UiInstance.pLcd->Lcd,X,Y,X1,Y1);
                UiInstance.ScreenBusy  =  1;
            }
        }
        break;

        case RECTANGLE :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();
            Y         =  *(DATA16*)PrimParPointer();
            X1        =  *(DATA16*)PrimParPointer();
            Y1        =  *(DATA16*)PrimParPointer();
            if (Blocked == 0)
            {
                dLcdRect(UiInstance.pLcd->Lcd,Color,X,Y,X1,Y1);
                UiInstance.ScreenBusy  =  1;
            }
        }
        break;

        case PIXEL :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();
            Y         =  *(DATA16*)PrimParPointer();
            if (Blocked == 0)
            {
                dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X,Y);
                UiInstance.ScreenBusy  =  1;
            }
        }
        break;

        case LINE :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();
            Y         =  *(DATA16*)PrimParPointer();
            X1        =  *(DATA16*)PrimParPointer();
            Y1        =  *(DATA16*)PrimParPointer();
            if (Blocked == 0)
            {
                dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X,Y,X1,Y1);
                UiInstance.ScreenBusy  =  1;
            }
        }
        break;

        case DOTLINE :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();
            Y         =  *(DATA16*)PrimParPointer();
            X1        =  *(DATA16*)PrimParPointer();
            Y1        =  *(DATA16*)PrimParPointer();
            On        =  *(DATA16*)PrimParPointer();
            Off       =  *(DATA16*)PrimParPointer();
            if (Blocked == 0)
            {
                dLcdDrawDotLine(UiInstance.pLcd->Lcd,Color,X,Y,X1,Y1,On,Off);
                UiInstance.ScreenBusy  =  1;
            }
        }
        break;

        case CIRCLE :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();
            Y         =  *(DATA16*)PrimParPointer();
            R         =  *(DATA16*)PrimParPointer();
            if (R)
            {
                if (Blocked == 0)
                {
                    dLcdDrawCircle(UiInstance.pLcd->Lcd,Color,X,Y,R);
                    UiInstance.ScreenBusy  =  1;
                }
            }
        }
        break;

        case FILLCIRCLE :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();
            Y         =  *(DATA16*)PrimParPointer();
            R         =  *(DATA16*)PrimParPointer();
            if (R)
            {
                if (Blocked == 0)
                {
                    dLcdDrawFilledCircle(UiInstance.pLcd->Lcd,Color,X,Y,R);
                    UiInstance.ScreenBusy  =  1;
                }
            }
        }
        break;

        case TEXT :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();
            Y         =  *(DATA16*)PrimParPointer();
            pText     =  (DATA8*)PrimParPointer();
            if (Blocked == 0)
            {
                dLcdDrawText(UiInstance.pLcd->Lcd,Color,X,Y,UiInstance.Font,pText);
                UiInstance.ScreenBusy  =  1;
            }
        }
        break;

        case ICON :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();
            Y         =  *(DATA16*)PrimParPointer();
            Type      =  *(DATA8*)PrimParPointer();
            No        =  *(DATA8*)PrimParPointer();
            if (Blocked == 0)
            {
                dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,X,Y,Type,No);
                UiInstance.ScreenBusy  =  1;
            }
        }
        break;

        case BMPFILE :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();
            Y         =  *(DATA16*)PrimParPointer();
            pText     =  (DATA8*)PrimParPointer();

            if (Blocked == 0)
            {
                if (cMemoryGetImage(pText,LCD_BUFFER_SIZE,pBmp) == OK)
                {
                    dLcdDrawBitmap(UiInstance.pLcd->Lcd,Color,X,Y,pBmp);
                    UiInstance.ScreenBusy  =  1;
                }
            }
        }
        break;

        case PICTURE :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();
            Y         =  *(DATA16*)PrimParPointer();
            pI        =  *(IP*)PrimParPointer();
            if (pI != NULL)
            {
                if (Blocked == 0)
                {
                    dLcdDrawBitmap(UiInstance.pLcd->Lcd,Color,X,Y,pI);
                    UiInstance.ScreenBusy  =  1;
                }
            }
        }
        break;

        case VALUE :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();
            Y         =  *(DATA16*)PrimParPointer();
            DataF     =  *(DATAF*)PrimParPointer();
            Figures   =  *(DATA8*)PrimParPointer();
            Decimals  =  *(DATA8*)PrimParPointer();

            if (isnan(DataF))
            {
                if (Figures < 0)
                {
                    Figures  =  0 - Figures;
                }
                for (Lng = 0;Lng < Figures;Lng++)
                {
                    GBuffer[Lng]  =  '-';
                }
                GBuffer[Lng]  =  0;
            }
            else
            {
                if (Figures < 0)
                {
                    Figures  =  0 - Figures;
                    snprintf((char*)GBuffer,24,"%.*f",Decimals,DataF);
                }
                else
                {
                    snprintf((char*)GBuffer,24,"%*.*f",Figures,Decimals,DataF);
                }
                if (GBuffer[0] == '-')
                { // Negative

                    Figures++;
                }
                GBuffer[Figures]  =  0;
            }
            pText     =  GBuffer;
            if (Blocked == 0)
            {
                dLcdDrawText(UiInstance.pLcd->Lcd,Color,X,Y,UiInstance.Font,pText);
                UiInstance.ScreenBusy  =  1;
            }
        }
        break;

        case VIEW_VALUE :
        {
            Color       =  *(DATA8*)PrimParPointer();
            X           =  *(DATA16*)PrimParPointer();
            Y           =  *(DATA16*)PrimParPointer();
            DataF       =  *(DATAF*)PrimParPointer();
            Figures     =  *(DATA8*)PrimParPointer();
            Decimals    =  *(DATA8*)PrimParPointer();
            if (Blocked == 0)
            {

                if (Figures < 0)
                {
                    Figures  =  0 - Figures;
                }
                TmpColor    =  Color;
                CharWidth   =  dLcdGetFontWidth(UiInstance.Font);
                CharHeight  =  dLcdGetFontHeight(UiInstance.Font);
                X1          =  ((CharWidth + 2) / 3) - 1;
                Y1          =  (CharHeight / 2);

                Lng         =  (DATA8)snprintf((char*)GBuffer,24,"%.*f",Decimals,DataF);

                if (Lng)
                {
                    if (GBuffer[0] == '-')
                    { // Negative

                        TmpColor  =  Color;
                        Lng--;
                        pText     =  &GBuffer[1];
                    }
                    else
                    { // Positive

                        TmpColor  =  1 - Color;
                        pText     =  GBuffer;
                    }

                    // Make sure negative sign is deleted from last time
                    dLcdDrawLine(UiInstance.pLcd->Lcd,1 - Color,X - X1,Y + Y1,X + (Figures * CharWidth),Y + Y1);
                    if (CharHeight > 12)
                    {
                        dLcdDrawLine(UiInstance.pLcd->Lcd,1 - Color,X - X1,Y + Y1 - 1,X + (Figures * CharWidth),Y + Y1 - 1);
                    }

                    // Check for "not a number"
                    Tmp         =  0;
                    while((pText[Tmp] != 0) && (pText[Tmp] != 'n'))
                    {
                        Tmp++;
                    }
                    if (pText[Tmp] == 'n')
                    { // "nan"

                        for (Tmp = 0;Tmp < (DATA16)Figures;Tmp++)
                        {
                            GBuffer[Tmp]  =  '-';
                        }
                        GBuffer[Tmp]    =  0;

                        // Draw figures
                        dLcdDrawText(UiInstance.pLcd->Lcd,Color,X,Y,UiInstance.Font,GBuffer);
                    }
                    else
                    { // Normal number

                        // Check number of figures
                        if (Lng > Figures)
                        { // Limit figures

                            for (Tmp = 0;Tmp < (DATA16)Figures;Tmp++)
                            {
                                GBuffer[Tmp]  =  '>';
                            }
                            GBuffer[Tmp]    =  0;
                            Lng             =  (DATA16)Figures;
                            pText           =  GBuffer;
                            TmpColor        =  1 - Color;

                            // Find X indent
                            Tmp             =  ((DATA16)Figures - Lng) * CharWidth;
                        }
                        else
                        { // Centre figures

                            // Find X indent
                            Tmp             =  ((((DATA16)Figures - Lng) + 1) / 2) * CharWidth;
                        }

                        // Draw figures
                        dLcdDrawText(UiInstance.pLcd->Lcd,Color,X + Tmp,Y,UiInstance.Font,pText);

                        // Draw negative sign
                        dLcdDrawLine(UiInstance.pLcd->Lcd,TmpColor,X - X1 + Tmp,Y + Y1,X + Tmp,Y + Y1);
                        if (CharHeight > 12)
                        {
                            dLcdDrawLine(UiInstance.pLcd->Lcd,TmpColor,X - X1 + Tmp,Y + Y1 - 1,X + Tmp,Y + Y1 - 1);
                        }
                    }
                }
                UiInstance.ScreenBusy  =  1;
            }
        }
        break;

        case VIEW_UNIT :
        {
            Color       =  *(DATA8*)PrimParPointer();
            X           =  *(DATA16*)PrimParPointer();
            Y           =  *(DATA16*)PrimParPointer();
            DataF       =  *(DATAF*)PrimParPointer();
            Figures     =  *(DATA8*)PrimParPointer();
            Decimals    =  *(DATA8*)PrimParPointer();
            Length      =  *(DATA8*)PrimParPointer();
            pUnit       =  (DATA8*)PrimParPointer();

            if (Blocked == 0)
            {
                if (Figures < 0)
                {
                    Figures  =  0 - Figures;
                }
                TmpColor    =  Color;
                CharWidth   =  dLcdGetFontWidth(LARGE_FONT);
                CharHeight  =  dLcdGetFontHeight(LARGE_FONT);
                X1          =  ((CharWidth + 2) / 3) - 1;
                Y1          =  (CharHeight / 2);

                Lng         =  (DATA8)snprintf((char*)GBuffer,24,"%.*f",Decimals,DataF);

                if (Lng)
                {
                    if (GBuffer[0] == '-')
                    { // Negative

                        TmpColor  =  Color;
                        Lng--;
                        pText     =  &GBuffer[1];
                    }
                    else
                    { // Positive

                        TmpColor  =  1 - Color;
                        pText     =  GBuffer;
                    }

                    // Make sure negative sign is deleted from last time
                    dLcdDrawLine(UiInstance.pLcd->Lcd,1 - Color,X - X1,Y + Y1,X + (Figures * CharWidth),Y + Y1);
                    if (CharHeight > 12)
                    {
                        dLcdDrawLine(UiInstance.pLcd->Lcd,1 - Color,X - X1,Y + Y1 - 1,X + (Figures * CharWidth),Y + Y1 - 1);
                    }

                    // Check for "not a number"
                    Tmp         =  0;
                    while((pText[Tmp] != 0) && (pText[Tmp] != 'n'))
                    {
                        Tmp++;
                    }
                    if (pText[Tmp] == 'n')
                    { // "nan"

                        for (Tmp = 0;Tmp < (DATA16)Figures;Tmp++)
                        {
                            GBuffer[Tmp]  =  '-';
                        }
                        GBuffer[Tmp]    =  0;

                        // Draw figures
                        dLcdDrawText(UiInstance.pLcd->Lcd,Color,X,Y,LARGE_FONT,GBuffer);
                    }
                    else
                    { // Normal number

                        // Check number of figures
                        if (Lng > Figures)
                        { // Limit figures

                            for (Tmp = 0;Tmp < (DATA16)Figures;Tmp++)
                            {
                                GBuffer[Tmp]  =  '>';
                            }
                            GBuffer[Tmp]    =  0;
                            Lng             =  (DATA16)Figures;
                            pText           =  GBuffer;
                            TmpColor        =  1 - Color;

                            // Find X indent
                            Tmp             =  ((DATA16)Figures - Lng) * CharWidth;
                        }
                        else
                        { // Centre figures

                            // Find X indent
                            Tmp             =  ((((DATA16)Figures - Lng) + 1) / 2) * CharWidth;
                        }
                        Tmp               =  0;

                        // Draw figures
                        dLcdDrawText(UiInstance.pLcd->Lcd,Color,X + Tmp,Y,LARGE_FONT,pText);

                        // Draw negative sign
                        dLcdDrawLine(UiInstance.pLcd->Lcd,TmpColor,X - X1 + Tmp,Y + Y1,X + Tmp,Y + Y1);
                        if (CharHeight > 12)
                        {
                            dLcdDrawLine(UiInstance.pLcd->Lcd,TmpColor,X - X1 + Tmp,Y + Y1 - 1,X + Tmp,Y + Y1 - 1);
                        }

                        Tmp               =  ((((DATA16)Lng))) * CharWidth;
                        snprintf((char*)GBuffer,Length,"%s",pUnit);
                        dLcdDrawText(UiInstance.pLcd->Lcd,Color,X + Tmp,Y,SMALL_FONT,GBuffer);

                    }
                }
                UiInstance.ScreenBusy  =  1;
            }
        }
        break;

        case NOTIFICATION :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();  // start x
            Y         =  *(DATA16*)PrimParPointer();  // start y
            Icon1     =  *(DATA8*)PrimParPointer();
            Icon2     =  *(DATA8*)PrimParPointer();
            Icon3     =  *(DATA8*)PrimParPointer();
            pText     =  (DATA8*)PrimParPointer();
            pState    =  (DATA8*)PrimParPointer();

            if (Blocked == 0)
            {
                if (cUiNotification(Color,X,Y,Icon1,Icon2,Icon3,pText,pState) == BUSY)
                {
                    SetObjectIp(TmpIp - 1);
                    SetDispatchStatus(BUSYBREAK);
                }
            }
            else
            {
                SetObjectIp(TmpIp - 1);
                SetDispatchStatus(BUSYBREAK);
            }
        }
        break;

        case QUESTION :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();  // start x
            Y         =  *(DATA16*)PrimParPointer();  // start y
            Icon1     =  *(DATA8*)PrimParPointer();
            Icon2     =  *(DATA8*)PrimParPointer();
            pText     =  (DATA8*)PrimParPointer();
            pState    =  (DATA8*)PrimParPointer();
            pAnswer   =  (DATA8*)PrimParPointer();

            if (Blocked == 0)
            {
                if (cUiQuestion(Color,X,Y,Icon1,Icon2,pText,pState,pAnswer) == BUSY)
                {
                    SetObjectIp(TmpIp - 1);
                    SetDispatchStatus(BUSYBREAK);
                }
            }
            else
            {
                SetObjectIp(TmpIp - 1);
                SetDispatchStatus(BUSYBREAK);
            }
        }
        break;


        case ICON_QUESTION :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();  // start x
            Y         =  *(DATA16*)PrimParPointer();  // start y
            pState    =  (DATA8*)PrimParPointer();
            pIcons    =  (DATA32*)PrimParPointer();

            if (Blocked == 0)
            {
                if (cUiIconQuestion(Color,X,Y,pState,pIcons) == BUSY)
                {
                    SetObjectIp(TmpIp - 1);
                    SetDispatchStatus(BUSYBREAK);
                }
            }
            else
            {
                SetObjectIp(TmpIp - 1);
                SetDispatchStatus(BUSYBREAK);
            }
        }
        break;


        case KEYBOARD :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();  // start x
            Y         =  *(DATA16*)PrimParPointer();  // start y
            No        =  *(DATA8*)PrimParPointer();   // Icon
            Lng       =  *(DATA8*)PrimParPointer();   // length
            pText     =  (DATA8*)PrimParPointer();    // default
            pCharSet  =  (DATA8*)PrimParPointer();    // valid char set
            pAnswer   =  (DATA8*)PrimParPointer();    // string

            if (VMInstance.Handle >= 0)
            {
                pAnswer  =  (DATA8*)VmMemoryResize(VMInstance.Handle,(DATA32)Lng);
            }

            if (Blocked == 0)
            {
                SelectedChar  =  cUiKeyboard(Color,X,Y,No,Lng,pText,pCharSet,pAnswer);

                // Wait for "ENTER"
                if (SelectedChar != 0x0D)
                {
                    SetObjectIp(TmpIp - 1);
                    SetDispatchStatus(BUSYBREAK);
                }
            }
            else
            {
                SetObjectIp(TmpIp - 1);
                SetDispatchStatus(BUSYBREAK);
            }
        }
        break;

        case BROWSE :
        {
            Type      =  *(DATA8*)PrimParPointer();   // Browser type
            X         =  *(DATA16*)PrimParPointer();  // start x
            Y         =  *(DATA16*)PrimParPointer();  // start y
            X1        =  *(DATA16*)PrimParPointer();  // size x
            Y1        =  *(DATA16*)PrimParPointer();  // size y
            Lng       =  *(DATA8*)PrimParPointer();   // length
            pType     =  (DATA8*)PrimParPointer();    // item type
            pAnswer   =  (DATA8*)PrimParPointer();    // item name

            if (VMInstance.Handle >= 0)
            {
                pAnswer  =  (DATA8*)VmMemoryResize(VMInstance.Handle,(DATA32)Lng);
            }

            if (Blocked == 0)
            {
                if (cUiBrowser(Type,X,Y,X1,Y1,Lng,pType,pAnswer) == BUSY)
                {
                    SetObjectIp(TmpIp - 1);
                    SetDispatchStatus(BUSYBREAK);
                }
            }
            else
            {
                SetObjectIp(TmpIp - 1);
                SetDispatchStatus(BUSYBREAK);
            }
        }
        break;

        case VERTBAR :
        {
            Color     =  *(DATA8*)PrimParPointer();
            X         =  *(DATA16*)PrimParPointer();  // start x
            Y         =  *(DATA16*)PrimParPointer();  // start y
            X1        =  *(DATA16*)PrimParPointer();  // size x
            Y1        =  *(DATA16*)PrimParPointer();  // size y
            Min       =  *(DATA16*)PrimParPointer();  // min
            Max       =  *(DATA16*)PrimParPointer();  // max
            Act       =  *(DATA16*)PrimParPointer();  // actual

            if (Blocked == 0)
            {
                cUiDrawBar(Color,X,Y,X1,Y1,Min,Max,Act);
            }
        }
        break;

        case SELECT_FONT :
        {
            UiInstance.Font  =  *(DATA8*)PrimParPointer();
            if (Blocked == 0)
            {
                if (UiInstance.Font >= FONTTYPES)
                {
                    UiInstance.Font  =  (FONTTYPES - 1);
                }
                if (UiInstance.Font < 0)
                {
                    UiInstance.Font  =  0;
                }
            }
        }
        break;

        case TOPLINE :
        {
            UiInstance.TopLineEnabled  =  *(DATA8*)PrimParPointer();
        }
        break;

        case FILLWINDOW :
        {
            Color     =  *(DATA8*)PrimParPointer();
            Y         =  *(DATA16*)PrimParPointer();  // start y
            Y1        =  *(DATA16*)PrimParPointer();  // size y
            if (Blocked == 0)
            {
                UiInstance.Font  =  NORMAL_FONT;

                if ((Y + Y1) < LCD_HEIGHT)
                {
                    if ((Color == 0) || (Color == 1))
                    {
                        Y        *=  ((LCD_WIDTH + 7) / 8);

                        if (Y1)
                        {
                            Y1     *=  ((LCD_WIDTH + 7) / 8);
                        }
                        else
                        {
                            Y1      =  LCD_BUFFER_SIZE - Y;
                        }

                        if (Color)
                        {
                            Color   =  -1;
                        }
                        memset(&(UiInstance.pLcd->Lcd[Y]),Color,Y1);
                    }
                    else
                    {
                        if (Y1 == 0)
                        {
                            Y1  =  LCD_HEIGHT;
                        }
                        Y2  =  ((LCD_WIDTH + 7) / 8);
                        for (Tmp = Y;Tmp < Y1;Tmp++)
                        {
                            Y3        =  Tmp * ((LCD_WIDTH + 7) / 8);
                            memset(&(UiInstance.pLcd->Lcd[Y3]),Color,Y2);
                            Color  =  ~Color;
                        }
                    }
                }

                UiInstance.ScreenBusy  =  1;
            }
        }
        break;

        case STORE :
        {
            No  =  *(DATA8*)PrimParPointer();
            if (Blocked == 0)
            {
                if (No < LCD_STORE_LEVELS)
                {
                    LCDCopy(&UiInstance.LcdSafe,&UiInstance.LcdPool[No],sizeof(LCD));
                }
            }
        }
        break;

        case RESTORE :
        {
            No  =  *(DATA8*)PrimParPointer();
            if (Blocked == 0)
            {
                if (No < LCD_STORE_LEVELS)
                {
                    LCDCopy(&UiInstance.LcdPool[No],&UiInstance.LcdSafe,sizeof(LCD));
                    UiInstance.ScreenBusy  =  1;
                }
            }
        }
        break;

        case GRAPH_SETUP :
        {
            X         =  *(DATA16*)PrimParPointer();  // start x
            X1        =  *(DATA16*)PrimParPointer();  // size y
            Items     =  *(DATA8*)PrimParPointer();   // items
            pOffset   =  (DATA16*)PrimParPointer();  // handle to offset Y
            pSpan     =  (DATA16*)PrimParPointer();  // handle to span y
            pMin      =  (DATAF*)PrimParPointer();  // handle to min
            pMax      =  (DATAF*)PrimParPointer();  // handle to max
            pVal      =  (DATAF*)PrimParPointer();  // handle to val

            if (Blocked == 0)
            {
                cUiGraphSetup(X,X1,Items,pOffset,pSpan,pMin,pMax,pVal);
            }
        }
        break;

        case GRAPH_DRAW :
        {
            View      =  *(DATA8*)PrimParPointer();   // view

            cUiGraphDraw(View,&Actual,&Lowest,&Highest,&Average);

            *(DATAF*)PrimParPointer()  =  Actual;
            *(DATAF*)PrimParPointer()  =  Lowest;
            *(DATAF*)PrimParPointer()  =  Highest;
            *(DATAF*)PrimParPointer()  =  Average;
        }
        break;

        case SCROLL :
        {
            Y  =  *(DATA16*)PrimParPointer();
            if ((Y > 0) && (Y < LCD_HEIGHT))
            {
                dLcdScroll(UiInstance.pLcd->Lcd,Y);
            }
        }
        break;

        case POPUP :
        {
            Open  =  *(DATA8*)PrimParPointer();
            if (Blocked == 0)
            {
                if (Open)
                {
                    if (!UiInstance.ScreenBusy)
                    {
                        TmpObjId                  =  CallingObjectId();

                        LCDCopy(&UiInstance.LcdSafe,&UiInstance.LcdSave,sizeof(UiInstance.LcdSave));
                        UiInstance.ScreenPrgId    =  TmpPrgId;
                        UiInstance.ScreenObjId    =  TmpObjId;
                        UiInstance.ScreenBlocked  =  1;
                    }
                    else
                    { // Wait on scrreen

                        SetObjectIp(TmpIp - 1);
                        SetDispatchStatus(BUSYBREAK);
                    }
                }
                else
                {
                    LCDCopy(&UiInstance.LcdSave,&UiInstance.LcdSafe,sizeof(UiInstance.LcdSafe));
                    dLcdUpdate(UiInstance.pLcd);

                    UiInstance.ScreenPrgId      =  -1;
                    UiInstance.ScreenObjId      =  -1;
                    UiInstance.ScreenBlocked    =  0;
                }
            }
            else
            { // Wait on not blocked

                SetObjectIp(TmpIp - 1);
                SetDispatchStatus(BUSYBREAK);
            }
        }
        break;
    }
}
