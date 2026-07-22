#pragma once

#include "Sphere.h"
#include "Ray.h"
#include "Light.h"
#include "Triangle.h"
#include "Rect.h"
#include "pch.h"
#include "Camera.h"
#include "Material.h"

#include <vector>
#include <chrono>
#include <iomanip>

class Raytracer
{
public:
	int width, height;
	Light light;
	std::vector<std::unique_ptr<Object>> objects;
	Camera camera;

	enum class DebugView { None, Normals, Depth, Albedo, PathTraced };
	DebugView debugView = DebugView::Normals;

	int spp = 128;
	int maxDepth = 8;

	Raytracer(const int& width, const int &height)
		:width(width), height(height),
		 camera(math::vec3(278.0f, 278.0f, -800.0f), // Position
				math::vec3(278.0f, 278.0f, 0.0f),	 // lookAt	
		        math::vec3(0.0f, 1.0f, 0.0f),		 // up
				40.0f,								 // vFovDegrees
				float(width) / float(height))		 // aspect
	{
		const math::vec3 white(0.73f, 0.73f, 0.73f);
		const math::vec3 red(0.65f, 0.05f, 0.05f);
		const math::vec3 green(0.12f, 0.45f, 0.15f);

		objects.push_back(std::make_unique<Rect>(math::vec3(0.0f, 0.0f, 0.0f),     math::vec3(555.0f, 0.0f, 555.0f),   1, white));
		objects.push_back(std::make_unique<Rect>(math::vec3(0.0f, 555.0f, 0.0f),   math::vec3(555.0f, 555.0f, 555.0f), 1, white));
		objects.push_back(std::make_unique<Rect>(math::vec3(0.0f, 0.0f, 555.0f),   math::vec3(555.0f, 555.0f, 555.0f), 2, white));
		objects.push_back(std::make_unique<Rect>(math::vec3(555.0f, 0.0f, 0.0f),   math::vec3(555.0f, 555.0f, 555.0f), 0, red));
		objects.push_back(std::make_unique<Rect>(math::vec3(0.0f, 0.0f, 0.0f),     math::vec3(0.0f, 555.0f, 555.0f),   0, green));

		objects.push_back(std::make_unique<Rect>(math::vec3(213.0f, 554.0f, 227.0f), math::vec3(343.0f, 554.0f, 332.0f), 1, math::vec3(1.0f)));

		objects.push_back(std::make_unique<Sphere>(math::vec3(185.0f, 90.0f, 150.0f), 90.0f, math::vec3(0.9f)));
		objects.push_back(std::make_unique<Sphere>(math::vec3(370.0f, 90.0f, 350.0f), 90.0f, math::vec3(0.9f)));

		objects[0]->material = std::make_unique<Lambertian>(white);
		objects[1]->material = std::make_unique<Lambertian>(white);
		objects[2]->material = std::make_unique<Lambertian>(white);
		objects[3]->material = std::make_unique<Lambertian>(red);
		objects[4]->material = std::make_unique<Lambertian>(green);
		objects[5]->material = std::make_unique<DiffuseLight>(math::vec3(30.0f));
		objects[6]->material = std::make_unique<Dielectric>(1.5f);
		objects[7]->material = std::make_unique<Metal>(math::vec3(0.9f), 0.0f);

		light = Light{ {278.0f, 500.0f, 279.5f} };
	}

	~Raytracer(){}

	Hit FindClosestCollision(const Ray& ray)
	{
		float closestD = 1e30f;
		Hit closestHit;

		for (size_t l = 0; l < objects.size(); l++)
		{
			auto hit = objects[l]->CheckRayCollision(ray);

			if (hit.distance >= 0.0f)
			{
				if (hit.distance < closestD)
				{
					closestD = hit.distance;
					closestHit = hit;
					closestHit.obj = objects[l].get();

					closestHit.uv = hit.uv;
				}
			}
		}

		return closestHit;
	}

	math::vec3 traceRay(Ray& ray, const int recurseLevel)
	{

		if (recurseLevel < 0)
			return math::vec3(0.0f);
		// Render first hit
		const auto hit = FindClosestCollision(ray);

		if (hit.distance >= 0.0f)
		{
			math::vec3 color(0.0f);
			math::vec3 phongColor(0.0f);

			// Diffuse
			const math::vec3 dirToLight = math::normalize(light.pos - hit.point);
			//const float diff = math::max(dot(hit.normal, dirToLight), 0.0f);

			// Specular
			const math::vec3 reflectDir = 2.0f * dot(hit.normal, dirToLight) * hit.normal - dirToLight;
			const float specular = math::pow(math::max(math::dot(-ray.dir, reflectDir), 0.0f), hit.obj->alpha);

			// Texture ambient effect.
			if (hit.obj->ambTexture)
			{
				phongColor += hit.obj->amb * hit.obj->ambTexture->Linear(hit.uv);
			}
			else
			{
				phongColor += hit.obj->amb;
			}

			// Texture diffuse effect.
			if (hit.obj->difTexture)
			{
				phongColor += hit.obj->dif * hit.obj->difTexture->Linear(hit.uv);
			}
			else
			{
				phongColor += hit.obj->dif;
			}

			phongColor += hit.obj->spec * specular;

			color += phongColor * (1.0f - hit.obj->reflection - hit.obj->transparency);

			if (hit.obj->reflection)
			{
				const auto reflectionDirection = math::normalize(2.0f * hit.normal * math::dot(-ray.dir, hit.normal) + ray.dir);
				Ray reflectionRay{ hit.point + reflectionDirection * 1e-4f, reflectionDirection };
				color += traceRay(reflectionRay, recurseLevel - 1) * hit.obj->reflection;
			}

			if (hit.obj->transparency)
			{
				const float ior = 1.5f;

				float eta; // sinTheta1 / sinTheta2
				math::vec3 normal;

				if (math::dot(ray.dir, hit.normal) < 0.0f)
				{
					eta = ior;
					normal = hit.normal;
				}
				else
				{
					eta = 1.0f / ior;
					normal = -hit.normal;
				}

				const float cosTheta1 = -math::dot(normal, ray.dir);
				const float sinTheta1 = sqrt(1.0f - cosTheta1 * cosTheta1);
				const float sinTheta2 = sinTheta1 / eta;
				const float cosTheta2 = sqrt(1.0f - sinTheta2 * sinTheta2);

				const math::vec3 m = math::normalize(math::dot(normal, -ray.dir) * normal + ray.dir);
				const math::vec3 a = m * sinTheta2;
				const math::vec3 b = -normal * cosTheta2;
				const math::vec3 refractedDirection = math::normalize(a + b); // transmission

				Ray refractionRay{ hit.point + refractedDirection * 1e-4f, refractedDirection };

				color += traceRay(refractionRay, recurseLevel - 1) * hit.obj->transparency;
			}
			return color;
		}

		return math::vec3(0.0f);
	}
	
	math::vec3 shadeNormal(const Hit& hit) const
	{
		return 0.5f * hit.normal + 0.5f;
	}

	math::vec3 shade(Ray& ray)
	{
		if (debugView == DebugView::None)
			return traceRay(ray, 5);

		const Hit hit = FindClosestCollision(ray);
		if (hit.distance < 0.0f)
			return math::vec3(0.0f);

		if (debugView == DebugView::Albedo)
			return hit.obj->dif;

		return shadeNormal(hit);
	}

	void renderDepthView(std::vector<math::vec4>& pixels)
	{
		std::vector<float> dist(size_t(width) * height, -1.0f);
		float dMin = 1e30f, dMax = 0.0f;

		for (int j = 0; j < height; j++)
			for (int i = 0; i < width; i++)
			{
				const float s = (i + 0.5f) / float(width);
				const float t = (j + 0.5f) / float(height);
				Ray ray = camera.GenerateRay(s, t);
				const Hit hit = FindClosestCollision(ray);
				if (hit.distance >= 0.0f)
				{
					dist[i + width * j] = hit.distance;
					dMin = math::min(dMin, hit.distance);
					dMax = math::max(dMax, hit.distance);
				}
			}

		const float range = math::max(dMax - dMin, 1e-6f);

		for (int j = 0; j < height; j++)
			for (int i = 0; i < width; i++)
			{
				const float d = dist[i + width * j];
				const float g = (d < 0.0f) ? 0.0f : 1.0f - (d - dMin) / range;
				pixels[i + width * j] = math::vec4(math::vec3(g), 1.0f);
			}
	}

	math::vec3 toneMapGamma(math::vec3 color) const
	{
		color = color / (1.0f + color);
		color = math::pow(color, math::vec3(1.0f / 2.2f));
		return color;
	}

	void renderPathTraced(std::vector<math::vec4>& pixels, int spp, int maxDepth)
	{
#pragma omp parallel for
		for (int j = 0; j < height; j++)
			for (int i = 0; i < width; i++)
			{
				RNG rng(makeSeed(i, j, 0));
				math::vec3 color(0.0f);
				for (int s = 0; s < spp; s++)
				{
					const float u = (i + rng.uniform()) / float(width);
					const float v = (j + rng.uniform()) / float(height);
					Ray ray = camera.GenerateRay(u, v);
					color += tracePath(ray, maxDepth, rng);
				}
				pixels[i + width * j] = math::vec4(toneMapGamma(color / float(spp)), 1.0f);
			}
	}

	double benchmarkRender(int sampleCount, int depth)
	{
		std::vector<math::vec4> scratch(size_t(width) * height);
		const auto t0 = std::chrono::steady_clock::now();
		renderPathTraced(scratch, sampleCount, depth);
		const auto t1 = std::chrono::steady_clock::now();
		return std::chrono::duration<double>(t1 - t0).count();
	}

	void benchmarkSppSweep(const std::vector<int>& sppList, int depth = 8)
	{
		for (int s : sppList)
		{
			const double seconds = benchmarkRender(s, depth);
			std::cout << "scene=cornell"
			          << " res=" << width << "x" << height
			          << " spp=" << s
			          << " depth=" << depth
			          << " threads=1"
			          << " time=" << std::fixed << std::setprecision(3) << seconds << "s"
			          << std::endl;
		}
	}

	math::vec3 tracePath(const Ray ray, int depth, RNG& rng)
	{
		if (depth <= 0) return math::vec3(0.0f);

		Hit hit = FindClosestCollision(ray);

		if (hit.distance >= 0)
		{
			math::vec3 emitted = hit.obj->material->Emitted();
			Ray scattered;
			math::vec3 attenuation;

			if (hit.obj->material->Scatter(ray, hit, rng, attenuation, scattered))
				return emitted + attenuation * tracePath(scattered, depth - 1, rng);
			else
				return emitted;
		}

		return math::vec3(0.0f);
	}


	void Render(std::vector<math::vec4>& pixels)
	{
		std::fill(pixels.begin(), pixels.end(), math::vec4(0.0f, 0.0f, 0.0f, 1.0f));

		if (debugView == DebugView::Depth)
		{
			renderDepthView(pixels);
			return;
		}

		if (debugView == DebugView::PathTraced)
		{
			renderPathTraced(pixels, spp, maxDepth);
			return;
		}

#pragma omp parallel for
		for (int j = 0; j < height; j++)
			for (int i = 0; i < width; i++)
			{
				const float s = (i + 0.5f) / float(width);
				const float t = (j + 0.5f) / float(height);

				Ray pixelRay = camera.GenerateRay(s, t);
				pixels[i + width * j] = math::vec4(math::clamp(shade(pixelRay), 0.0f, 1.0f), 1.0f);
			}
	}

};