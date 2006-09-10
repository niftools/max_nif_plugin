#include "StdAfx.h"
#include "KFImporter.h"
#include <kfm.h>
#include "gen/ControllerLink.h"
using namespace Niflib;

KFImporter::KFImporter(const TCHAR *Name,ImpInterface *I,Interface *GI, BOOL SuppressPrompts)
   : BaseClass()
{
   BaseInit(Name, I, GI, SuppressPrompts);
}

KFImporter::~KFImporter(void)
{
}

void KFImporter::ReadBlocks()
{
   try
   {
      kf = DynamicCast<NiControllerSequence>(ReadNifList(name.c_str()));
      BuildNodes();
   }
   catch (std::exception&)
   {
   }
   catch (...) 
   {
   }
}

bool KFImporter::DoImport()
{
   if (!suppressPrompts)
   {
      if (!ShowDialog())
         return true;
      ApplyAppSettings();
      SaveIniSettings();
   }

   ClearAnimation();
   return ImportAnimation();
}
