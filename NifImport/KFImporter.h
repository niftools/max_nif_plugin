/**********************************************************************
*<
FILE: KFImporter.h

DESCRIPTION:	KF Importer helper class 

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#ifndef _KFIMPORTER_H_
#define _KFIMPORTER_H_

#pragma once
#include "NIFImporter.h"
#include "KFMImporter.h"
#include "niutils.h"
#include "obj/NiControllerSequence.h"

class KFImporter : public KFMImporter
{
   typedef KFMImporter BaseClass;
public:
   KFImporter(const TCHAR *Name,ImpInterface *I,Interface *GI, BOOL SuppressPrompts);
   ~KFImporter(void);

   void ReadBlocks();
   virtual bool DoImport();
};


#endif 