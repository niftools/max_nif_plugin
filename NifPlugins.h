#ifndef __NIFPLUGINS_H__
#define __NIFPLUGINS_H__

#define NP_HASCOLL						_T("np_coll")
#define NP_HVK_LAYER					_T("np_hvk_layer")
#define NP_HVK_MATERIAL					_T("np_hvk_mtl")
#define NP_HVK_MASS						_T("np_hvk_mass")
#define NP_HVK_LINEAR_DAMPING			_T("np_hvk_ldamp")
#define NP_HVK_ANGULAR_DAMPING			_T("np_hvk_adamp")
#define NP_HVK_FRICTION					_T("np_hvk_frict")
#define NP_HVK_RESTITUTION				_T("np_hvk_resti")
#define NP_HVK_MAX_LINEAR_VELOCITY		_T("np_hvk_max_lvel")
#define NP_HVK_MAX_ANGULAR_VELOCITY		_T("np_hvk_max_avel")
#define NP_HVK_PENETRATION_DEPTH		_T("np_hvk_pdepth")
#define NP_HVK_MOTION_SYSTEM			_T("np_hvk_msys")
#define NP_HVK_QUALITY_TYPE				_T("np_hvk_qtype")

#define NP_DEFAULT_HVK_MATERIAL				9
#define NP_DEFAULT_HVK_LAYER				1
#define NP_DEFAULT_HVK_MASS					0
#define NP_DEFAULT_HVK_LINEAR_DAMPING		0.1f
#define NP_DEFAULT_HVK_ANGULAR_DAMPING		0.05f
#define NP_DEFAULT_HVK_FRICTION				0.3f
#define NP_DEFAULT_HVK_RESTITUTION			0.3f
#define NP_DEFAULT_HVK_MAX_LINEAR_VELOCITY	250.0f
#define NP_DEFAULT_HVK_MAX_ANGULAR_VELOCITY	31.4159f
#define NP_DEFAULT_HVK_PENETRATION_DEPTH	0.15f
#define NP_DEFAULT_HVK_MOTION_SYSTEM		7
#define NP_DEFAULT_HVK_QUALITY_TYPE			1

bool	npIsCollision(INode *node);
void	npSetCollision(INode *node, bool coll);

bool	npGetProp(INode *node, const TSTR &prop, int &value);
void	npSetProp(INode *node, const TSTR &prop, int value);

bool	npGetProp(INode *node, const TSTR &prop, float &value);
void	npSetProp(INode *node, const TSTR &prop, float value);

#endif //  __NIFPLUGINS_H__
