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
#include "c_ui.h"
#include "d_lcd.h"

#include <math.h>

void cUiGraphSetup(DATA16 StartX, DATA16 SizeX, DATA8 Items, DATA16 *pOffset,
                   DATA16 *pSpan, DATAF *pMin, DATAF *pMax, DATAF *pVal)
{
    DATA16  Item;
    DATA16  Pointer;

    UiInstance.Graph.Initialized  =  0;

    UiInstance.Graph.pOffset  =  pOffset;
    UiInstance.Graph.pSpan    =  pSpan;
    UiInstance.Graph.pMin     =  pMin;
    UiInstance.Graph.pMax     =  pMax;
    UiInstance.Graph.pVal     =  pVal;

    if (Items < 0)
    {
        Items  =  0;
    }
    if (Items > GRAPH_BUFFERS)
    {
        Items  =  GRAPH_BUFFERS;
    }


    UiInstance.Graph.GraphStartX  =  StartX;
    UiInstance.Graph.GraphSizeX   =  SizeX;
    UiInstance.Graph.Items        =  Items;
    UiInstance.Graph.Pointer      =  0;

    for (Item = 0;Item < UiInstance.Graph.Items;Item++)
    {
        for (Pointer = 0;Pointer < UiInstance.Graph.GraphSizeX;Pointer++)
        {
            UiInstance.Graph.Buffer[Item][Pointer]  =  DATAF_NAN;
        }
    }

    UiInstance.Graph.Initialized  =  1;

    // Simulate graph
    UiInstance.Graph.Value        =  UiInstance.Graph.pMin[0];
    UiInstance.Graph.Down         =  0;
    UiInstance.Graph.Inc          =  (UiInstance.Graph.pMax[0] - UiInstance.Graph.pMin[0]) / (DATAF)20;
}


void cUiGraphSample(void)
{
    DATAF   Sample;
    DATA16  Item;
    DATA16  Pointer;

    if (UiInstance.Graph.Initialized)
    { // Only if initialized

        if (UiInstance.Graph.Pointer < UiInstance.Graph.GraphSizeX)
        {
            for (Item = 0;Item < (UiInstance.Graph.Items);Item++)
            {
                // Insert sample
                Sample      =  UiInstance.Graph.pVal[Item];

                if (!(isnan(Sample)))
                {
                    UiInstance.Graph.Buffer[Item][UiInstance.Graph.Pointer]  =  Sample;
                }
                else
                {
                    UiInstance.Graph.Buffer[Item][UiInstance.Graph.Pointer]  =  DATAF_NAN;
                }
            }
            UiInstance.Graph.Pointer++;
        }
        else
        {
            // Scroll buffers
            for (Item = 0;Item < (UiInstance.Graph.Items);Item++)
            {
                for (Pointer = 0;Pointer < (UiInstance.Graph.GraphSizeX - 1);Pointer++)
                {
                    UiInstance.Graph.Buffer[Item][Pointer]  =  UiInstance.Graph.Buffer[Item][Pointer + 1];
                }

                // Insert sample
                Sample      =  UiInstance.Graph.pVal[Item];

                if (!(isnan(Sample)))
                {
                    UiInstance.Graph.Buffer[Item][Pointer]  =  Sample;
                }
                else
                {
                    UiInstance.Graph.Buffer[Item][Pointer]  =  DATAF_NAN;
                }
            }
        }
    }
}


void cUiGraphDraw(DATA8 View, DATAF *pActual, DATAF *pLowest, DATAF *pHighest,
                  DATAF *pAverage)
{
    DATAF   Sample;
    DATA8   Samples;
    DATA16  Value;
    DATA16  Item;
    DATA16  Pointer;
    DATA16  X;
    DATA16  Y1;
    DATA16  Y2;
    DATA8   Color = 1;

    *pActual    =  DATAF_NAN;
    *pLowest    =  DATAF_NAN;
    *pHighest   =  DATAF_NAN;
    *pAverage   =  DATAF_NAN;
    Samples     =  0;

    if (UiInstance.Graph.Initialized)
    { // Only if initialized

        if (UiInstance.ScreenBlocked == 0)
        {

            // View or all
            if ((View >= 0) && (View < UiInstance.Graph.Items))
            {
                Item  =  View;

                Y1  =  (UiInstance.Graph.pOffset[Item] + UiInstance.Graph.pSpan[Item]);

                // Draw buffers
                X  =  UiInstance.Graph.GraphStartX;
                for (Pointer = 0;Pointer < UiInstance.Graph.Pointer;Pointer++)
                {
                    Sample  =  UiInstance.Graph.Buffer[Item][Pointer];
                    if (!(isnan(Sample)))
                    {
                        *pActual      =  Sample;
                        if (isnan(*pAverage))
                        {
                            *pAverage   =  (DATAF)0;
                            *pLowest    =  *pActual;
                            *pHighest   =  *pActual;
                        }
                        else
                        {
                            if (*pActual < *pLowest)
                            {
                                *pLowest  =  *pActual;
                            }
                            if (*pActual > *pHighest)
                            {
                                *pHighest =  *pActual;
                            }
                        }
                        *pAverage    +=  *pActual;
                        Samples++;

                        // Scale Y axis
                        Value       =  (DATA16)((((Sample - UiInstance.Graph.pMin[Item]) * (DATAF)UiInstance.Graph.pSpan[Item]) / (UiInstance.Graph.pMax[Item] - UiInstance.Graph.pMin[Item])));

                        // Limit Y axis
                        if (Value > UiInstance.Graph.pSpan[Item])
                        {
                            Value  =  UiInstance.Graph.pSpan[Item];
                        }
                        if (Value < 0)
                        {
                            Value  =  0;
                        }
/*
                        printf("S=%-3d V=%3.0f L=%3.0f H=%3.0f A=%3.0f v=%3.0f ^=%3.0f O=%3d S=%3d Y=%d\n",Samples,*pActual,*pLowest,*pHighest,*pAverage,UiInstance.Graph.pMin[Item],UiInstance.Graph.pMax[Item],UiInstance.Graph.pOffset[Item],UiInstance.Graph.pSpan[Item],Value);
*/
                        Y2  =  (UiInstance.Graph.pOffset[Item] + UiInstance.Graph.pSpan[Item]) - Value;
                        if (Pointer > 1)
                        {
                            if (Y2 > Y1)
                            {
                                dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X - 2,Y1 - 1,X - 1,Y2 - 1);
                                dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X,Y1 + 1,X + 1,Y2 + 1);
                            }
                            else
                            {
                                if (Y2 < Y1)
                                {
                                    dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X,Y1 - 1,X + 1,Y2 - 1);
                                    dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X - 2,Y1 + 1,X - 1,Y2 + 1);
                                }
                                else
                                {
                                    dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X - 1,Y1 - 1,X,Y2 - 1);
                                    dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X - 1,Y1 + 1,X,Y2 + 1);
                                }
                            }
                            dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X - 1,Y1,X,Y2);
                        }
                        else
                        {
                            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X,Y2);
                        }

                        Y1  =  Y2;
                    }
                    X++;
                }
                if (Samples != 0)
                {
                    *pAverage  =  *pAverage / (DATAF)Samples;
                }

            }
            else
            {
                // Draw buffers
                for (Item = 0;Item < UiInstance.Graph.Items;Item++)
                {
                    Y1  =  (UiInstance.Graph.pOffset[Item] + UiInstance.Graph.pSpan[Item]);

                    X  =  UiInstance.Graph.GraphStartX + 1;
                    for (Pointer = 0;Pointer < UiInstance.Graph.Pointer;Pointer++)
                    {
                        Sample  =  UiInstance.Graph.Buffer[Item][Pointer];

                        // Scale Y axis
                        Value       =  (DATA16)((((Sample - UiInstance.Graph.pMin[Item]) * (DATAF)UiInstance.Graph.pSpan[Item]) / (UiInstance.Graph.pMax[Item] - UiInstance.Graph.pMin[Item])));

                        // Limit Y axis
                        if (Value > UiInstance.Graph.pSpan[Item])
                        {
                            Value  =  UiInstance.Graph.pSpan[Item];
                        }
                        if (Value < 0)
                        {
                            Value  =  0;
                        }
                        Y2  =  (UiInstance.Graph.pOffset[Item] + UiInstance.Graph.pSpan[Item]) - Value;
                        if (Pointer > 1)
                        {

                            if (Y2 > Y1)
                            {
                                dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X - 2,Y1 - 1,X - 1,Y2 - 1);
                                dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X,Y1 + 1,X + 1,Y2 + 1);
                            }
                            else
                            {
                                if (Y2 < Y1)
                                {
                                    dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X,Y1 - 1,X + 1,Y2 - 1);
                                    dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X - 2,Y1 + 1,X - 1,Y2 + 1);
                                }
                                else
                                {
                                    dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X - 1,Y1 - 1,X,Y2 - 1);
                                    dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X - 1,Y1 + 1,X,Y2 + 1);
                                }
                            }
                            dLcdDrawLine(UiInstance.pLcd->Lcd,Color,X - 1,Y1,X,Y2);

                        }
                        else
                        {
                            dLcdDrawPixel(UiInstance.pLcd->Lcd,Color,X,Y2);
                        }

                        Y1  =  Y2;
                        X++;
                    }
                }
            }
            UiInstance.ScreenBusy  =  1;
        }
    }
}
