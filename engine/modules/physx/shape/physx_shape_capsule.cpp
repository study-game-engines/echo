#include "physx_shape_capsule.h"
#include "../physx_module.h"

namespace Echo
{
	PhysxShapeCapsule::PhysxShapeCapsule()
	{
		// set default rotation
		Quaternion quat; quat.fromAxisAngle(Vector3::UNIT_Z, Math::PI_DIV2);
		setLocalOrientation(quat);
	}

	PhysxShapeCapsule::~PhysxShapeCapsule()
	{

	}

	void PhysxShapeCapsule::bindMethods()
	{
		CLASS_BIND_METHOD(PhysxShapeCapsule, getRadius);
		CLASS_BIND_METHOD(PhysxShapeCapsule, setRadius);
		CLASS_BIND_METHOD(PhysxShapeCapsule, getHeight);
		CLASS_BIND_METHOD(PhysxShapeCapsule, setHeight);

		CLASS_REGISTER_PROPERTY(PhysxShapeCapsule, "Radius", Variant::Type::Real, getRadius, setRadius);
		CLASS_REGISTER_PROPERTY(PhysxShapeCapsule, "Height", Variant::Type::Real, getHeight, setHeight);
	}

	void PhysxShapeCapsule::setRadius(float radius)
	{
		if (m_radius != radius)
		{
			m_radius = radius;

			physx::PxCapsuleGeometry geometry;
			if (m_pxShape && m_pxShape->getCapsuleGeometry(geometry))
			{
				geometry.radius = radius;
				m_pxShape->setGeometry(geometry);
			}
		}
	}

	void PhysxShapeCapsule::setHeight(float height)
	{ 
		if (m_height != height)
		{
			m_height = height;

			physx::PxCapsuleGeometry geometry;
			if (m_pxShape && m_pxShape->getCapsuleGeometry(geometry))
			{
				geometry.halfHeight = m_height * 0.5f;
				m_pxShape->setGeometry(geometry);
			}
		}
	}

	physx::PxShape* PhysxShapeCapsule::createPxShape()
	{
		using namespace physx;

		PxPhysics* physics = PhysxModule::instance()->getPxPhysics();
		if (physics)
		{
			physx::PxTransform pxTransform((physx::PxVec3&)getLocalPosition(), (physx::PxQuat&)getLocalOrientation());
			PxShape* shape = physics->createShape(PxCapsuleGeometry(m_radius, m_height*0.5f), *m_pxMaterial);
			shape->setLocalPose(pxTransform);

			return shape;
		}

		return nullptr;
	}

	bool PhysxShapeCapsule::sweep(const Vector3& unitDir, float distance)
	{
		physx::PxScene* pxScene = PhysxModule::instance()->getPxScene();
		if (pxScene)
		{
			physx::PxSweepBuffer hitCb;
			return pxScene->sweep(
				physx::PxCapsuleGeometry(m_radius, m_height * 0.5f),
				physx::PxTransform((physx::PxVec3&)getWorldPosition(), (physx::PxQuat&)getWorldOrientation()),
				(const physx::PxVec3&)unitDir,
				distance,
				hitCb);
		}

		return false;
	}
}