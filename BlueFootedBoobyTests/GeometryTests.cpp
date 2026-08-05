#include <gtest/gtest.h>
#include "Sphere.h"
#include "Rect.h"

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
TEST(RectIntersection, MissesWhenPlaneHitIsOutsideBounds)
{
	Rect floor(math::vec3(0, 0, 0), math::vec3(10, 0, 10), 1);
	Ray ray{ math::vec3(40, 5, 5), math::vec3(0, -1, 0) };

	Hit hit = floor.CheckRayCollision(ray);

	EXPECT_LT(hit.distance, 0.0f);
}

TEST(RectIntersection, HitsPlaneOnXAxis)
{
	Rect wallX(math::vec3(0, 0, 0), math::vec3(0, 4, 20), 0);
	Ray ray{ math::vec3(5, 2, 15), math::vec3(-1, 0, 0) };

	Hit hit = wallX.CheckRayCollision(ray);

	ASSERT_GT(hit.distance, 0.0f);
	EXPECT_FLOAT_EQ(hit.distance, 5.0f);
	EXPECT_FLOAT_EQ(hit.point.y, 2.0f);
	EXPECT_FLOAT_EQ(hit.point.z, 15.0f);
}

TEST(RectIntersection, HitsPlaneOnYAxis)
{
	Rect wallY(math::vec3(0, 0, 0), math::vec3(4, 0, 20), 1);
	Ray ray{ math::vec3(2, 5, 10), math::vec3(0, -1, 0) };

	Hit hit = wallY.CheckRayCollision(ray);

	ASSERT_GT(hit.distance, 0.0f);
	EXPECT_FLOAT_EQ(hit.distance, 5.0f);
	EXPECT_FLOAT_EQ(hit.point.x, 2.0f);
	EXPECT_FLOAT_EQ(hit.point.z, 10.0f);
}

TEST(RectIntersection, HitsPlaneOnZAxis)
{
	Rect wallZ(math::vec3(0, 0, 0), math::vec3(4, 4, 0), 2);
	Ray ray{ math::vec3(2, 4, 10), math::vec3(0, 0, -1) };

	Hit hit = wallZ.CheckRayCollision(ray);

	ASSERT_GT(hit.distance, 0.0f);
	EXPECT_FLOAT_EQ(hit.distance, 10.0f);
	EXPECT_FLOAT_EQ(hit.point.x, 2.0f);
	EXPECT_FLOAT_EQ(hit.point.y, 4.0f);
}

TEST(RectIntersection, PerpendicularRayHitsCenter)
{
	Rect wallZ(math::vec3(0, 0, 0), math::vec3(4, 4, 0), 2);
	Ray ray{ math::vec3(2, 2, 2), math::vec3(0, 0, -1) };

	Hit hit = wallZ.CheckRayCollision(ray);

	ASSERT_GT(hit.distance, 0.0f);
	EXPECT_FLOAT_EQ(hit.distance, 2.0f);
}

TEST(RectIntersection, NormalFlipsWhenHitFromBehind)
{
	Rect floor(math::vec3(0, 0, 0), math::vec3(10, 0, 10), 1);
	Ray ray{ math::vec3(5, -5, 5), math::vec3(0, 1, 0) };

	Hit hit = floor.CheckRayCollision(ray);
	
	ASSERT_GT(hit.distance, 0.0f);
	EXPECT_FLOAT_EQ(hit.distance, 5.0f);
	EXPECT_FLOAT_EQ(hit.normal.y, -1.0f);
}

TEST(RectIntersection, ObliqueRayHitsExpectedPoint)
{
	Rect floor(math::vec3(0, 0, 0), math::vec3(10, 0, 10), 1);
	const float s = 0.70710678f;
	Ray ray{ math::vec3(0, 5, 5), math::vec3(s, -s, 0) };

	Hit hit = floor.CheckRayCollision(ray);

	ASSERT_GT(hit.distance, 0.0f);
	EXPECT_NEAR(hit.distance, 7.0711f, 1e-3f);
	EXPECT_NEAR(hit.point.x, 5.0f, 1e-4f);
	EXPECT_NEAR(hit.point.z, 5.0f, 1e-4f);
}

TEST(RectIntersection, MissesWhenRayIsParallelToPlane)
{
	Rect floor(math::vec3(0, 0, 0), math::vec3(10, 0, 10), 1);
	Ray ray{ math::vec3(5, 5, 5), math::vec3(1, 0, 0) };   // dir.y == 0

	Hit hit = floor.CheckRayCollision(ray);

	EXPECT_LT(hit.distance, 0.0f);
}

TEST(RectIntersection, MissesWhenRectIsBehindRay)
{
	Rect floor(math::vec3(0, 0, 0), math::vec3(10, 0, 10), 1);
	Ray ray{ math::vec3(5,5,5), math::vec3(0,1,0) };

	Hit hit = floor.CheckRayCollision(ray);
	EXPECT_LT(hit.distance, 0.0f);
}