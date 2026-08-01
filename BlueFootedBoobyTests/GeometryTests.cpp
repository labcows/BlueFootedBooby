#include <gtest/gtest.h>
#include "Sphere.h"

/* Test Sphere Intersection */

TEST(SphereIntersection, HitsFrontFace)
{
	Sphere sphere(math::vec3(0.0f, 0.0f, 10.0f), 2.0f);
	Ray ray{ math::vec3(0.0f), math::vec3(0.0f, 0.0f, 1.0f) };

	Hit hit = sphere.CheckRayCollision(ray);

	EXPECT_FLOAT_EQ(hit.distance, 8.0f);   
	EXPECT_FLOAT_EQ(math::length(hit.normal), 1.0f);
	EXPECT_FLOAT_EQ(hit.normal.z, -1.0f);
}

TEST(SphereIntersection, RayStartingOnSurfaceDoesNotSelfHit)
{
	Sphere sphere(math::vec3(0.0f, 0.0f, 10.0f), 2.0f);
	Ray ray{ math::vec3(0.0f, 0.0f, 8.0f), math::vec3(0.0f, 0.0f, -1.0f) };

	EXPECT_LT(sphere.CheckRayCollision(ray).distance, 0.0f);
}

TEST(SphereIntersection, RayDoesNotHitSphere)
{
	Sphere sphere(math::vec3(0.0f, 0.0f, 10.0f), 2.0f);
	Ray ray{ math::vec3(0.0f, 0.0f, 0.0f), math::vec3(0.0f, 1.0f, 0.0f) };

	Hit hit = sphere.CheckRayCollision(ray);

	EXPECT_LT(hit.distance, 0.0f);
}

TEST(SphereIntersection, RayComesFromTheSphereInside)
{
	Sphere sphere(math::vec3(0.0f, 0.0f, 10.0f), 2.0f);
	Ray ray{ math::vec3(0.0f, 0.0f, 9.0f), math::vec3(0.0f, 0.0f, 1.0f) };

	Hit hit = sphere.CheckRayCollision(ray);

	EXPECT_FLOAT_EQ(hit.distance, 3.0f);        
	EXPECT_FLOAT_EQ(hit.point.z, 12.0f);        
}

TEST(SphereIntersection, TheSphereIsBehindTheRay)
{
	Sphere sphere(math::vec3(0.0f, 0.0f, -10.0f), 2.0f);
	Ray ray{ math::vec3(0.0f), math::vec3(0.0f, 0.0f, 1.0f) };

	Hit hit = sphere.CheckRayCollision(ray);

	EXPECT_LT(hit.distance, 0.0f);
}

/* Test Rect */
