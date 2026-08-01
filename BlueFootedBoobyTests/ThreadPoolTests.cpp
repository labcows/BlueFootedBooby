#include <gtest/gtest.h>
#include <atomic>
#include "ThreadPool.h"

TEST(ThreadPoolTest, EveryThreadExecutesOnlyOneTime)
{
    ThreadPool pool(16);
    const int jobCount = 1000;
    std::vector<int> counts(jobCount, 0);
    for (int i = 0; i < jobCount; i++)
        pool.Submit([&counts, i] { counts[i]++; });

    pool.WaitAll();
    for (int i = 0; i < jobCount; i++)
        EXPECT_EQ(counts[i], 1);
}   

TEST(ThreadPoolTest, ThreadPoolWaitsAllWorkers)
{
    ThreadPool pool(4);
    std::atomic<int> counter{ 0 };

    for (int i = 0; i < 20; i++)
        pool.Submit([&counter] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        counter++;
            });

    pool.WaitAll();
    EXPECT_EQ(counter, 20);
}

TEST(ThreadPoolTest, ReusesPoolAcrossBatches)
{
    ThreadPool pool(8);
    std::atomic<int> counter{ 0 };

    for (int i = 0; i < 100; i++) pool.Submit([&counter] { counter++; });
    pool.WaitAll();

    for (int i = 0; i < 100; i++) pool.Submit([&counter] { counter++; });
    pool.WaitAll();
    EXPECT_EQ(counter, 200);
}