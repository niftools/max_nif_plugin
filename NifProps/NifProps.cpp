#pragma warning( disable:4800 )

#include <map>
#include "NifProps.h"
#include "NifStrings.h"
#include "../NifPlugins.h"
#include "../NifGui.h"

using namespace std;

#define NifProps_CLASS_ID	Class_ID(0x3f25f18d, 0x99c3b095)

class NifProps : public UtilityObj 
{

public:

	NifProps();
	~NifProps();		

	void			BeginEditParams(Interface *ip,IUtil *iu);
	void			EndEditParams(Interface *ip,IUtil *iu);
	void			SelectionSetChanged(Interface *ip,IUtil *iu);
	void			Init(HWND hWnd);
	void			Destroy(HWND hWnd);
	void			DeleteThis() { }

	void			selectionChanged();

	BOOL			dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
		
	HWND			mPanel;
	IUtil			*mIU;
	Interface		*mIP;
	INode			*mNode;

	typedef map<UINT, ISpinnerControl*> Spinners;
	Spinners		mSpins;

	NpComboBox		mCbLayer;
	NpComboBox		mCbMaterial;
	NpComboBox		mCbMotionSystem;
	NpComboBox		mCbQualityType;

	void enableGUI(BOOL object, BOOL hvk);		
};

static NifProps plugin;


class NifPropsClassDesc : public ClassDesc2
{
	public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return &plugin; }
	const TCHAR *	ClassName() { return GetString(IDS_CLASS_NAME); }
	SClass_ID		SuperClassID() { return UTILITY_CLASS_ID; }
	Class_ID		ClassID() { return NifProps_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("NifProps"); }
	HINSTANCE		HInstance() { return hInstance; }
	

};

static NifPropsClassDesc NifPropsDesc;
ClassDesc2* GetNifPropsDesc() { return &NifPropsDesc; }




static BOOL CALLBACK NifPropsDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return plugin.dlgProc(hWnd, msg, wParam, lParam);
}



//--- NifProps -------------------------------------------------------

NifProps::NifProps()
{
	mIU = NULL;
	mIP = NULL;	
	mPanel = NULL;
}

NifProps::~NifProps()
{

}

void NifProps::BeginEditParams(Interface *ip, IUtil *iu) 
{
	mIP = ip;
	mIU = iu;

	mPanel = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_PANEL),
		NifPropsDlgProc,
		GetString(IDS_PARAMS),
		0);

	mSpins[IDC_SP_CENTER_X] = 
		SetupFloatSpinner(mPanel, IDC_SP_CENTER_X, IDC_ED_CENTER_X, -9999, 9999, 0, .1f);
	mSpins[IDC_SP_CENTER_Y] = 
		SetupFloatSpinner(mPanel, IDC_SP_CENTER_Y, IDC_ED_CENTER_Y, -9999, 9999, 0, .1f);
	mSpins[IDC_SP_CENTER_Z] = 
		SetupFloatSpinner(mPanel, IDC_SP_CENTER_Z, IDC_ED_CENTER_Z, -9999, 9999, 0, .1f);

	mSpins[IDC_SP_MASS] = 
		SetupFloatSpinner(mPanel, IDC_SP_MASS, IDC_ED_MASS, 0, 1000, 0, .1f);
	mSpins[IDC_SP_FRICTION] = 
		SetupFloatSpinner(mPanel, IDC_SP_FRICTION, IDC_ED_FRICTION, 0, 10, 0, .1f);
	mSpins[IDC_SP_RESTITUTION] = 
		SetupFloatSpinner(mPanel, IDC_SP_RESTITUTION, IDC_ED_RESTITUTION, 0, 10, 0, .1f);

	mSpins[IDC_SP_LINEAR_DAMPING] = 
		SetupFloatSpinner(mPanel, IDC_SP_LINEAR_DAMPING, IDC_ED_LINEAR_DAMPING, 0, 10, 0, .1f);
	mSpins[IDC_SP_ANGULAR_DAMPING] = 
		SetupFloatSpinner(mPanel, IDC_SP_ANGULAR_DAMPING, IDC_ED_ANGULAR_DAMPING, 0, 10, 0, .1f);

	mSpins[IDC_SP_MAX_LINEAR_VELOCITY] = 
		SetupFloatSpinner(mPanel, IDC_SP_MAX_LINEAR_VELOCITY, IDC_ED_MAX_LINEAR_VELOCITY, 0, 10, 0, .1f);
	mSpins[IDC_SP_MAX_ANGULAR_VELOCITY] = 
		SetupFloatSpinner(mPanel, IDC_SP_MAX_ANGULAR_VELOCITY, IDC_ED_MAX_ANGULAR_VELOCITY, 0, 10, 0, .1f);

	mSpins[IDC_SP_PENETRATION_DEPTH] = 
		SetupFloatSpinner(mPanel, IDC_SP_PENETRATION_DEPTH, IDC_ED_PENETRATION_DEPTH, 0, 10, 0, .1f);

	mNode = NULL;

	const char **str;
	str = HvkMaterialNames;
	while (*str)
	{
		mCbMaterial.add(*str);
		str++;
	}

	str = HvkLayerNames;
	while (*str)
	{
		mCbLayer.add(*str);
		str++;
	}

	str = HvkMotionSystems;
	while (*str)
	{
		mCbMotionSystem.add(*str);
		str++;
	}

	str = HvkQualityTypes;
	while (*str)
	{
		mCbQualityType.add(*str);
		str++;
	}

	selectionChanged();
}
	
void NifProps::EndEditParams(Interface *ip,IUtil *iu) 
{
	Spinners::iterator it;
	for (it=mSpins.begin(); it!=mSpins.end(); ++it)
		ReleaseISpinner(it->second);

	mIU = NULL;
	mIP = NULL;
	ip->DeleteRollupPage(mPanel);
	mPanel = NULL;
	mNode = NULL;
}

void NifProps::enableGUI(BOOL obj, BOOL hvk)
{
	EnableWindow(GetDlgItem(mPanel, IDC_GRP_OBJECT), obj);
	EnableWindow(GetDlgItem(mPanel, IDC_CHK_ISCOLL), obj);

	for (int i=IDC_HVK_BEGIN; i<=IDC_HVK_END; i++)
		EnableWindow(GetDlgItem(mPanel, i), hvk);
}

void NifProps::SelectionSetChanged(Interface *ip, IUtil *iu)
{
	selectionChanged();
}

void NifProps::selectionChanged()
{
	int numSel = mIP->GetSelNodeCount();
	if (numSel <= 0)
	{
		mNode = NULL;
		enableGUI(false, false);
		return;
	}

	bool singleSel = numSel==1;
	INode *nodeSel = mIP->GetSelNode(0);
	bool isColl = npIsCollision(nodeSel);

	enableGUI(singleSel, singleSel && isColl);
	mNode = singleSel?nodeSel:NULL;		
	
	if (!singleSel)
		return;

	CheckDlgButton(mPanel, IDC_CHK_ISCOLL, isColl);
	
	int mtl, lyr, msys, qtype;
	if (!npGetProp(nodeSel, NP_HVK_MATERIAL, mtl))
	{
		mtl = NP_DEFAULT_HVK_MATERIAL;
		npSetProp(nodeSel, NP_HVK_MATERIAL, mtl);

	} else
		mtl = max(0, min(mCbMaterial.count()-1, mtl));

	if (!npGetProp(nodeSel, NP_HVK_LAYER, lyr))
	{
		lyr = NP_DEFAULT_HVK_LAYER;
		npSetProp(nodeSel, NP_HVK_LAYER, lyr);

	} else
		lyr = max(0, min(mCbLayer.count()-1, lyr));

	if (!npGetProp(nodeSel, NP_HVK_MOTION_SYSTEM, msys))
	{
		msys = NP_DEFAULT_HVK_MOTION_SYSTEM;
		npSetProp(nodeSel, NP_HVK_MOTION_SYSTEM, msys);

	} else
		msys = max(0, min(mCbMotionSystem.count()-1, msys));

	if (!npGetProp(nodeSel, NP_HVK_QUALITY_TYPE, qtype))
	{
		qtype = NP_DEFAULT_HVK_QUALITY_TYPE;
		npSetProp(nodeSel, NP_HVK_QUALITY_TYPE, qtype);

	} else
		qtype = max(0, min(mCbQualityType.count()-1, qtype));

	mCbMaterial.select(mtl);
	mCbLayer.select(lyr);
	mCbMotionSystem.select(msys);
	mCbQualityType.select(lyr);
}

void NifProps::Init(HWND hWnd)
{
	mCbLayer.init(GetDlgItem(hWnd, IDC_CB_LAYER));
	mCbMaterial.init(GetDlgItem(hWnd, IDC_CB_MATERIAL));
	mCbMotionSystem.init(GetDlgItem(hWnd, IDC_CB_MOTION_SYSTEM));
	mCbQualityType.init(GetDlgItem(hWnd, IDC_CB_QUALITY_TYPE));
}

void NifProps::Destroy(HWND hWnd)
{

}

BOOL NifProps::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) 
	{
		case WM_INITDIALOG:
			Init(hWnd);			
			break;

		case WM_DESTROY:
			Destroy(hWnd);
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_CHK_ISCOLL:
					npSetCollision(mNode, IsDlgButtonChecked(hWnd, IDC_CHK_ISCOLL));
					selectionChanged();
					break;

				case IDC_CB_MATERIAL:
					if (mNode && HIWORD(wParam)==CBN_SELCHANGE)
					{
						int sel = mCbMaterial.selection();
						npSetProp(mNode, NP_HVK_MATERIAL, sel);
					}
					break;

				case IDC_CB_LAYER:
					if (mNode && HIWORD(wParam)==CBN_SELCHANGE)
					{
						int sel = mCbLayer.selection();
						npSetProp(mNode, NP_HVK_LAYER, sel);
					}
					break;

				case IDC_CB_MOTION_SYSTEM:
					if (mNode && HIWORD(wParam)==CBN_SELCHANGE)
					{
						int sel = mCbMotionSystem.selection();
						npSetProp(mNode, NP_HVK_MOTION_SYSTEM, sel);
					}
					break;

				case IDC_CB_QUALITY_TYPE:
					if (mNode && HIWORD(wParam)==CBN_SELCHANGE)
					{
						int sel = mCbQualityType.selection();
						npSetProp(mNode, NP_HVK_QUALITY_TYPE, sel);
					}
					break;
			}
			break;


		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			mIP->RollupMouseMessage(hWnd, msg, wParam, lParam); 
			break;

		default:
			return FALSE;
	}

	return TRUE;
}