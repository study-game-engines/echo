#include "shadow_camera.h"
#include "engine/core/main/Engine.h"

namespace Echo
{
	ShadowCamera::ShadowCamera()
	{
	}

	ShadowCamera::~ShadowCamera()
	{

	}

	void ShadowCamera::update(const AABB* visibleActorsAABB)
	{
		m_visibleActorsAABB = *visibleActorsAABB;

		if (visibleActorsAABB && visibleActorsAABB->isValid())
		{
			const Vector3 cameraDir = m_forward;

			float halfLen = visibleActorsAABB->getDiagonalLen() * 0.5f;
			m_position = visibleActorsAABB->getCenter() - cameraDir * halfLen;

			Matrix4 viewMat;
			Matrix4::LookAtRH(viewMat, m_position, m_position + cameraDir, Vector3::UNIT_Y);

			Matrix4 orthMat;
			calcOrthoRH(orthMat, *visibleActorsAABB, viewMat);

			m_viewProj = viewMat * orthMat;
			m_view = viewMat;
			m_proj = orthMat;

			Vector3 zAxis = -cameraDir;
			zAxis.normalize();

			m_up = Vector3::UNIT_Y;
			Vector3::Cross(m_right, m_up, zAxis);
			m_right.normalize();

			m_frustum.setOrtho(m_width, m_height, m_near, m_far);
			m_frustum.build(m_position, m_forward, m_up);
		}
	}

	void ShadowCamera::calcOrthoRH(Matrix4& oOrth, const AABB& box, const Matrix4& viewMat)
	{
		AABB orthAABB;
		for (int i = AABB::CORNER_NLT; i < AABB::CORNER_FRT; i++)
		{
			orthAABB.addPoint(viewMat.transform(box.getCorner(AABB::Corner(i))));
		}

		m_width = orthAABB.getDX();
		m_height = orthAABB.getDY();
		m_near = -orthAABB.vMax.z;
		m_far = -orthAABB.vMin.z;

		Matrix4::OrthoRH(oOrth, m_width, m_height, m_near, m_far);
	}

	void ShadowCamera::setDirection(const Vector3& dir)
	{
		m_forward = dir;
		m_forward.normalize();
	}

	void* ShadowCamera::getGlobalUniformValue(const String& name)
	{
		if (name == "u_ShadowCameraViewProjMatrix")
			return (void*)(&getViewProjMatrix());
		if (name == "u_ShadowCameraProjMatrix")
			return (void*)(&m_proj);
		else if (name == "u_ShadowCameraPosition")
			return (void*)(&m_position);
		else if (name == "u_ShadowCameraDirection")
			return (void*)(&m_forward);
		else if (name == "u_ShadowCameraNear")
			return (void*)(&m_near);
		else if (name == "u_ShadowCameraFar")
			return (void*)(&m_far);

		return nullptr;
	}
}
