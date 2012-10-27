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
#include "resource.h"
#include "shellapi.h"
#include "Hyperlinks.h"
using namespace Niflib;

static INT_PTR CALLBACK MaxNifImportOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
   static NifImporter *imp = NULL;
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

            imp = (NifImporter *)lParam;
            CenterWindow(hWnd,GetParent(hWnd));

            CheckDlgButton(hWnd, IDC_CHK_BONES, imp->importBones);
            CheckDlgButton(hWnd, IDC_CHK_SKIN, imp->enableSkinSupport);
            CheckDlgButton(hWnd, IDC_CHK_VCOLORS, imp->vertexColorMode);
            CheckDlgButton(hWnd, IDC_CHK_COLL, imp->enableCollision);
			EnableWindow(GetDlgItem(hWnd, IDC_CHK_COLL), TRUE);
            CheckDlgButton(hWnd, IDC_CHK_ANIMATION, imp->enableAnimations);
            CheckDlgButton(hWnd, IDC_CHK_LIGHTS, imp->enableLights);
            CheckDlgButton(hWnd, IDC_CHK_CAMERA, imp->enableCameras);
            
            //CheckDlgButton(hWnd, IDC_CHK_FURN, imp->);

            CheckDlgButton(hWnd, IDC_CHK_FLIP_UV, imp->flipUVTextures);
            CheckDlgButton(hWnd, IDC_CHK_SHOW_TEX, imp->showTextures);
            CheckDlgButton(hWnd, IDC_CHK_AUTOSMOOTH, imp->enableAutoSmooth);
            CheckDlgButton(hWnd, IDC_CHK_ILLEGAL, imp->removeIllegalFaces);
            CheckDlgButton(hWnd, IDC_CHK_REM_BONES, imp->removeUnusedImportedBones);
            CheckDlgButton(hWnd, IDC_CHK_CLEARANIM, imp->clearAnimation);
            CheckDlgButton(hWnd, IDC_CHK_KEYNOTES, imp->addNoteTracks);
            CheckDlgButton(hWnd, IDC_CHK_TIMETAGS, imp->addTimeTags);
            CheckDlgButton(hWnd, IDC_CHK_IGNORE_ROOT, imp->ignoreRootNode);
			CheckDlgButton(hWnd, IDC_CHK_USE_SHADER, imp->useNiftoolsShader);
            
            CheckDlgButton(hWnd, IDC_CHK_BIPED, imp->useBiped);
            CheckDlgButton(hWnd, IDC_CHK_UPB, !imp->importUPB);           
            
            string selection = (imp->appSettings) ? imp->appSettings->Name : "User";
            for (AppSettingsMap::iterator itr = TheAppSettings.begin(), end = TheAppSettings.end(); itr != end; ++itr)
               SendDlgItemMessage(hWnd, IDC_CB_GAME, CB_ADDSTRING, 0, LPARAM(itr->Name.c_str()));
            SendDlgItemMessage(hWnd, IDC_CB_GAME, CB_SELECTSTRING, WPARAM(-1), LPARAM(selection.c_str()));
            CheckDlgButton(hWnd, IDC_CHK_AUTO_DETECT, imp->autoDetect);

            SHAutoComplete(GetDlgItem(hWnd, IDC_ED_SKELETON), SHACF_FILESYSTEM);
            if (imp->HasSkeleton() && imp->appSettings && imp->importSkeleton) {
               SetDlgItemText(hWnd, IDC_ED_SKELETON, imp->skeleton.c_str());
            } else {
               EnableWindow(GetDlgItem(hWnd, IDC_STC_SKELETON), FALSE);
               EnableWindow(GetDlgItem(hWnd, IDC_ED_SKELETON), FALSE);
               EnableWindow(GetDlgItem(hWnd, IDC_BTN_BROWSE), FALSE);
               EnableWindow(GetDlgItem(hWnd, IDC_CHK_BIPED), FALSE);
               EnableWindow(GetDlgItem(hWnd, IDC_CHK_REM_BONES), FALSE);
            }

			// Weld Threshold
			TSTR weldThresh; weldThresh.printf("%g", imp->weldVertexThresh);
			CheckDlgButton(hWnd, IDC_CHK_WELD, imp->weldVertices);
			EnableWindow(GetDlgItem(hWnd, IDC_CHK_WELD), TRUE);

			SetDlgItemText(hWnd, IDC_EDIT_WELDTHRESH, weldThresh);
			EnableWindow(GetDlgItem(hWnd, IDC_EDIT_WELDTHRESH), imp->weldVertices);

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
               char tmp[MAX_PATH];
               switch (LOWORD(wParam))
               {
               case IDOK:

                  imp->importBones = IsDlgButtonChecked(hWnd, IDC_CHK_BONES) ? true : false;
                  imp->enableSkinSupport = IsDlgButtonChecked(hWnd, IDC_CHK_SKIN) ? true : false;
                  imp->vertexColorMode = (int)IsDlgButtonChecked(hWnd, IDC_CHK_VCOLORS);
                  imp->enableCollision = IsDlgButtonChecked(hWnd, IDC_CHK_COLL) ? true : false;
                  imp->enableCameras = IsDlgButtonChecked(hWnd, IDC_CHK_CAMERA) ? true : false;
                  imp->enableLights = IsDlgButtonChecked(hWnd, IDC_CHK_LIGHTS) ? true : false;
                  imp->enableAnimations = IsDlgButtonChecked(hWnd, IDC_CHK_ANIMATION) ? true : false;
                  //IsDlgButtonChecked(hWnd, IDC_CHK_FURN, imp->);

                  imp->flipUVTextures = IsDlgButtonChecked(hWnd, IDC_CHK_FLIP_UV) ? true : false;
                  imp->showTextures = IsDlgButtonChecked(hWnd, IDC_CHK_SHOW_TEX) ? true : false;
                  imp->enableAutoSmooth = IsDlgButtonChecked(hWnd, IDC_CHK_AUTOSMOOTH) ? true : false;
                  imp->removeDegenerateFaces =
                  imp->removeIllegalFaces = IsDlgButtonChecked(hWnd, IDC_CHK_ILLEGAL) ? true : false;
                  imp->removeUnusedImportedBones = IsDlgButtonChecked(hWnd, IDC_CHK_REM_BONES) ? true : false;
                  imp->clearAnimation = IsDlgButtonChecked(hWnd, IDC_CHK_CLEARANIM) ? true : false;
                  imp->addNoteTracks = IsDlgButtonChecked(hWnd, IDC_CHK_KEYNOTES) ? true : false;
                  imp->addTimeTags = IsDlgButtonChecked(hWnd, IDC_CHK_TIMETAGS) ? true : false;
                  imp->useBiped = IsDlgButtonChecked(hWnd, IDC_CHK_BIPED) ? true : false;
                  imp->importUPB = IsDlgButtonChecked(hWnd, IDC_CHK_UPB) ? false : true;
                  imp->ignoreRootNode = IsDlgButtonChecked(hWnd, IDC_CHK_IGNORE_ROOT) ? true : false;
				  imp->useNiftoolsShader = IsDlgButtonChecked(hWnd, IDC_CHK_USE_SHADER) ? true : false;
                  
                  GetDlgItemText(hWnd, IDC_CB_GAME, tmp, MAX_PATH);
                  if (AppSettings *appSettings = FindAppSetting(tmp)) {
                     imp->appSettings = appSettings;
                  }
                  imp->autoDetect = IsDlgButtonChecked(hWnd, IDC_CHK_AUTO_DETECT) ? true : false;
                  
                  GetDlgItemText(hWnd, IDC_ED_SKELETON, tmp, MAX_PATH);
                  imp->skeleton = tmp;

				  // Weld Threshold
				  imp->weldVertices = IsDlgButtonChecked(hWnd, IDC_CHK_WELD)? true : false;
				  GetDlgItemText(hWnd, IDC_EDIT_WELDTHRESH, tmp, MAX_PATH);
				  if (strlen(tmp) > 0) imp->weldVertexThresh = (float)atof(tmp);

                  EndDialog(hWnd, dlgRes=IDOK);
                  return TRUE;

               case IDCANCEL:
                  EndDialog(hWnd, dlgRes=IDCANCEL);
                  return TRUE;

               case IDC_BTN_BROWSE:
                  {
                     TCHAR filter[64], *pfilter=filter;
                     pfilter = _tcscpy(filter, shortDescription);
                     pfilter = _tcscat(pfilter, " (*.NIF)");
                     pfilter += strlen(pfilter);
                     *pfilter++ = '\0';
                     _tcscpy(pfilter, "*.NIF");
                     pfilter += strlen(pfilter);
                     *pfilter++ = '\0';
                     *pfilter++ = '\0';

                     GetDlgItemText(hWnd, IDC_ED_SKELETON, tmp, MAX_PATH);
                     GetLongPathName(tmp, tmp, _countof(tmp));

                     OPENFILENAME ofn;
                     memset(&ofn, 0, sizeof(ofn));
                     ofn.lStructSize = sizeof(ofn);
                     ofn.hwndOwner = imp->gi->GetMAXHWnd();
                     ofn.lpstrFilter = filter;
                     ofn.lpstrFile = tmp;
                     ofn.nMaxFile = _countof(tmp);
                     ofn.lpstrTitle = TEXT("Browse for Skeleton NIF...");
                     ofn.lpstrDefExt = TEXT("NIF");
                     ofn.Flags = OFN_HIDEREADONLY|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST;
                     if (GetOpenFileName(&ofn)) {
                        SetDlgItemText(hWnd, IDC_ED_SKELETON, tmp);
                     }
                  }
                  break;

			   case IDC_CHK_WELD:
				   EnableWindow(GetDlgItem(hWnd, IDC_EDIT_WELDTHRESH), IsDlgButtonChecked(hWnd, IDC_CHK_WELD));
				   break;

               case IDC_LBL_LINK:
                  ShellExecute(hWnd, "open", imp->webSite, NULL, NULL, SW_SHOWNORMAL);
                  break;

               case IDC_LBL_WIKI:
                  ShellExecute(hWnd, "open", imp->wikiSite, NULL, NULL, SW_SHOWNORMAL);
                  break;
               }
            }
            else if (HIWORD(wParam) == CBN_SELCHANGE)
            {
               if (LOWORD(wParam) == IDC_CB_GAME)
               {
                  char tmp[MAX_PATH];
                  GetDlgItemText(hWnd, IDC_CB_GAME, tmp, MAX_PATH);
                  if (AppSettings *appSettings = FindAppSetting(tmp))
                  {
                     string skeleton = imp->GetSkeleton(appSettings);
                     BOOL enable = imp->HasSkeleton() ? TRUE : FALSE;
                     if (enable) {
                        SetDlgItemText(hWnd, IDC_ED_SKELETON, skeleton.c_str());
                     }
                     EnableWindow(GetDlgItem(hWnd, IDC_STC_SKELETON), enable);
                     EnableWindow(GetDlgItem(hWnd, IDC_ED_SKELETON), enable);
                     EnableWindow(GetDlgItem(hWnd, IDC_BTN_BROWSE), enable);
                  }
               }
            }
         }
         break;
   }
   return FALSE;
}

bool NifImporter::ShowDialog()
{
   return (IDOK == DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_NIF_PANEL), GetActiveWindow(), MaxNifImportOptionsDlgProc, (LPARAM)this));
}