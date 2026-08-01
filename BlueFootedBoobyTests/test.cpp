#include <gtest/gtest.h>
#include "Raytracer.h"

TEST(Smoke, TileCoversEveryPixel)
{
    Raytracer rt(100, 80);
    const auto tiles = rt.buildTiles();

    int covered = 0;
    for (const Tile& t : tiles)
        covered += (t.x1 - t.x0) * (t.y1 - t.y0);

    EXPECT_EQ(covered, 100 * 80);
}