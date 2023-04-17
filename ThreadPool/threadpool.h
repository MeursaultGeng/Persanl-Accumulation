#pragma once

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <mutex>
#include <queue>
#include <functional>
#include <future>
#include <thread>
#include <utility>
#include <vector>
#include <condition_variable>

//a safe task queue
template <typename T>
class SafeQueue
{
private:
	std::queue<T> m_queue;
	std::mutex m_mutex;

public:
	SafeQueue(){}
	SafeQueue(SafeQueue &&other){}
	~SafeQueue(){}

	bool empty()
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		return m_queue.empty();
	}

	int size()
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		return m_queue.size();
	}

	void enqueue(T& t)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_queue.emplace(t);
	}

	bool dequeue(T& t)
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		if (m_queue.empty)
			return false;
		t = std::move(m_queue.front());

		m_queue.pop();
		return true;
	}
};

//a thread pool used to run the task cyclely
class ThreadPool
{
private:
	class ThreadWorker
	{
	private:
		int m_id;  //thread id
		ThreadPool* m_pool;

	public:
		//constructor of ThreadWorker
		ThreadWorker(ThreadPool *pool, const int id):m_pool(pool), m_id(id)
		{}

		// to overload the () function
		void operator()()
		{
			std::function<void()> func;
			bool dequeued;

			//to constantly run the task in the task queue, wait when the task queue is empty, 
			//and wake when task-adding action happens in the task queue
		    while (!m_pool->m_shutdown)
			{
				{
					std::unique_lock<std::mutex> lock(m_pool->m_condition_mutex);

					if (m_pool->m_queue.empty())
						m_pool->m_conditional_lock.wait(lock);

					dequeued = m_pool->m_queue.dequeue(func);
				}
				if (dequeued)
					func();
			}
		}
	};
	bool m_shutdown;
	SafeQueue<std::function<void()>> m_queue;
	std::vector<std::thread> m_threads;
	std::mutex m_condition_mutex;
	std::condition_variable m_conditional_lock;

public:
	//contructor of ThreadPool
	ThreadPool(const int n_threads = 4)    //n_threads represents the max amount of threads in the thread pool
		:m_threads(std::vector<std::thread>(n_threads)), m_shutdown(false)
	{}
	
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

	//to initialize the thread pool
	void init()
	{
		for (unsigned int i = 0; i < m_threads.size(); ++i)
			m_threads.at(i) = std::thread(ThreadWorker(this, i));
	}

	//to shut down the thread pool
	void shutdown()
	{
		m_shutdown = true;
		m_conditional_lock.notify_all();

		for (unsigned int i = 0; i < m_threads.size(); ++i)
			if (m_threads.at(i).joinable())
				m_threads.at(i).join();
	}

	template<typename F, typename... Args>
	auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
	{
		std::function<decltype(f(args...))()> func = std::bind(std::forward<F>f, std::forward<Args>(args...));  // something wrong happens here.

		auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

		std::function<void()> warpper_func = [task_ptr]()
		{
			(*task_ptr)();
		};

		m_queue.enqueue(wrapper_func);
		m_conditional_lock.notify_one();
		return task_ptr->get_future();
	}
};
#endif