/*===========================================================================*\
 | 
 |  FILE:	objectParams.h
 |			Skeleton project and code for a Utility
 |			3D Studio MAX R3.0
 | 
 |  AUTH:   Cleve Ard
 |			Render Group
 |			Copyright(c) Discreet 2000
 |
 |  HIST:	Started 9-8-00
 | 
\*===========================================================================*/

#ifndef _OBJECTPARAMS_H_
#define _OBJECTPARAMS_H_

#if VERSION_3DSMAX < (14000<<16) // Version 14 (2012)
#include "maxscrpt/maxscrpt.h"
#include "maxscrpt/Numbers.h"
#include "maxscrpt/Name.h"
#include "maxscrpt/ColorVal.h"
#include "maxscrpt/MAXObj.h"
static inline void set_error_trace_back_active( BOOL value ) { trace_back_active = value; }
#else
#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/name.h>
#include <maxscript/foundation/colors.h>
#include <maxscript/maxwrapper/mxsobjects.h>
#include <maxscript\kernel\collectable.h>
#endif
#include "assert1.h"

template <class T>
class ConvertMAXScriptToC;
inline Value* make_maxscript_value(bool v);
inline Value* make_maxscript_value(int v);
inline Value* make_maxscript_value(float v);
inline Value* make_maxscript_value(COLORREF rgb);
inline Value* make_maxscript_value(const Color& rgb);
inline Value* make_maxscript_value(LPCTSTR str);
inline Value* make_maxscript_value(ReferenceTarget* rtarg);

#if MAX_RELEASE <= 7000 // Version 7
inline void clear_error_source_data() {}
#endif

// Not all of the paramters for materials and shaders
// are published in the interfaces. ObjectParams class
// is used to access the properties of objects the same
// way the scriptor does, so you can get to properties.

// Set the array parameter named name to the value at time t.
template<class T>
bool setMAXScriptValue(ReferenceTarget* obj, LPTSTR name, TimeValue t, T value, int tabIndex)
{
	bool rval = false;					// Return false if fails
	assert(obj != NULL);
	init_thread_locals();
	push_alloc_frame();
	two_value_locals(prop, result);		// Keep two local variables
	save_current_frames();
	set_error_trace_back_active(FALSE);

	try {
		// Get the name of the parameter and then retrieve
		// the array holding the value we want to set.
		vl.prop = Name::intern(name);
		vl.result = MAXWrapper::get_property(obj, vl.prop, NULL);

		// Make sure it is the right type.
		if (vl.result != NULL && vl.result->tag == class_tag(MAXPB2ArrayParam)) {
			// OK. Now we make sure the tabIndex is within the array bounds.
			MAXPB2ArrayParam* array = static_cast<MAXPB2ArrayParam*>(vl.result);
			if (array->pblock != NULL && array->pdef != NULL
					&& tabIndex >= 0 && tabIndex < array->pblock->Count(array->pdef->ID)) {
				// Set the value in the array.
				array->pblock->SetValue(array->pdef->ID, 0, value, tabIndex);
				rval = true;		// Succeeded
			}
		}
	} catch ( ... ) {
		// Failed.
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}
	pop_value_locals();
	pop_alloc_frame();
	return rval;
}

// Set the parameter. Cannot be an array entry
template<class T>
bool setMAXScriptValue(ReferenceTarget* obj, LPTSTR name, TimeValue t, T& value)
{
	bool rval = false;					// return false if fails
	assert(obj != NULL);
	init_thread_locals();
	push_alloc_frame();
	two_value_locals(prop, val);			// Keep two local variables
	save_current_frames();
	set_error_trace_back_active( FALSE );

	try {
		// Get the name and value to set
		vl.prop = Name::intern(name);
		vl.val = make_maxscript_value(value);

		// Set the value.
		Value* val = MAXWrapper::set_property(obj, vl.prop, vl.val);
		if (val != NULL)
			rval = true;				// Succeeded
	} catch ( ... ) {
		// Failed.
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}
	pop_value_locals();
	pop_alloc_frame();
	return rval;
}

// Get the parameter from an array
template<class T>
bool getMAXScriptValue(ReferenceTarget* obj, LPTSTR name, TimeValue t, T& value, int tabIndex)
{
	bool rval = false;					// Return false if fails
	assert(obj != NULL);
	init_thread_locals();
	push_alloc_frame();
	two_value_locals(prop, result);		// Keep two local variables
	save_current_frames();
	set_error_trace_back_active(FALSE);

	try {
		// Get the name and the array holding the roperty.
		vl.prop = Name::intern(name);
		vl.result = MAXWrapper::get_property(obj, vl.prop, NULL);

		// Make sure it is they right type.
		if (vl.result != NULL && is_tab_param(vl.result)) {
			// OK. Now we make sure the tabIndex is within the array bounds.
			MAXPB2ArrayParam* array = static_cast<MAXPB2ArrayParam*>(vl.result);
			if (array->pblock != NULL && array->pdef != NULL
					&& tabIndex >= 0 && tabIndex < array->pblock->Count(array->pdef->ID)) {
				// Good. Get the value
				array->pblock->GetValue(array->pdef->ID, 0, value, Interval(0,0), tabIndex);
				rval = true;			// Succeeded
			}
		}
	} catch ( ... ) {
		// Failed.
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}
	pop_value_locals();
	pop_alloc_frame();
	return rval;
}

// Get the parameter
template<class T>
bool getMAXScriptValue(ReferenceTarget* obj, LPTSTR name, TimeValue t, T& value)
{
	bool rval = false;				// Return false if fails
	assert(obj != NULL);
	init_thread_locals();
	push_alloc_frame();
	two_value_locals(prop, result);	// Keep two local varaibles
	save_current_frames();
	set_error_trace_back_active( FALSE );

	try {
		// Get the name and the parameter value
		vl.prop = Name::intern(name);
		vl.result = MAXWrapper::get_property(obj, vl.prop, NULL);

		// Make sure it is valid.
		if (vl.result != NULL && vl.result != &undefined && vl.result != &unsupplied) {
			// Convert to C++ type.
			value = ConvertMAXScriptToC<T>::cvt(vl.result);
			rval = true;				// Succeeded
		}
	} catch ( ... ) {
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}
	pop_value_locals();
	pop_alloc_frame();
	return rval;
}

// Get the parameter controller
Control* getMAXScriptController(ReferenceTarget* obj, LPTSTR name, ParamDimension*& dim)
{
	Control* rval = NULL;
	assert(obj != NULL);
	init_thread_locals();
	push_alloc_frame();
	one_value_local(prop);			// Keep one local varaibles
	save_current_frames();
	set_error_trace_back_active( FALSE );

	try {
		// Get the name and the parameter value
		vl.prop = Name::intern(name);
		rval = MAXWrapper::get_max_prop_controller(obj, vl.prop, &dim);
	} catch ( ... ) {
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}
	pop_value_locals();
	pop_alloc_frame();
	return rval;
}

// Set the parameter controller
bool setMAXScriptController(ReferenceTarget* obj, LPTSTR name, Control* control, ParamDimension* dim)
{
	bool rval = false;
	assert(obj != NULL);
	init_thread_locals();
	push_alloc_frame();
	two_value_locals(prop, maxControl);			// Keep two local varaibles
	save_current_frames();
	set_error_trace_back_active( FALSE );

	try {
		// Get the name and the parameter value
		vl.prop = Name::intern(name);
		vl.maxControl = MAXControl::intern(control, dim);
		rval = MAXWrapper::set_max_prop_controller(obj, vl.prop,
			static_cast<MAXControl*>(vl.maxControl)) != 0;
	} catch ( ... ) {
		clear_error_source_data();
		restore_current_frames();
		MAXScript_signals = 0;
		if (progress_bar_up)
			MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
	}
	pop_value_locals();
	pop_alloc_frame();
	return rval;
}


// These helpers are used to convert C++ values to
// their MAXScript value. Maxscript uses different
// methods to handle the conversion, and these helpers
// allow the conversion to be templated. You may need
// to add more converter, if you need to access other
// parameter types.
inline Value* make_maxscript_value(bool v)
{
	return_protected(v ? &true_value : &false_value);
}

inline Value* make_maxscript_value(int v)
{
	return_protected(Integer::intern(v));
}

inline Value* make_maxscript_value(float v)
{
	return_protected(Float::intern(v));
}

inline Value* make_maxscript_value(COLORREF rgb)
{
	return new ColorValue(rgb);
}

inline Value* make_maxscript_value(const Color& rgb)
{
	return new ColorValue(rgb);
}

inline Value* make_maxscript_value(LPCTSTR str)
{
	return Name::intern(const_cast<LPTSTR>(str));
}

inline Value* make_maxscript_value(ReferenceTarget* rtarg)
{
	return MAXClass::make_wrapper_for(rtarg);
}

// These helpers convert MAXScript values to C++ values.
// MAXScript uses different methods for this conversion,
// and these helpers template the conversion. You will
// need to add more converters if you need more types.
template <class T>
class ConvertMAXScriptToC {
public:
	static T cvt(Value* val);
};

inline bool ConvertMAXScriptToC<bool>::cvt(Value* val)
{
	return val->to_bool() != 0;
}

inline int ConvertMAXScriptToC<int>::cvt(Value* val)
{
	return val->to_int();
}

inline float ConvertMAXScriptToC<float>::cvt(Value* val)
{
	return val->to_float();
}

inline Color ConvertMAXScriptToC<Color>::cvt(Value* val)
{
#if VERSION_3DSMAX < (14000<<16) // Version 14 (2012)
	Point3 pt3 = val->to_point3();
   return Color(pt3.x/255.0f, pt3.y/255.0f, pt3.z/255.0f);
#else
   return val->to_point3();
#endif
}

inline LPTSTR ConvertMAXScriptToC<LPTSTR>::cvt(Value* val)
{
	return val->to_string();
}

inline Texmap* ConvertMAXScriptToC<Texmap*>::cvt(Value* val)
{
	return val->to_texmap();
}

inline ReferenceTarget* ConvertMAXScriptToC<ReferenceTarget*>::cvt(Value* val)
{
	return val->to_reftarg();
}


#endif
