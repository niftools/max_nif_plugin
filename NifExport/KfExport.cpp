#include "pch.h"
#include "AppSettings.h"
#include "niutils.h"
#include  <io.h>

using namespace Niflib;

#define KFEXPORT_CLASS_ID	Class_ID(0xa57ff0a4, 0xa0374ffc)

static LPCTSTR KfExportSection = TEXT("MaxKfExport");

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
   const TCHAR		*ClassName() { return GetString(IDS_CLASS_NAME); }
   SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
   Class_ID		ClassID() { return KFEXPORT_CLASS_ID; }
   const TCHAR		*Category() { return GetString(IDS_CATEGORY); }

   const TCHAR		*InternalName() { return _T("KfExport"); }	// returns fixed parsable name (scripter-visible name)
   HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle


};

static KfExportClassDesc KfExportDesc;
ClassDesc2* GetKfExportDesc() { return &KfExportDesc; }


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
   return _T("http://niftools.sourceforge.net");
}

const TCHAR *KfExport::OtherMessage1() 
{		
   return _T("http://niftools.sourceforge.net");
}

const TCHAR *KfExport::OtherMessage2() 
{		
   return _T("http://niftools.sourceforge.net");
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
   // read application settings
   AppSettings::Initialize(i);

   return TRUE;
}