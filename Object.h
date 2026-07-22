#pragma once
#include "pch.h"

#include "Hit.h"
#include "Ray.h"
#include "Texture.h"

class Material;

class Object
{
public:
	math::vec3 amb = math::vec3(0.0f); // Ambient
	math::vec3 dif = math::vec3(0.0f); // Diffuse
	math::vec3 spec = math::vec3(0.0f); // Specular
	float alpha = 10.0f;
	float reflection = 0.0f;
	float transparency = 0.0f;

	Texture* ambTexture = nullptr;
	Texture* difTexture = nullptr;

	std::unique_ptr<Material> material = nullptr;


	Object(const math::vec3& color = { 1.0f, 1.0f, 1.0f }) : amb(color), dif(color), spec(color)
	{

	}

	virtual ~Object() = default;

	virtual Hit CheckRayCollision(const Ray& ray) const = 0;
};
