
#include "stdafx.h"

//--- Parameter map/block descriptors -------------------------------

FPInterfaceDesc thebhkHelperInterface(
	BHKHELPERINTERFACE_DESC, _T( "bhkHelper" ), -1 /*IDS_OPS*/, 0, FP_MIXIN,
		bhkHelperInterface::get_mesh, _T( "getMesh" ), 0, TYPE_MESH, 0, 0,

		properties,
		p_end
);

FPInterfaceDesc* bhkHelperInterface::GetDesc( )
{
	return &thebhkHelperInterface;
}

FPInterfaceDesc* GetbhkHelperInterfaceDesc( )
{
	return &thebhkHelperInterface;
}