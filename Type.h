#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

namespace math {
	using vec2 = glm::vec2;
	using vec3 = glm::vec3;
	using vec4 = glm::vec4;
	using mat4 = glm::mat4;
	using dvec3 = glm::dvec3;
	

	using glm::normalize;
	using glm::dot;
	using glm::cross;
	using glm::length;
	using glm::min;
	using glm::max;
	using glm::pow;
	using glm::clamp;
	using glm::abs;
	using glm::tan;       // for turning the camera's vertical FOV into a viewport size
	using glm::radians;   // degrees -> radians
	using glm::reflect;   // mirror reflection (Metal)
	using glm::refract;   // Snell refraction (Dielectric)
}