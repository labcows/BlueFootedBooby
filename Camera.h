#pragma once
#include "pch.h"
#include "Ray.h"

class Camera
{
public:
	math::vec3 position;
	math::vec3 lookAt;
	math::vec3 up;
	float      vFovDegrees;
	float      aspect;

	math::vec3 u, v, w;
	math::vec3 lowerLeftCorner;
	math::vec3 horizontal;
	math::vec3 vertical;

	Camera(const math::vec3& position,
	       const math::vec3& lookAt,
	       const math::vec3& up,
	       float vFovDegrees,
	       float aspect)
		: position(position), lookAt(lookAt), up(up),
		  vFovDegrees(vFovDegrees), aspect(aspect)
	{
		w = math::normalize(position - lookAt);
		u = math::normalize(math::cross(up, w));
		v = math::cross(w, u);

		const float theta = math::radians(vFovDegrees);
		const float viewportHeight = 2 * math::tan(theta / 2);
		const float viewportWidth = aspect * viewportHeight;

		horizontal = viewportWidth * u;
		vertical = viewportHeight * v;
		lowerLeftCorner = position - w - horizontal * 0.5f - vertical * 0.5f;
	}

	Ray GenerateRay(float s, float t) const
	{
		const math::vec3 point = lowerLeftCorner + s * horizontal + (1.0f - t) * vertical;

		Ray ray;
		ray.start = position;
		ray.dir   = math::normalize(point - position);
		return ray;
	}
};
