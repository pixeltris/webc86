#include "cpu.h"
#include "memmgr.h"
#include <stdarg.h>

static int32_t cpu_first_init = 1;

void cpu_init(CPU* cpu)
{
    memset(cpu, 0, sizeof(*cpu));
    
    handles_init(&cpu->FileHandles);
    handles_init(&cpu->DirHandles);
    handles_init(&cpu->SocketHandles);
    handles_init(&cpu->ThreadHandles);
}

void cpu_init_state(CPU* cpu, uint32_t virtualAddress, uint32_t addressOfEntryPoint, uint32_t imageSize, uint32_t heapSize, uint32_t stackSize)
{
    if (cpu_first_init)
    {
        cpu_first_init = 0;
        cpu_init_int_log2_table();
    }
    
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
    
    cpu->VirtualMemoryStackAddress = cpu_get_virtual_address(cpu, stackPtr);
    cpu->VirtualMemoryStackEndAddress = cpu->VirtualMemoryStackAddress + cpu->VirtualMemoryStackSize;
    
    cpu->VirtualMemorySize = cpu->VirtualMemoryStackSize + cpu->VirtualMemoryHeapSize;
    if (cpu->VirtualMemorySize != cpu->Memory.PoolSizeInBytes)
    {
        cpu_onerror(cpu, "Bad cpu memory size. Expected:%d actual:%d.\n", cpu->VirtualMemorySize, cpu->Memory.PoolSizeInBytes);
        return;
    }
    
    cpu->EIP = cpu->VirtualEntryPointAddress;
    cpu->Reg[REG_ESP] = cpu->VirtualMemoryStackEndAddress;
    
    fpu_init(cpu);
    
    cpu_allocate_data_imports(cpu);
}

void cpu_destroy(CPU* cpu)
{
    memmgr_destroy(&cpu->Memory);
    map_deinit(&cpu->ModuleExportsMap);
    map_deinit(&cpu->ImportsByName);
    map_free_values_and_deinit(&cpu->Modules);// Free loaded module infos memory
    map_deinit(&cpu->UnresolvedImportNames);
    
    handles_destroy(&cpu->FileHandles);
    handles_destroy(&cpu->DirHandles);
    handles_destroy(&cpu->SocketHandles);
    handles_destroy(&cpu->ThreadHandles);
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
            cpu_onerror(cpu, "%s\n", msg);
        }
        else
        {
            cpu_onerror(cpu, NULL);
        }
    }
}
#endif

void cpu_print_callstack(CPU* cpu, int32_t maxFrames)
{
    uint32_t ebp = (uint32_t)cpu->Reg[REG_EBP];
    printf("callstack: ");
    for (int32_t i = 0; i < maxFrames; i++)
    {
        uint32_t eip = cpu_readU32(cpu, ebp + 4);
        if (!cpu_is_valid_address(cpu, eip, 1))
        {
            break;
        }
        ebp = cpu_readU32(cpu, ebp);
        
        if (i != 0)
        {
            printf(", ");
        }
        printf("0x%08X", eip);
    }
    printf("\n");
}

void cpu_check_stack_memory(CPU* cpu)
{
    uint32_t esp = (uint32_t)cpu->Reg[REG_ESP];
    if (esp < cpu->VirtualMemoryStackAddress)
    {
        uint32_t overflowAmount = (uint32_t)(cpu->VirtualMemoryStackAddress - esp);
        cpu_onerror(cpu, "Emulator encountered a stack overflow (%u byte(s))\n", overflowAmount);
    }
    else if (esp >= cpu->VirtualMemoryStackEndAddress)
    {
        uint32_t underflowAmount = (uint32_t)(esp - cpu->VirtualMemoryStackEndAddress);
        cpu_onerror(cpu, "Emulator encountered a stack underflow (%u byte(s))\n", underflowAmount);
    }
}

void cpu_exec_call(CPU* cpu, uint32_t addr)
{
    if (addr >= cpu->VirtualMemoryAddress && addr < cpu->VirtualMemoryEndAddress)
    {
        if (addr < cpu->ImportsEndAddress && addr >= cpu->ImportsBeginAddress)
        {
            // Function import call
            ImportInfo* import = (ImportInfo*)cpu_get_real_address(cpu, addr);
            import->Callback(cpu);
            return;
        }
        else
        {
            cpu_push32(cpu, cpu->EIP);
            cpu->EIP = addr;
            return;
        }
    }
    cpu_onerror(cpu, "exec_call invalid function address 0x%08X EIP: 0x%08X\n", addr, cpu->EIP);
}

int32_t cpu_exec_check_jump_function(CPU* cpu, uint32_t addr)
{
    if (addr >= cpu->VirtualMemoryAddress && addr < cpu->VirtualMemoryEndAddress)
    {
        if (addr < cpu->ImportsEndAddress && addr >= cpu->ImportsBeginAddress)
        {
            // Get the return address into eip
            cpu->EIP = cpu_pop32(cpu);
            
            // Function import call
            ImportInfo* import = (ImportInfo*)cpu_get_real_address(cpu, addr);
            import->Callback(cpu);
            return 1;
        }
        else
        {
            return 0;
        }
    }
    cpu_onerror(cpu, "cpu_exec_check_jump_function invalid function address 0x%08X EIP: 0x%08X\n", addr, cpu->EIP);
    return 0;
}

uint32_t cpu_get_stack_reg(CPU* cpu)
{
    return (uint32_t)cpu->Reg[REG_ESP];
}

void cpu_set_stack_reg(CPU* cpu, uint32_t value)
{
    cpu->Reg[REG_ESP] = (int32_t)value;
}

void cpu_adjust_stack_reg(CPU* cpu, int32_t value)
{
    cpu->Reg[REG_ESP] += value;
}

uint32_t cpu_get_esp(CPU* cpu, int32_t mod)
{
    return (uint32_t)cpu->Reg[REG_ESP] + mod;
}

void cpu_set_esp(CPU* cpu, uint32_t value)
{
    cpu->Reg[REG_ESP] = (int32_t)value;
}

uint32_t cpu_get_eip(CPU* cpu)
{
    return cpu->EIP;
}

void cpu_set_eip(CPU* cpu, uint32_t addr)
{
    cpu->EIP = addr;
}

int32_t cpu_is_osize_32(CPU* cpu)
{
    return !((cpu->Prefixes & PREFIX_MASK_OPSIZE) == PREFIX_MASK_OPSIZE);
}

int32_t cpu_is_asize_32(CPU* cpu)
{
    return !((cpu->Prefixes & PREFIX_MASK_ADDRSIZE) == PREFIX_MASK_ADDRSIZE);
}

uint32_t cpu_get_seg_prefix_ds(CPU* cpu)
{
    return cpu_get_seg_prefix(cpu, REG_DS);
}

uint32_t cpu_get_seg_prefix_ss(CPU* cpu)
{
    return cpu_get_seg_prefix(cpu, REG_SS);
}

uint32_t cpu_get_seg_prefix_cs(CPU* cpu)
{
    return cpu_get_seg_prefix(cpu, REG_CS);
}

uint32_t cpu_get_seg_prefix(CPU* cpu, int32_t seg)
{
    return 0;
}

uint32_t cpu_get_seg(CPU* cpu, int32_t seg)
{
    return 0;
}

uint8_t cpu_get_reg8(CPU* cpu, int32_t index)
{
    if (index >= 4)
    {
        // AH, CH, DH, BH
        return (uint8_t)((cpu->Reg[index - 4] >> 8) & SIZE_MASK_8);
    }
    return (uint8_t)(cpu->Reg[index] & SIZE_MASK_8);
}

void cpu_set_reg8(CPU* cpu, int32_t index, uint8_t value)
{
    if (index >= 4)
    {
        // AH, CH, DH, BH
        cpu->Reg[index - 4] &= ~0x0000FF00;// H_MASK
        cpu->Reg[index - 4] |= (((int32_t)value >> 8) & SIZE_MASK_8);
    }
    else
    {
        cpu->Reg[index] &= ~0x000000FF;
        cpu->Reg[index] |= (value & SIZE_MASK_8);
    }
}

uint8_t cpu_getReg8(CPU* cpu, int32_t reg)
{
    return (uint8_t)cpu->Reg[reg];
}

int8_t cpu_getReg8s(CPU* cpu, int32_t reg)
{
    return (int8_t)cpu->Reg[reg];
}

uint16_t cpu_getReg16(CPU* cpu, int32_t reg)
{
    return (uint16_t)cpu->Reg[reg];
}

int16_t cpu_getReg16s(CPU* cpu, int32_t reg)
{
    return (int16_t)cpu->Reg[reg];
}

uint32_t cpu_getReg32(CPU* cpu, int32_t reg)
{
    return (uint32_t)cpu->Reg[reg];
}

int32_t cpu_getReg32s(CPU* cpu, int32_t reg)
{
    return (int32_t)cpu->Reg[reg];
}

void cpu_setReg8(CPU* cpu, int32_t reg, uint8_t val)
{
    cpu->Reg[reg] = val;
}

void cpu_setReg8s(CPU* cpu, int32_t reg, int8_t val)
{
    cpu->Reg[reg] = val;
}

void cpu_setReg16(CPU* cpu, int32_t reg, uint16_t val)
{
    cpu->Reg[reg] = val;
}

void cpu_setReg16s(CPU* cpu, int32_t reg, int16_t val)
{
    cpu->Reg[reg] = val;
}

void cpu_setReg32(CPU* cpu, int32_t reg, uint32_t val)
{
    cpu->Reg[reg] = (int32_t)val;
}

void cpu_setReg32s(CPU* cpu, int32_t reg, int32_t val)
{
    cpu->Reg[reg] = val;
}

void cpu_push16(CPU* cpu, uint16_t val)
{
    uint32_t sp = cpu_get_esp(cpu, -2);
    
    cpu_writeU16(cpu, sp, val);
    cpu_adjust_stack_reg(cpu, -2);
}

void cpu_push32(CPU* cpu, uint32_t val)
{
    uint32_t sp = cpu_get_esp(cpu, -4);
    
    cpu_writeU32(cpu, sp, val);
    cpu_adjust_stack_reg(cpu, -4);
}

uint16_t cpu_pop16(CPU* cpu)
{
    uint32_t sp = cpu_get_stack_reg(cpu);
    uint16_t result = cpu_readU16(cpu, sp);
    
    cpu_adjust_stack_reg(cpu, 2);
    return result;
}

int32_t cpu_pop32s(CPU* cpu)
{
    uint32_t sp = cpu_get_stack_reg(cpu);
    int32_t result = cpu_readI32(cpu, sp);
    
    cpu_adjust_stack_reg(cpu, 4);
    return result;
}

uint32_t cpu_pop32(CPU* cpu)
{
    return (uint32_t)cpu_pop32s(cpu);
}

void cpu_trigger_de(CPU* cpu)
{
    cpu_onerror(cpu, "trigger_de\n");
}

void cpu_trigger_ud(CPU* cpu)
{
    cpu_onerror(cpu, "trigger_ud\n");
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

int32_t cpu_is_valid_address(CPU* cpu, uint32_t addr, int32_t size)
{
    return addr >= cpu->VirtualMemoryAddress && addr < cpu->VirtualMemoryEndAddress;
}

uint32_t cpu_get_virtual_address(CPU* cpu, const void* realAddress)
{
    return memmgr_get_virtual_address(&cpu->Memory, (size_t)realAddress);
}

void* cpu_get_real_address(CPU* cpu, uint32_t virtualAddress)
{
    return memmgr_get_real_address(&cpu->Memory, virtualAddress);
}

void cpu_validate_virtual_address(CPU* cpu, uint32_t virtualAddress)
{
    if (virtualAddress && (virtualAddress < cpu->VirtualMemoryAddress || virtualAddress >= cpu->VirtualMemoryEndAddress))
    {
        cpu_onerror(cpu, "cpu_validate_virtual_address failed Address:0x%08X MemStart:0x%08X MemEnd:0x%08X\n", virtualAddress, cpu->VirtualMemory, cpu->VirtualMemoryEndAddress);
    }
}

void cpu_validate_real_address(CPU* cpu, const void* realAddress)
{
    cpu_validate_virtual_address(cpu, memmgr_get_virtual_address(&cpu->Memory, (size_t)realAddress));
}

uint8_t cpu_read_op0F(CPU* cpu)
{
    return cpu_fetchU8(cpu);
}

uint8_t cpu_read_sib(CPU* cpu)
{
    return cpu_fetchU8(cpu);
}

uint8_t cpu_read_op8(CPU* cpu)
{
    return cpu_fetchU8(cpu);
}

int8_t cpu_read_op8s(CPU* cpu)
{
    return cpu_fetchI8(cpu);
}

uint16_t cpu_read_op16(CPU* cpu)
{
    return cpu_fetchU16(cpu);
}

int16_t cpu_read_op16s(CPU* cpu)
{
    return cpu_fetchI16(cpu);
}

int32_t cpu_read_op32s(CPU* cpu)
{
    return cpu_fetchI32(cpu);
}

uint8_t cpu_read_disp8(CPU* cpu)
{
    return cpu_fetchU8(cpu);
}

int8_t cpu_read_disp8s(CPU* cpu)
{
    return cpu_fetchI8(cpu);
}

uint16_t cpu_read_disp16(CPU* cpu)
{
    return cpu_fetchU16(cpu);
}

int32_t cpu_read_disp32s(CPU* cpu)
{
    return cpu_fetchI32(cpu);
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

float cpu_fetchF32(CPU* cpu)
{
    float value = cpu_readF32(cpu, cpu->EIP);
    cpu->EIP += 4;
    return value;
}

double cpu_fetchF64(CPU* cpu)
{
    float value = cpu_readF64(cpu, cpu->EIP);
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

float cpu_readF32(CPU* cpu, uint32_t address)
{
    float* ptr = (float*)cpu_get_real_address(cpu, address);
    return *ptr;
}

double cpu_readF64(CPU* cpu, uint32_t address)
{
    double* ptr = (double*)cpu_get_real_address(cpu, address);
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

void cpu_writeF32(CPU* cpu, uint32_t address, float value)
{
    float* ptr = (float*)cpu_get_real_address(cpu, address);
    *ptr = value;
}

void cpu_writeF64(CPU* cpu, uint32_t address, double value)
{
    double* ptr = (double*)cpu_get_real_address(cpu, address);
    *ptr = value;
}

int32_t cpu_read_moffs(CPU* cpu)
{
    if (cpu_is_asize_32(cpu))
    {
        return ((int32_t)cpu_get_seg_prefix(cpu, REG_DS)) + cpu_fetchI32(cpu);
    }
    else
    {
        return ((int32_t)cpu_get_seg_prefix(cpu, REG_DS)) + cpu_fetchI16(cpu);
    }
}

uint8_t cpu_read_e8(CPU* cpu)
{
    if (cpu->ModRM < 0xC0)
    {
        return cpu_readU8(cpu, cpu_modrm_resolve(cpu, cpu->ModRM));
    }
    else
    {
        return cpu_get_reg8(cpu, cpu->ModRM & 7);
    }
}

int8_t cpu_read_e8s(CPU* cpu)
{
    return (int8_t)cpu_read_e8(cpu);
}

uint16_t cpu_read_e16(CPU* cpu)
{
    if (cpu->ModRM < 0xC0)
    {
        return cpu_readU16(cpu, cpu_modrm_resolve(cpu, cpu->ModRM));
    }
    else
    {
        return (uint16_t)(cpu->Reg[cpu->ModRM & 7] & SIZE_MASK_16);
    }
}

int16_t cpu_read_e16s(CPU* cpu)
{
    return (int16_t)cpu_read_e16(cpu);
}

uint32_t cpu_read_e32(CPU* cpu)
{
    if (cpu->ModRM < 0xC0)
    {
        return cpu_readU32(cpu, cpu_modrm_resolve(cpu, cpu->ModRM));
    }
    else
    {
        return (uint32_t)cpu->Reg[cpu->ModRM & 7];
    }
}

int32_t cpu_read_e32s(CPU* cpu)
{
    return (int32_t)cpu_read_e32(cpu);
}

void cpu_set_e8(CPU* cpu, uint8_t value)
{
    if (cpu->ModRM < 0xC0)
    {
        uint32_t addr = (uint32_t)cpu_modrm_resolve(cpu, cpu->ModRM);
        cpu_writeU8(cpu, addr, value);
    }
    else
    {
        cpu_set_reg8(cpu, cpu->ModRM & 7, value);
    }
}

void cpu_set_e16(CPU* cpu, uint16_t value)
{
    if(cpu->ModRM < 0xC0)
    {
        uint32_t addr = (uint32_t)cpu_modrm_resolve(cpu, cpu->ModRM);
        cpu_writeU16(cpu, addr, value);
    }
    else
    {
        cpu->Reg[cpu->ModRM & 7] = value;
    }
}

void cpu_set_e32(CPU* cpu, uint32_t value)
{
    if(cpu->ModRM < 0xC0)
    {
        uint32_t addr = (uint32_t)cpu_modrm_resolve(cpu, cpu->ModRM);
        cpu_writeU32(cpu, addr, value);
    }
    else
    {
        cpu->Reg[cpu->ModRM & 7] = (int32_t)value;
    }
}

uint8_t cpu_read_write_e8(CPU* cpu)
{
    if(cpu->ModRM < 0xC0)
    {
        uint32_t addr = (uint32_t)cpu_modrm_resolve(cpu, cpu->ModRM);
        cpu->TempAddr = addr;
        return cpu_readU8(cpu, addr);
    }
    else
    {
        return cpu_get_reg8(cpu, cpu->ModRM & 7);
    }
}

void cpu_write_e8(CPU* cpu, uint8_t value)
{
    if(cpu->ModRM < 0xC0)
    {
        cpu_writeU8(cpu, cpu->TempAddr, value);
    }
    else
    {
        cpu_set_reg8(cpu, cpu->ModRM & 7, value);
    }
}

uint16_t cpu_read_write_e16(CPU* cpu)
{
    if(cpu->ModRM < 0xC0)
    {
        uint32_t addr = (uint32_t)cpu_modrm_resolve(cpu, cpu->ModRM);
        cpu->TempAddr = addr;
        return cpu_readU16(cpu, addr);
    }
    else
    {
        return (uint16_t)(cpu->Reg[cpu->ModRM & 7] & SIZE_MASK_16);
    }
}

void cpu_write_e16(CPU* cpu, uint16_t value)
{
    if(cpu->ModRM < 0xC0)
    {
        cpu_writeU16(cpu, cpu->TempAddr, value);
    }
    else
    {
        cpu->Reg[cpu->ModRM & 7] = value;
    }
}

uint32_t cpu_read_write_e32(CPU* cpu)
{
    if(cpu->ModRM < 0xC0)
    {
        uint32_t addr = (uint32_t)cpu_modrm_resolve(cpu, cpu->ModRM);
        cpu->TempAddr = addr;
        return cpu_readU32(cpu, addr);
    }
    else
    {
        return (uint32_t)cpu->Reg[cpu->ModRM & 7];
    }
}

void cpu_write_e32(CPU* cpu, uint32_t value)
{
    if(cpu->ModRM < 0xC0)
    {
        cpu_writeU32(cpu, cpu->TempAddr, value);
    }
    else
    {
        cpu->Reg[cpu->ModRM & 7] = (int32_t)value;
    }
}

uint8_t cpu_read_g8(CPU* cpu)
{
    return cpu_get_reg8(cpu, cpu->ModRM >> 3 & 7);
}

void cpu_write_g8(CPU* cpu, uint8_t value)
{
    cpu_set_reg8(cpu, cpu->ModRM >> 3 & 7, value);
}

uint16_t cpu_read_g16(CPU* cpu)
{
    return (uint16_t)(cpu->Reg[cpu->ModRM >> 3 & 7] & SIZE_MASK_16);
}

int16_t cpu_read_g16s(CPU* cpu)
{
    return (int16_t)(cpu->Reg[cpu->ModRM >> 3 & 7] & SIZE_MASK_16);
}

void cpu_write_g16(CPU* cpu, uint16_t value)
{
    cpu->Reg[cpu->ModRM >> 3 & 7] = value;
}

int32_t cpu_read_g32s(CPU* cpu)
{
    return cpu->Reg[cpu->ModRM >> 3 & 7];
}

void cpu_write_g32(CPU* cpu, uint32_t value)
{
    cpu->Reg[cpu->ModRM >> 3 & 7] = (int32_t)value;
}

void cpu_jmpcc8(CPU* cpu, int32_t condition)
{
    int8_t val = cpu_fetchI8(cpu);
    if (condition)
    {
        cpu->EIP += val;
        //cpu_branch_taken(cpu);
    }
    else
    {
        //cpu_branch_not_taken(cpu);
    }
}

void cpu_jmp_rel16(CPU* cpu, int16_t rel16)
{
    cpu->EIP += rel16;
}

void cpu_jmpcc16(CPU* cpu, int32_t condition)
{
    uint16_t imm16 = cpu_fetchU16(cpu);
    if (condition)
    {
        cpu_jmp_rel16(cpu, imm16);
        //cpu_branch_taken(cpu);
    }
    else
    {
        //cpu_branch_not_taken(cpu);
    }
}

void cpu_jmpcc32(CPU* cpu, int32_t condition)
{
    int32_t imm32s = cpu_fetchI32(cpu);
    if (condition)
    {
        // don't change to `this.mem_eip += this.read_op32s()`,
        //   since read_op32s modifies mem_eip
        
        cpu->EIP += imm32s;
        //cpu_branch_taken(cpu);
    }
    else
    {
        //cpu_branch_not_taken(cpu);
    }
}

void cpu_setcc(CPU* cpu, int32_t condition)
{
    cpu_set_e8(cpu, condition);
}

int32_t cpu_getcf(CPU* cpu)
{
    if (cpu->FlagsChanged & 1)
    {
        return ((uint32_t)(cpu->LastOp1 ^ (cpu->LastOp1 ^ cpu->LastOp2) & (cpu->LastOp2 ^ cpu->LastAddResult))) >> cpu->LastOpSize & 1;
    }
    else
    {
        return cpu->Flags & 1;
    }
}

int32_t cpu_getpf(CPU* cpu)
{
    if (cpu->FlagsChanged & FLAG_PARITY)
    {
        // inverted lookup table
        return 0x9669 << 2 >> ((cpu->LastResult ^ cpu->LastResult >> 4) & 0xF) & FLAG_PARITY;
    }
    else
    {
        return cpu->Flags & FLAG_PARITY;
    }
}

int32_t cpu_getaf(CPU* cpu)
{
    if (cpu->FlagsChanged & FLAG_ADJUST)
    {
        return (cpu->LastOp1 ^ cpu->LastOp2 ^ cpu->LastAddResult) & FLAG_ADJUST;
    }
    else
    {
        return cpu->Flags & FLAG_ADJUST;
    }
}

int32_t cpu_getzf(CPU* cpu)
{
    if (cpu->FlagsChanged & FLAG_ZERO)
    {
        return (~cpu->LastResult & cpu->LastResult - 1) >> cpu->LastOpSize & 1;
    }
    else
    {
        return cpu->Flags & FLAG_ZERO;
    }
}

int32_t cpu_getsf(CPU* cpu)
{
    if (cpu->FlagsChanged & FLAG_SIGN)
    {
        return cpu->LastResult >> cpu->LastOpSize & 1;
    }
    else
    {
        return cpu->Flags & FLAG_SIGN;
    }
}

int32_t cpu_getof(CPU* cpu)
{
    if (cpu->FlagsChanged & FLAG_OVERFLOW)
    {
        return ((cpu->LastOp1 ^ cpu->LastAddResult) & (cpu->LastOp2 ^ cpu->LastAddResult)) >> cpu->LastOpSize & 1;
    }
    else
    {
        return cpu->Flags & FLAG_OVERFLOW;
    }
}

int32_t cpu_test_o(CPU* cpu)
{
    return cpu_getof(cpu);
}

int32_t cpu_test_b(CPU* cpu)
{
    return cpu_getcf(cpu);
}

int32_t cpu_test_z(CPU* cpu)
{
    return cpu_getzf(cpu);
}

int32_t cpu_test_s(CPU* cpu)
{
    return cpu_getsf(cpu);
}

int32_t cpu_test_p(CPU* cpu)
{
    return cpu_getpf(cpu);
}

int32_t cpu_test_be(CPU* cpu)
{
    // Idea:
    //    return this.last_op1 <= this.last_op2;
    return cpu_getcf(cpu) || cpu_getzf(cpu);
}

int32_t cpu_test_l(CPU* cpu)
{
    // Idea:
    //    return this.last_add_result < this.last_op2;
    return !cpu_getsf(cpu) != !cpu_getof(cpu);
}

int32_t cpu_test_le(CPU* cpu)
{
    // Idea:
    //    return this.last_add_result <= this.last_op2;
    return cpu_getzf(cpu) || !cpu_getsf(cpu) != !cpu_getof(cpu);
}

uint16_t cpu_xchg16(CPU* cpu, uint16_t memory_data, uint8_t modrm_byte)
{
    int32_t mod = modrm_byte >> 3 & 7;
    uint16_t tmp = (uint16_t)(cpu->Reg[mod] & SIZE_MASK_16);
    
    cpu->Reg[mod] = memory_data;
    
    return tmp;
}

uint32_t cpu_xchg32(CPU* cpu, uint32_t memory_data, uint8_t modrm_byte)
{
    int32_t mod = modrm_byte >> 3 & 7;
    uint32_t tmp = (uint32_t)cpu->Reg[mod];
    
    cpu->Reg[mod] = (int32_t)memory_data;
    
    return tmp;
}