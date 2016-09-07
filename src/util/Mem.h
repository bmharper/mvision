#pragma once

namespace sx
{
void* AlignedAlloc(size_t bytes, size_t alignment);
void* AlignedRealloc(size_t original_block_bytes, void* block, size_t bytes, size_t alignment);
void  AlignedFree(void* block);
}
