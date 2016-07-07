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

#include <stdio.h>
#include <string.h>

#include "POP2.xbm"
#include "POP3.xbm"

DATA8 cUiNotification(DATA8 Color, DATA16 X, DATA16 Y, DATA8 Icon1, DATA8 Icon2,
                      DATA8 Icon3, DATA8 *pText, DATA8 *pState)
{
    RESULT  Result = BUSY;
    NOTIFY  *pQ;
    DATA16  AvailableX;
    DATA16  UsedX;
    DATA16  Line;
    DATA16  CharIn;
    DATA16  CharOut;
    DATA8   Character;
    DATA16  X2;
    DATA16  Y2;
    DATA16  AvailableY;
    DATA16  UsedY;

    pQ      =  &UiInstance.Notify;

    if (*pState == 0)
    {
        *pState             =  1;
        pQ->ScreenStartX  =  X;
        pQ->ScreenStartY  =  Y;
        pQ->ScreenWidth   =  POP3_width;
        pQ->ScreenHeight  =  POP3_height;
        pQ->IconStartY    =  pQ->ScreenStartY + 10;
        pQ->IconWidth     =  dLcdGetIconWidth(LARGE_ICON);
        pQ->IconHeight    =  dLcdGetIconHeight(LARGE_ICON);
        pQ->IconSpaceX    =  pQ->IconWidth;

        pQ->YesNoStartX   =  pQ->ScreenStartX + (pQ->ScreenWidth / 2);
        pQ->YesNoStartX  -=  (pQ->IconWidth + 8) / 2;
        pQ->YesNoStartX   =  cUiAlignX(pQ->YesNoStartX);
        pQ->YesNoStartY   =  pQ->ScreenStartY + 40;

        pQ->LineStartX    =  pQ->ScreenStartX + 5;
        pQ->LineStartY    =  pQ->ScreenStartY + 39;
        pQ->LineEndX      =  pQ->LineStartX + 134;

        // Find no of icons
        pQ->NoOfIcons     =  0;
        if (Icon1 > ICON_NONE)
        {
            pQ->NoOfIcons++;
        }
        if (Icon2 > ICON_NONE)
        {
            pQ->NoOfIcons++;
        }
        if (Icon3 > ICON_NONE)
        {
            pQ->NoOfIcons++;
        }

        // Find no of text lines
        pQ->TextLines     =  0;
        if (pText[0])
        {
            pQ->IconStartX    =  pQ->ScreenStartX + 8;
            pQ->IconStartX    =  cUiAlignX(pQ->IconStartX);

            AvailableX          =  pQ->ScreenWidth;
            AvailableX         -=  ((pQ->IconStartX - pQ->ScreenStartX)) * 2;

            AvailableX         -=  pQ->NoOfIcons * pQ->IconSpaceX;


            pQ->NoOfChars     =  strlen((char*)pText);


            pQ->Font          =  SMALL_FONT;
            pQ->FontWidth     =  dLcdGetFontWidth(pQ->Font);
            UsedX               =  pQ->FontWidth * pQ->NoOfChars;

            Line                =  0;

            if (UsedX <= AvailableX)
            { // One line - small font

                if ((AvailableX - UsedX) >= 32)
                {
                    pQ->IconStartX += 32;
                }

                snprintf((char*)pQ->TextLine[Line],MAX_NOTIFY_LINE_CHARS,"%s",pText);
                Line++;
                pQ->TextLines++;

                pQ->TextStartX  =  pQ->IconStartX + (pQ->NoOfIcons * pQ->IconSpaceX) ;
                pQ->TextStartY  =  pQ->ScreenStartY + 18;
                pQ->TextSpaceY  =  dLcdGetFontHeight(pQ->Font) + 1;
            }
            else
            { // one or more lines - tiny font

                pQ->Font        =  TINY_FONT;
                pQ->FontWidth   =  dLcdGetFontWidth(pQ->Font);
                UsedX             =  pQ->FontWidth * pQ->NoOfChars;
                AvailableX       -=  pQ->FontWidth;

                CharIn            =  0;

                while ((pText[CharIn]) && (Line < MAX_NOTIFY_LINES))
                {
                    CharOut         =  0;
                    UsedX           =  0;
                    while ((pText[CharIn]) && (CharOut < MAX_NOTIFY_LINE_CHARS) && (UsedX < (AvailableX - pQ->FontWidth)))
                    {
                        Character     =  pText[CharIn];
                        if (Character == '_')
                        {
                            Character   =  ' ';
                        }
                        pQ->TextLine[Line][CharOut]  =  Character;
                        CharIn++;
                        CharOut++;
                        UsedX        +=  pQ->FontWidth;
                    }
                    while ((CharOut > 8) && (pText[CharIn] != ' ') && (pText[CharIn] != '_') && (pText[CharIn] != 0))
                    {
                        CharIn--;
                        CharOut--;
                    }
                    if (pText[CharIn] != 0)
                    {
                        CharIn++;
                    }
                    pQ->TextLine[Line][CharOut]  =  0;
                    Line++;
                }

                pQ->TextLines   =  Line;

                pQ->TextStartX  =  pQ->IconStartX + (pQ->NoOfIcons * pQ->IconSpaceX) + pQ->FontWidth ;
                pQ->TextSpaceY  =  dLcdGetFontHeight(pQ->Font) + 1;


                AvailableY        =  pQ->LineStartY - (pQ->ScreenStartY + 5);
                UsedY             =  pQ->TextLines * pQ->TextSpaceY;

                while (UsedY > AvailableY)
                {
                    pQ->TextLines--;
                    UsedY           =  pQ->TextLines * pQ->TextSpaceY;
                }
                Y2                =  (AvailableY - UsedY) / 2;

                pQ->TextStartY  =  pQ->ScreenStartY + Y2 + 5;
            }

        }
        else
        {
            pQ->IconStartX  =  pQ->ScreenStartX + (pQ->ScreenWidth / 2);
            pQ->IconStartX -=  (pQ->IconWidth + 8) / 2;
            pQ->IconStartX -=  (pQ->NoOfIcons / 2) * pQ->IconWidth;
            pQ->IconStartX  =  cUiAlignX(pQ->IconStartX);
            pQ->TextStartY    =  pQ->ScreenStartY + 8;
        }

        pQ->NeedUpdate    =  1;
    }


    if (pQ->NeedUpdate)
    {
//* UPDATE ***************************************************************************************************
        pQ->NeedUpdate    =  0;

        dLcdDrawPicture(UiInstance.pLcd->Lcd,Color,pQ->ScreenStartX,pQ->ScreenStartY,POP3_width,POP3_height,(UBYTE*)POP3_bits);

        X2                  =  pQ->IconStartX;

        if (Icon1 > ICON_NONE)
        {
            dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,X2,pQ->IconStartY,LARGE_ICON,Icon1);
            X2 +=  pQ->IconSpaceX;
        }
        if (Icon2 > ICON_NONE)
        {
            dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,X2,pQ->IconStartY,LARGE_ICON,Icon2);
            X2 +=  pQ->IconSpaceX;
        }
        if (Icon3 > ICON_NONE)
        {
            dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,X2,pQ->IconStartY,LARGE_ICON,Icon3);
            X2 +=  pQ->IconSpaceX;
        }

        Line  =  0;
        Y2    =  pQ->TextStartY;
        while (Line < pQ->TextLines)
        {
            dLcdDrawText(UiInstance.pLcd->Lcd,Color,pQ->TextStartX,Y2,pQ->Font,pQ->TextLine[Line]);
            Y2 +=  pQ->TextSpaceY;
            Line++;
        }

        dLcdDrawLine(UiInstance.pLcd->Lcd,Color,pQ->LineStartX,pQ->LineStartY,pQ->LineEndX,pQ->LineStartY);

        dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pQ->YesNoStartX,pQ->YesNoStartY,LARGE_ICON,YES_SEL);

        cUiUpdateLcd();
        UiInstance.ScreenBusy       =  0;
    }

    if (cUiButtonGetShortPress(ENTER_BUTTON))
    {
        cUiButtonClearAll();
        Result  =  OK;
        *pState =  0;
    }

    return (Result);
}


DATA8 cUiQuestion(DATA8 Color, DATA16 X, DATA16 Y, DATA8 Icon1, DATA8 Icon2,
                  DATA8 *pText, DATA8 *pState, DATA8 *pAnswer)
{
    RESULT  Result = BUSY;
    TQUESTION *pQ;
    DATA16  Inc;

    pQ      =  &UiInstance.Question;

    Inc  =  cUiButtonGetHorz();
    if (Inc != 0)
    {
        pQ->NeedUpdate    =  1;

        *pAnswer           +=  Inc;

        if (*pAnswer > 1)
        {
            *pAnswer          =  1;
            pQ->NeedUpdate  =  0;
        }
        if (*pAnswer < 0)
        {
            *pAnswer          =  0;
            pQ->NeedUpdate  =  0;
        }
    }

    if (*pState == 0)
    {
        *pState             =  1;
        pQ->ScreenStartX  =  X;
        pQ->ScreenStartY  =  Y;
        pQ->IconWidth     =  dLcdGetIconWidth(LARGE_ICON);
        pQ->IconHeight    =  dLcdGetIconHeight(LARGE_ICON);

        pQ->NoOfIcons     =  0;
        if (Icon1 > ICON_NONE)
        {
            pQ->NoOfIcons++;
        }
        if (Icon2 > ICON_NONE)
        {
            pQ->NoOfIcons++;
        }
        pQ->Default       =  *pAnswer;

        pQ->NeedUpdate    =  1;
    }


    if (pQ->NeedUpdate)
    {
//* UPDATE ***************************************************************************************************
        pQ->NeedUpdate    =  0;

        dLcdDrawPicture(UiInstance.pLcd->Lcd,Color,pQ->ScreenStartX,pQ->ScreenStartY,POP3_width,POP3_height,(UBYTE*)POP3_bits);
        pQ->ScreenWidth   =  POP3_width;
        pQ->ScreenHeight  =  POP3_height;

        pQ->IconStartX    =  pQ->ScreenStartX + (pQ->ScreenWidth / 2);
        if (pQ->NoOfIcons > 1)
        {
            pQ->IconStartX -=  pQ->IconWidth;
        }
        else
        {
            pQ->IconStartX -=  pQ->IconWidth / 2;
        }
        pQ->IconStartX    =  cUiAlignX(pQ->IconStartX);
        pQ->IconSpaceX    =  pQ->IconWidth;
        pQ->IconStartY    =  pQ->ScreenStartY + 10;

        pQ->YesNoStartX   =  pQ->ScreenStartX + (pQ->ScreenWidth / 2);
        pQ->YesNoStartX  -=  8;
        pQ->YesNoStartX  -=  pQ->IconWidth;
        pQ->YesNoStartX   =  cUiAlignX(pQ->YesNoStartX);
        pQ->YesNoSpaceX   =  pQ->IconWidth + 16;
        pQ->YesNoStartY   =  pQ->ScreenStartY + 40;

        pQ->LineStartX    =  pQ->ScreenStartX + 5;
        pQ->LineStartY    =  pQ->ScreenStartY + 39;
        pQ->LineEndX      =  pQ->LineStartX + 134;

        switch (pQ->NoOfIcons)
        {
            case 1 :
            {
                dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pQ->IconStartX,pQ->IconStartY,LARGE_ICON,Icon1);
            }
            break;

            case 2 :
            {
                dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pQ->IconStartX,pQ->IconStartY,LARGE_ICON,Icon1);
                dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pQ->IconStartX + pQ->IconSpaceX,pQ->IconStartY,LARGE_ICON,Icon2);
            }
            break;

        }

        if (*pAnswer == 0)
        {
            dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pQ->YesNoStartX,pQ->YesNoStartY,LARGE_ICON,NO_SEL);
            dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pQ->YesNoStartX + pQ->YesNoSpaceX,pQ->YesNoStartY,LARGE_ICON,YES_NOTSEL);
        }
        else
        {
            dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pQ->YesNoStartX,pQ->YesNoStartY,LARGE_ICON,NO_NOTSEL);
            dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pQ->YesNoStartX + pQ->YesNoSpaceX,pQ->YesNoStartY,LARGE_ICON,YES_SEL);
        }

        dLcdDrawLine(UiInstance.pLcd->Lcd,Color,pQ->LineStartX,pQ->LineStartY,pQ->LineEndX,pQ->LineStartY);

        cUiUpdateLcd();
        UiInstance.ScreenBusy       =  0;
    }
    if (cUiButtonGetShortPress(ENTER_BUTTON))
    {
        cUiButtonClearAll();
        Result    =  OK;
        *pState   =  0;
    }
    if (cUiButtonGetShortPress(BACK_BUTTON))
    {
        cUiButtonClearAll();
        Result    =  OK;
        *pState   =  0;
        *pAnswer  =  -1;
    }

    return (Result);
}

RESULT cUiIconQuestion(DATA8 Color, DATA16 X, DATA16 Y, DATA8 *pState,
                       DATA32 *pIcons)
{
    RESULT  Result = BUSY;
    IQUESTION *pQ;
    DATA32  Mask;
    DATA32  TmpIcons;
    DATA16  Tmp;
    DATA16  Loop;
    DATA8   Icon;

    pQ      =  &UiInstance.IconQuestion;

    if (*pState == 0)
    {
        *pState             =  1;
        pQ->ScreenStartX  =  X;
        pQ->ScreenStartY  =  Y;
        pQ->ScreenWidth   =  POP2_width;
        pQ->ScreenHeight  =  POP2_height;
        pQ->IconWidth     =  dLcdGetIconWidth(LARGE_ICON);
        pQ->IconHeight    =  dLcdGetIconHeight(LARGE_ICON);
        pQ->Frame         =  2;
        pQ->Icons         =  *pIcons;
        pQ->NoOfIcons     =  0;
        pQ->PointerX      =  0;

        TmpIcons   =  pQ->Icons;
        while (TmpIcons)
        {
            if (TmpIcons & 1)
            {
                pQ->NoOfIcons++;
            }
            TmpIcons >>= 1;
        }

        if (pQ->NoOfIcons)
        {
            pQ->IconStartY    =  pQ->ScreenStartY + ((pQ->ScreenHeight - pQ->IconHeight) / 2);

            pQ->IconSpaceX    =  ((pQ->ScreenWidth - (pQ->IconWidth * pQ->NoOfIcons)) / pQ->NoOfIcons) + pQ->IconWidth;
            pQ->IconSpaceX    =  pQ->IconSpaceX & ~7;

            Tmp                 =  pQ->IconSpaceX * pQ->NoOfIcons - (pQ->IconSpaceX - pQ->IconWidth);

            pQ->IconStartX    =  pQ->ScreenStartX + ((pQ->ScreenWidth - Tmp) / 2);
            pQ->IconStartX    =  (pQ->IconStartX + 7) & ~7;

            pQ->SelectStartX  =  pQ->IconStartX - 1;
            pQ->SelectStartY  =  pQ->IconStartY - 1;
            pQ->SelectWidth   =  pQ->IconWidth + 2;
            pQ->SelectHeight  =  pQ->IconHeight + 2;
            pQ->SelectSpaceX  =  pQ->IconSpaceX;
        }
#ifdef DEBUG
        printf("Shown icons %d -> 0x%08X\n",pQ->NoOfIcons,pQ->Icons);
#endif

        pQ->NeedUpdate    =  1;
    }

    if (pQ->NoOfIcons)
    {
        // Check for move pointer
        Tmp  =  cUiButtonGetHorz();
        if (Tmp)
        {
            pQ->PointerX +=  Tmp;

            pQ->NeedUpdate    =  1;

            if (pQ->PointerX < 0)
            {
                pQ->PointerX    =  0;
                pQ->NeedUpdate  =  0;
            }
            if (pQ->PointerX >= pQ->NoOfIcons)
            {
                pQ->PointerX    =  pQ->NoOfIcons - 1;
                pQ->NeedUpdate  =  0;
            }
        }
    }

    if (pQ->NeedUpdate)
    {
//* UPDATE ***************************************************************************************************
        pQ->NeedUpdate    =  0;

        dLcdDrawPicture(UiInstance.pLcd->Lcd,Color,pQ->ScreenStartX,pQ->ScreenStartY,POP2_width,POP2_height,(UBYTE*)POP2_bits);
        pQ->ScreenWidth   =  POP2_width;
        pQ->ScreenHeight  =  POP2_height;

        // Show icons
        Loop  =  0;
        Icon  =  0;
        TmpIcons   =  pQ->Icons;
        while (Loop < pQ->NoOfIcons)
        {
            while (!(TmpIcons & 1))
            {
                Icon++;
                TmpIcons >>=  1;
            }
            dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pQ->IconStartX + pQ->IconSpaceX * Loop,pQ->IconStartY,LARGE_ICON,Icon);
            Loop++;
            Icon++;
            TmpIcons >>=  1;
        }

        // Show selection
        dLcdInverseRect(UiInstance.pLcd->Lcd,pQ->SelectStartX + pQ->SelectSpaceX * pQ->PointerX,pQ->SelectStartY,pQ->SelectWidth,pQ->SelectHeight);

        // Update screen
        cUiUpdateLcd();
        UiInstance.ScreenBusy       =  0;
    }
    if (cUiButtonGetShortPress(ENTER_BUTTON))
    {
        if (pQ->NoOfIcons)
        {
            Mask  =  0x00000001;
            TmpIcons   =  pQ->Icons;
            Loop  =  pQ->PointerX + 1;

            do
            {
                if (TmpIcons & Mask)
                {
                    Loop--;
                }
                Mask <<=  1;
            }
            while (Loop && Mask);
            Mask >>=  1;
            *pIcons  =  Mask;
        }
        else
        {
            *pIcons  =  0;
        }
        cUiButtonClearAll();
        Result  =  OK;
        *pState =  0;
#ifdef DEBUG
        printf("Selecting icon %d -> 0x%08X\n",pQ->PointerX,*pIcons);
#endif
    }
    if (cUiButtonGetShortPress(BACK_BUTTON))
    {
        *pIcons  =  0;
        cUiButtonClearAll();
        Result  =  OK;
        *pState =  0;
    }

    return (Result);
}
