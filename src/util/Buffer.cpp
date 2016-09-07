#include "pch.h"
#include "Buffer.h"

namespace sx
{

ByteBuf::~ByteBuf()
{
	free(Data);
}

void ByteBuf::Push(uint8_t v)
{
	Ensure(Len + 1);
	Data[Len++] = v;
}

void ByteBuf::Push(const void* v, size_t size)
{
	Ensure(Len + size);
	memcpy(Data + Len, v, size);
	Len += size;
}

void ByteBuf::Ensure(size_t capacityAtLeast)
{
	if (Cap >= capacityAtLeast)
		return;
	size_t newCap = Cap == 0 ? 64 : Cap * 2;
	while (newCap < capacityAtLeast)
		newCap *= 2;
	uint8_t* newData = (uint8_t*) realloc(Data, newCap);
	SXASSERT(newData != nullptr);
	Data = newData;
	Cap = newCap;
}

}
