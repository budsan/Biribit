#pragma once

#include <utility>
#include <mutex>

template<class T> class RefSwap
{
	T* ptr[2];
	std::size_t sel;

	std::mutex mutex;
	unsigned int revision;

public:

	RefSwap(RefSwap&& other)
	{
		ptr[0] = other.ptr[0];
		ptr[1] = other.ptr[1];
		sel = other.sel;
		revision = revision;
	}

	template<class... Args> RefSwap(Args&&... args)
		: sel(0)
		, revision(0)
	{
		ptr[0] = new T(std::forward<Args>(args)...);
		ptr[1] = new T(std::forward<Args>(args)...);
	}

	~RefSwap() {
		delete ptr[0];
		delete ptr[1];
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
