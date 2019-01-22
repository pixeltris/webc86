#include "cpu.h"
#include "memmgr.h"
#include <stdarg.h>

#define PREFIX_MASK_OPSIZE 0x20
#define PREFIX_MASK_ADDRSIZE 0x40

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
        cpu_onerror(cpu, "Failed to allocate the stack %d.\n", (int32_t)cpu->VirtualMemoryStackSize);
        return;
    }
    
    cpu->VirtualMemorySize = cpu->VirtualMemoryStackSize + cpu->VirtualMemoryHeapSize;
    if (cpu->VirtualMemorySize != cpu->Memory.PoolSizeInBytes)
    {
        cpu_onerror(cpu, "Bad cpu memory size. Expected:%d actual:%d.\n", cpu->VirtualMemorySize, cpu->Memory.PoolSizeInBytes);
        return;
    }
    
    cpu->EIP = cpu->VirtualEntryPointAddress;
    
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

void cpu_onerror(CPU* cpu, char* error, ...)
{
    if (cpu->ErrorCode != 0)
    {
        cpu->Complete = 1;
        return;
    }
    
    if (error != NULL)
    {
        va_list args;
        va_start(args, error);
        vprintf(error, args);
        va_end(args);
    }
    cpu->ErrorCode = 1;
    cpu->Complete = 1;
    if (cpu->JmpBufInitialized)
    {
        longjmp(cpu->JmpBuf, 1);
    }
}

#if CPU_WITH_DEBUG
void cpu_dbg_assert(CPU* cpu, int32_t cond, char* msg)
{
    if (!cond)
    {
        if (msg != NULL)
        {
            printf("%s", msg);
        }
        cpu_onerror(cpu, NULL);
    }
}
#endif

int32_t cpu_is_osize_32(CPU* cpu)
{
    return !((cpu->Prefixes & PREFIX_MASK_OPSIZE) == PREFIX_MASK_OPSIZE);
}

int32_t cpu_is_asize_32(CPU* cpu)
{
    return !((cpu->Prefixes & PREFIX_MASK_ADDRSIZE) == PREFIX_MASK_ADDRSIZE);
}

void cpu_execute_instruction(CPU* cpu)
{
    cpu_execute_instruction_t32(cpu);
}

void cpu_execute_prefix_instruction(CPU* cpu)
{
    if (cpu_is_osize_32(cpu))
    {
        cpu_execute_instruction_t32(cpu);
    }
    else
    {
        cpu_execute_instruction_t16(cpu);
    }
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
        cpu_onerror(cpu, "cpu_validate_address failed Address:0x%08X MemStart:0x%08X MemEnd:0x%08X\n", address, cpu->VirtualMemory, cpu->VirtualMemoryEndAddress);
    }
}

uint8_t cpu_fetch_modrm(CPU* cpu)
{
    return cpu->ModRM = cpu_fetchU8(cpu);
}

int8_t cpu_fetchI8(CPU* cpu)
{
    int8_t value = cpu_readI8(cpu, cpu->EIP);
    cpu->EIP += 1;
    return value;
}

int16_t cpu_fetchI16(CPU* cpu)
{
    int16_t value = cpu_readI16(cpu, cpu->EIP);
    cpu->EIP += 2;
    return value;
}

int32_t cpu_fetchI32(CPU* cpu)
{
    int32_t value = cpu_readI32(cpu, cpu->EIP);
    cpu->EIP += 4;
    return value;
}

int64_t cpu_fetchI64(CPU* cpu)
{
    int64_t value = cpu_readI64(cpu, cpu->EIP);
    cpu->EIP += 8;
    return value;
}

uint8_t cpu_fetchU8(CPU* cpu)
{
    uint8_t value = cpu_readU8(cpu, cpu->EIP);
    cpu->EIP += 1;
    return value;
}

uint16_t cpu_fetchU16(CPU* cpu)
{
    uint16_t value = cpu_readU16(cpu, cpu->EIP);
    cpu->EIP += 2;
    return value;
}

uint32_t cpu_fetchU32(CPU* cpu)
{
    uint32_t value = cpu_readU32(cpu, cpu->EIP);
    cpu->EIP += 4;
    return value;
}

uint64_t cpu_fetchU64(CPU* cpu)
{
    uint64_t value = cpu_readU64(cpu, cpu->EIP);
    cpu->EIP += 8;
    return value;
}

int8_t cpu_readI8(CPU* cpu, uint32_t address)
{
    int8_t* ptr = (int8_t*)cpu_get_real_address(cpu, address);
    return *ptr;
}

int16_t cpu_readI16(CPU* cpu, uint32_t address)
{
    int16_t* ptr = (int16_t*)cpu_get_real_address(cpu, address);
    return *ptr;
}

int32_t cpu_readI32(CPU* cpu, uint32_t address)
{
    int32_t* ptr = (int32_t*)cpu_get_real_address(cpu, address);
    return *ptr;
}

int64_t cpu_readI64(CPU* cpu, uint32_t address)
{
    int64_t* ptr = (int64_t*)cpu_get_real_address(cpu, address);
    return *ptr;
}

uint8_t cpu_readU8(CPU* cpu, uint32_t address)
{
    uint8_t* ptr = (uint8_t*)cpu_get_real_address(cpu, address);
    return *ptr;
}

uint16_t cpu_readU16(CPU* cpu, uint32_t address)
{
    uint16_t* ptr = (uint16_t*)cpu_get_real_address(cpu, address);
    return *ptr;
}

uint32_t cpu_readU32(CPU* cpu, uint32_t address)
{
    uint32_t* ptr = (uint32_t*)cpu_get_real_address(cpu, address);
    return *ptr;
}

uint64_t cpu_readU64(CPU* cpu, uint32_t address)
{
    uint64_t* ptr = (uint64_t*)cpu_get_real_address(cpu, address);
    return *ptr;
}

void cpu_writeI8(CPU* cpu, uint32_t address, int8_t value)
{
    int8_t* ptr = (int8_t*)cpu_get_real_address(cpu, address);
    *ptr = value;
}

void cpu_writeI16(CPU* cpu, uint32_t address, int16_t value)
{
    int16_t* ptr = (int16_t*)cpu_get_real_address(cpu, address);
    *ptr = value;
}

void cpu_writeI32(CPU* cpu, uint32_t address, int32_t value)
{
    int32_t* ptr = (int32_t*)cpu_get_real_address(cpu, address);
    *ptr = value;
}

void cpu_writeI64(CPU* cpu, uint32_t address, int64_t value)
{
    int64_t* ptr = (int64_t*)cpu_get_real_address(cpu, address);
    *ptr = value;
}

void cpu_writeU8(CPU* cpu, uint32_t address, uint8_t value)
{
    uint8_t* ptr = (uint8_t*)cpu_get_real_address(cpu, address);
    *ptr = value;
}

void cpu_writeU16(CPU* cpu, uint32_t address, uint16_t value)
{
    uint16_t* ptr = (uint16_t*)cpu_get_real_address(cpu, address);
    *ptr = value;
}

void cpu_writeU32(CPU* cpu, uint32_t address, uint32_t value)
{
    uint32_t* ptr = (uint32_t*)cpu_get_real_address(cpu, address);
    *ptr = value;
}

void cpu_writeU64(CPU* cpu, uint32_t address, uint64_t value)
{
    uint64_t* ptr = (uint64_t*)cpu_get_real_address(cpu, address);
    *ptr = value;
}