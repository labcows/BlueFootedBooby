#pragma once
#include "Object.h"

class Rect : public Object
{
public:
	int axis;
	math::vec3 minCorner;
	math::vec3 maxCorner;

	Rect(const math::vec3& corner0, const math::vec3& corner1, int axis,
	     const math::vec3& color = math::vec3(1.0f))
		: Object(color), axis(axis)
	{
		minCorner = math::min(corner0, corner1);
		maxCorner = math::max(corner0, corner1);
	}

	Hit CheckRayCollision(const Ray& ray) const override
	{
		Hit hit = Hit{ -1.0f, math::vec3(0.0f), math::vec3(0.0f) };

		const float k = minCorner[axis];
		const float t = (k - ray.start[axis]) / ray.dir[axis];

		if (t < 1e-4f)
			return hit;

		const math::vec3 point = ray.start + t * ray.dir;

		const int a = (axis + 1) % 3;
		const int b = (axis + 2) % 3;

		if ((minCorner[a] <= point[a] && point[a] <= maxCorner[a]) && (minCorner[b] <= point[b] && point[b] <= maxCorner[b]))
		{
			hit.distance = t;
			hit.obj = this;
			hit.point = point;

			math::vec3 normal = math::vec3(0.0f);
			normal[axis] = 1.0f;

			if (math::dot(normal, ray.dir) > 0.0f)
				normal = -normal;
			hit.normal = normal;
		}

		return hit;
	}
};
