#include <atomic>
#include "ThreadPool.h"

void testThreadPool()
{
    std::cout << "=== ThreadPool test ===" << std::endl;

    // 1) 모든 작업이 정확히 한 번씩 실행되는가
    {
        ThreadPool pool(16);
        std::atomic<int> counter{ 0 };
        const int jobCount = 1000;

        for (int i = 0; i < jobCount; i++)
            pool.Submit([&counter] { counter++; });

        pool.WaitAll();
        std::cout << "  counter = " << counter
            << " (expected " << jobCount << ") "
            << (counter == jobCount ? "PASS" : "FAIL") << std::endl;
    }   // ← 여기서 소멸자 호출: 데드락 없이 통과해야 함
    std::cout << "  destructor OK" << std::endl;

    // 2) WaitAll이 실제로 기다리는가 (작업마다 지연을 줘서)
    {
        ThreadPool pool(4);
        std::atomic<int> counter{ 0 };

        for (int i = 0; i < 20; i++)
            pool.Submit([&counter] {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            counter++;
                });

        pool.WaitAll();
        std::cout << "  slow jobs = " << counter
            << " (expected 20) "
            << (counter == 20 ? "PASS" : "FAIL") << std::endl;
    }

    // 3) 재사용: 같은 풀에 두 번 제출
    {
        ThreadPool pool(8);
        std::atomic<int> counter{ 0 };

        for (int i = 0; i < 100; i++) pool.Submit([&counter] { counter++; });
        pool.WaitAll();

        for (int i = 0; i < 100; i++) pool.Submit([&counter] { counter++; });
        pool.WaitAll();

        std::cout << "  reuse = " << counter
            << " (expected 200) "
            << (counter == 200 ? "PASS" : "FAIL") << std::endl;
    }

    std::cout << "=== done ===" << std::endl;
}