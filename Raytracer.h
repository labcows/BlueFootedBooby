#pragma once

#include "Sphere.h"
#include "Ray.h"
#include "Light.h"
#include "Triangle.h"
#include "Rect.h"
#include "pch.h"
#include "Camera.h"
#include "Material.h"
#include "ThreadPool.h"

#include <vector>
#include <chrono>
#include <iomanip>
#include <execution>
#include <numeric>

struct Tile
{
	int x0, y0, x1, y1;
};

class Raytracer
{
public:
	int width, height;
	Camera camera;

	Light light;
	std::vector<std::unique_ptr<Object>> objects;

	enum class DebugView { None, Normals, Depth, Albedo, PathTraced, PathTracedTiled };
	DebugView debugView = DebugView::Normals;
	int spp = 128;
	int maxDepth = 8;

	std::vector<uint32_t> horizontalIter, verticalIter;
	static constexpr bool multithreaded = true;
	mutable ThreadPool pool;
	int tileSize = 32;

	Raytracer(const int& width, const int &height)
		:width(width), height(height),
		 camera(math::vec3(278.0f, 278.0f, -800.0f), // Position
				math::vec3(278.0f, 278.0f, 0.0f),	 // lookAt
		        math::vec3(0.0f, 1.0f, 0.0f),		 // up
				40.0f,								 // vFovDegrees
				float(width) / float(height)),		 // aspect
		 pool(std::thread::hardware_concurrency())
	{
		horizontalIter.resize(width);
		verticalIter.resize(height);
		std::iota(horizontalIter.begin(), horizontalIter.end(), 0);
		std::iota(verticalIter.begin(), verticalIter.end(), 0);

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

	Hit FindClosestCollision(const Ray& ray) const
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

	math::vec3 traceRay(const Ray& ray, const int recurseLevel) const
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

	math::vec3 shade(const Ray& ray) const
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

	void renderDepthView(std::vector<math::vec4>& pixels) const
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

	void renderPathTracedSerial(std::vector<math::vec4>& pixels, int spp, int maxDepth) const
	{
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

	void renderPathTracedPar(std::vector<math::vec4>& pixels, int spp, int maxDepth) const
	{
		std::for_each(std::execution::par, verticalIter.begin(), verticalIter.end(),
			[this, &pixels, spp, maxDepth](uint32_t j)
			{
				std::for_each(horizontalIter.begin(), horizontalIter.end(),
					[this, &pixels, j, spp, maxDepth](uint32_t i)
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
					});
			});
	}

	void renderPathTraced(std::vector<math::vec4>& pixels, int spp, int maxDepth) const
	{
		if constexpr (multithreaded)
			renderPathTracedPar(pixels, spp, maxDepth);
		else
			renderPathTracedSerial(pixels, spp, maxDepth);
	}

	std::vector<Tile> buildTiles() const
	{
		std::vector<Tile> tiles;

		for (int y = 0; y < height; y += tileSize)
			for (int x = 0; x < width; x += tileSize)
			{
				Tile t = { x, y, std::min(x + tileSize, width), std::min(y + tileSize, height) };
				tiles.push_back(t);
			}

		return tiles;
	}

	void renderTile(std::vector<math::vec4>& pixels, const Tile& tile, int spp, int maxDepth) const
	{
		for (int y = tile.y0; y < tile.y1; y++)
			for (int x = tile.x0; x < tile.x1; x++)
			{
				RNG rng(makeSeed(x, y, 0));
				math::vec3 color(0.0f);
				for (int s = 0; s < spp; s++)
				{
					const float u = (x + rng.uniform()) / float(width);
					const float v = (y + rng.uniform()) / float(height);
					Ray ray = camera.GenerateRay(u, v);
					color += tracePath(ray, maxDepth, rng);
				}
				pixels[x + width * y] = math::vec4(toneMapGamma(color / float(spp)), 1.0f);
			}
	}

	void renderTilesWithPool(std::vector<math::vec4>& pixels, ThreadPool& workers,
	                         int spp, int maxDepth) const
	{
		for (const Tile& tile : buildTiles())
			workers.Submit(
				[this, &pixels, tile, spp, maxDepth] {
					renderTile(pixels, tile, spp, maxDepth);
				});

		workers.WaitAll();
	}

	void renderPathTracedTiled(std::vector<math::vec4>& pixels, int spp, int maxDepth) const
	{
		renderTilesWithPool(pixels, pool, spp, maxDepth);
	}

	void showTileAnalysis() const
	{
		std::vector<Tile> tiles = buildTiles();

		int size = tiles.size();
		std::cout << "Total number of tiles: "
			<< size
			<< ", last tile:  "
			<< tiles[size - 1].x1 - tiles[size - 1].x0
			<< " * "
			<< tiles[size - 1].y1 - tiles[size - 1].y0
			<< std::endl;
	}

	enum class RenderMode { Serial, ParallelStd, TiledPool };

	static const char* renderModeName(RenderMode mode)
	{
		switch (mode)
		{
		case RenderMode::Serial:      return "serial (1 thread)";
		case RenderMode::ParallelStd: return "std::execution::par";
		case RenderMode::TiledPool:   return "tiled thread pool";
		}
		return "?";
	}

	void renderWithMode(std::vector<math::vec4>& pixels, RenderMode mode, int spp, int maxDepth) const
	{
		switch (mode)
		{
		case RenderMode::Serial:      renderPathTracedSerial(pixels, spp, maxDepth); break;
		case RenderMode::ParallelStd: renderPathTracedPar(pixels, spp, maxDepth);    break;
		case RenderMode::TiledPool:   renderPathTracedTiled(pixels, spp, maxDepth);  break;
		}
	}

	double benchmarkRender(RenderMode mode, int sampleCount, int depth,
	                       std::vector<math::vec4>* out = nullptr) const
	{
		std::vector<math::vec4> scratch(size_t(width) * height);
		const auto t0 = std::chrono::steady_clock::now();
		renderWithMode(scratch, mode, sampleCount, depth);
		const auto t1 = std::chrono::steady_clock::now();

		if (out) *out = std::move(scratch);
		return std::chrono::duration<double>(t1 - t0).count();
	}

	// Counts pixels that differ between two renders. Should be 0: the per-pixel RNG is
	// seeded from (x, y), so the work split must not change any pixel's value.
	static size_t countDifferingPixels(const std::vector<math::vec4>& a,
	                                   const std::vector<math::vec4>& b)
	{
		size_t diff = 0;
		for (size_t i = 0; i < a.size(); i++)
			if (a[i] != b[i]) diff++;
		return diff;
	}

	void benchmarkSppSweep(const std::vector<int>& sppList, RenderMode mode, int depth = 8) const
	{
		const unsigned threads = (mode == RenderMode::Serial) ? 1u : std::thread::hardware_concurrency();
		for (int s : sppList)
		{
			const double seconds = benchmarkRender(mode, s, depth);
			std::cout << "scene=cornell"
			          << " res=" << width << "x" << height
			          << " spp=" << s
			          << " depth=" << depth
			          << " mode=" << renderModeName(mode)
			          << " threads=" << threads
			          << " time=" << std::fixed << std::setprecision(3) << seconds << "s"
			          << std::endl;
		}
	}

	// Renders the same frame three ways, times each, and verifies they are pixel-identical.
	void benchmarkCompare(const std::vector<int>& sppList, int depth = 8) const
	{
		const unsigned threads = std::thread::hardware_concurrency();

		std::cout << "\n=== Render mode comparison ==="
		          << "  scene=cornell  res=" << width << "x" << height
		          << "  depth=" << depth
		          << "  threads=" << threads
		          << "  tile=" << tileSize << "x" << tileSize << "\n\n";

		std::cout << std::left  << std::setw(6)  << "spp"
		          << std::right << std::setw(12) << "serial"
		          << std::setw(12) << "par"
		          << std::setw(12) << "tiled"
		          << std::setw(10) << "par x"
		          << std::setw(10) << "tiled x"
		          << std::setw(14) << "pixel diff" << "\n";
		std::cout << std::string(76, '-') << "\n";

		for (int s : sppList)
		{
			std::vector<math::vec4> imgSerial, imgPar, imgTiled;

			const double tSerial = benchmarkRender(RenderMode::Serial,      s, depth, &imgSerial);
			const double tPar    = benchmarkRender(RenderMode::ParallelStd, s, depth, &imgPar);
			const double tTiled  = benchmarkRender(RenderMode::TiledPool,   s, depth, &imgTiled);

			const size_t diffPar   = countDifferingPixels(imgSerial, imgPar);
			const size_t diffTiled = countDifferingPixels(imgSerial, imgTiled);

			std::cout << std::fixed << std::setprecision(3)
			          << std::left  << std::setw(6)  << s
			          << std::right << std::setw(11) << tSerial << "s"
			          << std::setw(11) << tPar   << "s"
			          << std::setw(11) << tTiled << "s"
			          << std::setprecision(2)
			          << std::setw(9)  << (tSerial / tPar)   << "x"
			          << std::setw(9)  << (tSerial / tTiled) << "x"
			          << std::setw(14) << (diffPar + diffTiled)
			          << "\n";
		}

		std::cout << "\n(pixel diff must be 0 - the parallel splits must not change any pixel)\n"
		          << std::endl;
	}

	// Scaling curve for the hand-built pool: same frame rendered with N worker threads.
	// Only possible because the pool takes a thread count - std::execution::par does not.
	void benchmarkThreadScaling(const std::vector<unsigned>& threadCounts,
	                            int spp = 64, int depth = 8) const
	{
		std::cout << "\n=== Thread scaling (tiled pool) ==="
		          << "  scene=cornell  res=" << width << "x" << height
		          << "  spp=" << spp
		          << "  depth=" << depth
		          << "  tile=" << tileSize << "x" << tileSize << "\n\n";

		std::cout << std::left  << std::setw(10) << "threads"
		          << std::right << std::setw(12) << "time"
		          << std::setw(12) << "speedup"
		          << std::setw(14) << "efficiency"
		          << std::setw(14) << "pixel diff" << "\n";
		std::cout << std::string(62, '-') << "\n";

		std::vector<math::vec4> reference;
		double baseline = 0.0;

		for (unsigned n : threadCounts)
		{
			std::vector<math::vec4> scratch(size_t(width) * height);
			ThreadPool workers(n);

			const auto t0 = std::chrono::steady_clock::now();
			renderTilesWithPool(scratch, workers, spp, depth);
			const auto t1 = std::chrono::steady_clock::now();
			const double seconds = std::chrono::duration<double>(t1 - t0).count();

			if (reference.empty()) { reference = scratch; baseline = seconds; }
			const size_t diff = countDifferingPixels(reference, scratch);

			std::cout << std::fixed
			          << std::left  << std::setw(10) << n
			          << std::right << std::setprecision(3) << std::setw(11) << seconds << "s"
			          << std::setprecision(2) << std::setw(11) << (baseline / seconds) << "x"
			          << std::setw(13) << (baseline / seconds / double(n) * 100.0) << "%"
			          << std::setw(14) << diff
			          << "\n";
		}

		std::cout << "\n(efficiency = speedup / threads; SMT means 16 logical threads are not 16 cores)\n"
		          << std::endl;
	}

	math::vec3 tracePath(const Ray ray, int depth, RNG& rng) const
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


	void Render(std::vector<math::vec4>& pixels) const
	{
		std::fill(pixels.begin(), pixels.end(), math::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		showTileAnalysis();

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

		if (debugView == DebugView::PathTracedTiled)
		{
			renderPathTracedTiled(pixels, spp, maxDepth);
			return;
		}

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