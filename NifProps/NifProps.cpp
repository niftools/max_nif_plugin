#pragma warning( disable:4800 )

#include "NifProps.h"
#include "NifStrings.h"
#include "../NifPlugins.h"
#include "../NifGui.h"

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

	NpComboBox		mCbLayer;
	NpComboBox		mCbMaterial;

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

	selectionChanged();
}
	
void NifProps::EndEditParams(Interface *ip,IUtil *iu) 
{
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

	EnableWindow(GetDlgItem(mPanel, IDC_GRP_HAVOK), hvk);
	EnableWindow(GetDlgItem(mPanel, IDC_LBL_MATERIAL), hvk);
	EnableWindow(GetDlgItem(mPanel, IDC_CB_MATERIAL), hvk);
	EnableWindow(GetDlgItem(mPanel, IDC_LBL_LAYER), hvk);
	EnableWindow(GetDlgItem(mPanel, IDC_CB_LAYER), hvk);
	EnableWindow(GetDlgItem(mPanel, IDC_BTN_APPLY), hvk);
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
	bool isColl = isCollision(nodeSel);

	enableGUI(singleSel, singleSel && isColl);
	mNode = singleSel?nodeSel:NULL;		
	
	if (!singleSel)
		return;

	CheckDlgButton(mPanel, IDC_CHK_ISCOLL, isColl);
	
	int mtl, lyr;
	if (!getHvkMaterial(nodeSel, mtl))
	{
		mtl = NP_DEFAULT_HVK_MATERIAL;
		setHvkMaterial(nodeSel, mtl);

	} else
		mtl = max(0, min(mCbMaterial.count()-1, mtl));

	if (!getHvkLayer(nodeSel, lyr))
	{
		lyr = NP_DEFAULT_HVK_LAYER;
		setHvkLayer(nodeSel, lyr);

	} else
		lyr = max(0, min(mCbLayer.count()-1, lyr));

	mCbMaterial.select(mtl);
	mCbLayer.select(lyr);
}

void NifProps::Init(HWND hWnd)
{
	mCbLayer.init(GetDlgItem(hWnd, IDC_CB_LAYER));
	mCbMaterial.init(GetDlgItem(hWnd, IDC_CB_MATERIAL));
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
					setCollision(mNode, IsDlgButtonChecked(hWnd, IDC_CHK_ISCOLL));
					selectionChanged();
					break;

				case IDC_CB_MATERIAL:
					if (mNode && HIWORD(wParam)==CBN_SELCHANGE)
					{
						int sel = mCbMaterial.selection();
						setHvkMaterial(mNode, sel);
					}
					break;

				case IDC_CB_LAYER:
					if (mNode && HIWORD(wParam)==CBN_SELCHANGE)
					{
						int sel = mCbLayer.selection();
						setHvkLayer(mNode, sel);
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