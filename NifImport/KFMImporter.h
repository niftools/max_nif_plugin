/**********************************************************************
*<
FILE: KFMImporter.h

DESCRIPTION:	KFM Importer helper class 

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#ifndef _KFMIMPORTER_H_
#define _KFMIMPORTER_H_

#pragma once
#include "NIFImporter.h"
#include "niutils.h"
#include "obj/NiControllerSequence.h"

class KFMImporter : public NifImporter
{
   typedef NifImporter BaseClass;
public:
   KFMImporter(const TCHAR *Name,ImpInterface *I,Interface *GI, BOOL SuppressPrompts);
   KFMImporter();
   ~KFMImporter(void);

   void ReadBlocks();
   virtual bool DoImport();

   virtual bool isValid() const { return (NULL != root) || !kf.empty(); }

   // Implemented in ImportAnimation.cpp
   virtual bool ImportAnimation();
   virtual void SaveIniSettings();

   bool ShowDialog();

   std::vector<Niflib::NiControllerSequenceRef> kf;
};


#endif 