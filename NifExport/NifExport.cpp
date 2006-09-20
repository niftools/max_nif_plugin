#include "pch.h"
#include "AppSettings.h"
#include "niutils.h"
#include  <io.h>
#include "obj/NiControllerManager.h"
#include "obj/NiTimeController.h"
#include "obj/NiControllerSequence.h"

using namespace Niflib;

#define NifExport_CLASS_ID	Class_ID(0xa57ff0a4, 0xa0374ffb)

LPCTSTR NifExportSection = TEXT("MaxNifExport");

class NifExport : public SceneExport 
{
public:
		
	static HWND		hParams;
	int				mDlgResult;
   TSTR iniFileName;
   TSTR shortDescription;
   TSTR fileVersion;
		
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
	const TCHAR		*ClassName() { return GetString(IDS_NIF_CLASS_NAME); }
	SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID() { return NifExport_CLASS_ID; }
	const TCHAR		*Category() { return GetString(IDS_CATEGORY); }

	const TCHAR		*InternalName() { return _T("NifExport"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
	

};

static NifExportClassDesc NifExportDesc;
ClassDesc2* GetNifExportDesc() { return &NifExportDesc; }

extern list<NiObjectRef> GetAllObjectsByType( NiObjectRef const & root, const Type & type );



BOOL CALLBACK NifExportOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
	static NifExport *imp = NULL;

	switch (message) 
	{
		case WM_INITDIALOG:
			{
            imp = (NifExport *)lParam;

            // Append file version to dialog
            if (!imp->fileVersion.isNull()) {
               char buffer[256];
               GetWindowText(hWnd, buffer, _countof(buffer));
               _tcscat(buffer, TEXT(" "));
               _tcscat(buffer, imp->fileVersion);
               SetWindowText(hWnd, buffer);
            }

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
            CheckDlgButton(hWnd, IDC_CHK_AUTO_DETECT, Exporter::mAutoDetect);

            // Populate Type options
            SendDlgItemMessage(hWnd, IDC_CBO_ANIM_TYPE, CB_ADDSTRING, Exporter::NIF_WO_ANIM, LPARAM("NIF w/o Anim"));
            SendDlgItemMessage(hWnd, IDC_CBO_ANIM_TYPE, CB_ADDSTRING, Exporter::NIF_WO_KF, LPARAM("NIF with Anim"));
            SendDlgItemMessage(hWnd, IDC_CBO_ANIM_TYPE, CB_ADDSTRING, Exporter::SINGLE_KF_WITH_NIF, LPARAM("Single KF with NIF"));
            SendDlgItemMessage(hWnd, IDC_CBO_ANIM_TYPE, CB_ADDSTRING, Exporter::SINGLE_KF_WO_NIF, LPARAM("Single KF w/o NIF"));
            SendDlgItemMessage(hWnd, IDC_CBO_ANIM_TYPE, CB_ADDSTRING, Exporter::MULTI_KF_WITH_NIF, LPARAM("Multi KF with NIF"));
            SendDlgItemMessage(hWnd, IDC_CBO_ANIM_TYPE, CB_ADDSTRING, Exporter::MULTI_KF_WO_NIF, LPARAM("Multi KF w/o NIF"));
            SendDlgItemMessage(hWnd, IDC_CBO_ANIM_TYPE, CB_ADDSTRING, Exporter::NIF_WITH_MGR, LPARAM("NIF w/ Manager"));
            
            SendDlgItemMessage(hWnd, IDC_CBO_ANIM_TYPE, CB_SETCURSEL, WPARAM(Exporter::mExportType), 0);

            CheckDlgButton(hWnd, IDC_CHK_TRANSFORMS2, Exporter::mExportTransforms);
            SetDlgItemText(hWnd, IDC_ED_PRIORITY2, FormatText("%.1f", Exporter::mDefaultPriority));
            CheckDlgButton(hWnd, IDC_CHK_USE_TIME_TAGS, Exporter::mUseTimeTags);           

            // Skin
            CheckDlgButton(hWnd, IDC_CHK_SKIN, Exporter::mExportSkin);
            CheckDlgButton(hWnd, IDC_CHK_SKINPART, Exporter::mMultiplePartitions);
            SetDlgItemText(hWnd, IDC_ED_BONES_PART, FormatText("%d", Exporter::mBonesPerPartition));
            SetDlgItemText(hWnd, IDC_ED_BONES_VERTEX, FormatText("%d", Exporter::mBonesPerVertex));

            CheckDlgButton(hWnd, IDC_CHK_ALLOW_ACCUM, Exporter::mAllowAccum);

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
                  Exporter::mUserPropBuffer = IsDlgButtonChecked(hWnd, IDC_CHK_UPB);
                  Exporter::mFlattenHierarchy = IsDlgButtonChecked(hWnd, IDC_CHK_HIER);
                  Exporter::mRemoveUnreferencedBones = IsDlgButtonChecked(hWnd, IDC_CHK_REM_BONES);
                  Exporter::mSortNodesToEnd = IsDlgButtonChecked(hWnd, IDC_CHK_SORTNODES);
                  Exporter::mSkeletonOnly = IsDlgButtonChecked(hWnd, IDC_CHK_SKEL_ONLY);
                  Exporter::mExportCameras = IsDlgButtonChecked(hWnd, IDC_CHK_CAMERA);
                  Exporter::mGenerateBoneCollision = IsDlgButtonChecked(hWnd, IDC_CHK_BONE_COLL);

                  Exporter::mExportTransforms = IsDlgButtonChecked(hWnd, IDC_CHK_TRANSFORMS2);
                  Exporter::mUseTimeTags = IsDlgButtonChecked(hWnd, IDC_CHK_USE_TIME_TAGS);           

                  Exporter::mExportType = Exporter::ExportType(SendDlgItemMessage(hWnd, IDC_CBO_ANIM_TYPE, CB_GETCURSEL, 0, 0));
                  GetDlgItemText(hWnd, IDC_ED_PRIORITY2, tmp, MAX_PATH);
                  Exporter::mDefaultPriority = atof(tmp);

						GetDlgItemText(hWnd, IDC_ED_TEXPREFIX, tmp, MAX_PATH);
						Exporter::mTexPrefix = tmp;

						GetDlgItemText(hWnd, IDC_ED_WELDTHRESH, tmp, MAX_PATH);
						Exporter::mWeldThresh = atof(tmp);

                  Exporter::mAllowAccum = IsDlgButtonChecked(hWnd, IDC_CHK_ALLOW_ACCUM);

                  // Skin
                  Exporter::mExportSkin = IsDlgButtonChecked(hWnd, IDC_CHK_SKIN);
                  Exporter::mMultiplePartitions = IsDlgButtonChecked(hWnd, IDC_CHK_SKINPART);
                  GetDlgItemText(hWnd, IDC_ED_BONES_PART, tmp, MAX_PATH);
                  Exporter::mBonesPerPartition = atoi(tmp);
                  GetDlgItemText(hWnd, IDC_ED_BONES_VERTEX, tmp, MAX_PATH);
                  Exporter::mBonesPerVertex = atoi(tmp);

                  GetDlgItemText(hWnd, IDC_CB_GAME, tmp, MAX_PATH);
                  if (AppSettings *appSettings = FindAppSetting(tmp))
                  {
                     Exporter::mGameName = appSettings->Name;
                     GetDlgItemText(hWnd, IDC_CB_VERSION, tmp, MAX_PATH);
                     Exporter::mNifVersion = tmp;
                     GetDlgItemText(hWnd, IDC_CB_USER_VERSION, tmp, MAX_PATH);
                     Exporter::mNifUserVersion = strtol(tmp, &end, 0);
                  }
                  Exporter::mAutoDetect = IsDlgButtonChecked(hWnd, IDC_CHK_AUTO_DETECT);

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
   fileVersion = GetFileVersion(NULL);
}

NifExport::~NifExport() 
{

}

int NifExport::ExtCount()
{
	return 2;
}

const TCHAR *NifExport::Ext(int n)
{		
   switch (n)
   {
   case 0: return _T("KF");
   case 1: return _T("NIF");
   }
	return NULL;
}

const TCHAR *NifExport::LongDesc()
{
	return _T("Gamebryo File");
}
	
const TCHAR *NifExport::ShortDesc() 
{			
	return shortDescription;
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
	try
	{
      TCHAR path[MAX_PATH];
      GetFullPathName(name, MAX_PATH, path, NULL);
      PathRenameExtension(path, ".nif");

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
         string fname = path;
         // Locate which application to use. If Auto, find first app where this file appears in the root path list
         string curapp = GetIniValue<string>(NifExportSection, "CurrentApp", "AUTO", iniName);
         if (0 == _tcsicmp(curapp.c_str(), "AUTO")) {
            Exporter::mAutoDetect = true;
            // Scan Root paths
            for (AppSettingsMap::iterator itr = TheAppSettings.begin(), end = TheAppSettings.end(); itr != end; ++itr){
               if ((*itr).IsFileInRootPaths(fname)) {
                  appSettings = &(*itr);
                  break;
               }
            }
         } else {
            Exporter::mAutoDetect = false;
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
         if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_NIF_PANEL), GetActiveWindow(), NifExportOptionsDlgProc, (LPARAM)this) != IDOK)
            return true;

         // write config to registry
         Exporter::writeConfig(i);
         // write config to root node
         Exporter::writeConfig(i->GetRootNode());

         // Update the current app version
         appSettings = FindAppSetting(Exporter::mGameName);
         if (appSettings == NULL && !TheAppSettings.empty())
            appSettings = &TheAppSettings.front();

         if (Exporter::mAutoDetect){
            SetIniValue<string>(NifExportSection, "CurrentApp", "AUTO", iniName);
         } else {
            SetIniValue<string>(NifExportSection, "CurrentApp", appSettings->Name, iniName);
         }

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
      Exporter::mNifVersionInt = nifVersion;

      Exporter::ExportType exportType = Exporter::mExportType;

      // Hack so MW exports cleaner. Basically write tree without NiControllerManager
      if (  nifVersion <= VER_10_0_1_0 
         && (Exporter::mExportType != Exporter::NIF_WO_ANIM && Exporter::mExportType != Exporter::NIF_WITH_MGR)
         )
      {
         Exporter::mExportType = Exporter::NIF_WO_KF;
      }
      Niflib::NifInfo info(nifVersion, nifUserVer, nifUserVer);
      info.creator = Exporter::mCreatorName;
      info.exportInfo1 = "Niflib";
      info.exportInfo2 = FormatText("Niftools Max Plugins %s", fileVersion.data());

		Exporter::mSelectedOnly = (options&SCENE_EXPORT_SELECTED) != 0;
		Exporter exp(i, appSettings);
		
		Ref<NiNode> root = DynamicCast<NiNode>(CreateBlock( "NiNode" ));
		Exporter::Result result = exp.doExport(root, i->GetRootNode());

		if (result!=Exporter::Ok)
			throw exception("Unknown error.");

      if (exportType == Exporter::NIF_WO_ANIM || exportType == Exporter::NIF_WITH_MGR)
      {
         WriteNifTree(path, NiObjectRef(root), info);
      }
      else 
      {
         Niflib::ExportOptions export_type = EXPORT_NIF;
         switch (exportType) {
            case Exporter::SINGLE_KF_WITH_NIF: export_type = EXPORT_NIF_KF;       break;
            case Exporter::SINGLE_KF_WO_NIF:   export_type = EXPORT_KF;           break;
            case Exporter::MULTI_KF_WITH_NIF:  export_type = EXPORT_NIF_KF_MULTI; break;
            case Exporter::MULTI_KF_WO_NIF:    export_type = EXPORT_KF_MULTI;     break;
         }

         Niflib::NifGame game = KF_MW;
         if (nifVersion <= VER_4_0_0_2) {
            game = KF_MW;
         } else if (nifVersion <= VER_20_0_0_4) {
            game = KF_DAOC;
         } else {
            game = KF_CIV4;
         }

         // 
         //if (nifVersion <= VER_10_0_1_0) {

         //   // Now search and locate newer timeframe controllers and convert to keyframecontrollers
         //   list<NiObjectRef> mgrs = GetAllObjectsByType( root, NiControllerManager::TypeConst() );
         //   for ( list<NiObjectRef>::iterator it = mgrs.begin(); it != mgrs.end(); ++it) {
         //      if (NiControllerManagerRef mgr = DynamicCast<NiControllerManager>(*it)) {
         //         NiObjectNETRef target = mgr->GetTarget();
         //         target->RemoveController( StaticCast<NiTimeController>(mgr) );
         //         vector<NiControllerSequenceRef> seqs = mgr->GetControllerSequences();
         //         for (vector<NiControllerSequenceRef>::iterator itr = seqs.begin(); itr != seqs.end(); ++itr) {
         //            NiControllerSequenceRef seq = (*itr);
         //            MergeNifTrees(DynamicCast<NiNode>(target), seq, nifVersion, nifUserVer);
         //         }
         //      }
         //   }
         //}

         WriteFileGroup(path, StaticCast<NiObject>(root), info, export_type, game);
/*
         for (NodeList::iterator itr = mAnimationRoots.begin(); itr != mAnimationRoots.end(); ++itr){
            list<NiTimeControllerRef> ctlrs = (*itr)->GetControllers();
            if ( NiControllerManagerRef mgr = SelectFirstObjectOfType<NiControllerManager>( ctlrs ) ) {
               (*itr)->RemoveController( mgr );

               vector<NiControllerSequenceRef> seqs = mgr->GetControllerSequences();
               WriteNifTree()
            }
         }

         if (mExportType == SINGLE_KF_WITH_NIF || mExportType == MULTI_KF_WITH_NIF)
         {
            GetFullPathName(name, MAX_PATH, path, NULL);
            PathRenameExtension(path, ".nif");
            WriteNifTree(path, NiObjectRef(root), nifVersion, nifUserVer);
         }
*/
      }
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


