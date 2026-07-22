#pragma once
#include "Object.h"

class Sphere : public Object
{
public:
	math::vec3 center;
	float radius;

	Sphere(const math::vec3& center, const float radius, const math::vec3& color = math::vec3(1.0f))
		: Object(color), center(center), radius(radius)
	{
	}

	Hit CheckRayCollision(const Ray& ray) const override
	{
		Hit hit = Hit{ -1.0f, math::vec3(0.0f), math::vec3(0.0f) };

		const float b = 2.0f * math::dot(ray.dir, ray.start - this->center);
		const float c = math::dot(ray.start - this->center, ray.start - this->center) - this->radius * this->radius;

		const float det = b * b - 4.0f * c;
		if (det >= 0.0f)
		{
			const float sqrtDet = sqrt(det);
			const float eps = 1e-4f;

			// Nearest positive root. Try the near root; if it's behind us — or we're
			// INSIDE the sphere (the glass case) — fall back to the far root.
			float d = (-b - sqrtDet) / 2.0f;
			if (d < eps)
				d = (-b + sqrtDet) / 2.0f;

			if (d >= eps)
			{
				hit.distance = d;
				hit.point = ray.start + ray.dir * d;
				hit.normal = math::normalize(hit.point - this->center);
			}
		}

		return hit;
	}
};