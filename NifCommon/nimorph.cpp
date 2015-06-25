
#include "stdafx.h"

Modifier* GetMorpherModifier( INode* node )
{
	const Class_ID MORPHERMODIFIER_CLASS_ID( 0x17bb6854, 0xa5cba2a3 );

	Object* pObj = node->GetObjectRef( );
	if( !pObj )
		return NULL;

	while( pObj->SuperClassID( ) == GEN_DERIVOB_CLASS_ID )
	{
		IDerivedObject* pDerObj = (IDerivedObject*) ( pObj );
		int Idx = 0;

		while( Idx < pDerObj->NumModifiers( ) )
		{
			// Get the modifier
			Modifier* mod = pDerObj->GetModifier( Idx );
			if( mod->ClassID( ) == MORPHERMODIFIER_CLASS_ID )
				return mod;

			Idx++;
		}

		pObj = pDerObj->GetObjRef( );
	}

	return NULL;
}

TSTR MorpherGetName( Modifier* mod, int index )
{
	// Magic initialization stuff for maxscript
	static bool script_initialized = false;
	if( !script_initialized )
	{
		init_MAXScript( );
		script_initialized = true;
	}

	init_thread_locals( );
	push_alloc_frame( );
	six_value_locals( name, fn, mod, index, value, result );
	save_current_frames( );
	set_error_trace_back_active( FALSE );

	TSTR string;
	try
	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern( _T( "WM3_MC_GetName" ) );
		vl.fn = globals->get( vl.name );

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while( vl.fn != NULL && is_globalthunk( vl.fn ) )
			vl.fn = static_cast<GlobalThunk*>( vl.fn )->cell;

		while( vl.fn != NULL && is_constglobalthunk( vl.fn ) )
			vl.fn = static_cast<ConstGlobalThunk*>( vl.fn )->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		// class_tag(MAXScriptFunction)
		if( vl.fn != NULL && vl.fn->tag == class_tag( Primitive ) )
		{
			Value* args[ 2 ];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[ 0 ] = vl.mod = MAXModifier::intern( mod ); // The original material
			args[ 1 ] = vl.index = Integer::intern( index );

			// Call the function and save the result.
			vl.result = static_cast<Primitive*>( vl.fn )->apply( args, 2 );
			if( vl.result->tag == class_tag( String ) )
				string = vl.result->to_string( );
		}
	}
	catch( ... )
	{
		clear_error_source_data( );
		restore_current_frames( );
		MAXScript_signals = 0;

		if( progress_bar_up )
		{
			MAXScript_interface->ProgressEnd( );
			progress_bar_up = FALSE;
		}
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals( );
	pop_alloc_frame( );

	return string;
}

bool MorpherIsActive( Modifier* mod, int index )
{
	bool retval = false;

	// Magic initialization stuff for maxscript
	static bool script_initialized = false;
	if( !script_initialized )
	{
		init_MAXScript( );
		script_initialized = true;
	}

	init_thread_locals( );
	push_alloc_frame( );
	six_value_locals( name, fn, mod, index, value, result );
	save_current_frames( );
	set_error_trace_back_active( FALSE );

	try
	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern( _T( "WM3_MC_IsActive" ) );
		vl.fn = globals->get( vl.name );

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while( vl.fn != NULL && is_globalthunk( vl.fn ) )
			vl.fn = static_cast<GlobalThunk*>( vl.fn )->cell;

		while( vl.fn != NULL && is_constglobalthunk( vl.fn ) )
			vl.fn = static_cast<ConstGlobalThunk*>( vl.fn )->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		// class_tag(MAXScriptFunction)
		if( vl.fn != NULL && vl.fn->tag == class_tag( Primitive ) )
		{
			Value* args[ 2 ];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[ 0 ] = vl.mod = MAXModifier::intern( mod ); // The original material
			args[ 1 ] = vl.index = Integer::intern( index );

			// Call the function and save the result.
			vl.result = static_cast<Primitive*>( vl.fn )->apply( args, 2 );
			if( vl.result->tag == class_tag( Boolean ) )
				retval = vl.result->to_bool( );
		}
	}
	catch( ... )
	{
		clear_error_source_data( );
		restore_current_frames( );
		MAXScript_signals = 0;

		if( progress_bar_up )
		{
			MAXScript_interface->ProgressEnd( );
			progress_bar_up = FALSE;
		}
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals();
	pop_alloc_frame();

	return retval;
}

bool MorpherHasData( Modifier* mod, int index )
{
	bool retval = false;

	// Magic initialization stuff for maxscript
	static bool script_initialized = false;
	if( !script_initialized )
	{
		init_MAXScript( );
		script_initialized = true;
	}

	init_thread_locals( );
	push_alloc_frame( );
	six_value_locals( name, fn, mod, index, value, result );
	save_current_frames( );
	set_error_trace_back_active( FALSE );

	try
	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern( _T( "WM3_MC_HasData" ) );
		vl.fn = globals->get( vl.name );

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while( vl.fn != NULL && is_globalthunk( vl.fn ) )
			vl.fn = static_cast<GlobalThunk*>( vl.fn )->cell;

		while( vl.fn != NULL && is_constglobalthunk( vl.fn ) )
			vl.fn = static_cast<ConstGlobalThunk*>( vl.fn )->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		// class_tag(MAXScriptFunction)
		if( vl.fn != NULL && vl.fn->tag == class_tag( Primitive ) )
		{
			Value* args[ 2 ];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[ 0 ] = vl.mod = MAXModifier::intern( mod ); // The original material
			args[ 1 ] = vl.index = Integer::intern( index );

			// Call the function and save the result.
			vl.result = static_cast<Primitive*>( vl.fn )->apply( args, 2 );
			if( vl.result->tag == class_tag( Boolean ) )
				retval = vl.result->to_bool( );
		}
	}
	catch( ... )
	{
		clear_error_source_data( );
		restore_current_frames( );
		MAXScript_signals = 0;

		if( progress_bar_up )
		{
			MAXScript_interface->ProgressEnd( );
			progress_bar_up = FALSE;
		}
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals();
	pop_alloc_frame();

	return retval;
}

int MorpherNumProgMorphs( Modifier* mod, int index )
{
	int retval = 0;

	// Magic initialization stuff for maxscript
	static bool script_initialized = false;
	if( !script_initialized )
	{
		init_MAXScript( );
		script_initialized = true;
	}

	init_thread_locals( );
	push_alloc_frame( );
	six_value_locals( name, fn, mod, index, value, result );
	save_current_frames( );
	set_error_trace_back_active( FALSE );

	try
	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern( _T( "WM3_MC_HasData" ) );
		vl.fn = globals->get( vl.name );

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while( vl.fn != NULL && is_globalthunk( vl.fn ) )
			vl.fn = static_cast<GlobalThunk*>( vl.fn )->cell;

		while( vl.fn != NULL && is_constglobalthunk( vl.fn ) )
			vl.fn = static_cast<ConstGlobalThunk*>( vl.fn )->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		// class_tag(MAXScriptFunction)
		if( vl.fn != NULL && vl.fn->tag == class_tag( Primitive ) )
		{
			Value* args[ 2 ];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[ 0 ] = vl.mod = MAXModifier::intern( mod ); // The original material
			args[ 1 ] = vl.index = Integer::intern( index );

			// Call the function and save the result.
			vl.result = static_cast<Primitive*>( vl.fn )->apply( args, 2 );
			if( vl.result->tag == class_tag( Integer ) )
				retval = vl.result->to_int( );
		}
	}
	catch( ... )
	{
		clear_error_source_data( );
		restore_current_frames( );
		MAXScript_signals = 0;

		if( progress_bar_up )
		{
			MAXScript_interface->ProgressEnd( );
			progress_bar_up = FALSE;
		}
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals();
	pop_alloc_frame();

	return retval;
}

INode* MorpherGetProgMorph( Modifier* mod, int index, int morphIdx )
{
	INode* retval = NULL;

	// Magic initialization stuff for maxscript
	static bool script_initialized = false;
	if( !script_initialized )
	{
		init_MAXScript( );
		script_initialized = true;
	}

	init_thread_locals( );
	push_alloc_frame( );
	six_value_locals( name, fn, mod, index, midx, result );
	save_current_frames( );
	set_error_trace_back_active( FALSE );

	try
	{
		// Create the name of the maxscript function we want.
		// and look it up in the global names
		vl.name = Name::intern( _T( "WM3_MC_HasData" ) );
		vl.fn = globals->get( vl.name );

		// For some reason we get a global thunk back, so lets
		// check the cell which should point to the function.
		// Just in case if it points to another global thunk
		// try it again.
		while( vl.fn != NULL && is_globalthunk( vl.fn ) )
			vl.fn = static_cast<GlobalThunk*>( vl.fn )->cell;

		while( vl.fn != NULL && is_constglobalthunk( vl.fn ) )
			vl.fn = static_cast<ConstGlobalThunk*>( vl.fn )->cell;

		// Now we should have a MAXScriptFunction, which we can
		// call to do the actual conversion. If we didn't
		// get a MAXScriptFunction, we can't convert.
		// class_tag(MAXScriptFunction)
		if( vl.fn != NULL && vl.fn->tag == class_tag( Primitive ) )
		{
			Value* args[ 3 ];

			// Ok. WM3_MC_BuildFromNode takes three parameters
			args[ 0 ] = vl.mod = MAXModifier::intern( mod ); // The original material
			args[ 1 ] = vl.index = Integer::intern( index );
			args[ 2 ] = vl.midx = Integer::intern( morphIdx );

			// Call the function and save the result.
			vl.result = static_cast<Primitive*>( vl.fn )->apply( args, 2 );
			if( vl.result->tag == class_tag( MAXNode ) )
				retval = vl.result->to_node( );
		}
	}
	catch( ... )
	{
		clear_error_source_data( );
		restore_current_frames( );
		MAXScript_signals = 0;

		if( progress_bar_up )
		{
			MAXScript_interface->ProgressEnd( );
			progress_bar_up = FALSE;
		}
	}

	// Magic Max Script stuff to clear the frame and locals.
	pop_value_locals();
	pop_alloc_frame();

	return retval;
}
