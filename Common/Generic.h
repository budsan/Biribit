#ifndef GENERIC_H_
#define GENERIC_H_

#include <stddef.h>

namespace Generic
{

template <typename T> inline void Swap(T &a, T &b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

template <typename T> void SafeDelete(T* &toDelete) {
	if (toDelete != NULL) {
		delete toDelete;
		toDelete = NULL;
	}
}

template <typename T> void SafeDeleteArray(T* &toDelete) {
	if (toDelete != NULL) {
		delete[] toDelete;
		toDelete = NULL;
	}
}

struct TempBuffer
{
	 TempBuffer();
	~TempBuffer();

	void Ensure(unsigned int size);
	void Resize(unsigned int size);
	
	unsigned int size;
	char *data;
};

}// namespace Generic

#endif