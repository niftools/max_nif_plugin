#include "pch.h"
#include "AppSettings.h"
#include "niutils.h"
#include  <io.h>

using namespace Niflib;

#define NifExport_CLASS_ID	Class_ID(0xa57ff0a4, 0xa0374ffb)

static LPCTSTR NifExportSection = TEXT("MaxNifExport");

class NifExport : public SceneExport 
{
public:
		
	static HWND		hParams;
	int				mDlgResult;
		
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

	NifExport();
	~NifExport();		
};



class NifExportClassDesc : public ClassDesc2 
{
	public:
	int 			IsPublic() { return TRUE; }
	void			*Create(BOOL loading = FALSE) { return new NifExport(); }
	const TCHAR		*ClassName() { return GetString(IDS_CLASS_NAME); }
	SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID() { return NifExport_CLASS_ID; }
	const TCHAR		*Category() { return GetString(IDS_CATEGORY); }

	const TCHAR		*InternalName() { return _T("NifExport"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
	

};

static NifExportClassDesc NifExportDesc;
ClassDesc2* GetNifExportDesc() { return &NifExportDesc; }





BOOL CALLBACK NifExportOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
	static NifExport *imp = NULL;

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

				imp = (NifExport *)lParam;
				CenterWindow(hWnd,GetParent(hWnd));
				CheckDlgButton(hWnd, IDC_CHK_STRIPS, Exporter::mTriStrips);
				CheckDlgButton(hWnd, IDC_CHK_HIDDEN, Exporter::mExportHidden);
				CheckDlgButton(hWnd, IDC_CHK_FURN, Exporter::mExportFurn);
				CheckDlgButton(hWnd, IDC_CHK_LIGHTS, Exporter::mExportLights);
				CheckDlgButton(hWnd, IDC_CHK_VCOLORS, Exporter::mVertexColors);
				SetDlgItemText(hWnd, IDC_ED_TEXPREFIX, Exporter::mTexPrefix.c_str());
				CheckDlgButton(hWnd, IDC_CHK_COLL, Exporter::mExportCollision);
            CheckDlgButton(hWnd, IDC_CHK_REMAP, Exporter::mRemapIndices);

            CheckDlgButton(hWnd, IDC_CHK_EXTRA, Exporter::mExportExtraNodes);
            CheckDlgButton(hWnd, IDC_CHK_SKIN, Exporter::mExportSkin);
            CheckDlgButton(hWnd, IDC_CHK_UPB, Exporter::mUserPropBuffer);
            CheckDlgButton(hWnd, IDC_CHK_HIER, Exporter::mFlattenHierarchy);
            CheckDlgButton(hWnd, IDC_CHK_REM_BONES, Exporter::mRemoveUnreferencedBones);
            CheckDlgButton(hWnd, IDC_CHK_SORTNODES, Exporter::mSortNodesToEnd);
            CheckDlgButton(hWnd, IDC_CHK_SKEL_ONLY, Exporter::mSkeletonOnly);
            CheckDlgButton(hWnd, IDC_CHK_CAMERA, Exporter::mExportCameras);
            CheckDlgButton(hWnd, IDC_CHK_BONE_COLL, Exporter::mGenerateBoneCollision);

            string selection = Exporter::mGameName;
            string version = Exporter::mNifVersion;
            string userVer = FormatString("%d", Exporter::mNifUserVersion);
            for (AppSettingsMap::iterator itr = TheAppSettings.begin(), end = TheAppSettings.end(); itr != end; ++itr)
               SendDlgItemMessage(hWnd, IDC_CB_GAME, CB_ADDSTRING, 0, LPARAM(itr->Name.c_str()));
            SendDlgItemMessage(hWnd, IDC_CB_GAME, CB_SELECTSTRING, WPARAM(-1), LPARAM(selection.c_str()));
            SendDlgItemMessage(hWnd, IDC_CB_VERSION, WM_SETTEXT, 0, LPARAM(version.c_str()));
            SendDlgItemMessage(hWnd, IDC_CB_USER_VERSION, WM_SETTEXT, 0, LPARAM(userVer.c_str()));

				TSTR tmp;
				tmp.printf("%.4f", Exporter::mWeldThresh);
				SetDlgItemText(hWnd, IDC_ED_WELDTHRESH, tmp);

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

						Exporter::mTriStrips = IsDlgButtonChecked(hWnd, IDC_CHK_STRIPS);
						Exporter::mExportHidden = IsDlgButtonChecked(hWnd, IDC_CHK_HIDDEN);
						Exporter::mExportFurn = IsDlgButtonChecked(hWnd, IDC_CHK_FURN);
						Exporter::mExportLights = IsDlgButtonChecked(hWnd, IDC_CHK_LIGHTS);
						Exporter::mVertexColors = IsDlgButtonChecked(hWnd, IDC_CHK_VCOLORS);
						Exporter::mExportCollision = IsDlgButtonChecked(hWnd, IDC_CHK_COLL);
						Exporter::mRemapIndices = IsDlgButtonChecked(hWnd, IDC_CHK_REMAP);

                  Exporter::mExportExtraNodes = IsDlgButtonChecked(hWnd, IDC_CHK_EXTRA);
                  Exporter::mExportSkin = IsDlgButtonChecked(hWnd, IDC_CHK_SKIN);
                  Exporter::mUserPropBuffer = IsDlgButtonChecked(hWnd, IDC_CHK_UPB);
                  Exporter::mFlattenHierarchy = IsDlgButtonChecked(hWnd, IDC_CHK_HIER);
                  Exporter::mRemoveUnreferencedBones = IsDlgButtonChecked(hWnd, IDC_CHK_REM_BONES);
                  Exporter::mSortNodesToEnd = IsDlgButtonChecked(hWnd, IDC_CHK_SORTNODES);
                  Exporter::mSkeletonOnly = IsDlgButtonChecked(hWnd, IDC_CHK_SKEL_ONLY);
                  Exporter::mExportCameras = IsDlgButtonChecked(hWnd, IDC_CHK_CAMERA);
                  Exporter::mGenerateBoneCollision = IsDlgButtonChecked(hWnd, IDC_CHK_BONE_COLL);
							
						GetDlgItemText(hWnd, IDC_ED_TEXPREFIX, tmp, MAX_PATH);
						Exporter::mTexPrefix = tmp;

						GetDlgItemText(hWnd, IDC_ED_WELDTHRESH, tmp, MAX_PATH);
						Exporter::mWeldThresh = atof(tmp);

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


//--- NifExport -------------------------------------------------------
NifExport::NifExport()
{

}

NifExport::~NifExport() 
{

}

int NifExport::ExtCount()
{
	return 1;
}

const TCHAR *NifExport::Ext(int n)
{		
	return _T("nif");
}

const TCHAR *NifExport::LongDesc()
{
	return _T("Gamebryo File");
}
	
const TCHAR *NifExport::ShortDesc() 
{			
	return _T("Gamebryo File");
}

const TCHAR *NifExport::AuthorName()
{			
	return _T("Alexander \"Gundalf\" Stillich / The Niftools Team");
}

const TCHAR *NifExport::CopyrightMessage() 
{	
	return _T("http://www.niftools.org");
}

const TCHAR *NifExport::OtherMessage1() 
{		
	return _T("http://www.niftools.org");
}

const TCHAR *NifExport::OtherMessage2() 
{		
	return _T("http://www.niftools.org");
}

unsigned int NifExport::Version()
{				
	return Exporter::mVersion;
}

void NifExport::ShowAbout(HWND hWnd)
{			

}

BOOL NifExport::SupportsOptions(int ext, DWORD options)
{
	return TRUE;
}


int	NifExport::DoExport(const TCHAR *name, ExpInterface *ei, Interface *i, BOOL suppressPrompts, DWORD options)
{
   // read application settings
   AppSettings::Initialize(i);

   TCHAR iniName[MAX_PATH];
   LPCTSTR pluginDir = i->GetDir(APP_PLUGCFG_DIR);
   PathCombine(iniName, pluginDir, "MaxNifTools.ini");
   bool iniNameIsValid = (-1 != _taccess(iniName, 0));

   // Set whether Config should use registry or not
   Exporter::mUseRegistry = !iniNameIsValid || GetIniValue<bool>(NifExportSection, "UseRegistry", false, iniName);
	// read config from registry
	Exporter::readConfig(i);
	// read config from root node
	Exporter::readConfig(i->GetRootNode());

   // locate the "default" app setting
   AppSettings *appSettings = NULL;
   if (iniNameIsValid)
   {
      string fname = name;
      // Locate which application to use. If Auto, find first app where this file appears in the root path list
      string curapp = GetIniValue<string>(NifExportSection, "CurrentApp", "AUTO", iniName);
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

	if(!suppressPrompts)
	{
      Exporter::mGameName = appSettings->Name;
      Exporter::mNifVersion = appSettings->NiVersion;
      Exporter::mNifUserVersion = appSettings->NiUserVersion;

		if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PANEL), GetActiveWindow(), NifExportOptionsDlgProc, (LPARAM)this) != IDOK)
			return true;

		// write config to registry
		Exporter::writeConfig(i);
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

	try
	{
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
		
		Ref<NiNode> root = DynamicCast<NiNode>(CreateBlock( "NiNode" ));
		Exporter::Result result = exp.doExport(root, i->GetRootNode());

		if (result!=Exporter::Ok)
			throw exception("Unknown error.");

		WriteNifTree(name, NiObjectRef(root), nifVersion, nifUserVer);
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

	return true;
}


