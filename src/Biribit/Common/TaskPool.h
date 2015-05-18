#pragma once

///////////////////////////////////////////////////////////////////////////////
// Implementation notes:
//
// This class requires at least Visual Studio 2010. It uses lambda function and
// for it's strong recommended to use std::bind for queueing calls with params.
//
// Example:
// sys::Future<ret_type> res = pool.enqueue(std::bind(inst, Class::func, p1, ...));
//
/////////////////////////////////////////////////////////////////////////////////


#include <vector>
#include <string>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class TaskPool {
public:
	TaskPool(size_t threads = 1, const std::string& name = std::string("unnamed"));
	~TaskPool();

	void Stop();

	template<class F, class... Args> auto enqueue(F&& f, Args&&... args)->std::future<typename std::result_of<F(Args...)>::type>;
private:

	std::vector< std::thread > workers;
	std::queue< std::function<void()> > tasks;

	std::string name;
	std::mutex queue_mutex;
	std::condition_variable condition;
	bool stop;
};
 
inline TaskPool::TaskPool(size_t threads, const std::string& name) : stop(false), name(name)
{
	for (size_t i = 0; i<threads; ++i)
		workers.emplace_back([this]
	{
		for (;;)
		{
			std::function<void()> task;
			{
				std::unique_lock<std::mutex> lock(this->queue_mutex);
				this->condition.wait(lock, [this] {
					return this->stop || !this->tasks.empty();
				});

				if (this->stop && this->tasks.empty())
					return;

				task = std::move(this->tasks.front());
				this->tasks.pop();
			}

			task();
		}
	});
}

template<class F, class... Args> auto TaskPool::enqueue(F&& f, Args&&... args)
-> std::future<typename std::result_of<F(Args...)>::type>
{
	using return_type = typename std::result_of<F(Args...)>::type;
	auto task = std::make_shared< std::packaged_task<return_type()> >(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

	std::future<return_type> res = task->get_future();
	{
		std::unique_lock<std::mutex> lock(queue_mutex);

		// don't allow enqueueing after stopping the pool
		if (stop)
			throw std::runtime_error("enqueue on stopped TaskPool");

		tasks.emplace([task](){ (*task)(); });
	}
	condition.notify_one();
	return res;
}

inline void TaskPool::Stop()
{
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		stop = true;
	}
	condition.notify_all();
	for (std::thread &worker : workers)
		worker.join();
}

inline TaskPool::~TaskPool()
{
    Stop();
}