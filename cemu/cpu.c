#include <stdio.h>
#include "cpu.h"
#include "memmgr.h"

void cpu_init(CPU* cpu, uint32_t virtualAddress, uint32_t addressOfEntryPoint, uint32_t imageSize, uint32_t heapSize, uint32_t stackSize)
{
    cpu->VirtualMemory = (uint8_t*)cpu->Memory.Pool;
    cpu->VirtualMemoryAddress = virtualAddress;
    cpu->VirtualMemorySize = (uint32_t)cpu->Memory.PoolSizeInBytes;
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
        cpu_onerror(cpu, NULL);
        return;
    }
    
    cpu->VirtualMemorySize = cpu->VirtualMemoryStackSize + cpu->VirtualMemoryHeapSize;
    if (cpu->VirtualMemorySize != cpu->Memory.PoolSizeInBytes)
    {
        printf("Bad cpu memory size. Expected:%d actual:%d.\n", cpu->VirtualMemorySize, cpu->Memory.PoolSizeInBytes);
        cpu_onerror(cpu, NULL);
        return;
    }
    
    cpu_allocate_data_imports(cpu);
}

void cpu_destroy(CPU* cpu)
{
    memmgr_destroy(&cpu->Memory);
}

int32_t cpu_setjmp(CPU* cpu)
{
    cpu->JmpBufInitialized = 1;
    return setjmp(cpu->JmpBuf);
}

void cpu_onerror(CPU* cpu, char* error)
{
    if (cpu->ErrorCode != 0)
    {
        return;
    }
    
    if (error != NULL)
    {
        printf("%s", error);
    }
    cpu->ErrorCode = 1;
    if (cpu->JmpBufInitialized)
    {
        longjmp(cpu->JmpBuf, 1);
    }
}

void cpu_execute_instruction(CPU* cpu)
{
    cpu->Complete = 1;
}

uint32_t cpu_get_virtual_address(CPU* cpu, size_t realAddress)
{
    return memmgr_get_virtual_address(&cpu->Memory, realAddress);
}

size_t cpu_get_real_address(CPU* cpu, uint32_t virtualAddress)
{
    return memmgr_get_real_address(&cpu->Memory, virtualAddress);
}

void cpu_validate_address(CPU* cpu, uint32_t address)
{
    if (address < cpu->VirtualMemoryAddress || address >= cpu->VirtualMemoryEndAddress)
    {
        printf("cpu_validate_address failed Address:0x%08X MemStart:0x%08X MemEnd:0x%08X\n", address, cpu->VirtualMemory, cpu->VirtualMemoryEndAddress);
        cpu_onerror(cpu, NULL);
    }
}

void cpu_writeU32(CPU* cpu, uint32_t address, uint32_t value)
{
    uint32_t* ptr = (uint32_t*)cpu_get_real_address(cpu, address);
    *ptr = value;
}

uint32_t cpu_get_eip(CPU* cpu)
{
    return cpu->MemEIP + cpu->VirtualMemoryAddress;
}

void cpu_set_eip(CPU* cpu, uint32_t eip)
{
    cpu->MemEIP = eip - cpu->VirtualMemoryAddress;
}