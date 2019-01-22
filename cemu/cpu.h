#ifndef WEBC_CPU_H
#define WEBC_CPU_H

#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdio.h>
#include "memmgr.h"

#define CPU_WITH_DEBUG 1

typedef struct tCPU CPU;

typedef void(*FuncImportCallbackSig)(CPU* cpu);

typedef struct
{
    const char* TargetBinaryName;
    const char* DllName;
    const char* Name;
    FuncImportCallbackSig Callback;
    // For data imports
    uint32_t DataSize;
    uint32_t DataAddress;
    uint32_t ThunkAddress;
} ImportInfo;

struct tCPU
{
    // NOTE: There is slightly less heap than allocated due to the headers on each block in MemMgr
    
    uint8_t* VirtualMemory;// The virtual memory of our virtual process
    uint32_t VirtualMemoryAddress;// The base address of the virtual memory
    uint32_t VirtualMemorySize;// Image memory size + stack memory size + heap memory size
    uint32_t VirtualMemoryEndAddress;// The address where the virtual memory ends (VirtualMemoryAddress + VirtualMemorySize)
    uint32_t VirtualMemoryImageSize;// The fixed size of the executable in memory
    uint32_t VirtualMemoryHeapSize;// Size of the heap
    uint32_t VirtualMemoryStackAddress;// The address of the stack
    uint32_t VirtualMemoryStackEndAddress;// The address of the end of the stack (VirtualMemoryStackAddress + VirtualMemoryStackSize)
    uint32_t VirtualMemoryStackSize;// Size of the stack
    uint32_t VirtualEntryPointAddress;// The virtual address of the first instruction to execute
    
    MemMgr Memory;
    
    int32_t NumImports;
    ImportInfo* Imports;// Allocated into a single block of memory
    ImportInfo* UnhandledFunctionImport;// The fallback import handler
    uint32_t ImportsBeginAddress;// The address of where the imports start (virtul address)
    uint32_t ImportsEndAddress;// The address of where the imports end (virtul address) (ImportsBeginAddress + imports buffer size)

    uint32_t EIP;
    
    int32_t Prefixes;
    int32_t Flags;
    int32_t FlagsChanged;
    int32_t LastOp1;
    int32_t LastOp2;
    int32_t LastOpSize;
    int32_t LastAddResult;
    int32_t LastResult;
    uint8_t ModRM;
    size_t PhysAddr;
    int32_t Reg[8];
    
    jmp_buf JmpBuf;
    int32_t JmpBufInitialized;
    
    int32_t Complete;
    int32_t ErrorCode;
};

void cpu_init_imports(CPU* cpu);
ImportInfo* cpu_define_import_ex(CPU* cpu, int32_t* counter, const char* targetName, const char* dllName, const char* name, FuncImportCallbackSig function, uint32_t dataSize, uint32_t dataAddress);
ImportInfo* cpu_define_import(CPU* cpu, int32_t* counter, const char* targetName, const char* dllName, const char* name, FuncImportCallbackSig function);
ImportInfo* cpu_define_data_import(CPU* cpu, int32_t* counter, const char* targetName, const char* dllName, const char* name, uint32_t dataSize, uint32_t dataAddress);
ImportInfo* cpu_find_import(CPU* cpu, const char* targetName, const char* dllName, const char* name);

void cpu_init(CPU* cpu, uint32_t virtualAddress, uint32_t addressOfEntryPoint, uint32_t imageSize, uint32_t heapSize, uint32_t stackSize);
void cpu_destroy(CPU* cpu);
int32_t cpu_setjmp(CPU* cpu);
void cpu_onerror(CPU* cpu, char* error, ...);
#if CPU_WITH_DEBUG
void cpu_dbg_assert(CPU* cpu, int32_t cond, char* msg);
#else
#define cpu_dbg_assert(cpu, cond, msg)
#endif

int32_t cpu_is_osize_32(CPU* cpu);
int32_t cpu_is_asize_32(CPU* cpu);
int32_t cpu_modrm_resolve(CPU* cpu, uint8_t modrm);

uint32_t cpu_get_virtual_address(CPU* cpu, size_t realAddress);
size_t cpu_get_real_address(CPU* cpu, uint32_t virtualAddress);
void cpu_validate_address(CPU* cpu, uint32_t address);

uint8_t cpu_fetch_modrm(CPU* cpu);

int8_t cpu_fetchI8(CPU* cpu);
int16_t cpu_fetchI16(CPU* cpu);
int32_t cpu_fetchI32(CPU* cpu);
int64_t cpu_fetchI64(CPU* cpu);
uint8_t cpu_fetchU8(CPU* cpu);
uint16_t cpu_fetchU16(CPU* cpu);
uint32_t cpu_fetchU32(CPU* cpu);
uint64_t cpu_fetchU64(CPU* cpu);

int8_t cpu_readI8(CPU* cpu, uint32_t address);
int16_t cpu_readI16(CPU* cpu, uint32_t address);
int32_t cpu_readI32(CPU* cpu, uint32_t address);
int64_t cpu_readI64(CPU* cpu, uint32_t address);
uint8_t cpu_readU8(CPU* cpu, uint32_t address);
uint16_t cpu_readU16(CPU* cpu, uint32_t address);
uint32_t cpu_readU32(CPU* cpu, uint32_t address);
uint64_t cpu_readU64(CPU* cpu, uint32_t address);

void cpu_writeI8(CPU* cpu, uint32_t address, int8_t value);
void cpu_writeI16(CPU* cpu, uint32_t address, int16_t value);
void cpu_writeI32(CPU* cpu, uint32_t address, int32_t value);
void cpu_writeI64(CPU* cpu, uint32_t address, int64_t value);
void cpu_writeU8(CPU* cpu, uint32_t address, uint8_t value);
void cpu_writeU16(CPU* cpu, uint32_t address, uint16_t value);
void cpu_writeU32(CPU* cpu, uint32_t address, uint32_t value);
void cpu_writeU64(CPU* cpu, uint32_t address, uint64_t value);

void cpu_execute_instruction(CPU* cpu);
void cpu_execute_prefix_instruction(CPU* cpu);
void cpu_execute_instruction_t16(CPU* cpu);
void cpu_execute_instruction_t32(CPU* cpu);
void cpu_execute_instruction_0F_t16(CPU* cpu);
void cpu_execute_instruction_0F_t32(CPU* cpu);

#endif