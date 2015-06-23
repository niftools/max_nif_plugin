
#pragma once

//! The unique instance of the helper interface
extern CoreExport FPInterfaceDesc gbhkHelperDesc;

const Interface_ID BHKHELPERINTERFACE_DESC( 0xd4ecb630, 0x52c54024 );

extern FPInterfaceDesc* GetbhkHelperInterfaceDesc( );

class bhkHelperInterface : public FPMixinInterface
{
public:
	virtual const Mesh* GetMesh( ) const = 0;

	// Function Publishing system
	enum
	{
		get_mesh,
	};

	//Function Map For Mixin Interface
	//*************************************************
	BEGIN_FUNCTION_MAP
		FN_0(get_mesh, TYPE_MESH, GetMesh)							\
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc( ); // <--- Must Implement
	//**************************************************
};