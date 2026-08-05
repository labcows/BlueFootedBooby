#include <gtest/gtest.h>
#include <vector>

#include "Raytracer.h"

namespace
{
	std::vector<int> coverageCounts(const Raytracer& rt)
	{
		std::vector<int> counts(size_t(rt.width) * rt.height, 0);
		for (const Tile& t : rt.buildTiles())
			for (int y = t.y0; y < t.y1; y++)
				for (int x = t.x0; x < t.x1; x++)
					counts[size_t(y) * rt.width + x]++;
		return counts;
	}
}

TEST(BuildTiles, CoversEveryPixelExactlyOnce)
{
	Raytracer rt(100, 80);
	const std::vector<int> counts = coverageCounts(rt);

	int uncovered = 0, duplicated = 0;
	for (int c : counts)
	{
		if (c == 0)     uncovered++;
		else if (c > 1) duplicated++;
	}

	EXPECT_EQ(uncovered, 0)  << "pixels that no tile renders";
	EXPECT_EQ(duplicated, 0) << "pixels that more than one tile renders";
}

TEST(BuildTiles, StaysInsideTheImage)
{
	Raytracer rt(100, 80);

	for (const Tile& t : rt.buildTiles())
	{
		EXPECT_GE(t.x0, 0);
		EXPECT_GE(t.y0, 0);
		EXPECT_LE(t.x1, rt.width);
		EXPECT_LE(t.y1, rt.height);
		EXPECT_LT(t.x0, t.x1) << "tile has no width";
		EXPECT_LT(t.y0, t.y1) << "tile has no height";
	}
}

TEST(BuildTiles, ClampsEdgeTilesToTheImage)
{
	Raytracer rt(100, 80);
	rt.tileSize = 32;               

	const std::vector<Tile> tiles = rt.buildTiles();

	// 4 columns (x = 0, 32, 64, 96) by 3 rows (y = 0, 32, 64)
	EXPECT_EQ(tiles.size(), 12u);

	// The last tile starts at (96, 64), so it is only 4 wide and 16 tall.
	const Tile& last = tiles.back();
	EXPECT_EQ(last.x1 - last.x0, 4);
	EXPECT_EQ(last.y1 - last.y0, 16);
}

TEST(BuildTiles, DividesEvenlyWhenTheImageIsAMultiple)
{
	Raytracer rt(64, 64);
	rt.tileSize = 32;

	const std::vector<Tile> tiles = rt.buildTiles();

	EXPECT_EQ(tiles.size(), 4u);
	for (const Tile& t : tiles)
	{
		EXPECT_EQ(t.x1 - t.x0, 32);
		EXPECT_EQ(t.y1 - t.y0, 32);
	}
}

TEST(BuildTiles, ProducesOneTileWhenLargerThanTheImage)
{
	Raytracer rt(40, 24);
	rt.tileSize = 256;

	const std::vector<Tile> tiles = rt.buildTiles();

	ASSERT_EQ(tiles.size(), 1u);
	EXPECT_EQ(tiles[0].x0, 0);
	EXPECT_EQ(tiles[0].y0, 0);
	EXPECT_EQ(tiles[0].x1, 40);     // clamped to the image, not 256
	EXPECT_EQ(tiles[0].y1, 24);
}
