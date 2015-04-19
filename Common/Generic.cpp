#include "Generic.h"

namespace Generic
{

TempBuffer::TempBuffer()
: size(0),
  data(NULL)
{
}

TempBuffer::~TempBuffer()
{
	SafeDeleteArray(data);
}

void TempBuffer::Ensure(unsigned int _size)
{
	if(size < _size) Resize(_size);
}

void TempBuffer::Resize(unsigned int _size)
{
	Generic::SafeDeleteArray(data);
	size = _size;
	data = new char[size];
}

}// namespace Generic