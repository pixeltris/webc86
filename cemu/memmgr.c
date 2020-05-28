//----------------------------------------------------------------
// Statically-allocated memory manager
//
// by Eli Bendersky (eliben@gmail.com)
//
// This code is in the public domain.
//----------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "memmgr.h"

int memmgr_init(MemMgr* memMgr, size_t heapSize)
{
	if (memMgr->Pool != NULL)
	{
		return 1;
	}

	memMgr->Base.s.next = 0;
	memMgr->Base.s.size = 0;
	memMgr->FreePtr = 0;
	memMgr->PoolFreePos = 0;
	
	memMgr->BaseVirtualAddress = 0;
	memMgr->PoolSizeInBytes = heapSize;
	memMgr->Pool = calloc(heapSize, 1);
	if (memMgr->Pool == NULL)
	{
		return 2;
	}	
	return 0;
}

int memmgr_destroy(MemMgr* memMgr)
{
	if (memMgr->Pool == NULL)
	{
		return 0;
	}
	free(memMgr->Pool);
	memMgr->Pool = NULL;
	memMgr->PoolSizeInBytes = 0;
	memMgr->BaseVirtualAddress = 0;
	return 0;
}

int memmgr_is_initialized(MemMgr* memMgr)
{
	return memMgr->Pool != NULL;
}

size_t memmgr_get_base_virtual_address(MemMgr* memMgr)
{
	return memMgr->BaseVirtualAddress;
}

void memmgr_set_base_virtual_address(MemMgr* memMgr, size_t baseVirtualAddress)
{
    memMgr->BaseVirtualAddress = baseVirtualAddress;
}

size_t memmgr_get_virtual_address(MemMgr* memMgr, size_t realAddress)
{
    if (realAddress == 0)
    {
        return 0;
    }
	size_t dataAddress = memmgr_get_data_address(memMgr);
	return memMgr->BaseVirtualAddress + (realAddress - dataAddress);
}

void* memmgr_get_real_address(MemMgr* memMgr, size_t virtualAddress)
{
    if (virtualAddress == 0)
    {
        return 0;
    }
	size_t dataAddress = memmgr_get_data_address(memMgr);
	return (void*)(dataAddress + (virtualAddress - memMgr->BaseVirtualAddress));
}

size_t memmgr_get_size(MemMgr* memMgr)
{
	return memMgr->PoolSizeInBytes;
}

size_t memmgr_get_free_size(MemMgr* memMgr)
{
	return memMgr->PoolSizeInBytes - memMgr->PoolFreePos;
}

int memmgr_is_first_pointer(MemMgr* memMgr, void* ptr)
{
	mem_header_t* block = ((mem_header_t*)ptr) - 1;
	return (void*)block == (void*)memMgr->Pool;
}

size_t memmgr_get_data_address(MemMgr* memMgr)
{
	return (size_t)(((mem_header_t*)memMgr->Pool) + 1);
}

void* memmgr_calloc(MemMgr* memMgr, size_t num, size_t size)
{
	size_t totalSize = num * size;
	void* ptr = memmgr_alloc(memMgr, totalSize);
	if (ptr != NULL)
	{
		memset(ptr, 0, totalSize);
	}
	return ptr;
}

void* memmgr_realloc(MemMgr* memMgr, void* ptr, size_t size)
{
	if (ptr == NULL)
	{
		return memmgr_alloc(memMgr, size);
	}

	// acquire pointer to block header
	mem_header_t* block = ((mem_header_t*)ptr) - 1;

	// the block is already big enough, no change required
	size_t currentSize = (block->s.size - 1) * sizeof(mem_header_t);
	if (currentSize >= size)
	{
		return ptr;
	}

	//memmgr_alloc a new address, copy the data, and then memmgr_free the old block
	void* newPtr = memmgr_alloc(memMgr, size);
	if (newPtr == NULL)
	{
		return NULL;
	}
	memcpy(newPtr, ptr, currentSize);
	memmgr_free(memMgr, ptr);
	return newPtr;
}

void memmgr_print_stats(MemMgr* memMgr)
{
#ifdef DEBUG_MEMMGR_SUPPORT_STATS
	mem_header_t* p;

	printf("------ Memory manager stats ------\n\n");
	printf("Pool: free_pos = %lu (%lu bytes left)\n\n",
		memMgr->PoolFreePos, memMgr->PoolSizeInBytes - memMgr->PoolFreePos);

	p = (mem_header_t*)memMgr->Pool;

	while (p < (mem_header_t*)(memMgr->Pool + memMgr->PoolFreePos))
	{
		printf("  * Addr: %p; Size: %8lu\n",
			p, p->s.size);

		p += p->s.size;
	}

	printf("\nFree list:\n\n");

	if (memMgr->FreePtr)
	{
		p = memMgr->FreePtr;

		while (1)
		{
			printf("  * Addr: %p; Size: %8lu; Next: %p\n",
				p, p->s.size, p->s.next);

			p = p->s.next;

			if (p == memMgr->FreePtr)
				break;
		}
	}
	else
	{
		printf("Empty\n");
	}

	printf("\n");
#endif // DEBUG_MEMMGR_SUPPORT_STATS
}


static mem_header_t* get_mem_from_pool(MemMgr* memMgr, ulong nquantas)
{
	ulong total_req_size;

	mem_header_t* h;

	if (nquantas < MIN_POOL_ALLOC_QUANTAS)
		nquantas = MIN_POOL_ALLOC_QUANTAS;

	total_req_size = nquantas * sizeof(mem_header_t);

	if (memMgr->PoolFreePos + total_req_size <= memMgr->PoolSizeInBytes)
	{
		h = (mem_header_t*)(memMgr->Pool + memMgr->PoolFreePos);
		h->s.size = nquantas;
		memmgr_free(memMgr, (void*)(h + 1));
		memMgr->PoolFreePos += total_req_size;
	}
	else
	{
		return 0;
	}

	return memMgr->FreePtr;
}


// Allocations are done in 'quantas' of header size.
// The search for a free block of adequate size begins at the point 'freep'
// where the last block was found.
// If a too-big block is found, it is split and the tail is returned (this
// way the header of the original needs only to have its size adjusted).
// The pointer returned to the user points to the free space within the block,
// which begins one quanta after the header.
//
void* memmgr_alloc(MemMgr* memMgr, size_t nbytes)
{
	mem_header_t* p;
	mem_header_t* prevp;

	// Calculate how many quantas are required: we need enough to house all
	// the requested bytes, plus the header. The -1 and +1 are there to make sure
	// that if nbytes is a multiple of nquantas, we don't allocate too much
	//
	ulong nquantas = ((ulong)nbytes + sizeof(mem_header_t) - 1) / sizeof(mem_header_t) + 1;

	// First alloc call, and no free list yet ? Use 'base' for an initial
	// denegerate block of size 0, which points to itself
	//
	if ((prevp = memMgr->FreePtr) == 0)
	{
		memMgr->Base.s.next = memMgr->FreePtr = prevp = &memMgr->Base;
		memMgr->Base.s.size = 0;
	}

	for (p = prevp->s.next; ; prevp = p, p = p->s.next)
	{
		// big enough ?
		if (p->s.size >= nquantas)
		{
			// exactly ?
			if (p->s.size == nquantas)
			{
				// just eliminate this block from the free list by pointing
				// its prev's next to its next
				//
				prevp->s.next = p->s.next;
			}
			else // too big
			{
				p->s.size -= nquantas;
				p += p->s.size;
				p->s.size = nquantas;
			}

			memMgr->FreePtr = prevp;
			return (void*)(p + 1);
		}
		// Reached end of free list ?
		// Try to allocate the block from the pool. If that succeeds,
		// get_mem_from_pool adds the new block to the free list and
		// it will be found in the following iterations. If the call
		// to get_mem_from_pool doesn't succeed, we've run out of
		// memory
		//
		else if (p == memMgr->FreePtr)
		{
			if ((p = get_mem_from_pool(memMgr, nquantas)) == 0)
			{
#ifdef DEBUG_MEMMGR_FATAL
				printf("!! Memory allocation failed !!\n");
#endif
				return 0;
			}
		}
	}
}


// Scans the free list, starting at freep, looking the the place to insert the
// free block. This is either between two existing blocks or at the end of the
// list. In any case, if the block being freed is adjacent to either neighbor,
// the adjacent blocks are combined.
//
void memmgr_free(MemMgr* memMgr, void* ap)
{
    if (!ap)
    {
        return;
    }
    
	mem_header_t* block;
	mem_header_t* p;

	// acquire pointer to block header
	block = ((mem_header_t*)ap) - 1;

	// Find the correct place to place the block in (the free list is sorted by
	// address, increasing order)
	//
	for (p = memMgr->FreePtr; !(block > p && block < p->s.next); p = p->s.next)
	{
		// Since the free list is circular, there is one link where a
		// higher-addressed block points to a lower-addressed block.
		// This condition checks if the block should be actually
		// inserted between them
		//
		if (p >= p->s.next && (block > p || block < p->s.next))
			break;
	}

	// Try to combine with the higher neighbor
	//
	if (block + block->s.size == p->s.next)
	{
		block->s.size += p->s.next->s.size;
		block->s.next = p->s.next->s.next;
	}
	else
	{
		block->s.next = p->s.next;
	}

	// Try to combine with the lower neighbor
	//
	if (p + p->s.size == block)
	{
		p->s.size += block->s.size;
		p->s.next = block->s.next;
	}
	else
	{
		p->s.next = block;
	}

	memMgr->FreePtr = p;
}