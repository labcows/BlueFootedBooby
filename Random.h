#pragma once
#include "pch.h"
#include <random>
#include <cstdint>

class RNG	 // Random Number Generator
{
	std::mt19937 generator;
	std::uniform_real_distribution<float> dist{ 0.0f, 1.0f };

public:
	explicit RNG(uint32_t seed) : generator(seed) {}

	float uniform() { return dist(generator); }

	math::vec3 randomUnitVector()
	{
		math::vec3 point;
		while (true)
		{
			point = 2.0f * math::vec3(uniform(), uniform(), uniform()) - 1.0f;
			float len2 = math::dot(point, point);

			if (len2 > 1e-8f && len2 <= 1.0f)
				return math::normalize(point);
		}
	}
};

inline uint32_t makeSeed(int x, int y, int sample)
{
	uint32_t h = uint32_t(x) * 73856093u
	           ^ uint32_t(y) * 19349663u
	           ^ uint32_t(sample) * 83492791u;
	return h;
}
