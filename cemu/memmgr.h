//Taken 11th Dec 2018 - https://github.com/eliben/code-for-blog/blob/master/2008/memmgr
//Last commit 4th March 2018 - https://github.com/eliben/code-for-blog/blob/18842aabbb7d998ab9284c6b1a0158823341e89e/2008/memmgr
//----------------------------------------------------------------
// Statically-allocated memory manager
//
// by Eli Bendersky (eliben@gmail.com)
//  
// This code is in the public domain.
//----------------------------------------------------------------
#ifndef MEMMGR_H
#define MEMMGR_H

//
// Memory manager: dynamically allocates memory from 
// a fixed pool.
// 
// Usage: after calling memmgr_init() in your 
// initialization routine, just use memmgr_alloc() instead
// of malloc() and memmgr_free() instead of free().
// Naturally, you can use the preprocessor to define 
// malloc() and free() as aliases to memmgr_alloc() and 
// memmgr_free(). This way the manager will be a drop-in 
// replacement for the standard C library allocators, and can
// be useful for debugging memory allocation problems and 
// leaks.
//
// Preprocessor flags you can define to customize the 
// memory manager:
//
// DEBUG_MEMMGR_FATAL
//    Allow printing out a message when allocations fail
//
// DEBUG_MEMMGR_SUPPORT_STATS
//    Allow printing out of stats in function 
//    memmgr_print_stats When this is disabled, 
//    memmgr_print_stats does nothing.
//
// Note that in production code on an embedded system 
// you'll probably want to keep those undefined, because
// they cause printf to be called.
//
// POOL_SIZE
//    Size of the pool for new allocations. This is 
//    effectively the heap size of the application, and can 
//    be changed in accordance with the available memory 
//    resources.
//
// MIN_POOL_ALLOC_QUANTAS
//    Internally, the memory manager allocates memory in
//    quantas roughly the size of two ulong objects. To
//    minimize pool fragmentation in case of multiple allocations
//    and deallocations, it is advisable to not allocate
//    blocks that are too small.
//    This flag sets the minimal ammount of quantas for 
//    an allocation. If the size of a ulong is 4 and you
//    set this flag to 16, the minimal size of an allocation
//    will be 4 * 2 * 16 = 128 bytes
//    If you have a lot of small allocations, keep this value
//    low to conserve memory. If you have mostly large 
//    allocations, it is best to make it higher, to avoid 
//    fragmentation.
//
// Notes:
// 1. This memory manager is *not thread safe*. Use it only
//    for single thread/task applications.
// 

#define DEBUG_MEMMGR_SUPPORT_STATS 1

//#define POOL_SIZE 8 * 1024
#define MIN_POOL_ALLOC_QUANTAS 16


typedef unsigned char byte;
typedef unsigned long ulong;

typedef ulong Align;

union mem_header_union
{
	struct
	{
		// Pointer to the next block in the free list
		//
		union mem_header_union* next;

		// Size of the block (in quantas of sizeof(mem_header_t))
		//
		ulong size;
	} s;

	// Used to align headers in memory to a boundary
	//
	Align align_dummy;
};

typedef union mem_header_union mem_header_t;

typedef struct
{
    mem_header_t Base;// Initial empty list
    mem_header_t* FreePtr;// Start of free list
    byte* Pool;// Static pool for new allocations
    size_t PoolSizeInBytes;
    ulong PoolFreePos;
    size_t BaseVirtualAddress;// Used to map virtual addresses to/from our real memory addresses (x86 emulator related)
} MemMgr;

// Initialize the memory manager.
//
int memmgr_init(MemMgr* memMgr, size_t heapSize);

// Destroy the memory manager.
//
int memmgr_destroy(MemMgr* memMgr);

int memmgr_is_initialized(MemMgr* memMgr);

//used to map virtual addresses to/from our real memory addresses (x86 emulator related)
size_t memmgr_get_base_virtual_address(MemMgr* memMgr);
void memmgr_set_base_virtual_address(MemMgr* memMgr, size_t baseVirtualAddress);
size_t memmgr_get_virtual_address(MemMgr* memMgr, size_t realAddress);
void* memmgr_get_real_address(MemMgr* memMgr, size_t virtualAddress);

size_t memmgr_get_size(MemMgr* memMgr);
size_t memmgr_get_free_size(MemMgr* memMgr);

int memmgr_is_first_pointer(MemMgr* memMgr, void* ptr);
size_t memmgr_get_data_address(MemMgr* memMgr);

void* memmgr_calloc(MemMgr* memMgr, size_t num, size_t size);
void* memmgr_realloc(MemMgr* memMgr, void* ptr, size_t size);

// 'malloc' clone
//
void* memmgr_alloc(MemMgr* memMgr, size_t nbytes);

// 'free' clone
//
void memmgr_free(MemMgr* memMgr, void* ap);

// Prints statistics about the current state of the memory
// manager
//
void memmgr_print_stats(MemMgr* memMgr);


#endif // MEMMGR_H