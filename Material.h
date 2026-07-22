#pragma once
#include "pch.h"
#include "Ray.h"
#include "Hit.h"
#include "Random.h"

class Material
{
public:
	virtual ~Material() = default;

	virtual math::vec3 Emitted() const { return math::vec3(0.0f); }

	virtual bool Scatter(const Ray& in, const Hit& hit, RNG& rng,
	                     math::vec3& attenuation, Ray& scattered) const = 0;
};

class Lambertian : public Material
{
public:
	math::vec3 albedo;
	explicit Lambertian(const math::vec3& albedo) : albedo(albedo) {}

	bool Scatter(const Ray& in, const Hit& hit, RNG& rng,
	             math::vec3& attenuation, Ray& scattered) const override
	{
		math::vec3 scatterDir = hit.normal + rng.randomUnitVector();
		if (math::dot(scatterDir, scatterDir) < 1e-8f)
			scatterDir = hit.normal;

		scattered.dir = math::normalize(scatterDir);
		scattered.start = hit.point + hit.normal * 1e-3f;
		attenuation = albedo;
		return true;
	}
};

class Metal : public Material
{
public:
	math::vec3 albedo;
	float fuzz;
	Metal(const math::vec3& albedo, float fuzz = 0.0f) : albedo(albedo), fuzz(fuzz) {}

	bool Scatter(const Ray& in, const Hit& hit, RNG& rng,
	             math::vec3& attenuation, Ray& scattered) const override
	{
		math::vec3 reflected = math::reflect(math::normalize(in.dir), hit.normal);
		scattered.dir = math::normalize(reflected + fuzz * rng.randomUnitVector());
		scattered.start = hit.point + hit.normal * 1e-3f;
		attenuation = albedo;
		return math::dot(scattered.dir, hit.normal) > 0.0f;
	}
};

class Dielectric : public Material
{
public:
	float ior;
	explicit Dielectric(float ior = 1.5f) : ior(ior) {}

	bool Scatter(const Ray& in, const Hit& hit, RNG& rng,
	             math::vec3& attenuation, Ray& scattered) const override
	{
		const bool isEntering = math::dot(in.dir, hit.normal) < 0.0f;

		const math::vec3 normal = isEntering ? hit.normal : -hit.normal;
		const float eta = isEntering ? (1.0f / ior) : ior;

		const float cosTheta = math::clamp(math::dot(-math::normalize(in.dir), normal), 0.0f, 1.0f);
		const float sinTheta = sqrt(1 - cosTheta * cosTheta);

		const math::vec3 reflected = math::reflect(math::normalize(in.dir), normal);
		const math::vec3 refracted = math::refract(math::normalize(in.dir), normal, eta);

		const float r0 = ((1.0f - eta) / (1.0f + eta)) * ((1.0f - eta) / (1.0f + eta));
		const float reflectance = r0 + (1.0f - r0) * math::pow(1.0f - cosTheta, 5.0f);

		math::vec3 direction;
		if (eta * sinTheta > 1.0f || (reflectance > rng.uniform()))
			direction = reflected;
		else
			direction = refracted;

		scattered.dir = direction;
		scattered.start = hit.point + direction * 1e-3f;
		attenuation = math::vec3(1.0f);
		return true;
	}
};

class DiffuseLight : public Material
{
public:
	math::vec3 emission;
	explicit DiffuseLight(const math::vec3& emission) : emission(emission) {}

	math::vec3 Emitted() const override { return emission; }

	bool Scatter(const Ray& in, const Hit& hit, RNG& rng,
	             math::vec3& attenuation, Ray& scattered) const override
	{
		return false;
	}
};
