/**********************************************************************
 *<
	FILE: NifFurniture.cpp

	DESCRIPTION:	Appwizard generated plugin

	CREATED BY: 

	HISTORY: 

 *>	Copyright (c) 2003, All Rights Reserved.
 **********************************************************************/

#include "NifFurniture.h"

#define NifFurniture_CLASS_ID	Class_ID(0x5a38017d, 0x4006a661)

#define PBLOCK_REF	0

class NifFurnCreateCallBack : public CreateMouseCallBack
{

public:

	virtual int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat);
};

int NifFurnCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
{
	return CREATE_ABORT;
}

class NifFurniture : public HelperObject {
	public:

		// Parameter block
//		IParamBlock2	*pblock;	//ref 0

		// Loading/Saving
		IOResult Load(ILoad *iload) {return IO_OK;}
		IOResult Save(ISave *isave) {return IO_OK;}

		//From Animatable
		Class_ID ClassID() {return NifFurniture_CLASS_ID;}		
		SClass_ID SuperClassID() { return HELPER_CLASS_ID; }
		void GetClassName(TSTR& s) {s = GetString(IDS_CLASS_NAME);}

		RefTargetHandle Clone( RemapDir &remap );
		RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
			PartID& partID,  RefMessage message);

		int NumSubs() { return 1; }

//		int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
//		IParamBlock2* GetParamBlock(int i) { return pblock; } // return i'th ParamBlock
//		IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

		void DeleteThis() { delete this; }	
		// BaseObject
		CreateMouseCallBack* GetCreateMouseCallBack();
		// Object
		ObjectState Eval(TimeValue t);
		
		//Constructor/Destructor
		NifFurniture() {}
		~NifFurniture() {}		

};



class NifFurnitureClassDesc : public ClassDesc2 {
	public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return new NifFurniture(); }
	const TCHAR *	ClassName() { return GetString(IDS_CLASS_NAME); }
	SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
	Class_ID		ClassID() { return NifFurniture_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("NifFurniture"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
	

};

static NifFurnitureClassDesc NifFurnitureDesc;
ClassDesc2* GetNifFurnitureDesc() { return &NifFurnitureDesc; }


RefResult NifFurniture::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
			PartID& partID,  RefMessage message)
{
	return REF_SUCCEED;
}

RefTargetHandle NifFurniture::Clone( RemapDir &remap )
{
	return NULL;
}

ObjectState NifFurniture::Eval(TimeValue t)
{
	ObjectState os;
	return os;
}

CreateMouseCallBack *NifFurniture::GetCreateMouseCallBack()
{
	static NifFurnCreateCallBack cb;
	return &cb;
}
