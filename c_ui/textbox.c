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
#include "draw.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static const char Delimiter[][3] =
{
    [DEL_NONE]      = "",
    [DEL_TAB]       = "\t",
    [DEL_SPACE]     = " ",
    [DEL_RETURN]    = "\r",
    [DEL_COLON]     = ":",
    [DEL_COMMA]     = ",",
    [DEL_LINEFEED]  = "\n",
    [DEL_CRLF]      = "\r\n",
};

static DATA16 cUiTextboxGetLines(DATA8 *pText, DATA32 Size, DATA8 Del)
{
    DATA32  Point = 0;
    DATA16  Lines = 0;
    DATA8   DelPoi;

    if (Del < DELS)
    {
        while (pText[Point] && (Point < Size))
        {
            DelPoi  =  0;
            while ((pText[Point]) && (Point < Size) && (Delimiter[Del][DelPoi]) && (pText[Point] == Delimiter[Del][DelPoi]))
            {
                DelPoi++;
                Point++;
            }
            if (Delimiter[Del][DelPoi] == 0)
            {
                Lines++;
            }
            else
            {
                if ((pText[Point]) && (Point < Size))
                {
                    Point++;
                }
            }
        }
    }

    return (Lines);
}

void cUiTextboxAppendLine(DATA8 *pText, DATA32 Size, DATA8 Del, DATA8 *pLine,
                          DATA8 Font)
{
    DATA32  Point = 0;
    DATA8   DelPoi = 0;

    if (Del < DELS)
    {
        while ((pText[Point]) && (Point < Size))
        {
            Point++;
        }
        if ((Point < Size) && (Font))
        {
            pText[Point]  =  Font;
            Point++;
        }

        while ((Point < Size) && (*pLine))
        {
            pText[Point]  =  *pLine;
            Point++;
            pLine++;
        }
        while ((Point < Size) && (Delimiter[Del][DelPoi]))
        {
            pText[Point]  =  Delimiter[Del][DelPoi];
            Point++;
            DelPoi++;
        }
    }
}

static DATA32 cUiTextboxFindLine(DATA8 *pText, DATA32 Size, DATA8 Del,
                                 DATA16 Line, DATA8 *pFont)
{
    DATA32  Result = -1;
    DATA32  Point = 0;
    DATA8   DelPoi = 0;

    *pFont  =  0;
    if (Del < DELS)
    {
        Result  =  Point;
        while ((Line) && (pText[Point]) && (Point < Size))
        {

            DelPoi  =  0;
            while ((pText[Point]) && (Point < Size) && (Delimiter[Del][DelPoi]) && (pText[Point] == Delimiter[Del][DelPoi]))
            {
                DelPoi++;
                Point++;
            }
            if (Delimiter[Del][DelPoi] == 0)
            {
                Line--;
                if (Line)
                {
                    Result  =  Point;
                }
            }
            else
            {
                if ((pText[Point]) && (Point < Size))
                {
                    Point++;
                }
            }
        }
        if (Line != 0)
        {
            Result  = -1;
        }
        if (Result >= 0)
        {
            if ((pText[Result] > 0) && (pText[Result] < FONTTYPES))
            {
                *pFont  =  pText[Result];
                Result++;
            }
        }
    }

    return (Result);
}

void cUiTextboxReadLine(DATA8 *pText, DATA32 Size, DATA8 Del, DATA8 Lng,
                        DATA16 Line, DATA8 *pLine, DATA8 *pFont)
{
    DATA32  Start;
    DATA32  Point = 0;
    DATA8   DelPoi = 0;
    DATA8   Run = 1;

    Start  =  cUiTextboxFindLine(pText,Size,Del,Line,pFont);
    Point  =  Start;

    pLine[0]  =  0;

    if (Point >= 0)
    {
        while ((Run) && (pText[Point]) && (Point < Size))
        {
            DelPoi  =  0;
            while ((pText[Point]) && (Point < Size) && (Delimiter[Del][DelPoi]) && (pText[Point] == Delimiter[Del][DelPoi]))
            {
                DelPoi++;
                Point++;
            }
            if (Delimiter[Del][DelPoi] == 0)
            {
                Run  =  0;
            }
            else
            {
                if ((pText[Point]) && (Point < Size))
                {
                    Point++;
                }
            }
        }
        Point -= (DATA32)DelPoi;

        if (((Point - Start) + 1) < (DATA32)Lng)
        {
            Lng  =  (DATA8)(Point - Start) + 1;
        }
        snprintf((char*)pLine,Lng,"%s",(char*)&pText[Start]);
    }
}

RESULT cUiTextbox(DATA16 X, DATA16 Y, DATA16 X1, DATA16 Y1, DATA8 *pText,
                  DATA32 Size, DATA8 Del, DATA16 *pLine)
{
    RESULT  Result = BUSY;
    TXTBOX  *pB;
    DATA16  Item;
    DATA16  TotalItems;
    DATA16  Tmp;
    DATA16  Ypos;
    DATA8   Color;

    pB      =  &UiInstance.Txtbox;
    Color   =  FG_COLOR;

    if (*pLine < 0)
    {
//* INIT ***********************************************************************
        // Define screen
        pB->ScreenStartX  =  X;
        pB->ScreenStartY  =  Y;
        pB->ScreenWidth   =  X1;
        pB->ScreenHeight  =  Y1;

        pB->Font          =  UiInstance.Font;

        // calculate chars and lines on screen
        pB->CharWidth     =  dLcdGetFontWidth(pB->Font);
        pB->CharHeight    =  dLcdGetFontHeight(pB->Font);
        pB->Chars         =  (pB->ScreenWidth / pB->CharWidth);

        // calculate lines on screen
        pB->LineSpace     =  5;
        pB->LineHeight    =  pB->CharHeight + pB->LineSpace;
        pB->Lines         =  (pB->ScreenHeight / pB->LineHeight);

        // calculate start of text
        pB->TextStartX    =  cUiAlignX(pB->ScreenStartX);
        pB->TextStartY    =  pB->ScreenStartY + (pB->LineHeight - pB->CharHeight) / 2;

        // Calculate selection barBrowser
        pB->SelectStartX  =  pB->ScreenStartX;
        pB->SelectWidth   =  pB->ScreenWidth - (pB->CharWidth + 5);
        pB->SelectStartY  =  pB->TextStartY - 1;
        pB->SelectHeight  =  pB->CharHeight + 1;

        // Calculate scroll bar
        pB->ScrollWidth   =  5;
        pB->NobHeight     =  7;
        pB->ScrollStartX  =  pB->ScreenStartX + pB->ScreenWidth - pB->ScrollWidth;
        pB->ScrollStartY  =  pB->ScreenStartY + 1;
        pB->ScrollHeight  =  pB->ScreenHeight - 2;
        pB->ScrollSpan    =  pB->ScrollHeight - pB->NobHeight;

        pB->Items         =  cUiTextboxGetLines(pText,Size,Del);
        pB->ItemStart     =  1;
        pB->ItemPointer   =  1;

        pB->NeedUpdate    =  1;

        *pLine  =  0;
    }

    TotalItems  =  pB->Items;

    Tmp  =  cUiButtonGetVert();
    if (Tmp != 0)
    { // up/down arrow pressed

        pB->NeedUpdate    =  1;

        // Calculate item pointer
        pB->ItemPointer    +=  Tmp;
        if (pB->ItemPointer < 1)
        {
            pB->ItemPointer   =  1;
            pB->NeedUpdate    =  0;
        }
        if (pB->ItemPointer > TotalItems)
        {
            pB->ItemPointer   =  TotalItems;
            pB->NeedUpdate    =  0;
        }
    }

    // Calculate item start
    if (pB->ItemPointer < pB->ItemStart)
    {
        if (pB->ItemPointer > 0)
        {
            pB->ItemStart  =  pB->ItemPointer;
        }
    }
    if (pB->ItemPointer >= (pB->ItemStart + pB->Lines))
    {
        pB->ItemStart  =  pB->ItemPointer - pB->Lines;
        pB->ItemStart++;
    }

    if (cUiButtonGetShortPress(ENTER_BUTTON))
    {
        *pLine  =  pB->ItemPointer;

        Result  =  OK;
    }
    if (cUiButtonGetShortPress(BACK_BUTTON))
    {
        *pLine  =  -1;

        Result  =  OK;
    }


    if (pB->NeedUpdate)
    {
//* UPDATE *********************************************************************
        pB->NeedUpdate    =  0;

        // clear screen
        dLcdFillRect(UiInstance.pLcd->Lcd,BG_COLOR,pB->ScreenStartX,pB->ScreenStartY,pB->ScreenWidth,pB->ScreenHeight);

        Ypos  =  pB->TextStartY + 2;

        for (Tmp = 0;Tmp < pB->Lines;Tmp++)
        {
            Item          =  Tmp + pB->ItemStart;

            if (Item <= TotalItems)
            {
                cUiTextboxReadLine(pText,Size,Del,TEXTSIZE,Item,pB->Text,&pB->Font);

                // calculate chars and lines on screen
                pB->CharWidth     =  dLcdGetFontWidth(pB->Font);
                pB->CharHeight    =  dLcdGetFontHeight(pB->Font);

                // calculate lines on screen
                pB->LineSpace     =  2;
                pB->LineHeight    =  pB->CharHeight + pB->LineSpace;
                pB->Lines         =  (pB->ScreenHeight / pB->LineHeight);

                // Calculate selection barBrowser
                pB->SelectStartX  =  pB->ScreenStartX;
                pB->SelectWidth   =  pB->ScreenWidth - (pB->ScrollWidth + 2);
                pB->SelectStartY  =  pB->TextStartY - 1;
                pB->SelectHeight  =  pB->CharHeight + 1;

                pB->Chars         =  (pB->SelectWidth / pB->CharWidth);

                pB->Text[pB->Chars]  =  0;

                if ((Ypos + pB->LineHeight) <= (pB->ScreenStartY + pB->ScreenHeight))
                {
                    dLcdDrawText(UiInstance.pLcd->Lcd,Color,pB->TextStartX,Ypos,pB->Font,pB->Text);
                }
                else
                {
                    Tmp  =  pB->Lines;
                }
            }

            cUiDrawBar(1,pB->ScrollStartX,pB->ScrollStartY,pB->ScrollWidth,pB->ScrollHeight,0,TotalItems,pB->ItemPointer);

            if ((Ypos + pB->LineHeight) <= (pB->ScreenStartY + pB->ScreenHeight))
            {
                // Draw selection
                if (pB->ItemPointer == (Tmp + pB->ItemStart))
                {
                    dLcdInverseRect(UiInstance.pLcd->Lcd,pB->SelectStartX,Ypos - 1,pB->SelectWidth,pB->LineHeight);
                }
            }
            Ypos +=  pB->LineHeight;
        }

        // Update
        cUiUpdateLcd();
        UiInstance.ScreenBusy       =  0;
    }

    return(Result);
}
