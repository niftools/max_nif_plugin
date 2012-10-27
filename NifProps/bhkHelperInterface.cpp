/**********************************************************************
*<
FILE: bhkHelperInterface.cpp

DESCRIPTION:	Collision Helper Object Implementation

CREATED BY: tazpn (Theo)

HISTORY: 
  V1.0 - Derived from 3ds max prim sphere example

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#pragma warning( disable:4800 )
#include <max.h>
#include "MAX_Mem.h"
#include <map>
#include "NifProps.h"
#include "iparamm.h"
#include "Simpobj.h"
#include "notify.h"
#include "macroRec.h"
#include "bhkHelperInterface.h"
#include "NifPlugins.h"
#include "NifGui.h"
#include "NifStrings.h"
//--- Parameter map/block descriptors -------------------------------


FPInterfaceDesc thebhkHelperInterface(
   BHKHELPERINTERFACE_DESC, _T("bhkHelper"), -1/*IDS_OPS*/, 0, FP_MIXIN,

		bhkHelperInterface::get_mesh, _T("getMesh"), 0, TYPE_MESH, 0, 0,
   end);

FPInterfaceDesc *bhkHelperInterface::GetDesc() {
   return &thebhkHelperInterface;
}

FPInterfaceDesc* GetbhkHelperInterfaceDesc()
{
   return &thebhkHelperInterface;
}
