#pragma once
#include "pch.h"
#include <vector>
#include <string>

class Texture
{
public:
	int width, height, channels;
	std::vector<uint8_t> image;

	Texture(const std::string& filename);
	Texture(const int& width, const int& height, const std::vector<math::vec3>& pixels);
	
	math::vec3 GetWrapped(int i, int j)
	{
		i %= width; // i가 width면 0으로 바뀜
		j %= height;

		if (i < 0)
			i += width; // i가 -1이면 (width-1)로 바뀜
		if (j < 0)
			j += height;

		const float r = image[(i + width * j) * channels + 0] / 255.0f;
		const float g = image[(i + width * j) * channels + 1] / 255.0f;
		const float b = image[(i + width * j) * channels + 2] / 255.0f;

		return math::vec3(r, g, b);
	}
	math::vec3 InterpolateBilinear(
		const float& dx,
		const float& dy,
		const math::vec3& c00,
		const math::vec3& c10,
		const math::vec3& c01,
		const math::vec3& c11)
	{
		math::vec3 a = c00 * (1.0f - dx) + c10 * dx;
		math::vec3 b = c01 * (1.0f - dx) + c11 * dx;
		return a * (1.0f - dy) + b * dy;
	}

	math::vec3 Linear(const math::vec2& uv)
	{
		const math::vec2 xy = uv * math::vec2(width, height) - math::vec2(0.5f);
		const int i = int(floor(xy.x));
		const int j = int(floor(xy.y));
		const float dx = xy.x - float(i);
		const float dy = xy.y - float(j);

		return InterpolateBilinear(dx, dy, GetWrapped(i, j), GetWrapped(i + 1, j), GetWrapped(i, j + 1), GetWrapped(i + 1, j + 1));
	}

};