#include "StdAfx.h"
#include "KFImporter.h"
#include <kfm.h>
#include "gen/ControllerLink.h"
#include "obj/NiSequenceStreamHelper.h"
#include "obj/NiTextKeyExtraData.h"
#include "obj/NiTimeController.h"
#include <float.h>
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
	   Niflib::NifInfo info;
	   int opts=0;

	   // Handle Freedom Force Animation Import
	   std::vector<NiObjectRef> roots = ReadNifList(name.c_str(), &info);
	   kf = DynamicCast<NiControllerSequence>(roots);
	   if (kf.size() == 0)
	   {
		   std::vector<NiSequenceStreamHelperRef> helpers = DynamicCast<NiSequenceStreamHelper>(roots);
		   for (std::vector<NiSequenceStreamHelperRef>::iterator itr = helpers.begin(); itr != helpers.end(); ++itr)
		   {
			   NiSequenceStreamHelperRef helper = (*itr);

			   list< Ref<NiExtraData> > data = helper->GetExtraData();
			   NiTextKeyExtraDataRef textKey = DynamicCast<NiTextKeyExtraData>(data).front();
			   list< NiStringExtraDataRef > keys = DynamicCast<NiStringExtraData>(data);
			   list< Ref<NiTimeController> > controllers = helper->GetControllers();

			   if (keys.size() == controllers.size())
			   {
				   int nk = keys.size();

				   NiControllerSequenceRef seq = new NiControllerSequence();
				   seq->SetName( helper->GetName() );
				   seq->SetTargetName( keys.front()->GetData() ); // Set target name as first
				   seq->SetTextKey( textKey );

				   float start = FLT_MAX, stop = FLT_MIN;
				   vector< Key<string> > textKeys = textKey->GetKeys();
				   for (vector< Key<string> >::iterator tItr = textKeys.begin(); tItr != textKeys.end(); ++tItr) {
					   start = min(start, tItr->time);
					   stop = max(stop, tItr->time);
				   }
				   seq->SetStartTime( start );
				   seq->SetStopTime( stop );

				   list< NiStringExtraDataRef >::iterator keyItr = keys.begin();
				   list< NiTimeControllerRef >::iterator ctrlItr = controllers.begin();
				   for (int i=0; i<nk; ++i, ++keyItr, ++ctrlItr) {
					   seq->AddController( (*keyItr)->GetData(), (*ctrlItr) );
				   }
				   kf.push_back( seq );
			   }
		   }
	   }

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
