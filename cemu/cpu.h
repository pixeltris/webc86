#ifndef WEBC_CPU_H
#define WEBC_CPU_H

#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include "memmgr.h"

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
    
    // An offset into the virtualMemory array so that we don't have to keep calculating the offset each time EIP modified
    // Use cpu_get_eip() / cpu_set_eip() to access the regular version of the EIP
    uint32_t MemEIP;
    
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
void cpu_onerror(CPU* cpu, char* error);

uint32_t cpu_get_virtual_address(CPU* cpu, size_t realAddress);
size_t cpu_get_real_address(CPU* cpu, uint32_t virtualAddress);
void cpu_validate_address(CPU* cpu, uint32_t address);
uint32_t cpu_readI32(CPU* cpu, uint32_t address);
void cpu_writeU32(CPU* cpu, uint32_t address, uint32_t value);

uint32_t cpu_get_eip(CPU* cpu);
void cpu_set_eip(CPU* cpu, uint32_t eip);

void cpu_execute_instruction(CPU* cpu);

#endif