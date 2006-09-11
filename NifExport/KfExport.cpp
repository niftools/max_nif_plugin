#include "pch.h"
#include "AppSettings.h"
#include "niutils.h"
#include  <io.h>
#include "obj/NiControllerSequence.h"
using namespace Niflib;

#define KFEXPORT_CLASS_ID	Class_ID(0xa57ff0a4, 0xa0374ffc)

static LPCTSTR KfExportSection = TEXT("KfExport");

class KfExport : public SceneExport 
{
public:

   static HWND		hParams;
   int				mDlgResult;
   TSTR iniFileName;
   TSTR shortDescription;

   int				ExtCount();					// Number of extensions supported
   const TCHAR		*Ext(int n);					// Extension #n (i.e. "3DS")
   const TCHAR		*LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
   const TCHAR		*ShortDesc();				// Short ASCII description (i.e. "3D Studio")
   const TCHAR		*AuthorName();				// ASCII Author name
   const TCHAR		*CopyrightMessage();			// ASCII Copyright message
   const TCHAR		*OtherMessage1();			// Other message #1
   const TCHAR		*OtherMessage2();			// Other message #2
   unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
   void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box

   BOOL			SupportsOptions(int ext, DWORD options);
   int				DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0);

   KfExport();
   ~KfExport();		
};



class KfExportClassDesc : public ClassDesc2 
{
public:
   int 			IsPublic() { return TRUE; }
   void			*Create(BOOL loading = FALSE) { return new KfExport(); }
   const TCHAR		*ClassName() { return GetString(IDS_KF_CLASS_NAME); }
   SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
   Class_ID		ClassID() { return KFEXPORT_CLASS_ID; }
   const TCHAR		*Category() { return GetString(IDS_CATEGORY); }

   const TCHAR		*InternalName() { return _T("_KfExport"); }	// returns fixed parsable name (scripter-visible name)
   HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle


};

static KfExportClassDesc KfExportDesc;
ClassDesc2* GetKfExportDesc() { return &KfExportDesc; }


static BOOL CALLBACK KfExportOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
   static KfExport *imp = NULL;

   switch (message) 
   {
   case WM_INITDIALOG:
      {
         // Append file version to dialog
         TSTR fileVersion = GetFileVersion(NULL);
         if (!fileVersion.isNull()) {
            char buffer[256];
            GetWindowText(hWnd, buffer, _countof(buffer));
            _tcscat(buffer, TEXT(" "));
            _tcscat(buffer, fileVersion);
            SetWindowText(hWnd, buffer);
         }

         imp = (KfExport *)lParam;
         CenterWindow(hWnd,GetParent(hWnd));

         CheckDlgButton(hWnd, IDC_CHK_HIDDEN, Exporter::mExportHidden);
         CheckDlgButton(hWnd, IDC_CHK_LIGHTS, Exporter::mExportLights);
         CheckDlgButton(hWnd, IDC_CHK_CAMERA, Exporter::mExportCameras);
         CheckDlgButton(hWnd, IDC_CHK_TRANSFORMS, Exporter::mExportTransforms);
         SetDlgItemText(hWnd, IDC_ED_PRIORITY, FormatText("%.4f", Exporter::mDefaultPriority));

         string selection = Exporter::mGameName;
         string version = Exporter::mNifVersion;
         string userVer = FormatString("%d", Exporter::mNifUserVersion);
         for (AppSettingsMap::iterator itr = TheAppSettings.begin(), end = TheAppSettings.end(); itr != end; ++itr)
            SendDlgItemMessage(hWnd, IDC_CB_GAME, CB_ADDSTRING, 0, LPARAM(itr->Name.c_str()));
         SendDlgItemMessage(hWnd, IDC_CB_GAME, CB_SELECTSTRING, WPARAM(-1), LPARAM(selection.c_str()));
         SendDlgItemMessage(hWnd, IDC_CB_VERSION, WM_SETTEXT, 0, LPARAM(version.c_str()));
         SendDlgItemMessage(hWnd, IDC_CB_USER_VERSION, WM_SETTEXT, 0, LPARAM(userVer.c_str()));

         imp->mDlgResult = IDCANCEL;
      }
      return TRUE;

   case WM_CLOSE:
      EndDialog(hWnd, imp->mDlgResult);
      return TRUE;

   case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
      {
         char tmp[MAX_PATH], *end;
         bool close = false;
         switch (LOWORD(wParam))
         {
         case IDOK:
            // Validity Check
            GetDlgItemText(hWnd, IDC_CB_VERSION, tmp, MAX_PATH);
            if (tmp[0] != 0)
            {
               int nifVersion = GetVersion(tmp);
               if (!IsVersionSupported(nifVersion))
               {
                  MessageBox(hWnd, FormatString("Version '%s' is not a supported version.", tmp).c_str(), "NifExport", MB_OK|MB_ICONSTOP);
                  return FALSE;
               }
            }

            Exporter::mExportHidden = IsDlgButtonChecked(hWnd, IDC_CHK_HIDDEN);
            Exporter::mExportLights = IsDlgButtonChecked(hWnd, IDC_CHK_LIGHTS);
            Exporter::mExportCameras = IsDlgButtonChecked(hWnd, IDC_CHK_CAMERA);
            Exporter::mExportTransforms = IsDlgButtonChecked(hWnd, IDC_CHK_TRANSFORMS);

            GetDlgItemText(hWnd, IDC_ED_PRIORITY, tmp, MAX_PATH);
            Exporter::mDefaultPriority = atof(tmp);

            GetDlgItemText(hWnd, IDC_CB_GAME, tmp, MAX_PATH);
            if (AppSettings *appSettings = FindAppSetting(tmp))
            {
               Exporter::mGameName = appSettings->Name;
               GetDlgItemText(hWnd, IDC_CB_VERSION, tmp, MAX_PATH);
               Exporter::mNifVersion = tmp;
               GetDlgItemText(hWnd, IDC_CB_USER_VERSION, tmp, MAX_PATH);
               Exporter::mNifUserVersion = strtol(tmp, &end, 0);
            }

            EndDialog(hWnd, imp->mDlgResult=IDOK);
            close = true;
            break;

         case IDCANCEL:
            EndDialog(hWnd, imp->mDlgResult=IDCANCEL);
            close = true;
            break;
         }

         if (close)
            SendMessage(hWnd, WM_CLOSE, 0, 0);
      }
      else if (HIWORD(wParam) == STN_CLICKED)
      {
         if (LOWORD(wParam) == IDC_LBL_LINK)
         {
            ShellExecute(hWnd, "open", "http://www.niftools.org",
               NULL, NULL, SW_SHOWDEFAULT);
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
               string version = appSettings->NiVersion;
               string userVer = FormatString("%d", appSettings->NiUserVersion);
               SendDlgItemMessage(hWnd, IDC_CB_VERSION, WM_SETTEXT, 0, LPARAM(version.c_str()));
               SendDlgItemMessage(hWnd, IDC_CB_USER_VERSION, WM_SETTEXT, 0, LPARAM(userVer.c_str()));
            }
         }
      }
      break;
   }
   return FALSE;
}

//--- KfExport -------------------------------------------------------
KfExport::KfExport()
{
   Interface *gi = GetCOREInterface();
   TCHAR iniName[MAX_PATH];
   if (gi) {
      LPCTSTR pluginDir = gi->GetDir(APP_PLUGCFG_DIR);
      PathCombine(iniName, pluginDir, "MaxNifTools.ini");
   } else {
      GetModuleFileName(NULL, iniName, _countof(iniName));
      if (LPTSTR fname = PathFindFileName(iniName))
         fname = NULL;
      PathAddBackslash(iniName);
      PathAppend(iniName, "plugcfg");
      PathAppend(iniName, "MaxNifTools.ini");
   }
   iniFileName = iniName;
   shortDescription = GetIniValue<TSTR>("System", "ShortDescription", "Netimmerse/Gamebryo", iniFileName);
}

KfExport::~KfExport() 
{

}

int KfExport::ExtCount()
{
   return 1;
}

const TCHAR *KfExport::Ext(int n)
{		
   return _T("kf");
}

const TCHAR *KfExport::LongDesc()
{
   return _T("Gamebryo File");
}

const TCHAR *KfExport::ShortDesc() 
{			
   return shortDescription;
}

const TCHAR *KfExport::AuthorName()
{			
   return _T("Theo / The Niftools Team");
}

const TCHAR *KfExport::CopyrightMessage() 
{	
   return _T("http://www.niftools.org");
}

const TCHAR *KfExport::OtherMessage1() 
{		
   return _T("http://www.niftools.org");
}

const TCHAR *KfExport::OtherMessage2() 
{		
   return _T("http://www.niftools.org");
}

unsigned int KfExport::Version()
{				
   return Exporter::mVersion;
}

void KfExport::ShowAbout(HWND hWnd)
{			

}

BOOL KfExport::SupportsOptions(int ext, DWORD options)
{
   return TRUE;
}


int	KfExport::DoExport(const TCHAR *name, ExpInterface *ei, Interface *i, BOOL suppressPrompts, DWORD options)
{
   try
   {
      // read application settings
      AppSettings::Initialize(i);

      TCHAR iniName[MAX_PATH];
      LPCTSTR pluginDir = i->GetDir(APP_PLUGCFG_DIR);
      PathCombine(iniName, pluginDir, "MaxNifTools.ini");
      bool iniNameIsValid = (-1 != _taccess(iniName, 0));

      // read config from registry
      Exporter::readKfConfig(i);

      AppSettings *appSettings = NULL;
      if (iniNameIsValid)
      {
         string fname = name;
         // Locate which application to use. If Auto, find first app where this file appears in the root path list
         string curapp = GetIniValue<string>(KfExportSection, "CurrentApp", "AUTO", iniName);
         if (0 == _tcsicmp(curapp.c_str(), "AUTO")) {
            // Scan Root paths
            for (AppSettingsMap::iterator itr = TheAppSettings.begin(), end = TheAppSettings.end(); itr != end; ++itr){
               if ((*itr).IsFileInRootPaths(fname)) {
                  appSettings = &(*itr);
                  break;
               }
            }
         } else {
            appSettings = FindAppSetting(curapp);
         }
      }
      if (appSettings == NULL && !TheAppSettings.empty())
         appSettings = &TheAppSettings.front();

      Exporter::mGameName = appSettings->Name;
      Exporter::mNifVersion = appSettings->NiVersion;
      Exporter::mNifUserVersion = appSettings->NiUserVersion;

      if(!suppressPrompts)
      {
         if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_KF_PANEL), GetActiveWindow(), KfExportOptionsDlgProc, (LPARAM)this) != IDOK)
            return true;

         // write config to registry
         Exporter::writeKfConfig(i);

         // write config to root node
         Exporter::writeConfig(i->GetRootNode());

         // Update the current app version
         appSettings = FindAppSetting(Exporter::mGameName);
         if (appSettings == NULL && !TheAppSettings.empty())
            appSettings = &TheAppSettings.front();
         appSettings->NiVersion = Exporter::mNifVersion;
         appSettings->NiUserVersion = Exporter::mNifUserVersion;
         appSettings->WriteSettings(i);
      }

      int nifVersion = VER_20_0_0_5;
      int nifUserVer = Exporter::mNifUserVersion;

      if (!Exporter::mNifVersion.empty())
      {
         nifVersion = GetVersion(Exporter::mNifVersion);
         if (!IsVersionSupported(nifVersion))
            throw exception(FormatString("Version '%s' is not a supported version.").c_str());
      }

      Exporter::mSelectedOnly = (options&SCENE_EXPORT_SELECTED) != 0;
      Exporter exp(i, appSettings);

      Ref<NiControllerSequence> root = new NiControllerSequence();

      TCHAR fname[MAX_PATH];
      _tcscpy(fname, PathFindFileName(name));
      PathRemoveExtension(fname);
      root->SetName(fname);

      Exporter::Result result = exp.doAnimExport(root);

      if (result!=Exporter::Ok)
         throw exception("Unknown error.");

      WriteNifTree(name, StaticCast<NiObject>(root), nifVersion, nifUserVer);
   }

   catch (exception &e)
   {
      MessageBox(NULL, e.what(), "Export Error", MB_OK);
      return true;
   }

   catch (...)
   {
      MessageBox(NULL, "Unknown error.", "Export Error", MB_OK);
      return true;
   }

   return TRUE;
}