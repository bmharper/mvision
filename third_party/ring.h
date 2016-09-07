// This is free and unencumbered software released into the public domain.
#pragma once

namespace ring
{

/* Automatically sizing ring buffer

This is different to most ring buffer implementations, because here we automatically grow the
buffer size, when you try to write data, and there is not enough space available.

* Buffer is always a power of 2
* Buffer grows automatically
* Provides an easy to use copying API
* Provides a more complex zero-copy API

Based on recommendations from https://fgiesen.wordpress.com/2010/12/14/ring-buffers-and-queues/

zero-copy write example:

bool write(const uint8* src, uint srcSize, ring::Buf<uint8>& rbuf)
{
	uint srcPos = 0;
	while (srcSize != 0)
	{
		uint8* dst;
		size_t dstCount;
		if (b.WritePos(srcSize, dst, dstCount))
		{
			for (size_t i = 0; i < dstCount; i++)
				dst[i] = src[srcPos++];
			srcSize -= dstCount;
			b.IncrementWritePos(dstCount);
		}
	}
}

zero-copy read example:

bool read(ring::Buf<uint8>& rbuf)
{
	size_t maxRead = 10;
	while (rbuf.Size() != 0)
	{
		uint* dst;
		size_t dstCount;
		b.ReadPos(maxRead, dst, dstCount);
		for (size_t i = 0; i < dstCount; i++)
			dosomething(dst[i]);
		b.IncrementReadPos(dstCount);
	}
}
*/
template<typename T>
class Buf
{
public:
	typedef unsigned int uint;
	enum { InitialSize = 4 };

	~Buf();

	// zero-copy write
	// You may need to call this function more than once, if the write position wraps around.
	// After writing items into 'dst', you must call IncrementWritePos() to move the write position
	// ahead by however many items you have written (which must no more than dstCount).
	// count      The number of objects that you want to write.
	// dst        The output pointer, into which you can write at least one object.
	// dstCount   The number of objects that you can write into dst, which will be at least one, if the function returns true.
	// return     false only if the buffer needs to grow, and new T[] fails.
	bool	WritePos(size_t count, T*& dst, size_t& dstCount);

	// zero-copy read
	// You may need to call this function more than once, if the read position wraps around.
	// After reading items from 'dst', you must call IncrementReadPos() to move the read position
	// ahead by however many items you have read (which must no more than dstCount).
	// maxRead    The maximum number of items that you will read now.
	// dst        The read pointer.
	// dstCount   The number of objects that can be read out of 'dst'.
	void	ReadPos(size_t maxRead, T*& dst, size_t& dstCount) const;

	// Move the write position forward
	// Call this after calling WritePos(), and writing a number of items.
	void	IncrementWritePos(size_t count);

	// Move the read position forward
	// Call this after calling ReadPos(), and reading a number of items.
	void	IncrementReadPos(size_t count);

	// write
	// returns false only if the buffer needs to grow, and new T[] fails,
	bool	Write(const T* items, size_t count);

	// read
	// returns the number of items read, which is min(count, Size())
	size_t	Read(T* items, size_t count);

	// Write a single item
	// returns false only if the buffer needs to grow, and new T[] fails,
	bool	Write(const T& item);

	// Read a single item
	// Returns true if Size() was at least 1
	bool	Read(T& item);

	// Returns the number of items available to be read
	size_t	Size() const { return (WriteP - ReadP) & (DataSize - 1); } 

	// Returns the number of items that can still be written into the buffer.
	size_t	AvailableSpace() const { return DataSize == 0 ? 0 : DataSize - 1 - Size(); }

protected:
	T*		Data = nullptr;		// Data buffer
	uint	DataSize = 0;		// Size of Data. Always a power of 2.
	uint	ReadP = 0;			// Read position
	uint	WriteP = 0;			// Write position

	bool	EnsureCapacity(uint moreCount);
};

template<typename T>
Buf<T>::~Buf()
{
	delete[] Data;
}

template<typename T>
bool Buf<T>::WritePos(size_t count, T*& dst, size_t& dstCount)
{
	if (!EnsureCapacity((uint) count))
		return false;

	dst = Data + WriteP;
	if (count > DataSize - WriteP)
		dstCount = DataSize - WriteP;
	else
		dstCount = count;

	return true;
}

template<typename T>
void Buf<T>::ReadPos(size_t maxRead, T*& dst, size_t& dstCount) const
{
	uint size = (uint) Size();
	if (size > (uint) maxRead)
		size = (uint) maxRead;
	dst = Data + ReadP;
	if (size > DataSize - ReadP)
		dstCount = DataSize - ReadP;
	else
		dstCount = size;
}

template<typename T>
void Buf<T>::IncrementWritePos(size_t count)
{
	if (count > AvailableSpace())
		count = AvailableSpace();
	WritePos = (WritePos + count) & (DataSize - 1);
}

template<typename T>
void Buf<T>::IncrementReadPos(size_t count)
{
	if (count > Size())
		count = Size();
	ReadPos = (ReadPos + count) & (DataSize - 1);
}

template<typename T>
bool Buf<T>::Write(const T* items, size_t count)
{
	uint srcPos = 0;
	while (count != 0)
	{
		T* dst;
		size_t dstCount;
		if (!WritePos(count, dst, dstCount))
			return false;
		for (size_t i = 0; i < dstCount; i++)
			dst[i] = items[srcPos++];
		count -= dstCount;
		WritePos = (WritePos + dstCount) & (DataSize - 1);
	}
	return true;
}

template<typename T>
size_t Buf<T>::Read(T* items, size_t count)
{
	uint srcPos = 0;
	while (count != 0)
	{
		T* dst;
		size_t dstCount;
		ReadPos(count, dst, dstCount);
		if (dstCount == 0)
			break;
		for (size_t i = 0; i < dstCount; i++)
			items[srcPos++] = dst[i];
		count -= dstCount;
		ReadPos = (ReadPos + dstCount) & (DataSize - 1);
	}
	return srcPos;
}

template<typename T>
bool Buf<T>::Write(const T& item)
{
	return Write(&item, 1);
}

template<typename T>
bool Buf<T>::Read(T& item)
{
	return 1 == Read(&item, 1);
}

// Ensure our capacity is large enough to hold moreCount extra object. Grow by powers of 2.
template<typename T>
bool Buf<T>::EnsureCapacity(uint moreCount)
{
	static_assert(((InitialSize - 1) & InitialSize) == 0, "InitialSize must be a power of 2");

	// The +1 here is because we can only store DataSize-1 objects.
	uint needSize = moreCount + (uint) Size() + 1;
	if (needSize <= DataSize)
		return true;
	
	uint orgSize = DataSize;
	uint newSize = orgSize == 0 ? InitialSize : orgSize;
	while (newSize < needSize)
		newSize *= 2;

	T* newBuf = new T[newSize];
	if (newBuf == nullptr)
		return false;

	uint orgMask = orgSize - 1;
	for (uint i = ReadP, j = 0; i != WriteP; i = (i + 1) & orgMask, j++)
		newBuf[j] = Data[i];
	uint orgCount = (uint) Size();
	delete[] Data;
	Data = newBuf;
	DataSize = newSize;
	ReadP = 0;
	WriteP = orgCount;
	return true;
}

typedef Buf<unsigned char> ByteBuf;

}