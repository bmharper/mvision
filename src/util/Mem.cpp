#include "pch.h"
#include "Mem.h"
#include <malloc.h>
#include <stdlib.h>

namespace sx
{

//#define HAVE_POSIX_MEMALIGN 1

/*
How do we implement this manually?

Alignment must be a power of 2. If it is not, then AlignedAlloc returns NULL.

Some examples illustrating 4 byte alignment, allocating 8 bytes of usable memory:

. Wasted byte at start
# Our byte that tells us how many bytes of wasted space before the usable memory
- Usable, aligned memory
* Wasted byte at end

Bytes before alignment point
1					#--------***			(# = 1) There are 3 bytes extra at the end of the usable space
2					.#--------**			(# = 2) There are 2 bytes extra at the end of the usable space
3					..#--------*			(# = 3) There is 1 byte extra at the end of the usable space
4					...#--------			(# = 4) Original malloc result was perfect. We had to burn 4 bytes. Zero bytes extra at the end of the usable space.

We always allocate (bytes + alignment), we always waste "alignment" bytes.
*/

void* AlignedAlloc(size_t bytes, size_t alignment)
{
#ifdef _WIN32
	return _aligned_malloc(bytes, alignment);
#elif defined(HAVE_POSIX_MEMALIGN)
	void* p = NULL;
	if (0 != posix_memalign(&p, alignment, bytes))
		return NULL;
	return p;
#else
	// Since our offset is a single byte, we don't support more than 128-byte alignment.
	if (alignment > 128)
		return NULL;

	size_t alignment_mask = alignment - 1;

	// Ensure that alignment is a power of 2
	if ((alignment_mask & alignment) != 0)
		return NULL;

	size_t raw = (size_t) malloc(bytes + alignment);
	size_t usable = 0;
	if (raw)
	{
		usable = (raw + alignment) & ~alignment_mask;
		*((unsigned char*) (usable - 1)) = (unsigned char) (usable - raw);
	}
	return (void*) usable;
#endif
}

// alignment must be the same as the original block
void* AlignedRealloc(size_t original_block_bytes, void* block, size_t bytes, size_t alignment)
{
#ifdef _WIN32
	return _aligned_realloc(block, bytes, alignment);
#else
	void* p = AlignedAlloc(bytes, alignment);
	if (!p)
		return NULL;
	memcpy(p, block, original_block_bytes);
	AlignedFree(block);
	return p;
#endif
}

void AlignedFree(void* block)
{
#ifdef _WIN32
	_aligned_free(block);
#elif defined(HAVE_POSIX_MEMALIGN)
	free(block);
#else
	if (block != nullptr)
	{
		unsigned char* usable = (unsigned char*) block;
		unsigned char* raw = usable - usable[-1];
		free(raw);
	}
#endif
}

}

