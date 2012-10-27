#ifndef __NIFPLUGINS_H__
#define __NIFPLUGINS_H__

#include "nif_math.h"
using Niflib::Vector3;

/* node properties */


// Custom Properties shared with Havok
#define NP_HVK_MASS							_T("Mass")
#define NP_HVK_FRICTION						_T("Friction")
#define NP_HVK_RESTITUTION					_T("Ellasticity")
#define NP_HVK_PROXY_GEOM					_T("Proxy_Geometry")

#define NP_HVK_MASS_OLD						_T("np_hvk_mass")
#define NP_HVK_FRICTION_OLD				_T("np_hvk_frict")
#define NP_HVK_RESTITUTION_OLD			_T("np_hvk_resti")


/* default values */
#define NP_DEFAULT_HVK_EMPTY					3.402823466e+38F
#define NP_DEFAULT_HVK_MASS					0.0f
#define NP_DEFAULT_HVK_FRICTION				0.3f
#define NP_DEFAULT_HVK_RESTITUTION			0.3f
#define NP_DEFAULT_PROXY_GEOM			      _T("")


// Custom Properties
#define NP_HASCOLL							_T("np_coll")
#define NP_HVK_LAYER						_T("np_hvk_layer")
#define NP_HVK_MATERIAL						_T("np_hvk_mtl")
#define NP_HVK_CENTER						_T("np_hvk_center")
#define NP_HVK_LINEAR_DAMPING				_T("np_hvk_ldamp")
#define NP_HVK_ANGULAR_DAMPING				_T("np_hvk_adamp")
#define NP_HVK_MAX_LINEAR_VELOCITY			_T("np_hvk_max_lvel")
#define NP_HVK_MAX_ANGULAR_VELOCITY			_T("np_hvk_max_avel")
#define NP_HVK_PENETRATION_DEPTH			_T("np_hvk_pdepth")
#define NP_HVK_MOTION_SYSTEM				_T("np_hvk_msys")
#define NP_HVK_QUALITY_TYPE					_T("np_hvk_qtype")

#define NP_FRN_POS							_T("np_frn_pos")

#define NP_ANM_PRI							_T("np_anm_pri")

/* default values */
#define NP_INVALID_HVK_MATERIAL				-1
#define NP_DEFAULT_HVK_MATERIAL				0
#define NP_DEFAULT_HVK_LAYER				1
#define NP_DEFAULT_HVK_LINEAR_DAMPING		0.1f
#define NP_DEFAULT_HVK_ANGULAR_DAMPING		0.05f
#define NP_DEFAULT_HVK_MAX_LINEAR_VELOCITY	200.0f
#define NP_DEFAULT_HVK_MAX_ANGULAR_VELOCITY	200.0f
#define NP_DEFAULT_HVK_PENETRATION_DEPTH	0.15f
#define NP_DEFAULT_HVK_MOTION_SYSTEM		7
#define NP_DEFAULT_HVK_QUALITY_TYPE			1
#define NP_DEFAULT_HVK_FILTER				0

#define NP_DEFAULT_ANM_PRI	0.0f

#define NP_DEFAULT_FRN_MARKER_TYPE			NP_FRN_SLEEP_LEFT

/* other constants */
enum NpFrnPos
{
	NP_FRN_SLEEP_LEFT		= 1,
	NP_FRN_SLEEP_RIGHT		= 2,
	NP_FRN_BEDROLL_LEFT		= 3,
	NP_FRN_BEDROLL_RIGHT	= 4,
	NP_FRN_SIT_LEFT			= 11,
	NP_FRN_SIT_RIGHT		= 12,
	NP_FRN_SIT_BACK			= 13,
	NP_FRN_SIT_FRONT		= 14
};

/* property access */
bool	npIsCollision(INode *node);
void	npSetCollision(INode *node, bool coll);

bool	npGetProp(INode *node, const TSTR &prop, int &value, int def=0);
void	npSetProp(INode *node, const TSTR &prop, int value);

bool	npGetProp(INode *node, const TSTR &prop, float &value, float def=0.0f);
void	npSetProp(INode *node, const TSTR &prop, float value);

bool	npGetProp(INode *node, const TSTR &prop, Vector3 &value, const Vector3 def=Vector3(0,0,0));
void	npSetProp(INode *node, const TSTR &prop, const Vector3 &value);

bool	npGetProp(INode *node, const TSTR &prop, TSTR &value, TSTR def=TSTR());
void	npSetProp(INode *node, const TSTR &prop, const TSTR& value);

#endif //  __NIFPLUGINS_H__
