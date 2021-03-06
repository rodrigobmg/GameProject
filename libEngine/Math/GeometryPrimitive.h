#pragma once
#ifndef GeometryPrimitive_H_FABFC52A_8FFA_4419_B6C7_5A8B4C09CB4F
#define GeometryPrimitive_H_FABFC52A_8FFA_4419_B6C7_5A8B4C09CB4F

#include "Math/Vector2.h"
#include "Math/Vector3.h"

#include <type_traits>

namespace Math
{
	template< class VectorType >
	struct TBoundBox
	{
		VectorType min;
		VectorType max;


		bool isIntersect(TBoundBox const& rhs) const
		{
			return Math::BoxBoxTest(min, max, rhs.min, rhs.max);
		}

		bool isValid() const
		{
			return min <= max;
		}

		TBoundBox& operator += (TBoundBox const& rhs)
		{
			assert(rhs.isValid());
			min.min(rhs.min);
			max.max(rhs.max);
			return *this;
		}

		TBoundBox& operator += (VectorType const& pos)
		{
			min.min(pos);
			max.max(pos);
			return *this;
		}


		TBoundBox() {}
		TBoundBox(EForceInit)
		{
			min = VectorType::Zero();
			max = VectorType::Zero();
		}
	};

	FORCEINLINE float DistanceSqure(TBoundBox<Vector2> const& box, Vector2 const& pos)
	{
		Vector2 dMin = pos - box.min;
		Vector2 dMax = box.max - pos;

		float dx, dy;
		dx = Math::Abs(dMin.x) < Math::Abs(dMax.x) ? dMin.x : dMax.x;
		dy = Math::Abs(dMin.y) < Math::Abs(dMax.y) ? dMin.y : dMax.y;

		float result = 0;
		if( dx < 0 ) result += dx * dx;
		if( dy < 0 ) result += dy * dy;
		return result;
	}

	FORCEINLINE float DistanceSqure(TBoundBox<Vector3> const& box, Vector3 const& pos)
	{
		Vector3 dMin = pos - box.min;
		Vector3 dMax = box.max - pos;

		float dx, dy, dz;
		dx = Math::Abs(dMin.x) < Math::Abs(dMax.x) ? dMin.x : dMax.x;
		dy = Math::Abs(dMin.y) < Math::Abs(dMax.y) ? dMin.y : dMax.y;
		dz = Math::Abs(dMin.z) < Math::Abs(dMax.z) ? dMin.z : dMax.z;

		float result = 0;
		if( dx < 0 ) result += dx * dx;
		if( dy < 0 ) result += dy * dy;
		if( dz < 0 ) result += dz * dz;
		return result;
	}

	template< class VectorType >
	struct TRay
	{
		VectorType pos;
		VectorType dir;

		VectorType getPosition(float dist) const { return pos + dist * dir; }
		int     testIntersect(TBoundBox<VectorType> const& bound, float outDists[2]) const
		{
			if( !Math::LineBoxTest(pos, dir, bound.min, bound.max, outDists) )
				return 0;

			if( outDists[0] < 0 )
			{
				if( outDists[1] < 0 )
					return 0;
				outDists[0] = outDists[1];
				return 1;
			}
			return 2;
		}

	};


	//#MOVE
	// N.dot( V ) = d
	class  Plane
	{
	public:
		Plane() {}

		Plane(Vector3 const& v0, Vector3 const& v1, Vector3 const&  v2)
		{
			Vector3 d1 = v1 - v0;
			Vector3 d2 = v2 - v0;
			mNormal = d1.cross(d2);
			mNormal.normalize();

			mDistance = mNormal.dot(v0);
		}

		Plane(Vector3 const& normal, Vector3 const& pos)
			:mNormal(normal)
		{
			mNormal.normalize();
			mDistance = mNormal.dot(pos);
		}

		Plane(Vector3 const& n, float d)
			:mNormal(n)
			, mDistance(d)
		{
			mNormal.normalize();
		}

		static Plane FromVector4(Vector4 const& v)
		{
			Vector3 n = v.xyz();
			float mag = n.normalize();
			return Plane(n, -v.w / mag);
		}

#if 0
		void swap(Plane& p)
		{
			using std::swap;

			swap(mNormal, p.mNormal);
			swap(mDistance, p.mDistance);
		}
#endif

		float calcDistance(Vector3 const& p) const
		{
			return p.dot(mNormal) - mDistance;
		}
	private:
		Vector3 mNormal;
		float   mDistance;
	};


	class BoundSphere
	{
	public:
		BoundSphere(Vector3 const& c = Vector3(0, 0, 0), float r = 0)
			:center(c), radius(r)
		{
			assert(radius >= 0);
		}
		BoundSphere(Vector3 const& max, Vector3 const& min)
			:center(0.5f * (max + min))
		{
			Vector3 extrent = max - min;
			radius = 0.5f * Math::Max(Math::Max(extrent.x, extrent.y), extrent.z);
			assert(radius >= 0);
		}

		void merge(BoundSphere const& sphere)
		{
			Vector3 dir = sphere.center - center;
			float   dist = dir.length();

			if( radius > dist + sphere.radius )
				return;
			if( sphere.radius > dist + radius )
			{
				radius = sphere.radius;
				center = sphere.center;
				return;
			}

			float r = 0.5f * (radius + sphere.radius + dist);

			center += ((r - radius) / dist) * dir;
			radius = r;
		}

		void addPoint(Vector3 const& p)
		{
			Vector3 dir = p - center;
			float   dist2 = dir.length2();
			if( dist2 < radius * radius )
				return;

			radius = Math::Sqrt(dist2);
			//radius = 0.5f * ( radius + dist );
			//center += ( ( radius - dist ) / dist ) * dir;
		}
		bool testIntersect(Plane const& plane) const
		{
			float dist = plane.calcDistance(center);
			return Math::Abs(dist) < dist;
		}
		Vector3 const& getCenter() const { return center; }
		float          getRadius() const { return radius; }

		Vector3 center;
		float   radius;
	private:


	};

}

#endif // GeometryPrimitive_H_FABFC52A_8FFA_4419_B6C7_5A8B4C09CB4F