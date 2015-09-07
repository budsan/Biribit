#pragma once

#include <atomic>
#include <utility>
#include <mutex>
#include <memory>

template<class T> class RefSwap
{
	std::unique_ptr<T> ptr[2];
	std::atomic<std::size_t> sel;

public:

	RefSwap()
		: sel(0)
	{
		ptr[0] = std::unique_ptr<T>(new T());
		ptr[1] = std::unique_ptr<T>(new T());
	}

	RefSwap(RefSwap&& other)
	{
		std::swap(ptr[0], other.ptr[0]);
		std::swap(ptr[1], other.ptr[1]);
		sel = other.sel.load();
	}

	RefSwap(const RefSwap& other) : RefSwap()
	{
	}

	~RefSwap()
	{
	}

	const RefSwap& operator=(const RefSwap& other)
	{
		return *this;
	}

	const T& front(std::size_t* _revision) {
		if (_revision != nullptr)
			*_revision = sel;
		
		return *ptr[sel&1];
	}

	T& back() {
		return *ptr[(~sel)&1];
	}

	bool hasEverSwapped() {
		return sel > 0;
	}

	void swap() {
		sel++;
	}
};
