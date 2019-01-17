#ifndef WEBC_CPU_H
#define WEBC_CPU_H

#include <stdint.h>
#include "memmgr.h"

typedef void(*FuncImportCallbackSig)(struct CPU* cpu);

typedef struct
{
    const char* TargetBinaryName;
    const char* DllName;
    const char* Name;
    FuncImportCallbackSig Callback;
} ImportInfo;

typedef struct
{
    // NOTE: We get slightly less heap than allocated due to the headers on each block in MemMgr
    
    uint8_t* VirtualMemory;// The virtual memory of our virtual process
    int32_t VirtualMemoryAddress;// The base address of the virtual memory
    int32_t VirtualMemorySize;// Image memory size + stack memory size + heap memory size
    int32_t VirtualMemoryEndAddress;// The address where the virtual memory ends (VirtualMemoryAddress + VirtualMemorySize)
    int32_t VirtualMemoryImageSize;// The fixed size of the executable in memory
    int32_t VirtualMemoryHeapSize;// Size of the heap
    int32_t VirtualMemoryStackAddress;// The address of the stack
    int32_t VirtualMemoryStackEndAddress;// The address of the end of the stack (+1) (VirtualMemoryStackAddress + VirtualMemoryStackSize)
    int32_t VirtualMemoryStackSize;// Size of the stack
    int32_t VirtualEntryPointAddress;// The virtual address of the first instruction to execute
    
    MemMgr Memory;
    
    ImportInfo* Imports;// Allocated into a single block of memory
    ImportInfo* UnhandledFunctionImport;// The fallback import handler
    size_t FunctionImportsBegin;
    size_t FunctionImportsEnd;
    
    // An offset into the virtualMemory array so that we don't have to keep calculating the offset each time EIP modified
    // Use cpu_get_eip() / cpu_set_eip() to access the regular version of the EIP
    int32_t MemEIP;
    
    int32_t Initialized;
    int32_t Complete;
    int32_t Error;
} CPU;

void cpu_init_imports(CPU* cpu);
ImportInfo* cpu_define_import(CPU* cpu, const char* targetName, const char* dllName, const char* name, FuncImportCallbackSig function, int* counter);
ImportInfo* cpu_find_import(CPU* cpu, const char* targetName, const char* dllName, const char* name);

int32_t cpu_readI32(CPU* cpu, int32_t address);
int32_t cpu_init(CPU* cpu, int32_t virtualAddress, int32_t addressOfEntryPoint, int32_t imageSize, int32_t heapSize, int32_t stackSize);
void cpu_destroy(CPU* cpu);
int32_t cpu_get_eip(CPU* cpu);
void cpu_set_eip(CPU* cpu, int32_t eip);

#endif