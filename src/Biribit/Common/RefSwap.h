#pragma once

#include <utility>
#include <mutex>
#include <memory>

template<class T> class RefSwap
{
	std::unique_ptr<T> ptr[2];
	std::size_t sel;

	std::mutex mutex;
	unsigned int revision;

public:

	RefSwap()
		: sel(0)
		, revision(0)
	{
		ptr[0] = std::unique_ptr<T>(new T());
		ptr[1] = std::unique_ptr<T>(new T());
	}

	RefSwap(RefSwap&& other)
	{
		std::swap(ptr[0], other.ptr[0]);
		std::swap(ptr[1], other.ptr[1]);
		std::swap(sel, other.sel);
		std::swap(revision, other.revision);
	}

	RefSwap(const RefSwap& other) : RefSwap()
	{
	}


	template<class... Args> RefSwap(Args&&... args)
		: sel(0)
		, revision(0)
	{
		ptr[0] = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
		ptr[1] = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}

	~RefSwap()
	{
	}

	const RefSwap& operator=(const RefSwap& other)
	{
	}

	const T& front(unsigned int* _revision) {
		std::lock_guard<std::mutex> lock(mutex);
		if (_revision != nullptr)
			*_revision = revision;
		
		return *ptr[sel&1];
	}

	T& back() {
		return *ptr[(~sel)&1];
	}

	bool hasEverSwapped() {
		return revision > 0;
	}

	void swap() {
		std::lock_guard<std::mutex> lock(mutex);
		revision++;
		sel = ~sel;
	}
};
