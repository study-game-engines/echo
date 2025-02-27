#pragma once

#include "Sphere.h"

namespace Echo
{
	/**
	\brief AABB
	*/
	class AABB
	{
	public:
		/*
		   4--------7
		  /|       /|
		 / |      / |
		0--------3  |
		|  6-----|--5
		| /      | /
		|/       |/
		2--------1

		vMin (6)
		vMax (3)
		*/

		enum Corner
		{
			CORNER_NLT		= 0,		//!< near left top
			CORNER_NRB		= 1,		//!< near right bottom
			CORNER_NLB		= 2,		//!< near left bottom
			CORNER_NRT		= 3,		//!< near right top

			CORNER_FLT		= 4,		//!< far left top
			CORNER_FRB		= 5,		//!< far right bottom
			CORNER_FLB		= 6,		//!< far left bottom
			CORNER_FRT		= 7,		//!< far right top
		};

	public:
		Vector3		vMin;		//!< Min Point
		Vector3		vMax;		//!< Max Point

		static const AABB ZERO;

	public:
		inline AABB()
		{
			reset();
		}

		inline AABB(const Vector3&_vMin, const Vector3&_vMax)
			:vMin(_vMin)
			,vMax(_vMax)
		{
		}

		inline AABB(const AABB &box)
			:vMin(box.vMin)
			,vMax(box.vMax)
		{
		}

		inline AABB(Real minX, Real minY, Real minZ, Real maxX, Real maxY, Real maxZ)
			:vMin(minX, minY, minZ)
			,vMax(maxX, maxY, maxZ)
		{
		}

	public:
		inline const AABB& operator = (const AABB& rhs)
		{
			vMin = rhs.vMin;
			vMax = rhs.vMax;
			return *this;
		}

		inline bool operator == (const AABB& rhs) const
		{
			return (this->vMin == rhs.vMin) && (this->vMax == rhs.vMax);
		}

		inline bool operator != (const AABB& rhs) const
		{
			return !(*this == rhs);
		}

	public:
		inline void reset()
		{
			vMin.x = vMin.y = vMin.z =  1e30f;
			vMax.x = vMax.y = vMax.z = -1e30f;
		}

		inline void addPoint(const Vector3 &point)
		{
			vMax.x = Math::Max(point.x,vMax.x);
			vMax.y = Math::Max(point.y,vMax.y);
			vMax.z = Math::Max(point.z,vMax.z);

			vMin.x = Math::Min(point.x,vMin.x);
			vMin.y = Math::Min(point.y,vMin.y);
			vMin.z = Math::Min(point.z,vMin.z);
		}

		// union an AABB into this one.
		inline void unionBox(const AABB &box)
		{
			if (box.isValid())
			{
				Vector3::Max( vMax, vMax, box.vMax);
				Vector3::Min( vMin, vMin, box.vMin);
			}
		}

		// union two AABBs into this one.
		inline void unionBox(const AABB &box1, const AABB &box2)
		{
			if (box1.isValid() && box2.isValid())
			{
				Vector3::Max(vMax, box1.vMax, box2.vMax);
				Vector3::Min(vMin, box1.vMin, box2.vMin);
			}
		}
		
		inline bool isIntersected(const AABB &box) const
		{
#ifdef ECHO_EDITOR_MODE
			EchoAssert(isValid());
			EchoAssert(box.isValid());
#endif

			if (box.vMax.x < vMin.x)
				return false;
			else if(box.vMax.y < vMin.y)
				return false;
			else if(box.vMax.z < vMin.z)
				return false;

			else if(box.vMin.x > vMax.x)
				return false;
			else if(box.vMin.y > vMax.y)
				return false;
			else if(box.vMin.z > vMax.z)
				return false;

			return true;
		}

		bool isContain( const AABB& box) const
		{
			if( isPointInside( box.vMin) && isPointInside( box.vMax))
				return true;

			return false;
		}

		inline void fromBSphere(const Sphere &bs)
		{
			vMin.x = bs.c.x - bs.r;
			vMin.y = bs.c.y - bs.r;
			vMin.z = bs.c.z - bs.r;

			vMax.x = bs.c.x + bs.r;
			vMax.y = bs.c.y + bs.r;
			vMax.z = bs.c.z + bs.r;
		}

		inline Vector3 getCorner(Corner corner) const
		{
			switch(corner)
			{
			case CORNER_NLT:
				return Vector3(vMin.x, vMax.y, vMax.z);
			case CORNER_NRB:
				return Vector3(vMax.x, vMin.y, vMax.z);
			case CORNER_NLB:
				return Vector3(vMin.x, vMin.y, vMax.z);
			case CORNER_NRT:
				return vMax;
			case CORNER_FLT:
				return Vector3(vMin.x, vMax.y, vMin.z);
			case CORNER_FRB:
				return Vector3(vMax.x, vMin.y, vMin.z);
			case CORNER_FLB:
				return vMin;
			case CORNER_FRT:
				return Vector3(vMax.x, vMax.y, vMin.z);
			default:
				return Vector3::INVALID;
			}
		}

		inline Vector3 getSize() const
		{
			return (vMax - vMin);
		}

		inline bool isValid() const 
		{
			return getDX()>=0.f && getDY()>=0.f && getDZ()>=0.f;
		}

		inline bool isPointInside(const Vector3& v) const
		{
			return v.x > vMin.x && v.x < vMax.x
				&& v.y > vMin.y && v.y < vMax.y
				&& v.z > vMin.z && v.z < vMax.z;
		}

		inline void move(const Vector3& offset)
		{
			vMin += offset;
			vMax += offset;
		}

		inline Real getDX() const 
		{
			return vMax.x - vMin.x;
		}

		inline Real getDY() const
		{
			return vMax.y - vMin.y;
		}

		inline Real getDZ() const
		{
			return vMax.z - vMin.z;
		}

		Real getPerimeter() const { return (getDX() + getDY() + getDZ()) * 2.f; }

		inline Vector3 getCenter() const
		{
			return (vMin + vMax)*0.5f;
		}

		inline Real getMaxEdgeLen() const
		{
			return Math::Max3(getDX(), getDY(), getDZ());
		}

		AABB transform( const Matrix4& matrix) const
		{
			AABB box;
			if (isValid())
			{
				Vector3 basePoint(vMin);
				Vector3 xLeg(vMax.x, vMin.y, vMin.z);
				Vector3 yLeg(vMin.x, vMax.y, vMin.z);
				Vector3 zLeg(vMin.x, vMin.y, vMax.z);

				basePoint = basePoint * matrix;
				xLeg = xLeg * matrix;
				yLeg = yLeg * matrix;
				zLeg = zLeg * matrix;

				xLeg -= basePoint;
				yLeg -= basePoint;
				zLeg -= basePoint;

				Vector3 farPoint(basePoint + xLeg + yLeg + zLeg);

				box.vMin = basePoint;
				box.vMax = farPoint;

				box.addPoint(basePoint + xLeg);
				box.addPoint(basePoint + yLeg);
				box.addPoint(basePoint + zLeg);
				box.addPoint(basePoint + xLeg + yLeg);
				box.addPoint(basePoint + yLeg + zLeg);
				box.addPoint(basePoint + zLeg + xLeg);
			}
	
			return box;
		}

		inline Real getDiagonalLenSqr() const
		{
			Real dx = getDX();
			Real dy = getDY();
			Real dz = getDZ();
			return dx*dx + dy*dy + dz*dz;
		}

		inline Real getDiagonalLen() const 
		{
			return Math::Sqrt(getDiagonalLenSqr());
		}

		inline void inflate(Real x, Real y, Real z) 
		{
			vMin -= Vector3(x, y, z);
			vMax += Vector3(x, y, z);
		}

		inline void deflate(Real x, Real y, Real z)
		{
			vMin += Vector3(x, y, z);
			vMax -= Vector3(x, y, z);
		}

		static AABB fromString(const String& val);
		String& toString(String& s) const;

		void expandBy( float w)
		{
			vMin -= Vector3(w, w, w);
			vMax += Vector3(w, w, w);
		}

		// get normal by face index
		Vector3 getNormalByFaceIndex(const int faceIdx) const
		{
			switch (faceIdx)
			{
			case 0: return Vector3::NEG_UNIT_X;
			case 1: return Vector3::NEG_UNIT_Y;
			case 2: return Vector3::NEG_UNIT_Z;
			case 3: return Vector3::UNIT_X;
			case 4: return Vector3::UNIT_Y;
			case 5: return Vector3::UNIT_Z;
			default: return Vector3::INVALID;
			}
		}
	};
}
