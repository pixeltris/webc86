#ifndef WEBC_CPU_H
#define WEBC_CPU_H

#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdio.h>
#include "memmgr.h"

#define CPU_WITH_DEBUG 1

#define OPSIZE_8 7
#define OPSIZE_16 15
#define OPSIZE_32 31

#define SEG_PREFIX_NONE -1
#define SEG_PREFIX_ZERO 7
#define PREFIX_MASK_OPSIZE 0x20
#define PREFIX_MASK_ADDRSIZE 0x40

#define SIZE_MASK_8 0xFF
#define SIZE_MASK_16 0xFFFF
#define SIZE_MASK_32 0xFFFFFFFF

#define REG_EAX 0
#define REG_ECX 1
#define REG_EDX 2
#define REG_EBX 3
#define REG_ESP 4
#define REG_EBP 5
#define REG_ESI 6
#define REG_EDI 7

#define REG_ES 0
#define REG_CS 1
#define REG_SS 2
#define REG_DS 3
#define REG_FS 4
#define REG_GS 5

// flags register bitflags
#define FLAG_CARRY 1
#define FLAG_PARITY 4
#define FLAG_ADJUST 16
#define FLAG_ZERO 64
#define FLAG_SIGN 128
#define FLAG_TRAP 256
#define FLAG_INTERRUPT 512
#define FLAG_DIRECTION 1024
#define FLAG_OVERFLOW 2048

// default values of reserved flags bits
#define FLAGS_DEFAULT (1 << 1)

// bitmask to select non-reserved flags bits
#define FLAGS_MASK (FLAG_CARRY | FLAG_PARITY | FLAG_ADJUST | FLAG_ZERO | FLAG_SIGN | FLAG_TRAP | FLAG_INTERRUPT | FLAG_DIRECTION | FLAG_OVERFLOW)

// all arithmetic flags
#define FLAGS_ALL (FLAG_CARRY | FLAG_PARITY | FLAG_ADJUST | FLAG_ZERO | FLAG_SIGN | FLAG_OVERFLOW)

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

typedef union
{
    long double fp;
    uint8_t b[10];
    uint16_t s[4];
    uint32_t i[2];
    uint64_t ll;
} FPURegister;

typedef struct
{
    FPURegister St[8];
    int32_t StackEmpty;
    int32_t StackPtr;
    int32_t ControlWord;
    int32_t StatusWord;
    float Float32;
    double Float64;
} FPU;

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

    FPU Fpu;
    
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
    uint32_t TempAddr;
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

void cpu_print_callstack(CPU* cpu, int32_t maxFrames);
void cpu_check_stack_memory(CPU* cpu);
void cpu_exec_call(CPU* cpu, uint32_t addr);
int32_t cpu_exec_check_jump_function(CPU* cpu, uint32_t addr);

uint32_t cpu_get_stack_reg(CPU* cpu);
void cpu_set_stack_reg(CPU* cpu, uint32_t value);
void cpu_adjust_stack_reg(CPU* cpu, int32_t value);
uint32_t cpu_get_esp(CPU* cpu, int32_t mod);
void cpu_set_esp(CPU* cpu, uint32_t value);
uint32_t cpu_get_eip(CPU* cpu);
void cpu_set_eip(CPU* cpu, uint32_t addr);

int32_t cpu_is_osize_32(CPU* cpu);
int32_t cpu_is_asize_32(CPU* cpu);
int32_t cpu_modrm_resolve(CPU* cpu, uint8_t modrm);
int32_t cpu_sib_resolve(CPU* cpu, uint8_t mod);
uint32_t cpu_get_seg_prefix_ds(CPU* cpu);
uint32_t cpu_get_seg_prefix_ss(CPU* cpu);
uint32_t cpu_get_seg_prefix_cs(CPU* cpu);
uint32_t cpu_get_seg_prefix(CPU* cpu, int32_t seg);
uint32_t cpu_get_seg(CPU* cpu, int32_t seg);
uint8_t cpu_get_reg8(CPU* cpu, int32_t index);
void cpu_set_reg8(CPU* cpu, int32_t index, uint8_t value);

uint8_t cpu_getReg8(CPU* cpu, int32_t reg);
int8_t cpu_getReg8s(CPU* cpu, int32_t reg);
uint16_t cpu_getReg16(CPU* cpu, int32_t reg);
int16_t cpu_getReg16s(CPU* cpu, int32_t reg);
uint32_t cpu_getReg32(CPU* cpu, int32_t reg);
int32_t cpu_getReg32s(CPU* cpu, int32_t reg);
void cpu_setReg8(CPU* cpu, int32_t reg, uint8_t val);
void cpu_setReg8s(CPU* cpu, int32_t reg, int8_t val);
void cpu_setReg16(CPU* cpu, int32_t reg, uint16_t val);
void cpu_setReg16s(CPU* cpu, int32_t reg, int16_t val);
void cpu_setReg32(CPU* cpu, int32_t reg, uint32_t val);
void cpu_setReg32s(CPU* cpu, int32_t reg, int32_t val);

void cpu_push16(CPU* cpu, uint16_t val);
void cpu_push32(CPU* cpu, uint32_t val);
uint16_t cpu_pop16(CPU* cpu);
int32_t cpu_pop32s(CPU* cpu);
uint32_t cpu_pop32(CPU* cpu);

void cpu_trigger_de(CPU* cpu);
void cpu_trigger_ud(CPU* cpu);

int32_t cpu_is_valid_address(CPU* cpu, uint32_t addr, int32_t size);
uint32_t cpu_get_virtual_address(CPU* cpu, size_t realAddress);
size_t cpu_get_real_address(CPU* cpu, uint32_t virtualAddress);
void cpu_validate_address(CPU* cpu, uint32_t address);

// These are all fetch functions
uint8_t cpu_read_op0F(CPU* cpu);
uint8_t cpu_read_sib(CPU* cpu);
uint8_t cpu_read_op8(CPU* cpu);
int8_t cpu_read_op8s(CPU* cpu);
uint16_t cpu_read_op16(CPU* cpu);
int16_t cpu_read_op16s(CPU* cpu);
int32_t cpu_read_op32s(CPU* cpu);
uint8_t cpu_read_disp8(CPU* cpu);
int8_t cpu_read_disp8s(CPU* cpu);
uint16_t cpu_read_disp16(CPU* cpu);
int32_t cpu_read_disp32s(CPU* cpu);

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

int32_t cpu_read_moffs(CPU* cpu);

uint8_t cpu_read_e8(CPU* cpu);
int8_t cpu_read_e8s(CPU* cpu);
uint16_t cpu_read_e16(CPU* cpu);
int16_t cpu_read_e16s(CPU* cpu);
uint32_t cpu_read_e32(CPU* cpu);
int32_t cpu_read_e32s(CPU* cpu);
void cpu_set_e8(CPU* cpu, uint8_t value);
void cpu_set_e16(CPU* cpu, uint16_t value);
void cpu_set_e32(CPU* cpu, uint32_t value);
uint8_t cpu_read_write_e8(CPU* cpu);
void cpu_write_e8(CPU* cpu, uint8_t value);
uint16_t cpu_read_write_e16(CPU* cpu);
void cpu_write_e16(CPU* cpu, uint16_t value);
uint32_t cpu_read_write_e32(CPU* cpu);
void cpu_write_e32(CPU* cpu, uint32_t value);
uint8_t cpu_read_g8(CPU* cpu);
void cpu_write_g8(CPU* cpu, uint8_t value);
uint16_t cpu_read_g16(CPU* cpu);
int16_t cpu_read_g16s(CPU* cpu);
void cpu_write_g16(CPU* cpu, uint16_t value);
int32_t cpu_read_g32s(CPU* cpu);
void cpu_write_g32(CPU* cpu, uint32_t value);

void cpu_jmpcc8(CPU* cpu, int32_t condition);
void cpu_jmp_rel16(CPU* cpu, int16_t rel16);
void cpu_jmpcc16(CPU* cpu, int32_t condition);
void cpu_jmpcc32(CPU* cpu, int32_t condition);
void cpu_setcc(CPU* cpu, int32_t condition);

int32_t cpu_getcf(CPU* cpu);
int32_t cpu_getpf(CPU* cpu);
int32_t cpu_getaf(CPU* cpu);
int32_t cpu_getzf(CPU* cpu);
int32_t cpu_getsf(CPU* cpu);
int32_t cpu_getof(CPU* cpu);
int32_t cpu_test_o(CPU* cpu);
int32_t cpu_test_b(CPU* cpu);
int32_t cpu_test_z(CPU* cpu);
int32_t cpu_test_s(CPU* cpu);
int32_t cpu_test_p(CPU* cpu);
int32_t cpu_test_be(CPU* cpu);
int32_t cpu_test_l(CPU* cpu);
int32_t cpu_test_le(CPU* cpu);

uint16_t cpu_xchg16(CPU* cpu, uint16_t memory_data, uint8_t modrm_byte);
uint32_t cpu_xchg32(CPU* cpu, uint32_t memory_data, uint8_t modrm_byte);

void cpu_execute_instruction(CPU* cpu);
void cpu_execute_prefix_instruction(CPU* cpu);
void cpu_execute_instruction_t16(CPU* cpu);
void cpu_execute_instruction_t32(CPU* cpu);
void cpu_execute_instruction_0F_t16(CPU* cpu);
void cpu_execute_instruction_0F_t32(CPU* cpu);

#endif