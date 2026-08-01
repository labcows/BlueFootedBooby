#pragma once
#include "pch.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <vector>

class ThreadPool
{
public:
	ThreadPool(unsigned int numThreads)
	{
		numbers = std::min(numbers, numThreads);

		for (unsigned int i = 0; i < numbers; i++)
		{
			workers.emplace_back(&ThreadPool::WorkerLoop, this);
		}
	}

	void Submit(std::function<void()> job)
	{
		std::unique_lock lock(mutex);
		queue.push(job);

		remainingJobs++;

		lock.unlock();
		jobAvailable.notify_one();
	}

	void WorkerLoop()
	{
		while (true)
		{
			std::unique_lock lock(mutex);

			jobAvailable.wait(lock, [this] {return stopping || !queue.empty(); });

			if (stopping && queue.empty())
			{
				lock.unlock();
				break;
			}
				
			std::function<void()> job = std::move(queue.front()); 
			queue.pop();

			lock.unlock();

			job();

			bool allDone = false;
			{
				std::unique_lock lock2(mutex);
				remainingJobs--;
				allDone = (remainingJobs == 0);
			}

			if (allDone)	jobDone.notify_all();
		}

		return;
	}

	void WaitAll()
	{
		std::unique_lock lock(mutex);
		jobDone.wait(lock, [this] { return remainingJobs == 0; });
	}

	~ThreadPool()
	{
		{
			std::unique_lock lock(mutex); stopping = true;
		}
		jobAvailable.notify_all();
		for (auto& w : workers) w.join();
	}

private:
	unsigned numbers = std::thread::hardware_concurrency();
	unsigned remainingJobs = 0;
	bool stopping = false;

	std::condition_variable jobDone;
	std::condition_variable jobAvailable;

	std::mutex mutex;

	std::queue<std::function<void()>> queue;
	std::vector <std::thread> workers;
};