#include "StdAfx.h"
#include "KFMImporter.h"
#include <kfm.h>
#include "gen/ControllerLink.h"
using namespace Niflib;

KFMImporter::KFMImporter(const TCHAR *Name,ImpInterface *I,Interface *GI, BOOL SuppressPrompts)
   : BaseClass()
{
   BaseInit(Name, I, GI, SuppressPrompts);
}

KFMImporter::~KFMImporter(void)
{
}

void KFMImporter::ReadBlocks()
{
   try
   {
      Kfm kfm;
      int ver = kfm.Read(name);
      if (ver != VER_UNSUPPORTED)
      {
         TCHAR buffer[MAX_PATH];
         GetFullPathName(name.c_str(), MAX_PATH, buffer, NULL);
         PathRemoveFileSpec(buffer);
#ifdef USE_UNSUPPORTED_CODE
         //root = kfm.MergeActions(string(buffer));
         BuildNodes();

         string last_file;
         vector<NiControllerSequenceRef> ctrllist;

         for ( vector<KfmAction>::iterator it = kfm.actions.begin(); it != kfm.actions.end(); it++ ) {
            string action_filename = path + '\\' + it->action_filename;
            if (-1 != _taccess(action_filename.c_str(), 0)){
               if (action_filename != last_file) {
                   ctrllist = DynamicCast<NiControllerSequence>(ReadNifList(action_filename));
               }
               if ((*it).unk_int2 && (*it).unk_int2 <= ctrllist.size()) {
                  if (NiControllerSequenceRef ctrlSeq = ctrllist[(*it).unk_int2-1])
                     kf.push_back(ctrlSeq);
               }
            }
         }
#endif
      }
   }
   catch (std::exception&)
   {
   }
   catch (...) 
   {
   }
}

bool KFMImporter::DoImport()
{
   // might check if blocks exist and if not go ahead and import nif.
   return ImportAnimation();
   //return BaseClass::DoImport();
}
