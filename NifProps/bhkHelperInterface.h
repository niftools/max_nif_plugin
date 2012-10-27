/**********************************************************************
*<
FILE: bhkHelperInterface.hpp

DESCRIPTION:	Collision Helper Object Declration

CREATED BY: tazpn (Theo)

HISTORY: 
  V1.0 - Derived from 3ds max prim Helper example

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#ifndef __BHKHELPERINTERFACE__H
#define __BHKHELPERINTERFACE__H

#include <ifnpub.h>

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

//! The unique instance of the helper interface
extern CoreExport FPInterfaceDesc gbhkHelperDesc;

const Interface_ID BHKHELPERINTERFACE_DESC(0xd4ecb630, 0x52c54024);

extern FPInterfaceDesc* GetbhkHelperInterfaceDesc();

class bhkHelperInterface : public FPMixinInterface
{
public:			
   virtual const Mesh* GetMesh() const = 0 ;

   //Function Publishing System
   enum {  
      get_mesh,
   };

   //Function Map For Mixin Interface
   //*************************************************
   BEGIN_FUNCTION_MAP
     FN_0(get_mesh, TYPE_MESH, GetMesh)							\
   END_FUNCTION_MAP

   FPInterfaceDesc* GetDesc(); // <--- Must Implement
   //**************************************************
};

#endif