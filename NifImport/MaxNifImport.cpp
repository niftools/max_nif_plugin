/**********************************************************************
 *<
	FILE: MaxNifImport.cpp

	DESCRIPTION:	Appwizard generated plugin

	CREATED BY: tazpn (Theo)

	HISTORY: 

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/
#include "stdafx.h"
#include "MaxNifImport.h"
#include "KFMImporter.h"
#include "KFImporter.h"
using namespace Niflib;

#define MaxNifImport_CLASS_ID	Class_ID(0x794ac1c1, 0x8b4c64c7)

string shortDescription;

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
   shortDescription = GetIniValue<string>("System", "ShortDescription", "Netimmerse/Gamebryo", iniFileName.c_str());
}

MaxNifImport::~MaxNifImport() 
{

}

int MaxNifImport::ExtCount()
{
#ifdef USE_UNSUPPORTED_CODE
	return 3;
#else
   return 2;
#endif
}

const TCHAR *MaxNifImport::Ext(int n)
{		
   switch (n)
   {
   case 0: return _T("NIF");
   case 1: return _T("KF");
   case 2: return _T("KFM");
   }
   return _T("");	
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

      LPCTSTR ext = PathFindExtension(filename);

      if (_tcsicmp(ext, ".NIF") == 0)
      {
         NifImporter importer(filename, i, gi, suppressPrompts);
         if (!importer.isValid())
            return FALSE;
         ok = importer.DoImport();
      }
      else if (_tcsicmp(ext, ".KFM") == 0)
      {
         KFMImporter importer(filename, i, gi, suppressPrompts);
         if (!importer.isValid())
            return FALSE;
         ok = importer.DoImport();
      }
      else if (_tcsicmp(ext, ".KF") == 0)
      {
         KFImporter importer(filename, i, gi, suppressPrompts);
         if (!importer.isValid())
            return FALSE;
         ok = importer.DoImport();
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

