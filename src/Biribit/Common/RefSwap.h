#pragma once

#include <utility>

template<class T> class RefSwap
{
	T* ptr[2];
	std::size_t sel;

	bool ready;
	bool dirty;

	unsigned int revision;

public:

	template<class... Args> RefSwap(Args&&... args)
		: sel(0)
		, ready(true)
		, dirty(true)
		, revision(0)
	{
		ptr[0] = new T(std::forward<Args>(args)...);
		ptr[1] = new T(std::forward<Args>(args)...);
	}

	~RefSwap() {
		delete ptr[0];
		delete ptr[1];
	}

	T& front() {
		return *ptr[sel&1];
	}

	T& back() {
		return *ptr[(~sel)&1];
	}

	bool request()
	{
		if (ready && dirty){
			ready = false;
			return true;
		}

		return false;
	}

	void set_dirty()
	{
		dirty = true;
	}

	unsigned int get_revision()
	{
		return revision;
	}

	void swap() {
		revision++;
		sel = ~sel;
		ready = true;
		dirty = false;
	}
};
