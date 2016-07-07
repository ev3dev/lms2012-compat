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
#include "c_memory.h"
#include "c_ui.h"
#include "d_lcd.h"
#include "draw.h"

#include <stdio.h>
#include <string.h>

static const DATA8 FiletypeToNormalIcon[FILETYPES] = {
    [FILETYPE_UNKNOWN]  =  ICON_FOLDER,
    [TYPE_FOLDER]       =  ICON_FOLDER,
    [TYPE_SOUND]        =  ICON_SOUND,
    [TYPE_BYTECODE]     =  ICON_RUN,
    [TYPE_GRAPHICS]     =  ICON_IMAGE,
    [TYPE_DATALOG]      =  ICON_OBD,
    [TYPE_PROGRAM]      =  ICON_OBP,
    [TYPE_TEXT]         =  ICON_TEXT
};

#include "PCApp.xbm"

RESULT cUiBrowser(DATA8 Type, DATA16 X, DATA16 Y, DATA16 X1, DATA16 Y1,
                  DATA8 Lng, DATA8 *pType, DATA8 *pAnswer)
{
    RESULT  Result = BUSY;
    DATA32  Image;
    BROWSER *pB;
    PRGID   PrgId;
    OBJID   ObjId;
    DATA16  Tmp;
    DATA16  Indent;
    DATA16  Item;
    DATA16  TotalItems;
    DATA8   TmpType;
    DATA8   Folder;
    DATA8   OldPriority;
    DATA8   Priority;
    DATA8   Color;
    DATA16  Ignore;
    DATA8   Data8;
    DATA32  Total;
    DATA32  Free;
    RESULT  TmpResult;
    HANDLER TmpHandle;

#ifdef ALLOW_DEBUG_PULSE
    VMInstance.Pulse |=  vmPULSE_BROWSER;
#endif

    PrgId   =  CurrentProgramId();
    ObjId   =  CallingObjectId();
    pB      =  &UiInstance.Browser;

    Color   =  FG_COLOR;

    // Test ignore horizontal update
    if ((Type & 0x20))
    {
        Ignore  =  -1;
    }
    else
    {
        if ((Type & 0x10))
        {
            Ignore  =  1;
        }
        else
        {
            Ignore  =  0;
        }
    }

    // Isolate browser type
    Type   &=  0x0F;

    CheckUsbstick(&Data8,&Total,&Free,0);
    if (Data8)
    {
        UiInstance.UiUpdate   =  1;
    }
    CheckSdcard(&Data8,&Total,&Free,0);
    if (Data8)
    {
        UiInstance.UiUpdate   =  1;
    }

    if (ProgramStatusChange(USER_SLOT) == STOPPED)
    {
        if (Type != BROWSE_FILES)
        {
            Result              =  OK;
            *pType              =  0;
#ifdef DEBUG
            printf("Browser interrupted\n");
#endif
        }
    }

    if ((*pType == TYPE_REFRESH_BROWSER))
    {
        UiInstance.UiUpdate   =  1;
    }

    if ((*pType == TYPE_RESTART_BROWSER))
    {
        if (pB->hFiles)
        {
            cMemoryCloseFolder(pB->PrgId,&pB->hFiles);
        }
        if (pB->hFolders)
        {
            cMemoryCloseFolder(pB->PrgId,&pB->hFolders);
        }
        pB->PrgId         =  0;
        pB->ObjId         =  0;
//    pAnswer[0]          =  0;
        *pType              =  0;
#ifdef DEBUG
        printf("Restarting browser\n");
#endif
    }

    if ((pB->PrgId == 0) && (pB->ObjId == 0))
    {
//* INIT *****************************************************************************************************

        // Define screen
        pB->ScreenStartX  =  X;
        pB->ScreenStartY  =  Y;
        pB->ScreenWidth   =  X1;
        pB->ScreenHeight  =  Y1;

        // calculate lines on screen
        pB->LineSpace     =  5;
        pB->IconHeight    =  dLcdGetIconHeight(NORMAL_ICON);
        pB->LineHeight    =  pB->IconHeight + pB->LineSpace;
        pB->Lines         =  (pB->ScreenHeight / pB->LineHeight);

        // calculate chars and lines on screen
        pB->CharWidth     =  dLcdGetFontWidth(NORMAL_FONT);
        pB->CharHeight    =  dLcdGetFontHeight(NORMAL_FONT);
        pB->IconWidth     =  dLcdGetIconWidth(NORMAL_ICON);
        pB->Chars         =  ((pB->ScreenWidth - pB->IconWidth) / pB->CharWidth);

        // calculate start of icon
        pB->IconStartX    =  cUiAlignX(pB->ScreenStartX);
        pB->IconStartY    =  pB->ScreenStartY + pB->LineSpace / 2;

        // calculate start of text
        pB->TextStartX    =  cUiAlignX(pB->ScreenStartX + pB->IconWidth);
        pB->TextStartY    =  pB->ScreenStartY + (pB->LineHeight - pB->CharHeight) / 2;

        // Calculate selection barBrowser
        pB->SelectStartX  =  pB->ScreenStartX + 1;
        pB->SelectWidth   =  pB->ScreenWidth - (pB->CharWidth + 5);
        pB->SelectStartY  =  pB->IconStartY - 1;
        pB->SelectHeight  =  pB->IconHeight + 2;

        // Calculate scroll bar
        pB->ScrollWidth   =  6;
        pB->NobHeight     =  9;
        pB->ScrollStartX  =  pB->ScreenStartX + pB->ScreenWidth - pB->ScrollWidth;
        pB->ScrollStartY  =  pB->ScreenStartY + 1;
        pB->ScrollHeight  =  pB->ScreenHeight - 2;
        pB->ScrollSpan    =  pB->ScrollHeight - pB->NobHeight;

        strncpy((char*)pB->TopFolder,(char*)pAnswer,MAX_FILENAME_SIZE);

        pB->PrgId         =  PrgId;
        pB->ObjId         =  ObjId;

        pB->OldFiles        =  0;
        pB->Folders         =  0;
        pB->OpenFolder      =  0;
        pB->Files           =  0;
        pB->ItemStart       =  1;
        pB->ItemPointer     =  1;
        pB->NeedUpdate      =  1;
        UiInstance.UiUpdate   =  1;
    }

    if ((pB->PrgId == PrgId) && (pB->ObjId == ObjId))
    {
//* CTRL *****************************************************************************************************


        if (UiInstance.UiUpdate)
        {
            UiInstance.UiUpdate  =  0;
#ifdef DEBUG
        printf("Refreshing browser\n");
#endif

            if (pB->hFiles)
            {
                cMemoryCloseFolder(pB->PrgId,&pB->hFiles);
            }
            if (pB->hFolders)
            {
                cMemoryCloseFolder(pB->PrgId,&pB->hFolders);
            }

            pB->OpenFolder    =  0;
            pB->Files         =  0;
            *pType              =  0;

            switch (Type)
            {
                case BROWSE_FOLDERS :
                case BROWSE_FOLDS_FILES :
                {
                    if (cMemoryOpenFolder(PrgId,TYPE_FOLDER,pB->TopFolder,&pB->hFolders) == OK)
                    {
            #ifdef DEBUG
                        printf("\n%d %d Opening browser in %s\n",PrgId,ObjId,(char*)pB->TopFolder);
            #endif
//******************************************************************************************************
                        if (pB->OpenFolder)
                        {
                            cMemoryGetItem(pB->PrgId,pB->hFolders,pB->OpenFolder,FOLDERNAME_SIZE + SUBFOLDERNAME_SIZE,pB->SubFolder,&TmpType);
    #ifdef DEBUG
                            printf("Open  folder %3d (%s)\n",pB->OpenFolder,pB->SubFolder);
    #endif
                            if (strcmp((char*)pB->SubFolder,SDCARD_FOLDER) == 0)
                            {
                                Item    =  pB->ItemPointer;
                                cMemoryGetItemName(pB->PrgId,pB->hFolders,Item,MAX_FILENAME_SIZE,pB->Filename,pType,&Priority);
                                Result  =  cMemoryGetItem(pB->PrgId,pB->hFolders,Item,FOLDERNAME_SIZE + SUBFOLDERNAME_SIZE,pB->FullPath,pType);
                                *pType  =  TYPE_SDCARD;

                                snprintf((char*)pAnswer,Lng,"%s",(char*)pB->FullPath);
                            }
                            else
                            {
                                if (strcmp((char*)pB->SubFolder,USBSTICK_FOLDER) == 0)
                                {
                                    Item    =  pB->ItemPointer;
                                    cMemoryGetItemName(pB->PrgId,pB->hFolders,Item,MAX_FILENAME_SIZE,pB->Filename,pType,&Priority);
                                    Result  =  cMemoryGetItem(pB->PrgId,pB->hFolders,Item,FOLDERNAME_SIZE + SUBFOLDERNAME_SIZE,pB->FullPath,pType);
                                    *pType  =  TYPE_USBSTICK;

                                    snprintf((char*)pAnswer,Lng,"%s",(char*)pB->FullPath);
                                }
                                else
                                {
                                    Result  =  cMemoryOpenFolder(PrgId,FILETYPE_UNKNOWN,pB->SubFolder,&pB->hFiles);
                                    Result  =  BUSY;
                                }
                            }
                        }
//******************************************************************************************************
                    }
                    else
                    {
            #ifdef DEBUG
                        printf("\n%d %d Open error\n",PrgId,ObjId);
            #endif
                        pB->PrgId         =  0;
                        pB->ObjId         =  0;
                    }
                }
                break;

                case BROWSE_CACHE:
                {
                }
                break;

                case BROWSE_FILES :
                {
                    if (cMemoryOpenFolder(PrgId,FILETYPE_UNKNOWN,pB->TopFolder,&pB->hFiles) == OK)
                    {
            #ifdef DEBUG
                        printf("\n%d %d Opening browser in %s\n",PrgId,ObjId,(char*)pB->TopFolder);
            #endif

                    }
                    else
                    {
            #ifdef DEBUG
                        printf("\n%d %d Open error\n",PrgId,ObjId);
            #endif
                        pB->PrgId         =  0;
                        pB->ObjId         =  0;
                    }
                }
                break;

            }
        }

#ifndef DISABLE_SDCARD_SUPPORT
        if (strstr((char*)pB->SubFolder,SDCARD_FOLDER) != NULL)
        {
            pB->Sdcard  =  1;
        }
        else
        {
            pB->Sdcard  =  0;
        }
#endif

#ifndef DISABLE_USBSTICK_SUPPORT
        if (strstr((char*)pB->SubFolder,USBSTICK_FOLDER) != NULL)
        {
            pB->Usbstick  =  1;
        }
        else
        {
            pB->Usbstick  =  0;
        }
#endif

        TmpResult  =  OK;
        switch (Type)
        {
            case BROWSE_FOLDERS :
            case BROWSE_FOLDS_FILES :
            {
                // Collect folders in directory
                TmpResult  =  cMemoryGetFolderItems(pB->PrgId,pB->hFolders,&pB->Folders);

                // Collect files in folder
                if ((pB->OpenFolder) && (TmpResult == OK))
                {
                    TmpResult  =  cMemoryGetFolderItems(pB->PrgId,pB->hFiles,&pB->Files);
                }
            }
            break;

            case BROWSE_CACHE :
            {
                pB->Folders   =  (DATA16)cMemoryGetCacheFiles();
            }
            break;

            case BROWSE_FILES :
            {
                TmpResult  =  cMemoryGetFolderItems(pB->PrgId,pB->hFiles,&pB->Files);
            }
            break;

        }

        if ((pB->OpenFolder != 0) && (pB->OpenFolder == pB->ItemPointer))
        {
            if (cUiButtonGetShortPress(BACK_BUTTON))
            {
                // Close folder
    #ifdef DEBUG
                printf("Close folder %3d\n",pB->OpenFolder);
    #endif

                cMemoryCloseFolder(pB->PrgId,&pB->hFiles);
                if (pB->ItemPointer > pB->OpenFolder)
                {
                    pB->ItemPointer -=  pB->Files;
                }
                pB->OpenFolder  =  0;
                pB->Files       =  0;
            }
        }

#ifndef DISABLE_SDCARD_SUPPORT
        if (pB->Sdcard == 1)
        {
            if (pB->OpenFolder == 0)
            {
                if (cUiButtonGetShortPress(BACK_BUTTON))
                {
                    // Collapse sdcard
        #ifdef DEBUG
                    printf("Collapse sdcard\n");
        #endif
                    if (pB->hFiles)
                    {
                        cMemoryCloseFolder(pB->PrgId,&pB->hFiles);
                    }
                    if (pB->hFolders)
                    {
                        cMemoryCloseFolder(pB->PrgId,&pB->hFolders);
                    }
                    pB->PrgId         =  0;
                    pB->ObjId         =  0;
                    strcpy((char*)pAnswer,vmPRJS_DIR);
                    *pType              =  0;
                    pB->SubFolder[0]  =  0;
                }
            }
        }
#endif
#ifndef DISABLE_USBSTICK_SUPPORT
        if (pB->Usbstick == 1)
        {
            if (pB->OpenFolder == 0)
            {
                if (cUiButtonGetShortPress(BACK_BUTTON))
                {
                    // Collapse usbstick
        #ifdef DEBUG
                    printf("Collapse usbstick\n");
        #endif
                    if (pB->hFiles)
                    {
                        cMemoryCloseFolder(pB->PrgId,&pB->hFiles);
                    }
                    if (pB->hFolders)
                    {
                        cMemoryCloseFolder(pB->PrgId,&pB->hFolders);
                    }
                    pB->PrgId         =  0;
                    pB->ObjId         =  0;
                    strcpy((char*)pAnswer,vmPRJS_DIR);
                    *pType              =  0;
                    pB->SubFolder[0]  =  0;
                }
            }
        }
#endif
        if (pB->OldFiles != (pB->Files + pB->Folders))
        {
            pB->OldFiles    =  (pB->Files + pB->Folders);
            pB->NeedUpdate  =  1;
        }

        if (cUiButtonGetShortPress(ENTER_BUTTON))
        {
            pB->OldFiles      =  0;
            if (pB->OpenFolder)
            {
                if ((pB->ItemPointer > pB->OpenFolder) && (pB->ItemPointer <= (pB->OpenFolder + pB->Files)))
                { // File selected

                    Item    =  pB->ItemPointer - pB->OpenFolder;
                    Result  =  cMemoryGetItem(pB->PrgId,pB->hFiles,Item,Lng,pB->FullPath,pType);

                    snprintf((char*)pAnswer,Lng,"%s",(char*)pB->FullPath);


#ifdef DEBUG
                    printf("Select file %3d\n",Item);
#endif
                }
                else
                { // Folder selected

                    if (pB->OpenFolder == pB->ItemPointer)
                    { // Enter on open folder

                        Item    =  pB->OpenFolder;
                        Result  =  cMemoryGetItem(pB->PrgId,pB->hFolders,Item,Lng,pAnswer,pType);
#ifdef DEBUG
                        printf("Select folder %3d\n",Item);
#endif
                    }
                    else
                    { // Close folder

#ifdef DEBUG
                        printf("Close folder %3d\n",pB->OpenFolder);
#endif

                        cMemoryCloseFolder(pB->PrgId,&pB->hFiles);
                        if (pB->ItemPointer > pB->OpenFolder)
                        {
                            pB->ItemPointer -=  pB->Files;
                        }
                        pB->OpenFolder  =  0;
                        pB->Files       =  0;
                    }
                }
            }
            if (pB->OpenFolder == 0)
            { // Open folder

                switch (Type)
                {
                    case BROWSE_FOLDERS :
                    { // Folder

                        Item    =  pB->ItemPointer;
                        cMemoryGetItemName(pB->PrgId,pB->hFolders,Item,MAX_FILENAME_SIZE,pB->Filename,pType,&Priority);
                        Result  =  cMemoryGetItem(pB->PrgId,pB->hFolders,Item,FOLDERNAME_SIZE + SUBFOLDERNAME_SIZE,pB->FullPath,pType);

                        snprintf((char*)pAnswer,Lng,"%s/%s",(char*)pB->FullPath,(char*)pB->Filename);
                        *pType  =  TYPE_BYTECODE;
#ifdef DEBUG
                        printf("Select folder %3d\n",Item);
#endif
                    }
                    break;

                    case BROWSE_FOLDS_FILES :
                    { // Folder & File

                        pB->OpenFolder  =  pB->ItemPointer;
                        cMemoryGetItem(pB->PrgId,pB->hFolders,pB->OpenFolder,FOLDERNAME_SIZE + SUBFOLDERNAME_SIZE,pB->SubFolder,&TmpType);
#ifdef DEBUG
                        printf("Open  folder %3d (%s)\n",pB->OpenFolder,pB->SubFolder);
#endif
                        if (strcmp((char*)pB->SubFolder,SDCARD_FOLDER) == 0)
                        {
                            Item    =  pB->ItemPointer;
                            cMemoryGetItemName(pB->PrgId,pB->hFolders,Item,MAX_FILENAME_SIZE,pB->Filename,pType,&Priority);
                            Result  =  cMemoryGetItem(pB->PrgId,pB->hFolders,Item,FOLDERNAME_SIZE + SUBFOLDERNAME_SIZE,pB->FullPath,pType);
                            *pType  =  TYPE_SDCARD;

                            snprintf((char*)pAnswer,Lng,"%s",(char*)pB->FullPath);
                        }
                        else
                        {
                            if (strcmp((char*)pB->SubFolder,USBSTICK_FOLDER) == 0)
                            {
                                Item    =  pB->ItemPointer;
                                cMemoryGetItemName(pB->PrgId,pB->hFolders,Item,MAX_FILENAME_SIZE,pB->Filename,pType,&Priority);
                                Result  =  cMemoryGetItem(pB->PrgId,pB->hFolders,Item,FOLDERNAME_SIZE + SUBFOLDERNAME_SIZE,pB->FullPath,pType);
                                *pType  =  TYPE_USBSTICK;

                                snprintf((char*)pAnswer,Lng,"%s",(char*)pB->FullPath);
                            }
                            else
                            {
                                pB->ItemStart  =  pB->ItemPointer;
                                Result  =  cMemoryOpenFolder(PrgId,FILETYPE_UNKNOWN,pB->SubFolder,&pB->hFiles);

                                Result  =  BUSY;
                            }
                        }
                    }
                    break;

                    case BROWSE_CACHE :
                    { // Cache

                        Item    =  pB->ItemPointer;

                        cMemoryGetCacheName(Item,FOLDERNAME_SIZE + SUBFOLDERNAME_SIZE,(char*)pB->FullPath,(char*)pB->Filename,pType);
                        snprintf((char*)pAnswer,Lng,"%s",(char*)pB->FullPath);
                        Result  =  OK;
#ifdef DEBUG
                        printf("Select folder %3d\n",Item);
#endif
                    }
                    break;

                    case BROWSE_FILES :
                    { // File

                        if ((pB->ItemPointer > pB->OpenFolder) && (pB->ItemPointer <= (pB->OpenFolder + pB->Files)))
                        { // File selected

                            Item    =  pB->ItemPointer - pB->OpenFolder;

                            Result  =  cMemoryGetItem(pB->PrgId,pB->hFiles,Item,Lng,pB->FullPath,pType);

                            snprintf((char*)pAnswer,Lng,"%s",(char*)pB->FullPath);
                            Result  =  OK;
    #ifdef DEBUG
                            printf("Select file %3d\n",Item);
    #endif
                        }
                    }
                    break;

                }
            }
            pB->NeedUpdate    =  1;
        }

        TotalItems  =  pB->Folders + pB->Files;
        if (TmpResult == OK)
        {
            if (TotalItems)
            {
                if (pB->ItemPointer > TotalItems)
                {

                    pB->ItemPointer   =  TotalItems;
                    pB->NeedUpdate    =  1;
                }
                if (pB->ItemStart > pB->ItemPointer)
                {
                    pB->ItemStart     = pB->ItemPointer;
                    pB->NeedUpdate    =  1;
                }
            }
            else
            {
                pB->ItemStart       =  1;
                pB->ItemPointer     =  1;
            }
        }
        else
        {
            if (TmpResult == FAIL)
            {
                pB->ItemPointer     =  TotalItems;
                pB->ItemStart       =  pB->ItemPointer;
                pB->NeedUpdate      =  1;
            }
        }

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

        if (pB->NeedUpdate)
        {
//* UPDATE ***************************************************************************************************
            pB->NeedUpdate    =  0;

#ifdef DEBUG
#ifndef DISABLE_SDCARD_SUPPORT
            if (pB->Sdcard)
            {
                printf("SDCARD\n");
            }
#endif
#ifndef DISABLE_USBSTICK_SUPPORT
            if (pB->Usbstick)
            {
                printf("USBSTICK\n");
            }
#endif
            printf("Folders = %3d, OpenFolder = %3d, Files = %3d, ItemStart = %3d, ItemPointer = %3d, TotalItems = %3d\n\n",pB->Folders,pB->OpenFolder,pB->Files,pB->ItemStart,pB->ItemPointer,TotalItems);
#endif

            // clear screen
            dLcdFillRect(UiInstance.pLcd->Lcd,BG_COLOR,pB->ScreenStartX,pB->ScreenStartY,pB->ScreenWidth,pB->ScreenHeight);

            OldPriority   =  0;
            for (Tmp = 0;Tmp < pB->Lines;Tmp++)
            {
                Item          =  Tmp + pB->ItemStart;
                Folder        =  1;
                Priority      =  OldPriority;

                if (Item <= TotalItems)
                {
                    if (pB->OpenFolder)
                    {
                        if ((Item > pB->OpenFolder) && (Item <= (pB->OpenFolder + pB->Files)))
                        {
                            Item -=  pB->OpenFolder;
                            Folder  =  0;
                        }
                        else
                        {
                            if (Item > pB->OpenFolder)
                            {
                                Item -=  pB->Files;
                            }
                        }
                    }
#ifdef DEBUG
                    if (pB->ItemPointer == (Tmp + pB->ItemStart))
                    {
                        printf("> ");
                    }
                    else
                    {
                        printf("  ");
                    }
#endif

//*** Graphics ***********************************************************************************************

                    if (Folder)
                    { // Show folder

                        switch (Type)
                        {
                            case BROWSE_FOLDERS :
                            {
                                cMemoryGetItemName(pB->PrgId,pB->hFolders,Item,pB->Chars,pB->Filename,&TmpType,&Priority);
                                if (cMemoryGetItemIcon(pB->PrgId,pB->hFolders,Item,&TmpHandle,&Image) == OK)
                                {
                                    dLcdDrawBitmap(UiInstance.pLcd->Lcd,Color,pB->IconStartX,pB->IconStartY + (Tmp * pB->LineHeight),(IP)Image);
                                    cMemoryCloseFile(pB->PrgId,TmpHandle);
                                }
                                else
                                {
                                    dLcdDrawPicture(UiInstance.pLcd->Lcd,Color,pB->IconStartX,pB->IconStartY + (Tmp * pB->LineHeight),PCApp_width,PCApp_height,(UBYTE*)PCApp_bits);
                                }

                                pB->Text[0]  =  0;
                                if (strcmp((char*)pB->Filename,"Bluetooth") == 0)
                                {
                                    if (UiInstance.BtOn)
                                    {
                                        pB->Text[0]  =  '+';
                                    }
                                    else
                                    {
                                        pB->Text[0]  =  '-';
                                    }
                                }
                                else
                                {
                                    if (strcmp((char*)pB->Filename,"WiFi") == 0)
                                    {
                                        if (UiInstance.WiFiOn)
                                        {
                                            pB->Text[0]  =  '+';
                                        }
                                        else
                                        {
                                            pB->Text[0]  =  '-';
                                        }
                                    }
                                    else
                                    {
                                        if (cMemoryGetItemText(pB->PrgId,pB->hFolders,Item,pB->Chars,pB->Text) != OK)
                                        {
                                            pB->Text[0]  =  0;
                                        }
                                    }
                                }
                                switch (pB->Text[0])
                                {
                                    case 0 :
                                    {
                                    }
                                    break;

                                    case '+' :
                                    {
                                        Indent  =  (pB->Chars - 1) *  pB->CharWidth - dLcdGetIconWidth(MENU_ICON);
                                        dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pB->TextStartX + Indent,(pB->TextStartY - 2) + (Tmp * pB->LineHeight),MENU_ICON,ICON_CHECKED);
                                    }
                                    break;

                                    case '-' :
                                    {
                                        Indent  =  (pB->Chars - 1) *  pB->CharWidth - dLcdGetIconWidth(MENU_ICON);
                                        dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pB->TextStartX + Indent,(pB->TextStartY - 2) + (Tmp * pB->LineHeight),MENU_ICON,ICON_CHECKBOX);
                                    }
                                    break;

                                    default :
                                    {
                                        Indent  =  ((pB->Chars - 1) - (DATA16)strlen((char*)pB->Text)) * pB->CharWidth;
                                        dLcdDrawText(UiInstance.pLcd->Lcd,Color,pB->TextStartX + Indent,pB->TextStartY + (Tmp * pB->LineHeight),NORMAL_FONT,pB->Text);
                                    }
                                    break;

                                }

                            }
                            break;

                            case BROWSE_FOLDS_FILES :
                            {
                                cMemoryGetItemName(pB->PrgId,pB->hFolders,Item,pB->Chars,pB->Filename,&TmpType,&Priority);
                                dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pB->IconStartX,pB->IconStartY + (Tmp * pB->LineHeight),NORMAL_ICON,FiletypeToNormalIcon[TmpType]);

                                if ((Priority == 1) || (Priority == 2))
                                {
                                    dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pB->IconStartX,pB->IconStartY + (Tmp * pB->LineHeight),NORMAL_ICON,ICON_FOLDER2);
                                }
                                else
                                {
                                    if (Priority == 3)
                                    {
                                        dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pB->IconStartX,pB->IconStartY + (Tmp * pB->LineHeight),NORMAL_ICON,ICON_SD);
                                    }
                                    else
                                    {
                                        if (Priority == 4)
                                        {
                                            dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pB->IconStartX,pB->IconStartY + (Tmp * pB->LineHeight),NORMAL_ICON,ICON_USB);
                                        }
                                        else
                                        {
                                            dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pB->IconStartX,pB->IconStartY + (Tmp * pB->LineHeight),NORMAL_ICON,FiletypeToNormalIcon[TmpType]);
                                        }
                                    }
                                }
                                if (Priority != OldPriority)
                                {
                                    if ((Priority == 1) || (Priority >= 3))
                                    {
                                        if (Tmp)
                                        {
                                            dLcdDrawDotLine(UiInstance.pLcd->Lcd,Color,pB->SelectStartX,pB->SelectStartY + ((Tmp - 1) * pB->LineHeight) + pB->LineHeight - 2,pB->SelectStartX + pB->SelectWidth,pB->SelectStartY + ((Tmp - 1) * pB->LineHeight) + pB->LineHeight - 2,1,2);
                                        }
                                    }
                                }
                            }
                            break;

                            case BROWSE_CACHE :
                            {
                                cMemoryGetCacheName(Item,pB->Chars,(char*)pB->FullPath,(char*)pB->Filename,&TmpType);
                                dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pB->IconStartX,pB->IconStartY + (Tmp * pB->LineHeight),NORMAL_ICON,FiletypeToNormalIcon[TmpType]);
                            }
                            break;

                            case BROWSE_FILES :
                            {
                                cMemoryGetItemName(pB->PrgId,pB->hFiles,Item,pB->Chars,pB->Filename,&TmpType,&Priority);
                                dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pB->IconStartX,pB->IconStartY + (Tmp * pB->LineHeight),NORMAL_ICON,FiletypeToNormalIcon[TmpType]);
                            }
                            break;

                        }
                        // Draw folder name
                        dLcdDrawText(UiInstance.pLcd->Lcd,Color,pB->TextStartX,pB->TextStartY + (Tmp * pB->LineHeight),NORMAL_FONT,pB->Filename);

                        // Draw open folder
                        if (Item == pB->OpenFolder)
                        {
                            dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,144,pB->IconStartY + (Tmp * pB->LineHeight),NORMAL_ICON,ICON_OPENFOLDER);
                        }

                        // Draw selection
                        if (pB->ItemPointer == (Tmp + pB->ItemStart))
                        {
                            dLcdInverseRect(UiInstance.pLcd->Lcd,pB->SelectStartX,pB->SelectStartY + (Tmp * pB->LineHeight),pB->SelectWidth + 1,pB->SelectHeight);
                        }

                        // Draw end line
                        switch (Type)
                        {
                            case BROWSE_FOLDERS :
                            case BROWSE_FOLDS_FILES :
                            case BROWSE_FILES :
                            {
                                if (((Tmp + pB->ItemStart) == TotalItems) && (Tmp < (pB->Lines - 1)))
                                {
                                    dLcdDrawDotLine(UiInstance.pLcd->Lcd,Color,pB->SelectStartX,pB->SelectStartY + (Tmp * pB->LineHeight) + pB->LineHeight - 2,pB->SelectStartX + pB->SelectWidth,pB->SelectStartY + (Tmp * pB->LineHeight) + pB->LineHeight - 2,1,2);
                                }
                            }
                            break;

                            case BROWSE_CACHE :
                            {
                                if (((Tmp + pB->ItemStart) == 1) && (Tmp < (pB->Lines - 1)))
                                {
                                    dLcdDrawDotLine(UiInstance.pLcd->Lcd,Color,pB->SelectStartX,pB->SelectStartY + (Tmp * pB->LineHeight) + pB->LineHeight - 2,pB->SelectStartX + pB->SelectWidth,pB->SelectStartY + (Tmp * pB->LineHeight) + pB->LineHeight - 2,1,2);
                                }
                                if (((Tmp + pB->ItemStart) == TotalItems) && (Tmp < (pB->Lines - 1)))
                                {
                                    dLcdDrawDotLine(UiInstance.pLcd->Lcd,Color,pB->SelectStartX,pB->SelectStartY + (Tmp * pB->LineHeight) + pB->LineHeight - 2,pB->SelectStartX + pB->SelectWidth,pB->SelectStartY + (Tmp * pB->LineHeight) + pB->LineHeight - 2,1,2);
                                }
                            }
                            break;

                        }

#ifdef DEBUG
                        printf("%s %d %d %d\n",(char*)pB->Filename,Item,pB->OpenFolder,Priority);
#endif
                    }
                    else
                    { // Show file

                        // Get file name and type
                        cMemoryGetItemName(pB->PrgId,pB->hFiles,Item,pB->Chars - 1,pB->Filename,&TmpType,&Priority);

                        // show File icons
                        dLcdDrawIcon(UiInstance.pLcd->Lcd,Color,pB->IconStartX + pB->CharWidth,pB->IconStartY + (Tmp * pB->LineHeight),NORMAL_ICON,FiletypeToNormalIcon[TmpType]);

                        // Draw file name
                        dLcdDrawText(UiInstance.pLcd->Lcd,Color,pB->TextStartX + pB->CharWidth,pB->TextStartY + (Tmp * pB->LineHeight),NORMAL_FONT,pB->Filename);

                        // Draw folder line
                        if ((Tmp == (pB->Lines - 1)) || (Item == pB->Files))
                        {
                            dLcdDrawLine(UiInstance.pLcd->Lcd,Color,pB->IconStartX + pB->CharWidth - 3,pB->SelectStartY + (Tmp * pB->LineHeight),pB->IconStartX + pB->CharWidth - 3,pB->SelectStartY + (Tmp * pB->LineHeight) + pB->SelectHeight - 1);
                        }
                        else
                        {
                            dLcdDrawLine(UiInstance.pLcd->Lcd,Color,pB->IconStartX + pB->CharWidth - 3,pB->SelectStartY + (Tmp * pB->LineHeight),pB->IconStartX + pB->CharWidth - 3,pB->SelectStartY + (Tmp * pB->LineHeight) + pB->LineHeight - 1);
                        }

                        // Draw selection
                        if (pB->ItemPointer == (Tmp + pB->ItemStart))
                        {
                            dLcdInverseRect(UiInstance.pLcd->Lcd,pB->SelectStartX + pB->CharWidth,pB->SelectStartY + (Tmp * pB->LineHeight),pB->SelectWidth + 1 - pB->CharWidth,pB->SelectHeight);
                        }

#ifdef DEBUG
                        printf(" | %s\n",(char*)pB->Filename);
#endif

                    }

//************************************************************************************************************
                }
#ifdef DEBUG
                else
                {
                    printf("\n");
                }
#endif
                OldPriority  =  Priority;
            }
#ifdef DEBUG
            printf("\n");
#endif

            cUiDrawBar(1,pB->ScrollStartX,pB->ScrollStartY,pB->ScrollWidth,pB->ScrollHeight,0,TotalItems,pB->ItemPointer);

            // Update
            cUiUpdateLcd();
            UiInstance.ScreenBusy       =  0;
        }

        if (Result != OK)
        {
            Tmp  =  cUiButtonTestHorz();
            if (Ignore == Tmp)
            {
                Tmp  =  cUiButtonGetHorz();
                Tmp  =  0;
            }

            if ((Tmp != 0) || (cUiButtonTestShortPress(BACK_BUTTON)) || (cUiButtonTestLongPress(BACK_BUTTON)))
            {
                if (Type != BROWSE_CACHE)
                {
                    if (pB->OpenFolder)
                    {
                        if (pB->hFiles)
                        {
                            cMemoryCloseFolder(pB->PrgId,&pB->hFiles);
                        }
                    }
                    if (pB->hFolders)
                    {
                        cMemoryCloseFolder(pB->PrgId,&pB->hFolders);
                    }
                }
                pB->PrgId         =  0;
                pB->ObjId         =  0;
                pB->SubFolder[0]  =  0;
                pAnswer[0]          =  0;
                *pType              =  0;

    #ifdef DEBUG
                printf("%d %d Closing browser with [%s] type [%d]\n",PrgId,ObjId,(char*)pAnswer,*pType);
    #endif
                Result  =  OK;
            }
        }
        else
        {
            pB->NeedUpdate    =  1;
        }
    }
    else
    {
        pAnswer[0]          =  0;
        *pType              =  TYPE_RESTART_BROWSER;
        Result              =  FAIL;
    }

    if (*pType > 0)
    {
#ifndef DISABLE_SDCARD_SUPPORT
        if (pB->Sdcard)
        {
            *pType |=  TYPE_SDCARD;
        }
#endif
#ifndef DISABLE_USBSTICK_SUPPORT
        if (pB->Usbstick)
        {
            *pType |=  TYPE_USBSTICK;
        }
#endif
    }

    if (Result != BUSY)
    {
//* EXIT *****************************************************************************************************

#ifdef DEBUG
        printf("%d %d Return from browser with [%s] type [0x%02X]\n\n",PrgId,ObjId,(char*)pAnswer,*pType);
#endif
    }

    return (Result);
}
