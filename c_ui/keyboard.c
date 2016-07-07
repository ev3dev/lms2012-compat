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
#include "button.h"
#include "c_ui.h"
#include "d_lcd.h"

#include <string.h>

#define MAX_KEYB_DEEPT      3
#define MAX_KEYB_WIDTH      12
#define MAX_KEYB_HEIGHT     4

#include "keyboardc.xbm"
#include "keyboards.xbm"
#include "keyboardn.xbm"

DATA8 cUiKeyboard(DATA8 Color, DATA16 X, DATA16 Y, DATA8 Icon, DATA8 Lng,
                  DATA8 *pText, DATA8 *pCharSet, DATA8 *pAnswer)
{
    KEYB    *pK;
    DATA16  Width;
    DATA16  Height;
    DATA16  Inc;
    DATA16  SX;
    DATA16  SY;
    DATA16  X3;
    DATA16  X4;
    DATA16  Tmp;
    DATA8   TmpChar;
    DATA8   SelectedChar = 0;

    //            Value    Marking
    //  table  >  0x20  -> normal rect
    //  table  =  0x20  -> spacebar
    //  table  =  0     -> end
    //  table  =  0x01  -> num
    //  table  =  0x02  -> cap
    //  table  =  0x03  -> non cap
    //  table  =  0x04  -> big cap
    //  table  =  0x08  -> backspace
    //  table  =  0x0D  -> enter
    //


    DATA8     KeyboardLayout[MAX_KEYB_DEEPT][MAX_KEYB_HEIGHT][MAX_KEYB_WIDTH] =
    {
        {
            {  'Q','W','E','R','T','Y','U','I','O','P',  0x08,0x00 },
            { 0x03,'A','S','D','F','G','H','J','K','L',  0x0D,0x00 },
            { 0x01,'Z','X','C','V','B','N','M',0x0D,0x0D,0x0D,0x00 },
            {  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',  0x0D,0x00 }
        },
        {
            {  'q','w','e','r','t','y','u','i','o','p',  0x08,0x00 },
            { 0x02,'a','s','d','f','g','h','j','k','l',  0x0D,0x00 },
            { 0x01,'z','x','c','v','b','n','m',0x0D,0x0D,0x0D,0x00 },
            {  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',  0x0D,0x00 }
        },
        {
            {  '1','2','3','4','5','6','7','8','9','0',  0x08,0x00 },
            { 0x04,'+','-','=','<','>','/','\\','*',':', 0x0D,0x00 },
            { 0x04,'(',')','_','.','@','!','?',0x0D,0x0D,0x0D,0x00 },
            {  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',  0x0D,0x00 }
        }
    };

    pK      =  &UiInstance.Keyboard;

    if (*pCharSet != 0)
    {
        pK->CharSet       =  *pCharSet;
        *pCharSet           =  0;
        pK->ScreenStartX  =  X;
        pK->ScreenStartY  =  Y;

        if ((Icon >= 0) && (Icon < N_ICON_NOS))
        {
            pK->IconStartX  =  cUiAlignX(pK->ScreenStartX + 7);
            pK->IconStartY  =  pK->ScreenStartY + 4;
            pK->TextStartX  =  pK->IconStartX + dLcdGetIconWidth(NORMAL_ICON);
        }
        else
        {
            pK->TextStartX  =  cUiAlignX(pK->ScreenStartX + 9);
        }
        pK->TextStartY    =  pK->ScreenStartY + 7;
        pK->StringStartX  =  pK->ScreenStartX + 8;
        pK->StringStartY  =  pK->ScreenStartY + 22;
        pK->KeybStartX    =  pK->ScreenStartX + 13;
        pK->KeybStartY    =  pK->ScreenStartY + 40;
        pK->KeybSpaceX    =  11;
        pK->KeybSpaceY    =  14;
        pK->KeybHeight    =  13;
        pK->KeybWidth     =  9;
        pK->Layout        =  0;
        pK->PointerX      =  10;
        pK->PointerY      =  1;
        pK->NeedUpdate    =  1;
    }

    Width             =  strlen((char*)KeyboardLayout[pK->Layout][pK->PointerY]) - 1;
    Height            =  MAX_KEYB_HEIGHT - 1;

    Inc               =  cUiButtonGetHorz();
    pK->PointerX   +=  Inc;
    if (pK->PointerX < 0)
    {
        pK->PointerX  =  0;
    }
    if (pK->PointerX > Width)
    {
        pK->PointerX  =  Width;
    }
    Inc               =  cUiButtonGetVert();
    pK->PointerY   +=  Inc;
    if (pK->PointerY < 0)
    {
        pK->PointerY  =  0;
    }
    if (pK->PointerY > Height)
    {
        pK->PointerY  =  Height;
    }


    TmpChar  =  KeyboardLayout[pK->Layout][pK->PointerY][pK->PointerX];

    if (cUiButtonGetShortPress(BACK_BUTTON))
    {
        SelectedChar  =  0x0D;
        pAnswer[0]    =  0;
    }
    if (cUiButtonGetShortPress(ENTER_BUTTON))
    {
        SelectedChar  =  TmpChar;

        switch (SelectedChar)
        {
            case 0x01 :
            {
                pK->Layout  =  2;
            }
            break;

            case 0x02 :
            {
                pK->Layout  =  0;
            }
            break;

            case 0x03 :
            {
                pK->Layout  =  1;
            }
            break;

            case 0x04 :
            {
                pK->Layout  =  0;
            }
            break;

            case 0x08 :
            {
                Tmp  =  (DATA16)strlen((char*)pAnswer);
                if (Tmp)
                {
                    Tmp--;
                    pAnswer[Tmp]  =  0;
                }
            }
            break;

            case '\r' :
            {
            }
            break;

            default :
            {
                if (ValidateChar(&SelectedChar,pK->CharSet) == OK)
                {
                    Tmp           =  (DATA16)strlen((char*)pAnswer);
                    pAnswer[Tmp]  =  SelectedChar;
                    if (++Tmp >= Lng)
                    {
                        Tmp--;
                    }
                    pAnswer[Tmp]  =  0;
                }
            }
            break;


        }

        TmpChar  =  KeyboardLayout[pK->Layout][pK->PointerY][pK->PointerX];

        pK->NeedUpdate    =  1;
    }

    if ((pK->OldX != pK->PointerX) || (pK->OldY != pK->PointerY))
    {
        pK->OldX  =  pK->PointerX;
        pK->OldY  =  pK->PointerY;
        pK->NeedUpdate    =  1;
    }

    if (pK->NeedUpdate)
    {
//* UPDATE ***************************************************************************************************
        pK->NeedUpdate    =  0;

        switch (pK->Layout)
        {
            case 0 :
            {
                dLcdDrawPicture(UiInstance.pLcd->Lcd,Color,pK->ScreenStartX,pK->ScreenStartY,keyboardc_width,keyboardc_height,(UBYTE*)keyboardc_bits);
            }
            break;

            case 1 :
            {
                dLcdDrawPicture(UiInstance.pLcd->Lcd,Color,pK->ScreenStartX,pK->ScreenStartY,keyboards_width,keyboards_height,(UBYTE*)keyboards_bits);
            }
            break;

            case 2 :
            {
                dLcdDrawPicture(UiInstance.pLcd->Lcd,Color,pK->ScreenStartX,pK->ScreenStartY,keyboardn_width,keyboardn_height,(UBYTE*)keyboardn_bits);
            }
            break;

        }
        if ((Icon >= 0) && (Icon < N_ICON_NOS))
        {
            dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pK->IconStartX,pK->IconStartY,NORMAL_ICON,Icon);
        }
        if (pText[0])
        {
            dLcdDrawText(UiInstance.pLcd->Lcd,Color,pK->TextStartX,pK->TextStartY,SMALL_FONT,pText);
        }


        X4  =  0;
        X3  =  strlen((char*)pAnswer);
        if (X3 > 15)
        {
            X4  =  X3 - 15;
        }

        dLcdDrawText(UiInstance.pLcd->Lcd,Color,pK->StringStartX,pK->StringStartY,NORMAL_FONT,&pAnswer[X4]);
        dLcdDrawChar(UiInstance.pLcd->Lcd,Color,pK->StringStartX + (X3 - X4) * 8,pK->StringStartY,NORMAL_FONT,'_');



        SX  =  pK->KeybStartX + pK->PointerX * pK->KeybSpaceX;
        SY  =  pK->KeybStartY + pK->PointerY * pK->KeybSpaceY;

        switch (TmpChar)
        {
            case 0x01 :
            case 0x02 :
            case 0x03 :
            {
                dLcdInverseRect(UiInstance.pLcd->Lcd,SX - 8,SY,pK->KeybWidth + 8,pK->KeybHeight);
            }
            break;

            case 0x04 :
            {
                dLcdInverseRect(UiInstance.pLcd->Lcd,SX - 8,pK->KeybStartY + 1 * pK->KeybSpaceY,pK->KeybWidth + 8,pK->KeybHeight * 2 + 1);
            }
            break;

            case 0x08 :
            {
                dLcdInverseRect(UiInstance.pLcd->Lcd,SX + 2,SY,pK->KeybWidth + 5,pK->KeybHeight);
            }
            break;

            case 0x0D :
            {
                SX  =  pK->KeybStartX + 112;
                SY  =  pK->KeybStartY + 1 * pK->KeybSpaceY;
                dLcdInverseRect(UiInstance.pLcd->Lcd,SX,SY,pK->KeybWidth + 5,pK->KeybSpaceY + 1);
                SX  =  pK->KeybStartX + 103;
                SY  =  pK->KeybStartY + 1 + 2 * pK->KeybSpaceY;
                dLcdInverseRect(UiInstance.pLcd->Lcd,SX,SY,pK->KeybWidth + 14,pK->KeybSpaceY * 2 - 4);
            }
            break;

            case 0x20 :
            {
                dLcdInverseRect(UiInstance.pLcd->Lcd,pK->KeybStartX + 11,SY + 1,pK->KeybWidth + 68,pK->KeybHeight - 3);
            }
            break;

            default :
            {
                dLcdInverseRect(UiInstance.pLcd->Lcd,SX + 1,SY,pK->KeybWidth,pK->KeybHeight);
            }
            break;

        }
        cUiUpdateLcd();
        UiInstance.ScreenBusy       =  0;
    }

    return (SelectedChar);
}
