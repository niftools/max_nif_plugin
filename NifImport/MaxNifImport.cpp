/**********************************************************************
 *<
	FILE: MaxNifImport.cpp

	DESCRIPTION:	Appwizard generated plugin

	CREATED BY: tazpn (Theo)

	HISTORY: 

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/
#include "stdafx.h"
#include <math.h>
#include <io.h>
#include <sstream>
#include "MaxNifImport.h"
#include <cs/Biped8Api.h>
#include <scenetraversal.h> 
#include <plugapi.h>
#include <triobj.h> 
#include <bitmap.h>
#include <modstack.h>
#include <iskin.h>
#include <strclass.h>
#include "objectParams.h"
#undef ALPHA_NONE

#include <math.h>
#include <string.h>

#include "niflib.h"
#include "obj\NiObject.h"
#include "obj\NiNode.h"
#include "obj\NiTriShape.h"
#include "obj\NiTriShapeData.h"
#include "obj\NiTriStrips.h"
#include "obj\NiTriStripsData.h"
#include "obj\NiMaterialProperty.h"
#include "obj\NiTexturingProperty.h"
#include "obj\NiSourceTexture.h"
#include "obj\NiExtraData.h"
#include "obj\BSBound.h"
#include "obj\NiSkinData.h"
#include "obj\NiSkinInstance.h"
#include "obj\NiSkinPartition.h"

#include "niutils.h"
#include "AppSettings.h"

#ifndef ASSERT
#ifdef _DEBUG
#include <crtdbg.h>
#define ASSERT _ASSERTE
#else
#define ASSERT(exp)
#endif
#endif

using namespace Niflib;

#define MaxNifImport_CLASS_ID	Class_ID(0x794ac1c1, 0x8b4c64c7)

// Define the standard section names used in the ini file
LPCTSTR NifImportSection = TEXT("MaxNifImport");
LPCTSTR SystemSection = TEXT("System");
LPCTSTR BipedImportSection = TEXT("BipedImport");

struct NiNodeNameEquivalence : public NumericStringEquivalence
{
   bool operator()(const NiNodeRef& n1, const NiNodeRef& n2) const { 
      return NumericStringEquivalence::operator()(n1->GetName(), n2->GetName());
   }
};

// NIF Importer
class NifImporter
{
public:
   string name;
   string path;
   ImpInterface *i;
   Interface *gi;
   BOOL suppressPrompts;
   bool iniFileValid;
   string iniFileName;
   AppSettings *appSettings;

   // Ini settings
   bool showTextures; // show textures in viewport
   bool removeIllegalFaces;
   bool removeDegenerateFaces;
   bool enableAutoSmooth;
   float autoSmoothAngle;
   bool flipUVTextures;
   bool enableSkinSupport;
   bool goToSkeletonBindPosition;

   // Biped/Bones related settings
   string skeleton;
   float bipedHeight;
   string skeletonCheck;
   float bipedAngle;
   float bipedAnkleAttach;
   bool bipedTrianglePelvis;
   bool importSkeleton;
   bool useBiped;
   bool hasSkeleton;
   bool isBiped;
   bool removeUnusedImportedBones;
   bool forceRotation;
   bool browseForSkeleton;
   string defaultSkeletonName;

   float minBoneWidth;
   float maxBoneWidth;
   float boneWidthToLengthRatio;

   vector<NiObjectRef> blocks;
   vector<NiNodeRef> nodes;

   NifImporter(const TCHAR *Name,ImpInterface *I,Interface *GI, BOOL SuppressPrompts)
      : name(Name), i(I), gi(GI), suppressPrompts(SuppressPrompts)
   {
      char buffer[MAX_PATH] = {0}, *p = NULL;
      GetFullPathName(Name, _countof(buffer), buffer, &p);
      if (p) *p = 0;
      path = buffer;

      // Load ini settings
      iniFileValid = false;
      LoadIniSettings();

      // load file
      blocks = ReadNifList( name );
      nodes = DynamicCast<NiNode>(blocks);

      // Apply post processing checks after reading blocks
      if (isValid()){
         if (goToSkeletonBindPosition && !nodes.empty())
            nodes[0]->GoToSkeletonBindPosition();

         hasSkeleton = HasSkeleton();
         isBiped = IsBiped();
         skeleton = (appSettings != NULL) ? appSettings->Skeleton : "";
         importSkeleton &= hasSkeleton;

         // Guess that the skeleton is the same one in the current directory
         if (importSkeleton && !defaultSkeletonName.empty()) {
            TCHAR buffer[MAX_PATH];
            GetFullPathName(name.c_str(), _countof(buffer), buffer, NULL);
            PathRemoveFileSpec(buffer);
            PathAddBackslash(buffer);
            PathAppend(buffer, defaultSkeletonName.c_str());
            if (-1 != _taccess(buffer, 0))
               skeleton = buffer;
         }
      }
   }
   bool isValid() const { return (0 != blocks.size()); }

   // Ini File related routines
   void LoadIniSettings();
   void SaveIniSettings();

   // Generic IniFile reading routine
   template<typename T>
   T GetIniValue(LPCTSTR Section, LPCTSTR Setting, T Default){
      if (!iniFileValid) 
         return Default;
      return ::GetIniValue<T>(Section, Setting, Default, iniFileName.c_str());
   }
   // Generic IniFile reading routine
   template<typename T>
   void SetIniValue(LPCTSTR Section, LPCTSTR Setting, T value){
      if (!iniFileValid) 
         return;
      ::SetIniValue<T>(Section, Setting, value, iniFileName.c_str());
   }

   bool HasSkeleton();
   bool IsBiped();
   void ImportBones(vector<NiNodeRef>& bones);
   void ImportBones(NiNodeRef blocks);
   void ImportBipeds(vector<NiNodeRef>& blocks);
   void AlignBiped(IBipMaster* master, NiNodeRef block);
   void PositionBiped(IBipMaster* master, NiNodeRef block, bool Recurse = false);
   void RotateBiped(IBipMaster* master, NiNodeRef block, bool Recurse = false);
   void ScaleBiped(IBipMaster* master, NiNodeRef block, bool Recurse = false);
   bool ImportMeshes(NiNodeRef block);
   string FindImage(const string& name);

   void SetTrangles(Mesh& mesh, vector<Triangle>& v, bool hasTexture);
   bool ImportMesh(NiTriShapeRef triShape);
   bool ImportMesh(NiTriStripsRef triStrips);
   bool ImportMaterialAndTextures(ImpNode *node, NiAVObjectRef avObject);
   bool ImportTransform(ImpNode *node, NiAVObjectRef avObject);
   bool ImportMesh(ImpNode *node, TriObject *o, NiTriBasedGeomRef triGeom, NiTriBasedGeomDataRef triGeomData, bool hasTexture);

   bool ImportSkin(ImpNode *node, NiTriBasedGeomRef triGeom);
   Texmap* CreateTexture(TexDesc& desc);

   INode *CreateBone(const string& name, Point3 startPos, Point3 endPos, Point3 zAxis);
};




class MaxNifImport : public SceneImport {
	public:

		static HWND hParams;
		
		int				ExtCount();					// Number of extensions supported
		const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
		const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
		const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
		const TCHAR *	AuthorName();				// ASCII Author name
		const TCHAR *	CopyrightMessage();			// ASCII Copyright message
		const TCHAR *	OtherMessage1();			// Other message #1
		const TCHAR *	OtherMessage2();			// Other message #2
		unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
		void			   ShowAbout(HWND hWnd);		// Show DLL's "About..." box
		int				DoImport(const TCHAR *name,ImpInterface *i,Interface *gi, BOOL suppressPrompts=FALSE);	// Import file
		
		//Constructor/Destructor
		MaxNifImport();
		~MaxNifImport();		

      string iniFileName;
      string shortDescription;
};



class MaxNifImportClassDesc : public ClassDesc2 {
	public:
	int 			   IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return new MaxNifImport(); }
	const TCHAR *	ClassName() { return GetString(IDS_CLASS_NAME); }
	SClass_ID		SuperClassID() { return SCENE_IMPORT_CLASS_ID; }
	Class_ID		   ClassID() { return MaxNifImport_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("MaxNifImport"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
	

};

static MaxNifImportClassDesc MaxNifImportDesc;
ClassDesc2* GetMaxNifImportDesc() { return &MaxNifImportDesc; }

BOOL CALLBACK MaxNifImportOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
	static NifImporter *imp = NULL;

	switch(message) {
		case WM_INITDIALOG:
         {
			   imp = (NifImporter *)lParam;
			   CenterWindow(hWnd,GetParent(hWnd));

            char buffer[33] = {0};
            sprintf(buffer, "%f", imp->bipedHeight);
            SetWindowText(GetDlgItem(hWnd, IDC_EDITHEIGHT), buffer);
         }
			return TRUE;

		case WM_CLOSE:
         {
            char buffer[33] = {0}, *end = NULL;
            GetWindowText(GetDlgItem(hWnd, IDC_EDITHEIGHT), buffer, 33);
            imp->bipedHeight = strtod(buffer, &end);
			   EndDialog(hWnd, 0);
         }
			return TRUE;

      case WM_COMMAND : 
         {
            switch (wParam)
            {
            case IDCLOSE :
               SendMessage(hWnd, WM_CLOSE, 0, 0);
               return TRUE;
            }
         }
         break;
	}
	return FALSE;
}


//--- MaxNifImport -------------------------------------------------------
MaxNifImport::MaxNifImport()
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
   shortDescription = GetIniValue<string>(SystemSection, "ShortDescription", "Netimmerse/Gamebryo", iniFileName.c_str());
}

MaxNifImport::~MaxNifImport() 
{

}

int MaxNifImport::ExtCount()
{
	//TODO: Returns the number of file name extensions supported by the plug-in.
	return 1;
}

const TCHAR *MaxNifImport::Ext(int n)
{		
	//TODO: Return the 'i-th' file name extension (i.e. "3DS").
	return _T("NIF");
}

const TCHAR *MaxNifImport::LongDesc()
{
	//TODO: Return long ASCII description (i.e. "Targa 2.0 Image File")
	return _T("Netimmerse/Gamebryo");
}
	
const TCHAR *MaxNifImport::ShortDesc() 
{			
	//TODO: Return short ASCII description (i.e. "Targa")
	return shortDescription.c_str();
}

const TCHAR *MaxNifImport::AuthorName()
{			
	//TODO: Return ASCII Author name
	return _T("Theo");
}

const TCHAR *MaxNifImport::CopyrightMessage() 
{	
	// Return ASCII Copyright message
	return _T(
      "Copyright (c) 2006, NIF File Format Library and Tools\n" 
      "All rights reserved."
      );
}

const TCHAR *MaxNifImport::OtherMessage1() 
{		
	return _T(
      "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n"
      "\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n"
      "LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS\n"
      "FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE\n"
      "COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,\n"
      "INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,\n"
      "BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n"
      "LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n"
      "CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\n"
      "LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN\n"
      "ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\n"
      "POSSIBILITY OF SUCH DAMAGE.\n"
      );
}

const TCHAR *MaxNifImport::OtherMessage2() 
{		
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int MaxNifImport::Version()
{				
	//TODO: Return Version number * 100 (i.e. v3.01 = 301)
	return 1 * 100;
}

void MaxNifImport::ShowAbout(HWND hWnd)
{			
	// Optional
}


int MaxNifImport::DoImport(const TCHAR *filename,ImpInterface *i, Interface *gi, BOOL suppressPrompts)
{
   bool ok = true;
   try 
   {
      HoldSuspend myHold(TRUE);

      AppSettings::Initialize(gi);

      std::string current_file = filename;

      NifImporter importer(filename, i, gi, suppressPrompts);
      if (!importer.isValid())
         return FALSE;

      //if(!suppressPrompts)
      //	DialogBoxParam(hInstance, 
      //			MAKEINTRESOURCE(IDD_PANEL), 
      //			GetActiveWindow(), 
      //			MaxNifImportOptionsDlgProc, (LPARAM)&importer);

      // 
      if (importer.isBiped)
      {
         if (importer.useBiped){
            importer.ImportBipeds(importer.nodes);
         } else {
            importer.ImportBones(DynamicCast<NiNode>(importer.nodes[0]->GetChildren()));
         }
      }
      else
      {
         vector<string> importedBones;
         if (importer.importSkeleton)
         {
            if (importer.browseForSkeleton)
            {
               TCHAR filter[64], *pfilter=filter;
               pfilter = _tcscpy(filter, this->ShortDesc());
               pfilter = _tcscat(pfilter, " (*.NIF)");
               pfilter += strlen(pfilter);
               *pfilter++ = '\0';
               _tcscpy(pfilter, "*.NIF");
               pfilter += strlen(pfilter);
               *pfilter++ = '\0';
               *pfilter++ = '\0';

               TCHAR filename[MAX_PATH];
               GetFullPathName(importer.skeleton.c_str(), _countof(filename), filename, NULL);

               OPENFILENAME ofn;
               memset(&ofn, 0, sizeof(ofn));
               ofn.lStructSize = sizeof(ofn);
               ofn.hwndOwner = gi->GetMAXHWnd();
               ofn.lpstrFilter = filter;
               ofn.lpstrFile = filename;
               ofn.nMaxFile = _countof(filename);
               ofn.lpstrTitle = TEXT("Browse for Skeleton NIF...");
               ofn.lpstrDefExt = TEXT("NIF");
               ofn.Flags = OFN_HIDEREADONLY|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST;
               importer.importSkeleton = GetOpenFileName(&ofn) ? true : false;
               if (importer.importSkeleton) {
                  importer.skeleton = filename;
               }
            }
            if (importer.importSkeleton && !importer.skeleton.empty()) {
               NifImporter skelImport(importer.skeleton.c_str(), i, gi, suppressPrompts);
               if (skelImport.isValid())
               {
                  if (skelImport.useBiped){
                     skelImport.ImportBipeds(skelImport.nodes);
                  } else {
                     skelImport.ImportBones(DynamicCast<NiNode>(skelImport.nodes[0]->GetChildren()));
                     if (importer.removeUnusedImportedBones){
                        importedBones = GetNamesOfNodes(skelImport.nodes);
                     }
                  }
               }
            }
         }
         if (importer.isValid()) {

            importer.ImportBones(DynamicCast<NiNode>(importer.nodes[0]->GetChildren()));
            ok = importer.ImportMeshes(importer.nodes[0]);

            if (importer.importSkeleton && importer.removeUnusedImportedBones){
               vector<string> importedNodes = GetNamesOfNodes(importer.nodes);
               sort(importedBones.begin(), importedBones.end());
               sort(importedNodes.begin(), importedNodes.end());
               vector<string> results;
               results.resize(importedBones.size());
               vector<string>::iterator end = set_difference ( 
                  importedBones.begin(), importedBones.end(),
                  importedNodes.begin(), importedNodes.end(), results.begin());
               for (vector<string>::iterator itr = results.begin(); itr != end; ++itr){
                  if (INode *node = gi->GetINodeByName((*itr).c_str())){
                     node->Delete(0, TRUE);
                  }
               }
            }
         }
      }
   }
   catch( exception & e ) 
   {
      e=e;
      ok = false;
   }
   catch( exception * e ) 
   {
      e=e;
      ok = false;
   }
   catch( ... ) 
   {
      ok = false;
   }
   return ok ? TRUE : FALSE;
}

void NifImporter::LoadIniSettings()
{
   TCHAR iniName[MAX_PATH];
   LPCTSTR pluginDir = gi->GetDir(APP_PLUGCFG_DIR);
   PathCombine(iniName, pluginDir, "MaxNifTools.ini");
   this->iniFileName = iniName;
   iniFileValid = (-1 != _access(iniName, 0));

   // Locate which application to use. If Auto, find first app where this file appears in the root path list
   appSettings = NULL;
   string curapp = GetIniValue<string>(NifImportSection, "CurrentApp", "AUTO");
   if (0 == _tcsicmp(curapp.c_str(), "AUTO")) {
      // Scan Root paths
      for (AppSettingsMap::iterator itr = TheAppSettings.begin(), end = TheAppSettings.end(); itr != end; ++itr){
         if ((*itr).IsFileInRootPaths(this->name)) {
            appSettings = &(*itr);
            break;
         }
      }
   } else {
      appSettings = FindAppSetting(curapp);
   }

   useBiped = GetIniValue<bool>(NifImportSection, "UseBiped", false);
   skeletonCheck = GetIniValue<string>(NifImportSection, "SkeletonCheck", "Bip*");
   showTextures = GetIniValue<bool>(NifImportSection, "ShowTextures", true);

   removeIllegalFaces = GetIniValue<bool>(NifImportSection, "RemoveIllegalFaces", true);
   removeDegenerateFaces = GetIniValue<bool>(NifImportSection, "RemoveDegenerateFaces", true);
   enableAutoSmooth = GetIniValue<bool>(NifImportSection, "EnableAutoSmooth", true);
   autoSmoothAngle = GetIniValue<float>(NifImportSection, "AutoSmoothAngle", 30.0f);
   flipUVTextures = GetIniValue<bool>(NifImportSection, "FlipUVTextures", true);
   enableSkinSupport = GetIniValue<bool>(NifImportSection, "EnableSkinSupport", true);

   bipedHeight = GetIniValue<float>(BipedImportSection, "BipedHeight", 131.90f);
   bipedAngle = GetIniValue<float>(BipedImportSection, "BipedAngle", 90.0f);
   bipedAnkleAttach = GetIniValue<float>(BipedImportSection, "BipedAnkleAttach", 0.2f);
   bipedTrianglePelvis = GetIniValue<bool>(BipedImportSection, "BipedTrianglePelvis", false);
   removeUnusedImportedBones = GetIniValue<bool>(BipedImportSection, "RemoveUnusedImportedBones", false);
   forceRotation = GetIniValue<bool>(BipedImportSection, "ForceRotation", true);
   browseForSkeleton = GetIniValue<bool>(BipedImportSection, "BrowseForSkeleton", true);
   defaultSkeletonName = GetIniValue<string>(BipedImportSection, "DefaultSkeletonName", "Skeleton.Nif");

   minBoneWidth = GetIniValue<float>(BipedImportSection, "MinBoneWidth", 0.5f);
   maxBoneWidth = GetIniValue<float>(BipedImportSection, "MaxBoneWidth", 3.0f);
   boneWidthToLengthRatio = GetIniValue<float>(BipedImportSection, "BoneWidthToLengthRatio", 0.25f);

   goToSkeletonBindPosition = (appSettings ? appSettings->goToSkeletonBindPosition : false);
}

void NifImporter::SaveIniSettings()
{
   SetIniValue<bool>(NifImportSection, "UseBiped", useBiped);
   SetIniValue<string>(NifImportSection, "Skeleton", skeleton);
   SetIniValue<string>(NifImportSection, "SkeletonCheck", skeletonCheck);

   SetIniValue<float>(BipedImportSection, "BipedHeight", bipedHeight);
   SetIniValue<float>(BipedImportSection, "BipedAngle", bipedAngle);
   SetIniValue<float>(BipedImportSection, "BipedAnkleAttach", bipedAnkleAttach);
   SetIniValue<bool>(BipedImportSection, "BipedTrianglePelvis", bipedTrianglePelvis);
}

bool NifImporter::HasSkeleton()
{
   if (!skeletonCheck.empty()){
      vector<NiNodeRef> bipedRoots = SelectNodesByName(nodes, skeletonCheck.c_str());
      return !bipedRoots.empty();
   }
   return false;
}

bool NifImporter::IsBiped()
{
   if (hasSkeleton){
      list<NiExtraDataRef> extraData = nodes[0]->GetExtraData();
      if (!extraData.empty()) {
         return ( DynamicCast<BSBound>(extraData).size() != 0 );
      }
   }
   return false;
}

void NifImporter::ImportBipeds(vector<NiNodeRef>& nodes)
{
   IBipMaster* master = NULL;
   try 
   {
      vector<NiNodeRef> bipedRoots = SelectNodesByName(nodes, "Bip??");
      std::stable_sort(bipedRoots.begin(), bipedRoots.end(), NiNodeNameEquivalence());
      for (vector<NiNodeRef>::iterator bipedItr = bipedRoots.begin(); bipedItr != bipedRoots.end(); ++bipedItr)
      {
         string bipname = (*bipedItr)->GetName();
         string match = bipname + "*";
         vector<NiNodeRef> bipedNodes = SelectNodesByName(nodes, match.c_str());

         float height = this->bipedHeight;
#if USE_CUSTOM_BSBOUND
         list<NiExtraDataRef> extraData = nodes[0]->GetExtraData();
         if (!extraData.empty()) {
            BSBoundRef bound = SelectFirst<BSBound>(extraData);
            if (bound) {
               array<float,6> floats = bound->GetUnknownFloats();
               height = floats[2] * 2.0f;
            }
         }
#endif
         float angle = TORAD(bipedAngle);
         Point3 wpos(0.0f,0.0f,0.0f);
         BOOL arms = (CountNodesByName(bipedNodes, FormatText("%s L UpperArm", bipname.c_str())) > 0) ? TRUE : FALSE;
         BOOL triPelvis = bipedTrianglePelvis ? TRUE : FALSE;
         int nnecklinks=CountNodesByName(bipedNodes, FormatText("%s Neck*", bipname.c_str()));
         int nspinelinks=CountNodesByName(bipedNodes, FormatText("%s Spine*", bipname.c_str()));
         int nleglinks = 3 + CountNodesByName(bipedNodes, FormatText("%s L HorseLink", bipname.c_str()));
         int ntaillinks = CountNodesByName(bipedNodes, FormatText("%s Tail*", bipname.c_str()));
         int npony1links = CountNodesByName(bipedNodes, FormatText("%s Ponytail1*", bipname.c_str()));
         int npony2links = CountNodesByName(bipedNodes, FormatText("%s Ponytail2*", bipname.c_str()));
         int numfingers = CountNodesByName(bipedNodes, FormatText("%s L Finger?", bipname.c_str()));
         int nfinglinks = CountNodesByName(bipedNodes, FormatText("%s L Finger0*", bipname.c_str()));
         int numtoes = CountNodesByName(bipedNodes, FormatText("%s L Toe?", bipname.c_str()));
         int ntoelinks = CountNodesByName(bipedNodes, FormatText("%s L Toe0*", bipname.c_str()));
         BOOL prop1exists = CountNodesByName(bipedNodes, FormatText("%s Prop1", bipname.c_str())) ? TRUE : FALSE;
         BOOL prop2exists = CountNodesByName(bipedNodes, FormatText("%s Prop2", bipname.c_str())) ? TRUE : FALSE;
         BOOL prop3exists = CountNodesByName(bipedNodes, FormatText("%s Prop3", bipname.c_str())) ? TRUE : FALSE;
         int forearmTwistLinks = CountNodesByName(bipedNodes, FormatText("%s L Fore*Twist*", bipname.c_str()));
         int upperarmTwistLinks = CountNodesByName(bipedNodes, FormatText("%s L Up*Twist*", bipname.c_str()));
         int thighTwistLinks = CountNodesByName(bipedNodes, FormatText("%s L Thigh*Twist*", bipname.c_str()));
         int calfTwistLinks = CountNodesByName(bipedNodes, FormatText("%s L Calf*Twist*", bipname.c_str()));
         int horseTwistLinks = CountNodesByName(bipedNodes, FormatText("%s L Horse*Twist*", bipname.c_str()));

         NiNodeRef root = nodes[0];
         IBipMaster* master = CreateNewBiped(height, angle, wpos, arms, triPelvis, nnecklinks, nspinelinks, 
            nleglinks, ntaillinks, npony1links, npony2links, numfingers, nfinglinks, numtoes, 
            ntoelinks, bipedAnkleAttach, prop1exists, prop2exists, prop3exists,
            forearmTwistLinks, upperarmTwistLinks, thighTwistLinks,
            calfTwistLinks, horseTwistLinks);
         master->SetRootName(const_cast<TCHAR*>(bipname.c_str()));
         if (master)
         {
            master->BeginModes(BMODE_FIGURE, FALSE);
            master->SetTrianglePelvis(FALSE);
            master->SetDisplaySettings(BDISP_BONES);
            LPCTSTR bipname = master->GetRootName();

            // Rename twists, if necessary
            RenameNode(gi, FormatText("%s L ForeTwist", bipname), FormatText("%s L ForearmTwist", bipname));
            RenameNode(gi, FormatText("%s R ForeTwist", bipname), FormatText("%s R ForearmTwist", bipname));
            RenameNode(gi, FormatText("%s R LUpArmTwist", bipname), FormatText("%s L UpperArmTwist", bipname));
            RenameNode(gi, FormatText("%s R LUpArmTwist", bipname), FormatText("%s R UpperArmTwist", bipname));

            NiNodeRef nifBip = FindNodeByName(nodes, bipname);
            //AlignBiped(master, nifBip);
            ScaleBiped(master, nifBip, true);
            PositionBiped(master, nifBip, true);
            RotateBiped(master, nifBip, true);
         }
      }
   }
   catch( exception & e ) 
   {
      e=e;
   }
   catch( ... ) 
   {
   }
   if (master)
      master->EndModes(BMODE_FIGURE, TRUE);
}

void NifImporter::AlignBiped(IBipMaster* master, NiNodeRef block)
{
   string name = block->GetName();
   Matrix44 wt = block->GetWorldTransform();
   Matrix44 lt = block->GetLocalTransform();

   Vector3 pos; Matrix33 rot; float scale;
   wt.Decompose(pos, rot, scale);

   INode *node = gi->GetINodeByName(name.c_str());
   if (node != NULL) 
   {
      Matrix3 m3 = node->GetNodeTM(TimeValue(), NULL); // local translation m
      master->SetBipedPos(Point3(pos.x, pos.y, pos.z), 0, node, TRUE);

      Matrix3 m(rot.rows[0].data, rot.rows[1].data, rot.rows[2].data, Point3(0,0,0));
      Matrix3 im = Inverse(m);

      Point3 p; Quat q; Point3 s;
      DecomposeMatrix(im, p, q, s);
      master->SetBipedRot(q, 0, node, TRUE);
   }
   else
   {

   }
   vector<NiNodeRef> nodes = DynamicCast<NiNode>(block->GetChildren());
   for (vector<NiNodeRef>::iterator itr = nodes.begin(), end = nodes.end(); itr != end; ++itr){
      AlignBiped(master, *itr);
   }
}

void NifImporter::PositionBiped(IBipMaster* master, NiNodeRef block, bool Recurse)
{
   string name = block->GetName();

   Matrix44 wt = block->GetWorldTransform();
   Matrix44 lt = block->GetLocalTransform();

   Vector3 pos; Matrix33 rot; float scale;
   wt.Decompose(pos, rot, scale);

   INode *node = gi->GetINodeByName(name.c_str());
   if (node != NULL) 
   {
      Matrix3 m3 = node->GetNodeTM(TimeValue(), NULL); // local translation m
      master->SetBipedPos(Point3(pos.x, pos.y, pos.z), 0, node, TRUE);

      Matrix3 m(rot.rows[0].data, rot.rows[1].data, rot.rows[2].data, Point3(0,0,0));
      Matrix3 im = Inverse(m);
      Point3 p; Quat q; Point3 s;
      DecomposeMatrix(im, p, q, s);
      master->SetBipedRot(q, 0, node, TRUE);
   }
   else
   {

   }
   if (Recurse)
   {
      vector<NiNodeRef> nodes = DynamicCast<NiNode>(block->GetChildren());
      for (vector<NiNodeRef>::iterator itr = nodes.begin(), end = nodes.end(); itr != end; ++itr){
         PositionBiped(master, *itr, Recurse);
      }
   }
}

void NifImporter::RotateBiped(IBipMaster* master, NiNodeRef block, bool Recurse)
{
   string name = block->GetName();

   Matrix44 wt = block->GetWorldTransform();
   Matrix44 lt = block->GetLocalTransform();

   Vector3 pos; Matrix33 rot; float scale;
   wt.Decompose(pos, rot, scale);

   INode *node = gi->GetINodeByName(name.c_str());
   if (node != NULL) 
   {
      Matrix3 m3 = node->GetNodeTM(TimeValue(), NULL); // local translation m
      master->SetBipedPos(Point3(pos.x, pos.y, pos.z), 0, node, TRUE);

      Matrix3 m(rot.rows[0].data, rot.rows[1].data, rot.rows[2].data, Point3(0,0,0));
      Matrix3 im = Inverse(m);

      Point3 p; Quat q; Point3 s;
      DecomposeMatrix(im, p, q, s);
      master->SetBipedRot(q, 0, node, TRUE);
   }
   else
   {

   }
   if (Recurse)
   {
      vector<NiNodeRef> nodes = DynamicCast<NiNode>(block->GetChildren());
      for (vector<NiNodeRef>::iterator itr = nodes.begin(), end = nodes.end(); itr != end; ++itr){
         RotateBiped(master, *itr, Recurse);
      }
   }
}

void NifImporter::ScaleBiped(IBipMaster* master, NiNodeRef block, bool Recurse)
{
   string name = block->GetName();

   Matrix44 wt = block->GetWorldTransform();
   Matrix44 lt = block->GetLocalTransform();

   Vector3 pos; Matrix33 rot; float scale;
   wt.Decompose(pos, rot, scale);

   INode *node = gi->GetINodeByName(name.c_str());
   if (node != NULL) 
   {
      Matrix3 m3 = node->GetNodeTM(TimeValue(), NULL); // local translation m
      master->SetBipedPos(Point3(pos.x, pos.y, pos.z), 0, node, TRUE);

      Matrix3 m(rot.rows[0].data, rot.rows[1].data, rot.rows[2].data, Point3(0,0,0));
      Matrix3 im = Inverse(m);

      Point3 p; Quat q; Point3 s;
      DecomposeMatrix(im, p, q, s);
      master->SetBipedRot(q, 0, node, TRUE);
   }
   else
   {

   }
   if (Recurse)
   {
      vector<NiNodeRef> nodes = DynamicCast<NiNode>(block->GetChildren());
      for (vector<NiNodeRef>::iterator itr = nodes.begin(), end = nodes.end(); itr != end; ++itr){
         ScaleBiped(master, *itr, Recurse);
      }
   }
}


INode *NifImporter::CreateBone(const string& name, Point3 startPos, Point3 endPos, Point3 zAxis)
{
   if (FPInterface * fpBones = GetCOREInterface(Interface_ID(0x438aff72, 0xef9675ac)))
   {
      FunctionID createBoneID = fpBones->FindFn(TEXT("createBone"));
      FPValue result;
      FPParams params (3, TYPE_POINT3, &startPos, TYPE_POINT3, &endPos, TYPE_POINT3, &zAxis);     
      FPStatus status = fpBones->Invoke(createBoneID, result, &params);
      if (status == FPS_OK && result.type == TYPE_INODE)
      {
         if (INode *n = result.n)
         {
            n->SetName(const_cast<TCHAR*>(name.c_str()));
            float len = fabs(Length(startPos)-Length(endPos));
            float width = max(minBoneWidth, min(maxBoneWidth, len * boneWidthToLengthRatio));
            if (Object* o = n->GetObjectRef())
            {
               setMAXScriptValue(o->GetReference(0), "width", 0, width);
               setMAXScriptValue(o->GetReference(0), "height", 0, width);
            }
         }
         return result.n;
      }
      fpBones->ReleaseInterface();
   }
   return NULL;
}

void NifImporter::ImportBones(vector<NiNodeRef>& bones)
{
   for (vector<NiNodeRef>::iterator itr = bones.begin(), end = bones.end(); itr != end; ++itr){
      ImportBones(*itr);
   }
}

void NifImporter::ImportBones(NiNodeRef node)
{
   try 
   {
      string name = node->GetName();
      vector<NiNodeRef> children = DynamicCast<NiNode>(node->GetChildren());
      NiNodeRef parent = node->GetParent();

      PosRotScale prs = prsDefault;
      Matrix3 im = TOMATRIX3(node->GetWorldTransform(), true);
      Point3 p = im.GetTrans();
      Quat q(im);
      q.Normalize();
      Vector3 ppos;
      Point3 zAxis(0,1,0);
      if (!children.empty()) {
         for (vector<NiNodeRef>::iterator itr=children.begin(), end = children.end(); itr != end; ++itr) {
            Matrix44 cwt = (*itr)->GetWorldTransform();
            Vector3 cpos; Matrix33 crot; float cscale;
            cwt.Decompose(cpos, crot, cscale);
            ppos += cpos;
         }
         ppos /= children.size();
      }
      else if (parent)
      {
         Matrix44 pwt = parent->GetWorldTransform();
         Matrix33 prot; float pscale;
         pwt.Decompose(ppos, prot, pscale);
         if (forceRotation)
            prs = prsPos;
      }
      else
      {
         if (forceRotation)
            prs = prsPos;
      }
      Point3 pp(ppos.x, ppos.y, ppos.z);

      INode *bone = gi->GetINodeByName(name.c_str());
      if (bone)
      {
         // Is there a better way of "Affect Pivot Only" behaviors?
         INode *pinode = bone->GetParentNode();
         if (pinode)
            bone->Detach(0,1);
         PositionAndRotateNode(bone, p, q, prs);
         if (pinode)
            pinode->AttachChild(bone, 1);
      }
      else
      {
         if (bone = CreateBone(name, p, pp, zAxis))
         {
            PositionAndRotateNode(bone, p, q, prs);
            if (parent)
            {
               if (INode *pn = gi->GetINodeByName(parent->GetName().c_str()))
                  pn->AttachChild(bone, 1);
            }
         }
      }
      if (bone)
      {
         ImportBones(children);
      }
   }
   catch( exception & e ) 
   {
      e=e;
   }
   catch( ... ) 
   {
   }
}

bool NifImporter::ImportMeshes(NiNodeRef node)
{
   bool ok = true;
   try 
   {
      vector<NiTriShapeRef> trinodes = DynamicCast<NiTriShape>(node->GetChildren());
      for (vector<NiTriShapeRef>::iterator itr = trinodes.begin(), end = trinodes.end(); ok && itr != end; ++itr){
         ok |= ImportMesh(*itr);
      }
      vector<NiTriStripsRef> tristrips = DynamicCast<NiTriStrips>(node->GetChildren());
      for (vector<NiTriStripsRef>::iterator itr = tristrips.begin(), end = tristrips.end(); ok && itr != end; ++itr){
         ok |= ImportMesh(*itr);
      }
      vector<NiNodeRef> nodes = DynamicCast<NiNode>(node->GetChildren());
      for (vector<NiNodeRef>::iterator itr = nodes.begin(), end = nodes.end(); itr != end; ++itr){
         ok |= ImportMeshes(*itr);
      }
   }
   catch( exception & e ) 
   {
      e=e;
      ok = false;
   }
   catch( ... ) 
   {
      ok = false;
   }
   return ok;
}

Texmap* NifImporter::CreateTexture(TexDesc& desc)
{
   BitmapManager *bmpMgr = TheManager;
   if (NiSourceTextureRef texSrc = desc.source){
      string filename = texSrc->GetExternalFileName();
      if (bmpMgr->CanImport(filename.c_str())){
         BitmapTex *bmpTex = NewDefaultBitmapTex();
         bmpTex->SetName(texSrc->GetName().c_str());
         bmpTex->SetMapName(const_cast<TCHAR*>(FindImage(filename).c_str()));
         bmpTex->SetAlphaAsMono(TRUE);
         bmpTex->SetAlphaSource(ALPHA_NONE);
         return bmpTex;
      }
   }
   return NULL;
}

bool NifImporter::ImportMaterialAndTextures(ImpNode *node, NiAVObjectRef avObject)
{
   // Texture
   NiTexturingPropertyRef texRef = avObject->GetPropertyByType( NiTexturingProperty::TypeConst() );
   NiMaterialPropertyRef matRef = avObject->GetPropertyByType( NiMaterialProperty::TypeConst() );
   
   bool hasTexture = (texRef && matRef);
   if (hasTexture){
      StdMat2 *m = NewDefaultStdMat();
      m->SetName(matRef->GetName().c_str());
      if (showTextures)
         m->SetMtlFlag(MTL_DISPLAY_ENABLE_FLAGS, TRUE);
      m->SetAmbient(TOCOLOR(matRef->GetAmbientColor()),0);
      m->SetDiffuse(TOCOLOR(matRef->GetDiffuseColor()),0);
      m->SetSpecular(TOCOLOR(matRef->GetSpecularColor()),0);
      Color c = TOCOLOR(matRef->GetEmissiveColor());
      if (c.r != 0 || c.b != 0 || c.g != 0) {
         m->SetSelfIllumColorOn(TRUE);
         m->SetSelfIllumColor(c,0);
      }
      m->SetShinStr(0.0,0);
      m->SetShininess(matRef->GetGlossiness()/100.0,0);
      m->SetOpacity(matRef->GetTransparency(),0);

      // Handle Base/Detail ???
      if (texRef->HasTexture(DECAL_0_MAP)){
         if (Texmap* tex = CreateTexture(texRef->GetTexture(DECAL_0_MAP)))
            m->SetSubTexmap(ID_DI, tex);
         if (texRef->HasTexture(BASE_MAP)){
            m->LockAmbDiffTex(FALSE);
            if (Texmap* tex = CreateTexture(texRef->GetTexture(BASE_MAP)))
               m->SetSubTexmap(ID_AM, tex);
         }
      } else if (texRef->HasTexture(BASE_MAP)) {
         if (Texmap* tex = CreateTexture(texRef->GetTexture(BASE_MAP)))
            m->SetSubTexmap(ID_DI, tex);
      } 
      // Handle Bump map
      if (texRef->HasTexture(BUMP_MAP)) {
         if (Texmap* tex = CreateTexture(texRef->GetTexture(BUMP_MAP)))
            m->SetSubTexmap(ID_BU, tex);
      }
      // Shiny map
      if (texRef->HasTexture(GLOSS_MAP)) {
         if (Texmap* tex = CreateTexture(texRef->GetTexture(GLOSS_MAP)))
            m->SetSubTexmap(ID_SS, tex);
      }

      // Self illumination
      if (texRef->HasTexture(GLOW_MAP)) {
         if (Texmap* tex = CreateTexture(texRef->GetTexture(GLOW_MAP)))
            m->SetSubTexmap(ID_SI, tex);
      }

      gi->GetMaterialLibrary().Add(m);
      node->GetINode()->SetMtl(m);
   }
   return hasTexture;
}

bool NifImporter::ImportTransform(ImpNode *node, NiAVObjectRef avObject)
{
   node->SetTransform(0,TOMATRIX3(avObject->GetWorldTransform()));   
   return true;
}

bool NifImporter::ImportMesh(ImpNode *node, TriObject *o, NiTriBasedGeomRef triGeom, NiTriBasedGeomDataRef triGeomData, bool hasTexture)
{
   Mesh& mesh = o->GetMesh();

   // Vertex info
   {
      int nVertices = triGeomData->GetVertexCount();
      vector<Vector3> vertices = triGeomData->GetVertices();
      mesh.setNumVerts(nVertices);
      for (int i=0; i < nVertices; ++i){
         Vector3 &v = vertices[i];
         mesh.verts[i].Set(v.x, v.y, v.z);
      }
   }
   if (hasTexture) // uv texture info
   {
      int nUVSet = triGeomData->GetUVSetCount();
      int n = 0;
      for (int j=0; j<nUVSet; j++){
         vector<TexCoord> texCoords = triGeomData->GetUVSet(j);
         n = texCoords.size();
         mesh.setNumTVerts(n, FALSE);
         for (int i=0; i<n; ++i) {
            TexCoord& texCoord = texCoords[i];
            mesh.tVerts[i].Set(texCoord.u, (flipUVTextures) ? -texCoord.v : texCoord.v, 0);
         }
      }
   }
   if (removeDegenerateFaces)
      mesh.RemoveDegenerateFaces();
   if (removeIllegalFaces)
      mesh.RemoveIllegalFaces();
   if (enableAutoSmooth)
      mesh.AutoSmooth(TORAD(autoSmoothAngle), FALSE, FALSE);

   // Normals
   {
      mesh.checkNormals(TRUE);
      vector<Vector3> n = triGeomData->GetNormals();
      for (int i=0; i<n.size(); i++){
         Vector3 v = n[i];
         mesh.setNormal(i, Point3(v.x, v.y, v.z));
      }
   }
   // vertex coloring
   {
      bool hasAlpha = false;
      vector<Color4> cv = triGeomData->GetColors();
      mesh.setNumVertCol(cv.size());
      for (int i=0; i<cv.size(); i++){
         Color4& c = cv[i];
         mesh.vertCol[i].Set(c.r, c.g, c.b);
         hasAlpha |= (c.a != 0.0);
      }
   }
   return true;
}

void NifImporter::SetTrangles(Mesh& mesh, vector<Triangle>& v, bool hasTexture)
{
   int n = v.size();
   mesh.setNumFaces(n);
   if (hasTexture)
      mesh.setNumTVFaces(n);
   for (int i=0; i<n; ++i) {
      Triangle& t = v[i];
      Face& f = mesh.faces[i];
      f.setVerts(t.v1, t.v2, t.v3);
      f.Show();
      f.setEdgeVisFlags(EDGE_VIS, EDGE_VIS, EDGE_VIS);
      if (hasTexture) {
         TVFace& tf = mesh.tvFace[i];
         tf.setTVerts(t.v1, t.v2, t.v3);
      }
   }
}

bool NifImporter::ImportMesh(NiTriShapeRef triShape)
{
   bool ok = true;

   ImpNode *node = i->CreateNode();
   if(!node) return false;
   TriObject *triObject = CreateNewTriObject();
   node->Reference(triObject);
   string name = triShape->GetName();
   node->SetName(name.c_str());

   // Texture
   bool hasTexture = ImportMaterialAndTextures(node, triShape);
   ImportTransform(node, triShape);

   Mesh& mesh = triObject->GetMesh();
   NiTriShapeDataRef triShapeData = DynamicCast<NiTriShapeData>(triShape->GetData());
   if (triShapeData == NULL)
      return false;

   ok |= ImportMesh(node, triObject, triShape, triShapeData, hasTexture);

   // Triangles and texture vertices
   if (ok)
   {
      vector<Triangle> v = triShapeData->GetTriangles();
      SetTrangles(mesh, v, hasTexture);
   }
   if (enableSkinSupport)
      ImportSkin(node, triShape);

   i->AddNodeToScene(node);   

   if (enableAutoSmooth){
      if (TriObject *tri = GetTriObject(node->GetINode()->GetObjectRef())){
        tri->GetMesh().AutoSmooth(TORAD(autoSmoothAngle), FALSE, FALSE);
      }
   }

   return ok;
}

bool NifImporter::ImportMesh(NiTriStripsRef triStrips)
{
   bool ok = true;

   ImpNode *node = i->CreateNode();
   if(!node) return false;
   TriObject *triObject = CreateNewTriObject();
   node->Reference(triObject);
   string name = triStrips->GetName();
   node->SetName(name.c_str());

   // Texture
   bool hasTexture = ImportMaterialAndTextures(node, triStrips);
   ImportTransform(node, triStrips);

   Mesh& mesh = triObject->GetMesh();
   NiTriStripsDataRef triStripsData = DynamicCast<NiTriStripsData>(triStrips->GetData());
   if (triStripsData == NULL)
      return false;

   ok |= ImportMesh(node, triObject, triStrips, triStripsData, hasTexture);

   // Triangles and texture vertices
   if (ok)
   {
      vector<Triangle> v = triStripsData->GetTriangles();
      SetTrangles(mesh, v, hasTexture);
   }
   if (enableSkinSupport)
      ImportSkin(node, triStrips);

   i->AddNodeToScene(node);   

   // apply autosmooth after object creation for it to take hold
   if (enableAutoSmooth){
      if (TriObject *tri = GetTriObject(node->GetINode()->GetObjectRef())){
         tri->GetMesh().AutoSmooth(TORAD(autoSmoothAngle), FALSE, FALSE);
      }
   }

   return ok;
}

struct VertexHolder
{
   VertexHolder() : vertIndex(0), count(0) {}

   int vertIndex;
   int count;
   Tab<INode*> boneNodeList;
   Tab<float> weights;
};

bool NifImporter::ImportSkin(ImpNode *node, NiTriBasedGeomRef triGeom)
{
   bool ok = true;
   NiSkinInstanceRef nifSkin = triGeom->GetSkinInstance();
   if (!nifSkin) 
      return false;

   INode *tnode = node->GetINode();

   NiSkinDataRef data = nifSkin->GetSkinData();
   NiSkinPartitionRef part = nifSkin->GetSkinPartition();

   vector<NiNodeRef> nifBones = nifSkin->GetBones();

   //create a skin modifier and add it
   Modifier *skinMod = GetSkin(tnode);
   TriObject *triObject = GetTriObject(tnode->GetObjectRef());
   Mesh& m = triObject->GetMesh();

   //get the skin interface
   if (ISkin *skin = (ISkin *) skinMod->GetInterface(I_SKIN)){
      ISkinImportData* iskinImport = (ISkinImportData*) skinMod->GetInterface(I_SKINIMPORTDATA);

      // Create Bone List
      Tab<INode*> bones;
      int i=0;
      for (vector<NiNodeRef>::iterator itr = nifBones.begin(), end = nifBones.end(); itr != end; ++itr, ++i){
         string name = (*itr)->GetName();
         INode *boneRef = gi->GetINodeByName(name.c_str());
         bones.Append(1, &boneRef);
         iskinImport->AddBoneEx(boneRef, TRUE);

         // Set Bone Transform
         //Matrix3 tm = boneRef->GetObjectTM(0);
         //Matrix3 m = TOMATRIX3(data->GetBoneTransform(i));
         //iskinImport->SetBoneTm(boneRef, tm, m);
      }
      ObjectState os = tnode->EvalWorldState(0);

      // Need to trigger ModifyObject in BonesDefMod prior to adding vertices or nothing is added
      GetCOREInterface()->ForceCompleteRedraw();

      // Need to get a list of bones and weights for each vertex.
      vector<VertexHolder> vertexHolders;
      vertexHolders.resize(m.numVerts);
      for (int i=0, n=data->GetBoneCount();i<n; ++i){
         if (INode *boneRef = bones[i]){
            vector<SkinWeight> weights = data->GetBoneWeights(i);
            for (vector<SkinWeight>::iterator itr=weights.begin(), end=weights.end(); itr != end; ++itr){
               VertexHolder& h = vertexHolders[itr->index];
               h.vertIndex = itr->index;
               ++h.count;
               h.weights.Append(1, &itr->weight);
               h.boneNodeList.Append(1, &boneRef);
            }
         }
      }

      // Assign the weights 
      for (vector<VertexHolder>::iterator itr=vertexHolders.begin(), end=vertexHolders.end(); itr != end; ++itr){
         VertexHolder& h = (*itr);
         if (h.count){
            float sum = 0.0f;
            for (int i=0; i<h.count; ++i)
               sum += h.weights[i];
            ASSERT(fabs(sum-1.0) < 0.001);
            BOOL add = iskinImport->AddWeights(tnode, h.vertIndex, h.boneNodeList, h.weights);
            add = add;
         }
      }
   }
   return ok;
}

string NifImporter::FindImage(const string& name)
{
   TCHAR buffer[MAX_PATH];

   // Simply check for fully qualified path
   if (PathIsRoot(name.c_str())) {
      if (-1 != _taccess(name.c_str(), 0))
         return string(buffer);
   } 
   if (!path.empty()) {
      PathCombine(buffer, path.c_str(), name.c_str()); // try as-is
      if (-1 != _taccess(buffer, 0))
         return string(buffer);

      // try only filename in nif directory
      PathCombine(buffer, path.c_str(), PathFindFileName(name.c_str()));
      if (-1 != _taccess(buffer, 0))
         return string(buffer);
   }
   if (appSettings != NULL) {
      return appSettings->FindImage(name);
   }
   return name;
}