#ifndef WEBC_CPU_H
#define WEBC_CPU_H

#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdio.h>
#include "memmgr.h"
#include "collections.h"

#define PLATFORM_WINDOWS 1
#define PLATFORM_WEB 0
#define PLATFORM_NDS 0
#define PLATFORM_N3DS 0
#define PLATFORM_PSP 0
#define PLATFORM_PSVITA 0

#define CPU_WITH_DEBUG 1

typedef uint32_t CPU_SIZE_T;

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

#define MAX_MODULE_NAME_LEN 260
#define MAX_FUNC_NAME_LEN 4096
#define MAX_PATH_LEN 4096

// Helpers for manually implemented function imports
// Note: CPU_STACK_END is only required for stdcall / fastcall (to clean up the stack)
#define CPU_STACK_RETURN(cpu, x) cpu->Reg[REG_EAX] = (int32_t)x
#define CPU_STACK_RETURN_I64(cpu, x) cpu->Reg[REG_EAX] = (int32_t)x; cpu->Reg[REG_EDX] = (int32_t)(x >> 0x1F)
#define CPU_STACK_BEGIN(cpu) int32_t cpu##funcStackOffset = 0
#define CPU_STACK_END(cpu) cpu_adjust_stack_reg(cpu, cpu##funcStackOffset)
#define CPU_STACK_RESET(cpu) cpu##funcStackOffset = 0
#define CPU_STACK_OFFSET(cpu) cpu##funcStackOffset
#define CPU_STACK_GOTO(cpu, x) cpu##funcStackOffset = x
#define CPU_STACK_POP_PTR(cpu) (void*)cpu_get_real_address(cpu, cpu_readU32(cpu, cpu_get_esp(cpu, cpu##funcStackOffset))); cpu##funcStackOffset += 4
#define CPU_STACK_POP_PTR_CONST(cpu) (const void*)cpu_get_real_address(cpu, cpu_readU32(cpu, cpu_get_esp(cpu, cpu##funcStackOffset))); cpu##funcStackOffset += 4
#define CPU_STACK_POP_SIZE_T(cpu) (CPU_SIZE_T)cpu_readU32(cpu, cpu_get_esp(cpu, cpu##funcStackOffset)); cpu##funcStackOffset += 4
#define CPU_STACK_POP_U8(cpu) cpu_readU8(cpu, cpu_get_esp(cpu, cpu##funcStackOffset)); cpu##funcStackOffset += 1
#define CPU_STACK_POP_I8(cpu) cpu_readI8(cpu, cpu_get_esp(cpu, cpu##funcStackOffset)); cpu##funcStackOffset += 1
#define CPU_STACK_POP_U16(cpu) cpu_readU16(cpu, cpu_get_esp(cpu, cpu##funcStackOffset)); cpu##funcStackOffset += 2
#define CPU_STACK_POP_I16(cpu) cpu_readI16(cpu, cpu_get_esp(cpu, cpu##funcStackOffset)); cpu##funcStackOffset += 2
#define CPU_STACK_POP_U32(cpu) cpu_readU32(cpu, cpu_get_esp(cpu, cpu##funcStackOffset)); cpu##funcStackOffset += 4
#define CPU_STACK_POP_I32(cpu) cpu_readI32(cpu, cpu_get_esp(cpu, cpu##funcStackOffset)); cpu##funcStackOffset += 4
#define CPU_STACK_POP_U64(cpu) cpu_readU64(cpu, cpu_get_esp(cpu, cpu##funcStackOffset)); cpu##funcStackOffset += 8
#define CPU_STACK_POP_I64(cpu) cpu_readI64(cpu, cpu_get_esp(cpu, cpu##funcStackOffset)); cpu##funcStackOffset += 8
#define CPU_STACK_POP_F32(cpu) cpu_readF32(cpu, cpu_get_esp(cpu, cpu##funcStackOffset)); cpu##funcStackOffset += 4
#define CPU_STACK_POP_F64(cpu) cpu_readF64(cpu, cpu_get_esp(cpu, cpu##funcStackOffset)); cpu##funcStackOffset += 8
#define CPU_STACK_POP_STRUCT(cpu, target) for (int32_t popStructOffset = 0; popStructOffset < sizeof(*target); popStructOffset++) { ((uint8_t*)target)[popStructOffset] = cpu_readU8(cpu, cpu_get_esp(cpu, cpu##funcStackOffset)); cpu##funcStackOffset += 1; }

typedef struct tCPU CPU;

typedef void(*FuncImportCallbackSig)(CPU* cpu);

typedef struct
{
    const char* DllName;
    const char* Name;
    FuncImportCallbackSig Callback;
    // For data imports
    uint32_t DataSize;
    uint32_t DataAddress;
    uint32_t ThunkAddress;
} ImportInfo;

typedef map_t(ImportInfo*) map_ImportInfo_t;

typedef struct
{
    int32_t IsLoaded;
    uint32_t VirtualAddress;// The virtual address of the loaded module
    uint32_t VirtualAddressOfEntryPoint;// Virtual address of the first instruction to execute
    uint32_t ImageSize;// ntHeader.OptionalHeader.SizeOfImage
    char Name[MAX_MODULE_NAME_LEN];// Includes the dll extension
    char Path[MAX_PATH_LEN];// Full path of the dll on disk
} ModuleInfo;
typedef map_t(ModuleInfo*) map_ModuleInfo_t;

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

typedef struct
{
    // The following is copied from CPU. When switching 'threads', we swap these with CPU.
    // Note: This way of implementing threads means we can't use real threading.
    //       To use real threading things would need to be refactored to pass around CPUContext instead of CPU (and hold a CPU* reference here).
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
} CPUContext;

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

    ModuleInfo* MainModule;// Exe module
    map_ModuleInfo_t Modules;// NOTE: Modules may be in here but not loaded. Also includes the main exe module as well.
    
    // Map "DllName_FuncName" to target function address. This is used for functions in actual dlls (as opposed to imports defined manually in the emulator)
    map_uint32_t ModuleExportsMap;
    
    int32_t NumImports;
    map_ImportInfo_t ImportsByName;// Imports with hash map lookup ("DllName_FuncName" e.g. "msvcrt_fopen")
    ImportInfo* Imports;// Allocated into a single block of memory
    ImportInfo* UnhandledFunctionImport;// The fallback import handler
    uint32_t ImportsBeginAddress;// The address of where the imports start (virtul address)
    uint32_t ImportsEndAddress;// The address of where the imports end (virtul address) (ImportsBeginAddress + imports buffer size)
    map_str_t UnresolvedImportNames;// Address -> name
    
    HandleCollection FileHandles;// fopen / fclose
    HandleCollection DirHandles;// opendir / closedir
    HandleCollection SocketHandles;
    HandleCollection ThreadHandles;
    
    uint32_t AtExitFuncPtr;// For atexit()
    // These should be per thread... (and probably inside a struct)
    uint32_t Statics_tmpname;// char* size L_tmpnam
    uint32_t Statics_inet_ntoa;// char* size 16
    uint32_t Statics_streerror;// char* size varies
    uint32_t Statics_errno;
    
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
void cpu_allocate_data_imports(CPU* cpu);
ImportInfo* cpu_define_import_ex(CPU* cpu, int32_t* counter, const char* dllName, const char* name, FuncImportCallbackSig function, uint32_t dataSize);
ImportInfo* cpu_define_import(CPU* cpu, int32_t* counter, const char* dllName, const char* name, FuncImportCallbackSig function);
ImportInfo* cpu_define_data_import(CPU* cpu, int32_t* counter, const char* dllName, const char* name, uint32_t dataSize);
ImportInfo* cpu_find_import(CPU* cpu, const char* fullFuncName);
void import_unresolved(CPU* cpu);
void import_ignore(CPU* cpu);

CPU_SIZE_T cpu_loaddll(CPU* cpu, const char* fileName);

void cpu_init(CPU* cpu);
void cpu_init_state(CPU* cpu, uint32_t virtualAddress, uint32_t addressOfEntryPoint, uint32_t imageSize, uint32_t heapSize, uint32_t stackSize);
void cpu_destroy(CPU* cpu);
void cpu_onerror(CPU* cpu, char* error, ...);
#if CPU_WITH_DEBUG
void cpu_dbg_assert(CPU* cpu, int32_t cond, char* msg);
#else
#define cpu_dbg_assert(cpu, cond, msg)
#endif

void cpu_set_command_line_args(CPU* cpu, char** args, int nargs);
void cpu_add_command_line_arg(CPU* cpu, char* arg);
void cpu_add_command_line_args(CPU* cpu, char** args, int nargs);
int32_t cpu_get_command_line_arg_count(CPU* cpu);
char* cpu_get_command_line_arg(CPU* cpu, int32_t index, char* str, int strLen);

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
uint32_t cpu_get_virtual_address(CPU* cpu, const void* realAddress);
void* cpu_get_real_address(CPU* cpu, uint32_t virtualAddress);
void cpu_validate_virtual_address(CPU* cpu, uint32_t virtualAddress);
void cpu_validate_real_address(CPU* cpu, const void* realAddress);

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
float cpu_fetchF32(CPU* cpu);
double cpu_fetchF64(CPU* cpu);

int8_t cpu_readI8(CPU* cpu, uint32_t address);
int16_t cpu_readI16(CPU* cpu, uint32_t address);
int32_t cpu_readI32(CPU* cpu, uint32_t address);
int64_t cpu_readI64(CPU* cpu, uint32_t address);
uint8_t cpu_readU8(CPU* cpu, uint32_t address);
uint16_t cpu_readU16(CPU* cpu, uint32_t address);
uint32_t cpu_readU32(CPU* cpu, uint32_t address);
uint64_t cpu_readU64(CPU* cpu, uint32_t address);
float cpu_readF32(CPU* cpu, uint32_t address);
double cpu_readF64(CPU* cpu, uint32_t address);

void cpu_writeI8(CPU* cpu, uint32_t address, int8_t value);
void cpu_writeI16(CPU* cpu, uint32_t address, int16_t value);
void cpu_writeI32(CPU* cpu, uint32_t address, int32_t value);
void cpu_writeI64(CPU* cpu, uint32_t address, int64_t value);
void cpu_writeU8(CPU* cpu, uint32_t address, uint8_t value);
void cpu_writeU16(CPU* cpu, uint32_t address, uint16_t value);
void cpu_writeU32(CPU* cpu, uint32_t address, uint32_t value);
void cpu_writeU64(CPU* cpu, uint32_t address, uint64_t value);
void cpu_writeF32(CPU* cpu, uint32_t address, float value);
void cpu_writeF64(CPU* cpu, uint32_t address, double value);

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

int8_t cpu_add8(CPU* cpu, int8_t dest, int8_t src);
int16_t cpu_add16(CPU* cpu, int16_t dest, int16_t src);
int32_t cpu_add32(CPU* cpu, int32_t dest, int32_t src);
int8_t cpu_adc8(CPU* cpu, int8_t dest, int8_t src);
int16_t cpu_adc16(CPU* cpu, int16_t dest, int16_t src);
int32_t cpu_adc32(CPU* cpu, int32_t dest, int32_t src);
int8_t cpu_sub8(CPU* cpu, int8_t dest, int8_t src);
int16_t cpu_sub16(CPU* cpu, int16_t dest, int16_t src);
int32_t cpu_sub32(CPU* cpu, int32_t dest, int32_t src);
int8_t cpu_cmp8(CPU* cpu, int8_t dest, int8_t src);
int16_t cpu_cmp16(CPU* cpu, int16_t dest, int16_t src);
int32_t cpu_cmp32(CPU* cpu, int32_t dest, int32_t src);
int8_t cpu_sbb8(CPU* cpu, int8_t dest, int8_t src);
int16_t cpu_sbb16(CPU* cpu, int16_t dest, int16_t src);
int32_t cpu_sbb32(CPU* cpu, int32_t dest, int32_t src);
int32_t cpu_add(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t op_size);
int32_t cpu_adc(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t op_size);
int32_t cpu_sub(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t op_size);
int32_t cpu_sbb(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t op_size);
int8_t cpu_inc8(CPU* cpu, int8_t dest);
int16_t cpu_inc16(CPU* cpu, int16_t dest);
int32_t cpu_inc32(CPU* cpu, int32_t dest);
int8_t cpu_dec8(CPU* cpu, int8_t dest);
int16_t cpu_dec16(CPU* cpu, int16_t dest);
int32_t cpu_dec32(CPU* cpu, int32_t dest);
int32_t cpu_inc(CPU* cpu, int32_t dest_operand, int32_t op_size);
int32_t cpu_dec(CPU* cpu, int32_t dest_operand, int32_t op_size);
int8_t cpu_neg8(CPU* cpu, int8_t dest);
int16_t cpu_neg16(CPU* cpu, int16_t dest);
int32_t cpu_neg32(CPU* cpu, int32_t dest);
int32_t cpu_neg(CPU* cpu, int32_t dest_operand, int32_t op_size);
void cpu_mul8(CPU* cpu, uint8_t source_operand);
void cpu_imul8(CPU* cpu, int8_t source_operand);
void cpu_mul16(CPU* cpu, uint16_t source_operand);
void cpu_imul16(CPU* cpu, int16_t source_operand);
int16_t cpu_imul_reg16(CPU* cpu, int16_t operand1, int16_t operand2);
void cpu_mul32(CPU* cpu, uint32_t source_operand);
void cpu_imul32(CPU* cpu, int32_t source_operand);
int32_t cpu_imul_reg32(CPU* cpu, int32_t operand1, int32_t operand2);
void cpu_div8(CPU* cpu, uint8_t source_operand);
void cpu_idiv8(CPU* cpu, int8_t source_operand);
void cpu_div16(CPU* cpu, uint16_t source_operand);
void cpu_idiv16(CPU* cpu, int16_t source_operand);
void cpu_div32(CPU* cpu, uint32_t source_operand);
void cpu_idiv32(CPU* cpu, int32_t source_operand);
int8_t cpu_and8(CPU* cpu, int8_t dest, int8_t src);
int16_t cpu_and16(CPU* cpu, int16_t dest, int16_t src);
int32_t cpu_and32(CPU* cpu, int32_t dest, int32_t src);
int32_t cpu_test8(CPU* cpu, int8_t dest, int8_t src);
int32_t cpu_test16(CPU* cpu, int16_t dest, int16_t src);
int32_t cpu_test32(CPU* cpu, int32_t dest, int32_t src);
int8_t cpu_or8(CPU* cpu, int8_t dest, int8_t src);
int16_t cpu_or16(CPU* cpu, int16_t dest, int16_t src);
int32_t cpu_or32(CPU* cpu, int32_t dest, int32_t src);
int8_t cpu_xor8(CPU* cpu, int8_t dest, int8_t src);
int16_t cpu_xor16(CPU* cpu, int16_t dest, int16_t src);
int32_t cpu_xor32(CPU* cpu, int32_t dest, int32_t src);
int32_t cpu_and(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t op_size);
int32_t cpu_or(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t op_size);
int32_t cpu_xor(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t op_size);
int8_t cpu_rol8(CPU* cpu, int8_t dest_operand, int32_t count);
int16_t cpu_rol16(CPU* cpu, int16_t dest_operand, int32_t count);
int32_t cpu_rol32(CPU* cpu, int32_t dest_operand, int32_t count);
int8_t cpu_rcl8(CPU* cpu, int8_t dest_operand, int32_t count);
int16_t cpu_rcl16(CPU* cpu, int16_t dest_operand, int32_t count);
int32_t cpu_rcl32(CPU* cpu, int32_t dest_operand, int32_t count);
int8_t cpu_ror8(CPU* cpu, int8_t dest_operand, int32_t count);
int16_t cpu_ror16(CPU* cpu, int16_t dest_operand, int32_t count);
int32_t cpu_ror32(CPU* cpu, int32_t dest_operand, int32_t count);
int8_t cpu_rcr8(CPU* cpu, int8_t dest_operand, int32_t count);
int16_t cpu_rcr16(CPU* cpu, int16_t dest_operand, int32_t count);
int32_t cpu_rcr32(CPU* cpu, int32_t dest_operand, int32_t count);
uint8_t cpu_shl8(CPU* cpu, uint8_t dest_operand, int32_t count);
uint16_t cpu_shl16(CPU* cpu, uint16_t dest_operand, int32_t count);
uint32_t cpu_shl32(CPU* cpu, uint32_t dest_operand, int32_t count);
uint8_t cpu_shr8(CPU* cpu, uint8_t dest_operand, int32_t count);
uint16_t cpu_shr16(CPU* cpu, uint16_t dest_operand, int32_t count);
uint32_t cpu_shr32(CPU* cpu, uint32_t dest_operand, int32_t count);
int8_t cpu_sar8(CPU* cpu, int8_t dest_operand, int32_t count);
int16_t cpu_sar16(CPU* cpu, int16_t dest_operand, int32_t count);
int32_t cpu_sar32(CPU* cpu, int32_t dest_operand, int32_t count);
int16_t cpu_shrd16(CPU* cpu, int16_t dest_operand, int16_t source_operand, int32_t count);
int32_t cpu_shrd32(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t count);
int16_t cpu_shld16(CPU* cpu, int16_t dest_operand, int16_t source_operand, int32_t count);
int32_t cpu_shld32(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t count);
int64_t cpu_integer_round(CPU* cpu, long double f, int32_t rc);
void cpu_init_int_log2_table();
uint16_t cpu_bsr16(CPU* cpu, uint16_t old, uint16_t bit_base);
uint32_t cpu_bsr32(CPU* cpu, uint32_t old, uint32_t bit_base);

void fpu_init(CPU* cpu);
void fpu_push(CPU* cpu, long double x);
long double fpu_get_sti(CPU* cpu, int32_t i);
long double fpu_get_st0(CPU* cpu);
float fpu_load_m32(CPU* cpu, uint32_t addr);
int32_t fpu_op_D8_reg(CPU* cpu, uint8_t imm8);
int32_t fpu_op_D8_mem(CPU* cpu, uint8_t imm8, uint32_t addr);
int32_t fpu_op_D9_reg(CPU* cpu, uint8_t imm8);
int32_t fpu_op_D9_mem(CPU* cpu, uint8_t imm8, uint32_t addr);
int32_t fpu_op_DA_reg(CPU* cpu, uint8_t imm8);
int32_t fpu_op_DA_mem(CPU* cpu, uint8_t imm8, uint32_t addr);
int32_t fpu_op_DB_reg(CPU* cpu, uint8_t imm8);
int32_t fpu_op_DB_mem(CPU* cpu, uint8_t imm8, uint32_t addr);
int32_t fpu_op_DC_reg(CPU* cpu, uint8_t imm8);
int32_t fpu_op_DC_mem(CPU* cpu, uint8_t imm8, uint32_t addr);
int32_t fpu_op_DD_reg(CPU* cpu, uint8_t imm8);
int32_t fpu_op_DD_mem(CPU* cpu, uint8_t imm8, uint32_t addr);
int32_t fpu_op_DE_reg(CPU* cpu, uint8_t imm8);
int32_t fpu_op_DE_mem(CPU* cpu, uint8_t imm8, uint32_t addr);
int32_t fpu_op_DF_reg(CPU* cpu, uint8_t imm8);
int32_t fpu_op_DF_mem(CPU* cpu, uint8_t imm8, uint32_t addr);

#endif