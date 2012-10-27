/**********************************************************************
*<
FILE: ImporterCore.cpp

DESCRIPTION:	Core Import helper routines

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "stdafx.h"
#include "MaxNifImport.h"
#include "KFMImporter.h"
#include "resource.h"
#include "shellapi.h"
#include "Hyperlinks.h"
using namespace Niflib;

static INT_PTR CALLBACK MaxNifImportOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
   static KFMImporter *imp = NULL;
   static DWORD dlgRes = IDCANCEL; 

   switch(message) {
      case WM_INITDIALOG:
         {
            dlgRes = IDCANCEL;

            // Append file version to dialog
            TSTR version = GetFileVersion(NULL);
            if (!version.isNull()) {
               char buffer[256];
               GetWindowText(hWnd, buffer, _countof(buffer));
               _tcscat(buffer, TEXT(" "));
               _tcscat(buffer, version);
               SetWindowText(hWnd, buffer);
            }

            imp = (KFMImporter *)lParam;
            CenterWindow(hWnd,GetParent(hWnd));

            CheckDlgButton(hWnd, IDC_CHK_CLEARANIM, imp->clearAnimation);
            CheckDlgButton(hWnd, IDC_CHK_KEYNOTES, imp->addNoteTracks);
            CheckDlgButton(hWnd, IDC_CHK_TIMETAGS, imp->addTimeTags);

            ConvertStaticToHyperlink(hWnd, IDC_LBL_LINK);
            ConvertStaticToHyperlink(hWnd, IDC_LBL_WIKI);

         }
         return TRUE;

      case WM_CLOSE:
         {
            EndDialog(hWnd, dlgRes);
         }
         return TRUE;

      case WM_COMMAND : 
         {
            if (HIWORD(wParam) == BN_CLICKED)
            {
               switch (LOWORD(wParam))
               {
               case IDOK:
                  imp->clearAnimation = IsDlgButtonChecked(hWnd, IDC_CHK_CLEARANIM) ? true : false;
                  imp->addNoteTracks = IsDlgButtonChecked(hWnd, IDC_CHK_KEYNOTES) ? true : false;
                  imp->addTimeTags = IsDlgButtonChecked(hWnd, IDC_CHK_TIMETAGS) ? true : false;
                  EndDialog(hWnd, dlgRes=IDOK);
                  return TRUE;

               case IDCANCEL:
                  EndDialog(hWnd, dlgRes=IDCANCEL);
                  return TRUE;

               case IDC_LBL_LINK:
                  ShellExecute(hWnd, "open", imp->webSite, NULL, NULL, SW_SHOWNORMAL);
                  break;

               case IDC_LBL_WIKI:
                  ShellExecute(hWnd, "open", imp->wikiSite, NULL, NULL, SW_SHOWNORMAL);
                  break;
               }
            }
         }
         break;
   }
   return FALSE;
}

bool KFMImporter::ShowDialog()
{
   return (IDOK == DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_KF_PANEL), GetActiveWindow(), MaxNifImportOptionsDlgProc, (LPARAM)this));
}