/* Copyright (c) 2006, NIF File Format Library and Tools
All rights reserved.  Please see niflib.h for licence. */

#include "bhkRigidBody.h"
#include "../gen/QuaternionXYZW.h"
#include "AbhkConstraint.h"
using namespace Niflib;

//Definition of TYPE constant
const Type bhkRigidBody::TYPE("bhkRigidBody", &BHK_RIGID_BODY_PARENT::TypeConst() );

bhkRigidBody::bhkRigidBody() BHK_RIGID_BODY_CONSTRUCT {}

bhkRigidBody::~bhkRigidBody() {}

void bhkRigidBody::Read( istream& in, list<uint> & link_stack, unsigned int version, unsigned int user_version ) {
	BHK_RIGID_BODY_READ
}

void bhkRigidBody::Write( ostream& out, map<NiObjectRef,uint> link_map, unsigned int version, unsigned int user_version ) const {
	BHK_RIGID_BODY_WRITE
}

string bhkRigidBody::asString( bool verbose ) const {
	BHK_RIGID_BODY_STRING
}

void bhkRigidBody::FixLinks( const vector<NiObjectRef> & objects, list<uint> & link_stack, unsigned int version, unsigned int user_version ) {
	BHK_RIGID_BODY_FIXLINKS
}

list<NiObjectRef> bhkRigidBody::GetRefs() const {
	BHK_RIGID_BODY_GETREFS
}

const Type & bhkRigidBody::GetType() const {
	return TYPE;
};

void bhkRigidBody::SetTranslation(const Vector3 &v)
{
	translation = v;
}

void bhkRigidBody::SetRotation(const QuaternionXYZW &q)
{
	rotation = q;
}

void bhkRigidBody::SetLayerCopy(int l)
{
	layerCopy_ = l;
}

void bhkRigidBody::SetCenter(const Vector3 &v)
{
	center = v;
}

void bhkRigidBody::SetMass(float f)
{
	mass = f;
}

void bhkRigidBody::SetLinearDamping(float f)
{
	linearDamping = f;
}

void bhkRigidBody::SetAngularDamping(float f)
{
	angularDamping = f;
}

void bhkRigidBody::SetFriction(float f)
{
	friction = f;
}

void bhkRigidBody::SetRestitution(float f)
{
	restitution = f;
}

void bhkRigidBody::SetMaxLinearVelocity(float f)
{
	maxLinearVelocity = f;
}

void bhkRigidBody::SetMaxAngularVelocity(float f)
{
	maxAngularVelocity = f;
}

void bhkRigidBody::SetPenetrationDepth(float f)
{
	penDepth = f;
}

void bhkRigidBody::SetMotionSystem(int i)
{
	motionSystem_ = i;
}

void bhkRigidBody::SetQualityType(int i)
{
	qualityType = i;
}
