#pragma once

#include "../util/Error.h"

namespace sx
{

// It would be good to extend ring::Buf so that it can do the "magic ring buffer" thing,
// and then we should be able to replace some uses of this with that.
class ByteBuf
{
public:
	uint8_t*	Data = nullptr;
	size_t		Cap = 0;
	size_t		Len = 0;

	~ByteBuf();
	void Push(uint8_t v);
	void Push(const void* v, size_t size);
	void Ensure(size_t capacityAtLeast);
};

}

namespace std
{

inline void swap(sx::ByteBuf& a, sx::ByteBuf& b)
{
	char tmp[sizeof(sx::ByteBuf)];
	memcpy(tmp, &a, sizeof(a));
	memcpy(&a, &b, sizeof(a));
	memcpy(&b, tmp, sizeof(a));
}

}
