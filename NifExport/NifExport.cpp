#include "pch.h"

using namespace Niflib;

#define NifExport_CLASS_ID	Class_ID(0xa57ff0a4, 0xa0374ffb)

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
				imp = (NifExport *)lParam;
				CenterWindow(hWnd,GetParent(hWnd));
				CheckDlgButton(hWnd, IDC_CHK_STRIPS, Exporter::mTriStrips);
				CheckDlgButton(hWnd, IDC_CHK_HIDDEN, Exporter::mExportHidden);
				CheckDlgButton(hWnd, IDC_CHK_FURN, Exporter::mExportFurn);
				CheckDlgButton(hWnd, IDC_CHK_LIGHTS, Exporter::mExportLights);
				CheckDlgButton(hWnd, IDC_CHK_VCOLORS, Exporter::mVertexColors);
				SetDlgItemText(hWnd, IDC_ED_TEXPREFIX, Exporter::mTexPrefix.c_str());
				CheckDlgButton(hWnd, IDC_CHK_COLL, Exporter::mExportCollision);

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
				char tmp[MAX_PATH];
				bool close = false;
				switch (LOWORD(wParam))
				{
					case IDOK:
						Exporter::mTriStrips = IsDlgButtonChecked(hWnd, IDC_CHK_STRIPS);
						Exporter::mExportHidden = IsDlgButtonChecked(hWnd, IDC_CHK_HIDDEN);
						Exporter::mExportFurn = IsDlgButtonChecked(hWnd, IDC_CHK_FURN);
						Exporter::mExportLights = IsDlgButtonChecked(hWnd, IDC_CHK_LIGHTS);
						Exporter::mVertexColors = IsDlgButtonChecked(hWnd, IDC_CHK_VCOLORS);
						Exporter::mExportCollision = IsDlgButtonChecked(hWnd, IDC_CHK_COLL);
							
						GetDlgItemText(hWnd, IDC_ED_TEXPREFIX, tmp, MAX_PATH);
						Exporter::mTexPrefix = tmp;

						GetDlgItemText(hWnd, IDC_ED_WELDTHRESH, tmp, MAX_PATH);
						Exporter::mWeldThresh = atof(tmp);

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

			if (HIWORD(wParam) == STN_CLICKED)
			{
				if (LOWORD(wParam) == IDC_LBL_LINK)
				{
					ShellExecute(hWnd, "open", "http://niftools.sourceforge.net",
						         NULL, NULL, SW_SHOWDEFAULT);
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
	return _T("Gambryo File");
}
	
const TCHAR *NifExport::ShortDesc() 
{			
	return _T("Gambryo File");
}

const TCHAR *NifExport::AuthorName()
{			
	return _T("Alexander Stillich aka Gundalf / The Niftools Team");
}

const TCHAR *NifExport::CopyrightMessage() 
{	
	return _T("http://niftools.sourceforge.net");
}

const TCHAR *NifExport::OtherMessage1() 
{		
	return _T("http://niftools.sourceforge.net");
}

const TCHAR *NifExport::OtherMessage2() 
{		
	return _T("http://niftools.sourceforge.net");
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
	// read config from registry
	Exporter::readConfig();
	// read config from root node
	Exporter::readConfig(i->GetRootNode());

	if(!suppressPrompts)
	{
		if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PANEL), GetActiveWindow(), NifExportOptionsDlgProc, (LPARAM)this) != IDOK)
			return true;


		// write config to registry
		Exporter::writeConfig();
		// write config to root node
		Exporter::writeConfig(i->GetRootNode());
	}

	try
	{
		Exporter::mSelectedOnly = (options&SCENE_EXPORT_SELECTED) != 0;
		Exporter exp(i);
		
		Ref<NiNode> root = DynamicCast<NiNode>(CreateBlock( "NiNode" ));
		Exporter::Result result = exp.export(root, i->GetRootNode());

		if (result!=Exporter::Ok)
			throw exception("Unknown error.");

		WriteNifTree("d:\\test.nif", NiObjectRef(root), VER_20_0_0_5, 11);
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


