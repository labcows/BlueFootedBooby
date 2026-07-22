#pragma once
#include "Object.h"

class Triangle : public Object
{
public:
	math::vec3 v0{}, v1{}, v2{};
	math::vec2 uv0{}, uv1{}, uv2{};

public:
	Triangle() = default;

	Triangle(math::vec3 v0, math::vec3 v1, math::vec3 v2, math::vec2 uv0 = math::vec2(0.0f), math::vec2 uv1 = math::vec2(0.0f), math::vec2 uv2 = math::vec2(0.0f))
		: v0(v0), v1(v1), v2(v2), uv0(uv0), uv1(uv1), uv2(uv2)
	{
	}

	Hit CheckRayCollision(const Ray& ray) const override
	{
		Hit hit = Hit(-1.0f, math::vec3(0.0f), math::vec3(0.0f));

		math::vec3 point, faceNormal;
		float t, w0, w1;
		if (IntersectRayTriangle(ray.start, ray.dir, this->v0, this->v1,
			this->v2, point, faceNormal, t, w0, w1))
		{
			hit.distance = t;
			hit.point = point;	// ray.start + ray.dir * t;
			hit.normal = faceNormal;
			hit.uv = uv0 * w0 + uv1 * w1 + uv2 * (1.0f - w0 - w1);
		}

		return hit;
	};

	bool IntersectRayTriangle(const math::vec3& orig, const math::vec3& dir,
		const math::vec3& v0, const math::vec3& v1,
		const math::vec3& v2, math::vec3& point, math::vec3& faceNormal,
		float& t, float& w0, float& w1) const
	{
		faceNormal = math::normalize(math::cross(v1 - v0, v2 - v0));
		
		// Backface culing
		if (math::dot(-dir, faceNormal) < 0.0f)
			return false;

		if (math::abs(math::dot(dir, faceNormal)) < 1e-2f)
			return false;

		t = (math::dot(v0, faceNormal) - math::dot(orig, faceNormal)) / math::dot(dir, faceNormal);

		if (t < 0.0f)
			return false;

		point = orig + t * dir;

		const math::vec3 cross0 = math::cross(point - v2, v1 - v2);
		const math::vec3 cross1 = math::cross(point - v0, v2 - v0);
		const math::vec3 cross2 = math::cross(v1 - v0, point - v0);

		if (dot(cross0, faceNormal) < 0.0f)
			return false;
		if (dot(cross1, faceNormal) < 0.0f)
			return false;
		if (dot(cross2, faceNormal) < 0.0f)
			return false;
		
		const float area0 = math::length(cross0) * 0.5f;
		const float area1 = math::length(cross1) * 0.5f;
		const float area2 = math::length(cross2) * 0.5f;

		const float areaSum = area0 + area1 + area2;

		w0 = area0 / areaSum;
		w1 = area1 / areaSum;

		return true;
	}

};