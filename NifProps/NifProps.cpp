#pragma warning( disable:4800 )

#include <map>
#include "NifProps.h"
#include "NifStrings.h"
#include "NifPlugins.h"
#include "NifGui.h"

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

   INT_PTR			dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
   INT_PTR			dlgAnimProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
		
   HWND			mPanel;
   HWND			mAnimPanel;
	IUtil			*mIU;
	Interface		*mIP;
	INode			*mNode;

	typedef map<UINT, ISpinnerControl*> Spinners;
	Spinners		mSpins;

	NpComboBox		mCbLayer;
	NpComboBox		mCbMaterial;
	NpComboBox		mCbMotionSystem;
	NpComboBox		mCbQualityType;

	void enableGUI(BOOL object, BOOL hvk, BOOL anim);		
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




static INT_PTR CALLBACK NifPropsDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   return plugin.dlgProc(hWnd, msg, wParam, lParam);
}

static INT_PTR CALLBACK NifAnimPropsDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   return plugin.dlgAnimProc(hWnd, msg, wParam, lParam);
}



//--- NifProps -------------------------------------------------------

NifProps::NifProps()
{
	mIU = NULL;
	mIP = NULL;	
	mPanel = NULL;
   mAnimPanel = NULL;
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
	str = NpHvkMaterialNames;
	while (*str)
	{
		mCbMaterial.add(*str);
		str++;
	}

	str = NpHvkLayerNames;
	while (*str)
	{
		mCbLayer.add(*str);
		str++;
	}

	str = NpHvkMotionSystems;
	while (*str)
	{
		mCbMotionSystem.add(*str);
		str++;
	}

	str = NpHvkQualityTypes;
	while (*str)
	{
		mCbQualityType.add(*str);
		str++;
	}

   
   //mAnimPanel = ip->AddRollupPage(
   //   hInstance,
   //   MAKEINTRESOURCE(IDD_ANIM_PANEL),
   //   NifAnimPropsDlgProc,
   //   GetString(IDS_ANIM_PARAMS),
   //   0);

   mSpins[IDC_SP_ANIM_PRIORITY] = 
      SetupFloatSpinner(mPanel, IDC_SP_ANIM_PRIORITY, IDC_ED_ANIM_PRIORITY, 0, 255, 25, .1f);

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

void NifProps::enableGUI(BOOL obj, BOOL hvk, BOOL anim)
{
	EnableWindow(GetDlgItem(mPanel, IDC_GRP_OBJECT), obj);
	EnableWindow(GetDlgItem(mPanel, IDC_CHK_ISCOLL), obj);

/*	EnableWindow(GetDlgItem(mPanel, IDC_GRP_HAVOK), hvk);
	EnableWindow(GetDlgItem(mPanel, IDC_LBL_MATERIAL), hvk);
	EnableWindow(GetDlgItem(mPanel, IDC_CB_MATERIAL), hvk);
	EnableWindow(GetDlgItem(mPanel, IDC_LBL_LAYER), hvk);
	EnableWindow(GetDlgItem(mPanel, IDC_CB_LAYER), hvk);
*/
   for (int i=IDC_HVK_BEGIN; i<=IDC_HVK_END; i++)
      EnableWindow(GetDlgItem(mPanel, i), hvk);

   for (int i=IDC_ANIM_BEGIN; i<=IDC_ANIM_END; i++)
      EnableWindow(GetDlgItem(mPanel, i), anim);
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
		enableGUI(false, false, false);
		return;
	}

	bool singleSel = numSel==1;
	INode *nodeSel = mIP->GetSelNode(0);
	bool isColl = npIsCollision(nodeSel);

	enableGUI(singleSel, singleSel && isColl, true);
	mNode = singleSel?nodeSel:NULL;		
	
	if (!singleSel)
		return;

	CheckDlgButton(mPanel, IDC_CHK_ISCOLL, isColl);
	
	int mtl, lyr, msys, qtype;
	float mass, lindamp, angdamp, frict, resti, maxlinvel, maxangvel;
	Vector3 center;

   // Handle compatibility
   npGetProp(nodeSel, NP_HVK_MASS_OLD, mass, NP_DEFAULT_HVK_EMPTY);
   if (mass != NP_DEFAULT_HVK_EMPTY)
      npGetProp(nodeSel, NP_HVK_MASS, mass, NP_DEFAULT_HVK_MASS);
   npGetProp(nodeSel, NP_HVK_FRICTION_OLD, frict, NP_DEFAULT_HVK_EMPTY);
   if (frict != NP_DEFAULT_HVK_EMPTY)
      npGetProp(nodeSel, NP_HVK_FRICTION, frict, NP_DEFAULT_HVK_FRICTION);
   npGetProp(nodeSel, NP_HVK_RESTITUTION_OLD, resti, NP_DEFAULT_HVK_EMPTY);
   if (resti != NP_DEFAULT_HVK_EMPTY)
      npGetProp(nodeSel, NP_HVK_RESTITUTION, resti, NP_DEFAULT_HVK_RESTITUTION);

	npGetProp(nodeSel, NP_HVK_MATERIAL, mtl, NP_DEFAULT_HVK_MATERIAL);
	npGetProp(nodeSel, NP_HVK_LAYER, lyr, NP_DEFAULT_HVK_LAYER);
	npGetProp(nodeSel, NP_HVK_MOTION_SYSTEM, msys, NP_DEFAULT_HVK_MOTION_SYSTEM);
	npGetProp(nodeSel, NP_HVK_QUALITY_TYPE, qtype, NP_DEFAULT_HVK_QUALITY_TYPE);
	npGetProp(nodeSel, NP_HVK_LINEAR_DAMPING, lindamp, NP_DEFAULT_HVK_LINEAR_DAMPING);
	npGetProp(nodeSel, NP_HVK_ANGULAR_DAMPING, angdamp, NP_DEFAULT_HVK_ANGULAR_DAMPING);
	npGetProp(nodeSel, NP_HVK_MAX_LINEAR_VELOCITY, maxlinvel, NP_DEFAULT_HVK_FRICTION);
	npGetProp(nodeSel, NP_HVK_MAX_ANGULAR_VELOCITY, maxangvel, NP_DEFAULT_HVK_MAX_ANGULAR_VELOCITY);
	npGetProp(nodeSel, NP_HVK_CENTER, center);

	mCbMaterial.select(max(0, min(mtl, mCbMaterial.count()-1)));
	mCbLayer.select(max(0, min(lyr, mCbLayer.count()-1)));
	mCbMotionSystem.select(max(0, min(msys, mCbMotionSystem.count()-1)));
	mCbQualityType.select(max(0, min(qtype, mCbQualityType.count()-1)));

	//mSpins[IDC_SP_CENTER_X]->SetValue(center.x, TRUE);
	//mSpins[IDC_SP_CENTER_Y]->SetValue(center.y, TRUE);
	//mSpins[IDC_SP_CENTER_Z]->SetValue(center.z, TRUE);

	mSpins[IDC_SP_MASS]->SetValue(mass, TRUE);
	mSpins[IDC_SP_LINEAR_DAMPING]->SetValue(lindamp, TRUE);
	mSpins[IDC_SP_ANGULAR_DAMPING]->SetValue(angdamp, TRUE);
	mSpins[IDC_SP_FRICTION]->SetValue(frict, TRUE);
	mSpins[IDC_SP_MAX_LINEAR_VELOCITY]->SetValue(maxlinvel, TRUE);
	mSpins[IDC_SP_MAX_ANGULAR_VELOCITY]->SetValue(maxangvel, TRUE);

   float priority;
   npGetProp(nodeSel, NP_ANM_PRI, priority, NP_DEFAULT_ANM_PRI);
   mSpins[IDC_SP_ANIM_PRIORITY]->SetValue(priority, TRUE);

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

INT_PTR NifProps::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Vector3 center;

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

		case CC_SPINNER_CHANGE:
			if (!mNode)
				break;

			switch (LOWORD(wParam))
			{
				case IDC_SP_CENTER_X:
					npGetProp(mNode, NP_HVK_CENTER, center);
					center.x = mSpins[IDC_SP_CENTER_X]->GetFVal();
					npSetProp(mNode, NP_HVK_CENTER, center);
					break;
				case IDC_SP_CENTER_Y:
					npGetProp(mNode, NP_HVK_CENTER, center);
					center.y = mSpins[IDC_SP_CENTER_Y]->GetFVal();
					npSetProp(mNode, NP_HVK_CENTER, center);
					break;
				case IDC_SP_CENTER_Z:
					npGetProp(mNode, NP_HVK_CENTER, center);
					center.z = mSpins[IDC_SP_CENTER_Z]->GetFVal();
					npSetProp(mNode, NP_HVK_CENTER, center);
					break;

				case IDC_SP_MASS:
					npSetProp(mNode, NP_HVK_MASS, mSpins[IDC_SP_MASS]->GetFVal());
					break;
				case IDC_SP_FRICTION:
					npSetProp(mNode, NP_HVK_FRICTION, mSpins[IDC_SP_FRICTION]->GetFVal());
					break;
				case IDC_SP_RESTITUTION:
					npSetProp(mNode, NP_HVK_RESTITUTION, mSpins[IDC_SP_RESTITUTION]->GetFVal());
					break;

				case IDC_SP_LINEAR_DAMPING:
					npSetProp(mNode, NP_HVK_LINEAR_DAMPING, mSpins[IDC_SP_LINEAR_DAMPING]->GetFVal());
					break;
				case IDC_SP_ANGULAR_DAMPING:
					npSetProp(mNode, NP_HVK_ANGULAR_DAMPING, mSpins[IDC_SP_ANGULAR_DAMPING]->GetFVal());
					break;

				case IDC_SP_MAX_LINEAR_VELOCITY:
					npSetProp(mNode, NP_HVK_MAX_LINEAR_VELOCITY, mSpins[IDC_SP_MAX_LINEAR_VELOCITY]->GetFVal());
					break;
				case IDC_SP_MAX_ANGULAR_VELOCITY:
					npSetProp(mNode, NP_HVK_MAX_ANGULAR_VELOCITY, mSpins[IDC_SP_MAX_ANGULAR_VELOCITY]->GetFVal());
					break;

				case IDC_SP_PENETRATION_DEPTH:
					npSetProp(mNode, NP_HVK_PENETRATION_DEPTH, mSpins[IDC_SP_PENETRATION_DEPTH]->GetFVal());
					break;

            case IDC_SP_ANIM_PRIORITY:
               for (int i=0, n=mIP->GetSelNodeCount(); i<n; ++i) {
                  npSetProp(mIP->GetSelNode(i), NP_ANM_PRI, mSpins[IDC_SP_ANIM_PRIORITY]->GetFVal());
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