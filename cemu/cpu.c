#include <stdio.h>
#include "cpu.h"
#include "memmgr.h"

int32_t cpu_init(CPU* cpu, int32_t virtualAddress, int32_t addressOfEntryPoint, int32_t imageSize, int32_t heapSize, int32_t stackSize)
{
    cpu->VirtualMemory = (uint8_t*)cpu->Memory.Pool;
    cpu->VirtualMemoryAddress = virtualAddress;
    cpu->VirtualMemorySize = (int32_t)cpu->Memory.PoolSizeInBytes;
    cpu->VirtualMemoryEndAddress = cpu->VirtualMemoryAddress + cpu->VirtualMemorySize;
    cpu->VirtualMemoryImageSize = imageSize;
    cpu->VirtualMemoryHeapSize = heapSize;
    cpu->VirtualMemoryStackSize = stackSize;
    cpu->VirtualEntryPointAddress = addressOfEntryPoint;
    
    // Allocate memory for the stack
    void* stackPtr = memmgr_alloc(&cpu->Memory, cpu->VirtualMemoryStackSize);
    if (!stackPtr)
    {
        printf("Failed to allocate the stack %d.\n", (int32_t)cpu->VirtualMemoryStackSize);
        return 1;
    }
    
    cpu->VirtualMemorySize = cpu->VirtualMemoryImageSize + cpu->VirtualMemoryStackSize + cpu->VirtualMemoryHeapSize;
    if (cpu->VirtualMemorySize != cpu->Memory.PoolSizeInBytes)
    {
        printf("Bad cpu memory size. Expected:%d actual:%d.\n", (int32_t)cpu->VirtualMemorySize, (int32_t)cpu->Memory.PoolSizeInBytes);
        return 1;
    }

    return 0;
}

void cpu_destroy(CPU* cpu)
{
}