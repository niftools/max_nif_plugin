/**********************************************************************
*<
FILE: NIUtils.cpp

DESCRIPTION:	NifImporter Utilities

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "pch.h"
#include "niutils.h"
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <malloc.h>
#include <sstream>
#include <modstack.h>
#include <iparamb2.h>
#include <iskin.h>
#include "../NifProps/bhkRigidBodyInterface.h"

#ifdef USE_BIPED
#  include <cs/BipedApi.h>
#  include <cs/OurExp.h> 
#endif
#if VERSION_3DSMAX < (14000<<16) // Version 14 (2012)
#include "maxscrpt\Strings.h"
#include "maxscrpt\Parser.h"
//static inline void set_error_trace_back_active( BOOL value ) { trace_back_active = value; }
#else
#include <maxscript/maxscript.h>
#include <maxscript/compiler/parser.h>
#endif

using namespace std;
using namespace Niflib;

Modifier *GetMorpherModifier(INode* node)
{
	const Class_ID MORPHERMODIFIER_CLASS_ID(0x17bb6854, 0xa5cba2a3);

	Object* pObj = node->GetObjectRef();
	if (!pObj) return NULL;
	while (pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		IDerivedObject* pDerObj = (IDerivedObject *)(pObj);
		int Idx = 0;
		while (Idx < pDerObj->NumModifiers())
		{
			// Get the modifier. 
			Modifier* mod = pDerObj->GetModifier(Idx);
			if (mod->ClassID() == MORPHERMODIFIER_CLASS_ID)
			{
				return mod;
			}
			Idx++;
		}
		pObj = pDerObj->GetObjRef();
	}
	return NULL;
}

Modifier *CreateMorpherModifier(INode* node)
{
	const Class_ID MORPHERMODIFIER_CLASS_ID(0x17bb6854, 0xa5cba2a3);

	Modifier *mod = GetMorpherModifier(node);
	if (mod == NULL)
	{
		IDerivedObject *dobj = CreateDerivedObject(node->GetObjectRef());
		mod = (Modifier*) CreateInstance(OSM_CLASS_ID, MORPHERMODIFIER_CLASS_ID);
		dobj->SetAFlag(A_LOCK_TARGET);
		dobj->AddModifier(mod);
		dobj->ClearAFlag(A_LOCK_TARGET);
		node->SetObjectRef(dobj);
	}
	return mod;
}

// CallMaxscript
// Send the string to maxscript 
//
void MorpherBuildFromNode(Modifier* mod, int index, INode *target)
{
	// Magic initialization stuff for maxscript.
	static bool script_initialized = false;
	if (!script_initialized) {
		init_MAXScript();
		script_initialized = TRUE;
	}
	init_thread_locals();
	push_alloc_frame();
	six_value_locals(name, fn, mod, index, target, result);
	save_current_frames();
	set_error_trace_back_active(FALSE);

	try	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern(_T("WM3_MC_BuildFromNode"));
		vl.fn = globals->get(vl.name);

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while (vl.fn != NULL && is_globalthunk(vl.fn))
			vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;
		while (vl.fn != NULL && is_constglobalthunk(vl.fn))
			vl.fn = static_cast<ConstGlobalThunk*>(vl.fn)->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		if (vl.fn != NULL && vl.fn->tag == class_tag(Primitive)) {
			Value* args[3];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[0] = vl.mod = MAXModifier::intern(mod);	// The original material
			args[1] = vl.index = Integer::intern(index);
			args[2] = vl.target = MAXNode::intern(target);

			// Call the function and save the result.
			vl.result = static_cast<Primitive*>(vl.fn)->apply(args, 3);
		}
	} catch (...) {
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals();
	pop_alloc_frame();
}

TSTR MorpherGetName(Modifier* mod, int index)
{
	// Magic initialization stuff for maxscript.
	static bool script_initialized = false;
	if (!script_initialized) {
		init_MAXScript();
		script_initialized = TRUE;
	}
	init_thread_locals();
	push_alloc_frame();
	six_value_locals(name, fn, mod, index, value, result);
	save_current_frames();
	set_error_trace_back_active(FALSE);
	TSTR string;
	try	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern(_T("WM3_MC_GetName"));
		vl.fn = globals->get(vl.name);

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while (vl.fn != NULL && is_globalthunk(vl.fn))
			vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;
		while (vl.fn != NULL && is_constglobalthunk(vl.fn))
			vl.fn = static_cast<ConstGlobalThunk*>(vl.fn)->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		// class_tag(MAXScriptFunction)
		if (vl.fn != NULL && vl.fn->tag == class_tag(Primitive)) {
			Value* args[2];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[0] = vl.mod = MAXModifier::intern(mod);	// The original material
			args[1] = vl.index = Integer::intern(index);

			// Call the function and save the result.
			vl.result = static_cast<Primitive*>(vl.fn)->apply(args, 2);
			if (vl.result->tag == class_tag(String)) {
				string = vl.result->to_string();
			}
		}
	} catch (...) {
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals();
	pop_alloc_frame();
	return string;
}

void MorpherSetName(Modifier* mod, int index, TSTR& name)
{
	// Magic initialization stuff for maxscript.
	static bool script_initialized = false;
	if (!script_initialized) {
		init_MAXScript();
		script_initialized = TRUE;
	}
	init_thread_locals();
	push_alloc_frame();
	six_value_locals(name, fn, mod, index, value, result);
	save_current_frames();
	set_error_trace_back_active(FALSE);
	String* value = new String(name);

	try	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern(_T("WM3_MC_SetName"));
		vl.fn = globals->get(vl.name);

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while (vl.fn != NULL && is_globalthunk(vl.fn))
			vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;
		while (vl.fn != NULL && is_constglobalthunk(vl.fn))
			vl.fn = static_cast<ConstGlobalThunk*>(vl.fn)->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		// class_tag(MAXScriptFunction)
		if (vl.fn != NULL && vl.fn->tag == class_tag(Primitive)) {
			Value* args[3];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[0] = vl.mod = MAXModifier::intern(mod);	// The original material
			args[1] = vl.index = Integer::intern(index);
			args[2] = vl.value = value;

			// Call the function and save the result.
			vl.result = static_cast<Primitive*>(vl.fn)->apply(args, 3);
		}
	} catch (...) {
		value->collect();
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals();
	pop_alloc_frame();
}

void MorpherRebuild(Modifier* mod, int index)
{
	// Magic initialization stuff for maxscript.
	static bool script_initialized = false;
	if (!script_initialized) {
		init_MAXScript();
		script_initialized = TRUE;
	}
	init_thread_locals();
	push_alloc_frame();
	five_value_locals(name, fn, mod, index, result);
	save_current_frames();
	set_error_trace_back_active(FALSE);
	try	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern(_T("WM3_MC_Rebuild"));
		vl.fn = globals->get(vl.name);

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while (vl.fn != NULL && is_globalthunk(vl.fn))
			vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;
		while (vl.fn != NULL && is_constglobalthunk(vl.fn))
			vl.fn = static_cast<ConstGlobalThunk*>(vl.fn)->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		// class_tag(MAXScriptFunction)
		if (vl.fn != NULL && vl.fn->tag == class_tag(Primitive)) {
			Value* args[2];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[0] = vl.mod = MAXModifier::intern(mod);	// The original material
			args[1] = vl.index = Integer::intern(index);

			// Call the function and save the result.
			vl.result = static_cast<Primitive*>(vl.fn)->apply(args, 2);
		}
	} catch (...) {
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals();
	pop_alloc_frame();
}

bool MorpherIsActive(Modifier* mod, int index)
{
	bool retval = false;
	// Magic initialization stuff for maxscript.
	static bool script_initialized = false;
	if (!script_initialized) {
		init_MAXScript();
		script_initialized = TRUE;
	}
	init_thread_locals();
	push_alloc_frame();
	five_value_locals(name, fn, mod, index, result);
	save_current_frames();
	set_error_trace_back_active(FALSE);
	try	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern(_T("WM3_MC_IsActive"));
		vl.fn = globals->get(vl.name);

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while (vl.fn != NULL && is_globalthunk(vl.fn))
			vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;
		while (vl.fn != NULL && is_constglobalthunk(vl.fn))
			vl.fn = static_cast<ConstGlobalThunk*>(vl.fn)->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		// class_tag(MAXScriptFunction)
		if (vl.fn != NULL && vl.fn->tag == class_tag(Primitive)) {
			Value* args[2];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[0] = vl.mod = MAXModifier::intern(mod);	// The original material
			args[1] = vl.index = Integer::intern(index);

			// Call the function and save the result.
			vl.result = static_cast<Primitive*>(vl.fn)->apply(args, 2);
			if (vl.result->tag == class_tag(Boolean))
				retval = vl.result->to_bool();
		}
	} catch (...) {
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals();
	pop_alloc_frame();
	return retval;
}

bool MorpherHasData(Modifier* mod, int index)
{
	bool retval = false;
	// Magic initialization stuff for maxscript.
	static bool script_initialized = false;
	if (!script_initialized) {
		init_MAXScript();
		script_initialized = TRUE;
	}
	init_thread_locals();
	push_alloc_frame();
	five_value_locals(name, fn, mod, index, result);
	save_current_frames();
	set_error_trace_back_active(FALSE);
	try	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern(_T("WM3_MC_HasData"));
		vl.fn = globals->get(vl.name);

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while (vl.fn != NULL && is_globalthunk(vl.fn))
			vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;
		while (vl.fn != NULL && is_constglobalthunk(vl.fn))
			vl.fn = static_cast<ConstGlobalThunk*>(vl.fn)->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		// class_tag(MAXScriptFunction)
		if (vl.fn != NULL && vl.fn->tag == class_tag(Primitive)) {
			Value* args[2];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[0] = vl.mod = MAXModifier::intern(mod);	// The original material
			args[1] = vl.index = Integer::intern(index);

			// Call the function and save the result.
			vl.result = static_cast<Primitive*>(vl.fn)->apply(args, 2);
			if (vl.result->tag == class_tag(Boolean))
				retval = vl.result->to_bool();
		}
	} catch (...) {
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals();
	pop_alloc_frame();
	return retval;
}

int MorpherNumProgMorphs(Modifier* mod, int index)
{
	int retval = 0;
	// Magic initialization stuff for maxscript.
	static bool script_initialized = false;
	if (!script_initialized) {
		init_MAXScript();
		script_initialized = TRUE;
	}
	init_thread_locals();
	push_alloc_frame();
	five_value_locals(name, fn, mod, index, result);
	save_current_frames();
	set_error_trace_back_active(FALSE);
	try	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern(_T("WM3_MC_HasData"));
		vl.fn = globals->get(vl.name);

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while (vl.fn != NULL && is_globalthunk(vl.fn))
			vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;
		while (vl.fn != NULL && is_constglobalthunk(vl.fn))
			vl.fn = static_cast<ConstGlobalThunk*>(vl.fn)->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		// class_tag(MAXScriptFunction)
		if (vl.fn != NULL && vl.fn->tag == class_tag(Primitive)) {
			Value* args[2];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[0] = vl.mod = MAXModifier::intern(mod);	// The original material
			args[1] = vl.index = Integer::intern(index);

			// Call the function and save the result.
			vl.result = static_cast<Primitive*>(vl.fn)->apply(args, 2);
			if (vl.result->tag == class_tag(Integer))
				retval = vl.result->to_int();
		}
	} catch (...) {
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals();
	pop_alloc_frame();
	return retval;
}

INode *MorpherGetProgMorph(Modifier* mod, int index, int morphIdx)
{
	INode *retval = 0;
	// Magic initialization stuff for maxscript.
	static bool script_initialized = false;
	if (!script_initialized) {
		init_MAXScript();
		script_initialized = TRUE;
	}
	init_thread_locals();
	push_alloc_frame();
	six_value_locals(name, fn, mod, index, midx, result);
	save_current_frames();
	set_error_trace_back_active(FALSE);
	try	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern(_T("WM3_MC_HasData"));
		vl.fn = globals->get(vl.name);

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while (vl.fn != NULL && is_globalthunk(vl.fn))
			vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;
		while (vl.fn != NULL && is_constglobalthunk(vl.fn))
			vl.fn = static_cast<ConstGlobalThunk*>(vl.fn)->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		// class_tag(MAXScriptFunction)
		if (vl.fn != NULL && vl.fn->tag == class_tag(Primitive)) {
			Value* args[3];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[0] = vl.mod = MAXModifier::intern(mod);	// The original material
			args[1] = vl.index = Integer::intern(index);
			args[2] = vl.midx = Integer::intern(morphIdx);

			// Call the function and save the result.
			vl.result = static_cast<Primitive*>(vl.fn)->apply(args, 3);
			if (vl.result->tag == class_tag(MAXNode))
				retval = vl.result->to_node();
		}
	} catch (...) {
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals();
	pop_alloc_frame();
	return retval;
}

int MorpherGetNumVerts(Modifier* mod, int index)
{
	int retval = 0;
	// Magic initialization stuff for maxscript.
	static bool script_initialized = false;
	if (!script_initialized) {
		init_MAXScript();
		script_initialized = TRUE;
	}
	init_thread_locals();
	push_alloc_frame();
	five_value_locals(name, fn, mod, index, result);
	save_current_frames();
	set_error_trace_back_active(FALSE);
	try	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern(_T("WM3_MC_NumPts"));
		vl.fn = globals->get(vl.name);

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while (vl.fn != NULL && is_globalthunk(vl.fn))
			vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;
		while (vl.fn != NULL && is_constglobalthunk(vl.fn))
			vl.fn = static_cast<ConstGlobalThunk*>(vl.fn)->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		// class_tag(MAXScriptFunction)
		if (vl.fn != NULL && vl.fn->tag == class_tag(Primitive)) {
			Value* args[2];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[0] = vl.mod = MAXModifier::intern(mod);	// The original material
			args[1] = vl.index = Integer::intern(index);

			// Call the function and save the result.
			vl.result = static_cast<Primitive*>(vl.fn)->apply(args, 2);
			if (vl.result->tag == class_tag(Integer))
				retval = vl.result->to_int();
		}
	} catch (...) {
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals();
	pop_alloc_frame();
	return retval;
}

void MorpherGetMorphVerts(Modifier* mod, int index, vector<Vector3>& verts)
{
	int nverts = MorpherGetNumVerts(mod, index);
	verts.resize(nverts, Vector3(0.0f,0.0f,0.0f));
	if (nverts == 0)
		return;

	// Magic initialization stuff for maxscript.
	static bool script_initialized = false;
	if (!script_initialized) {
		init_MAXScript();
		script_initialized = TRUE;
	}
	init_thread_locals();
	push_alloc_frame();
	six_value_locals(name, fn, mod, index, midx, result);
	save_current_frames();
	set_error_trace_back_active(FALSE);
	try	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern(_T("WM3_MC_GetMorphPoint"));
		vl.fn = globals->get(vl.name);

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while (vl.fn != NULL && is_globalthunk(vl.fn))
			vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;
		while (vl.fn != NULL && is_constglobalthunk(vl.fn))
			vl.fn = static_cast<ConstGlobalThunk*>(vl.fn)->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		// class_tag(MAXScriptFunction)
		if (vl.fn != NULL && vl.fn->tag == class_tag(Primitive)) {
			Value* args[3];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[0] = vl.mod = MAXModifier::intern(mod);	// The original material
			args[1] = vl.index = Integer::intern(index);

			for (int i=0; i<nverts; ++i)
			{
				args[2] = vl.midx = Integer::intern(i);
				// Call the function and save the result.
				vl.result = static_cast<Primitive*>(vl.fn)->apply(args, 3);
				if (vl.result->tag == class_tag(Point3Value))
					verts[i] = TOVECTOR3(vl.result->to_point3());
			}
		}
	} catch (...) {
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals();
	pop_alloc_frame();
}