#include "cpu.h"
#include <math.h>

#define FPU_LOG_OP 0
#define FPU_INDEFINITE_NAN NAN

#define FPU_C0 0x100
#define FPU_C1 0x200
#define FPU_C2 0x400
#define FPU_C3 0x4000
#define FPU_RESULT_FLAGS (FPU_C0 | FPU_C1 | FPU_C2 | FPU_C3)
#define FPU_STACK_TOP 0x3800

#define FPU_PC (3 << 8)
#define FPU_RC (3 << 10)
#define FPU_IF (1 << 12)
    
#define FPU_EX_SF (1 << 6)
#define FPU_EX_P (1 << 5)
#define FPU_EX_U (1 << 4)
#define FPU_EX_O (1 << 3)
#define FPU_EX_Z (1 << 2)
#define FPU_EX_D (1 << 1)
#define FPU_EX_I (1 << 0)

#define TWO_POW_63 0x8000000000000000ULL

void fpu_init(CPU* cpu)
{
    cpu->Fpu.StackEmpty = 0xFF;
    cpu->Fpu.ControlWord = 0x37F;
}

int32_t fpu_load_status_word(CPU* cpu)
{
    return cpu->Fpu.StatusWord & ~(7 << 11) | cpu->Fpu.StackPtr;
}

void fpu_stack_fault(CPU* cpu)
{
    cpu->Fpu.StatusWord |= FPU_EX_SF | FPU_EX_I;
    cpu_onerror(cpu, "fpu_stack_fault eip: %08X\n", cpu->EIP);
}

void fpu_invalid_arithmatic(CPU* cpu)
{
    cpu->Fpu.StatusWord |= FPU_EX_I;
}

int64_t fpu_integer_round(CPU* cpu, long double f)
{
    int32_t rc = cpu->Fpu.ControlWord >> 10 & 3;
    return cpu_integer_round(cpu, f, rc);
}

void fpu_push(CPU* cpu, long double x)
{
    cpu->Fpu.StackPtr = cpu->Fpu.StackPtr - 1 & 7;
    
    if (cpu->Fpu.StackEmpty >> cpu->Fpu.StackPtr & 1)
    {
        cpu->Fpu.StatusWord &= ~FPU_C1;
        cpu->Fpu.StackEmpty &= ~(1 << cpu->Fpu.StackPtr);
        cpu->Fpu.St[cpu->Fpu.StackPtr].fp = x;
    }
    else
    {
        cpu->Fpu.StatusWord |= FPU_C1;
        fpu_stack_fault(cpu);
        cpu->Fpu.St[cpu->Fpu.StackPtr].fp = FPU_INDEFINITE_NAN;
    }
}

void fpu_pop(CPU* cpu)
{
    cpu->Fpu.StackEmpty |= 1 << cpu->Fpu.StackPtr;
    cpu->Fpu.StackPtr = cpu->Fpu.StackPtr + 1 & 7;
}

long double fpu_get_sti(CPU* cpu, int32_t i)
{
    i = i + cpu->Fpu.StackPtr & 7;
    
    if (cpu->Fpu.StackEmpty >> i & 1)
    {
        cpu->Fpu.StatusWord &= ~FPU_C1;
        fpu_stack_fault(cpu);
        return FPU_INDEFINITE_NAN;
    }
    else
    {
        return cpu->Fpu.St[i].fp;
    }
}

long double fpu_get_st0(CPU* cpu)
{
    if (cpu->Fpu.StackEmpty >> cpu->Fpu.StackPtr & 1)
    {
        cpu->Fpu.StatusWord &= ~FPU_C1;
        fpu_stack_fault(cpu);
        return FPU_INDEFINITE_NAN;
    }
    else
    {
        return cpu->Fpu.St[cpu->Fpu.StackPtr].fp;
    }
}

long double fpu_load_m80(CPU* cpu, uint32_t addr)
{
    FPURegister* m80 = (FPURegister*)cpu_get_real_address(cpu, addr);
    return m80->fp;
}

void fpu_store_m80(CPU* cpu, uint32_t addr, long double n)
{
    FPURegister* m80 = (FPURegister*)cpu_get_real_address(cpu, addr);
    m80->fp = n;
}

double fpu_load_m64(CPU* cpu, uint32_t addr)
{
    int64_t m64 = cpu_readI64(cpu, addr);
    cpu->Fpu.Float64 = *(double*)&m64;
    return cpu->Fpu.Float64;
}

void fpu_store_m64(CPU* cpu, uint32_t addr, double i)
{
    cpu->Fpu.Float64 = fpu_get_sti(cpu, i);
    cpu_writeI64(cpu, addr, *(int64_t*)&cpu->Fpu.Float64);
}

void fpu_fcom(CPU* cpu, long double y)
{
    long double x = fpu_get_st0(cpu);
    
    cpu->Fpu.StatusWord &= ~FPU_RESULT_FLAGS;
    
    if (x > y)
    {
    }
    else if (y > x)
    {
        cpu->Fpu.StatusWord |= FPU_C0;
    }
    else if (x == y)
    {
        cpu->Fpu.StatusWord |= FPU_C3;
    }
    else
    {
        cpu->Fpu.StatusWord |= (FPU_C0 | FPU_C2 | FPU_C3);
    }
}

void fpu_fucom(CPU* cpu, long double y)
{
    // TODO
    fpu_fcom(cpu, y);
}

void fpu_fcomi(CPU* cpu, long double y)
{
    long double x = cpu->Fpu.St[cpu->Fpu.StackPtr].fp;
    
    cpu->FlagsChanged &= ~(1 | FLAG_PARITY | FLAG_ZERO);
    cpu->Flags &= ~(1 | FLAG_PARITY | FLAG_ZERO);
    if (x > y)
    {
    }
    else if (y > x)
    {
        cpu->Flags |= 1;
    }
    else if(x == y)
    {
        cpu->Flags |= FLAG_ZERO;
    }
    else
    {
        cpu->Flags |= 1 | FLAG_PARITY | FLAG_ZERO;
    }
}

void fpu_fucomi(CPU* cpu, long double y)
{
    // TODO
    fpu_fcomi(cpu, y);
}

float fpu_load_m32(CPU* cpu, uint32_t addr)
{
    *((int32_t*)&cpu->Fpu.Float32) = cpu_readI32(cpu, addr);
    return cpu->Fpu.Float32;
}

void fpu_store_m32(CPU* cpu, uint32_t addr, float x)
{
    cpu->Fpu.Float32 = x;
    cpu_writeI32(cpu, addr, *((int32_t*)&cpu->Fpu.Float32));
}

int32_t fpu_op_D8_reg(CPU* cpu, uint8_t imm8)
{
    int32_t mod = imm8 >> 3 & 7;
    long double sti = fpu_get_sti(cpu, imm8 & 7);
    long double st0 = fpu_get_st0(cpu);
    
    switch (mod)
    {
        case 0:
            // fadd
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 + sti;
            break;
        case 1:
            // fmul
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 * sti;
            break;
        case 2:
            // fcom
            fpu_fcom(cpu, sti);
            break;
        case 3:
            // fcomp
            fpu_fcom(cpu, sti);
            fpu_pop(cpu);
            break;
        case 4:
            // fsub
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 - sti;
            break;
        case 5:
            // fsubr
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = sti - st0;
            break;
        case 6:
            // fdiv
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 / sti;
            break;
        case 7:
            // fdivr
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = sti / st0;
            break;
        default:
            return 0;
    }
    return 1;
}

int32_t fpu_op_D8_mem(CPU* cpu, uint8_t imm8, uint32_t addr)
{
    int32_t mod = imm8 >> 3 & 7;
    float m32 = fpu_load_m32(cpu, addr);
    
    long double st0 = fpu_get_st0(cpu);
    
    switch (mod)
    {
        case 0:
            // fadd
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 + m32;
            break;
        case 1:
            // fmul
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 * m32;
            break;
        case 2:
            // fcom
            fpu_fcom(cpu, m32);
            break;
        case 3:
            // fcomp
            fpu_fcom(cpu, m32);
            fpu_pop(cpu);
            break;
        case 4:
            // fsub
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 - m32;
            break;
        case 5:
            // fsubr
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = m32 - st0;
            break;
        case 6:
            // fdiv
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 / m32;
            break;
        case 7:
            // fdivr
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = m32 / st0;
            break;
        default:
            return 0;
    }
    return 1;
}

int32_t fpu_op_D9_reg(CPU* cpu, uint8_t imm8)
{
    int32_t mod = imm8 >> 3 & 7;
    int32_t low = imm8 & 7;
    long double sti;
    
    switch (mod)
    {
        case 0:
            // fld
            sti = fpu_get_sti(cpu, low);
            fpu_push(cpu, sti);
            break;
        case 1:
            // fxch
            sti = fpu_get_sti(cpu, low);
            
            cpu->Fpu.St[cpu->Fpu.StackPtr + low & 7].fp = fpu_get_st0(cpu);
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = sti;
            break;
        default:
            return 0;
    }
    return 1;
}

int32_t fpu_op_D9_mem(CPU* cpu, uint8_t imm8, uint32_t addr)
{
    int32_t mod = imm8 >> 3 & 7;
    float m32;
    
    switch (mod)
    {
        case 0:
            // fld
            m32 = fpu_load_m32(cpu, addr);
            fpu_push(cpu, m32);
            break;
        case 2:
            // fst
            fpu_store_m32(cpu, addr, (float)fpu_get_st0(cpu));
            break;
        case 3:
            // fstp
            fpu_store_m32(cpu, addr, (float)fpu_get_st0(cpu));
            fpu_pop(cpu);
            break;
        default:
            return 0;
    }
    return 1;
}

int32_t fpu_op_DA_reg(CPU* cpu, uint8_t imm8)
{
    int32_t mod = imm8 >> 3 & 7;
    int32_t low = imm8 & 7;
    
    switch (mod)
    {
        case 5:
            if (low == 1)
            {
                // fucompp
                fpu_fcom(cpu, fpu_get_sti(cpu, 1));
                fpu_pop(cpu);
                fpu_pop(cpu);
            }
            else
            {
                return 0;
            }
            break;
        default:
            return 0;
    }
    return 1;
}

int32_t fpu_op_DA_mem(CPU* cpu, uint8_t imm8, uint32_t addr)
{
    cpu_onerror(cpu, "fpu_op_D9_mem not handled\n");
    return 0;
}

int32_t fpu_op_DB_reg(CPU* cpu, uint8_t imm8)
{
    cpu_onerror(cpu, "fpu_op_DB_reg not handled\n");
    return 0;
}

int32_t fpu_op_DB_mem(CPU* cpu, uint8_t imm8, uint32_t addr)
{
    int32_t mod = imm8 >> 3 & 7;
    int32_t int32, st0;
    int64_t st0l;
    
    switch (mod)
    {
        case 0:
            // fild
            int32 = cpu_readI32(cpu, addr);
            fpu_push(cpu, int32);
            break;
        case 2:
            // fist
            st0l = fpu_integer_round(cpu, fpu_get_st0(cpu));
            if (st0l <= INT32_MAX && st0l >= INT32_MIN)
            {
                // TODO: Invalid operation
                cpu_writeI32(cpu, addr, (int32_t)st0l);
            }
            else
            {
                fpu_invalid_arithmatic(cpu);
                cpu_writeU32(cpu, addr, 0x80000000);
            }
            break;
        case 3:
            // fistp
            st0l = fpu_integer_round(cpu, fpu_get_st0(cpu));
            if (st0l <= INT32_MAX && st0l >= INT32_MIN)
            {
                // TODO: Invalid operation
                cpu_writeI32(cpu, addr, st0l);
            }
            else
            {
                fpu_invalid_arithmatic(cpu);
                cpu_writeU32(cpu, addr, 0x80000000);
            }
            break;
        case 5:
            // fld
            fpu_push(cpu, fpu_load_m80(cpu, addr));
            break;
        case 7:
            // fstp
            fpu_store_m80(cpu, addr, fpu_get_st0(cpu));
            fpu_pop(cpu);
            break;
        default:
            return 0;
    }
    return 1;
}

int32_t fpu_op_DC_reg(CPU* cpu, uint8_t imm8)
{
    int32_t mod = imm8 >> 3 & 7;
    int32_t low = imm8 & 7;
    int32_t low_ptr = cpu->Fpu.StackPtr + low & 7;
    long double sti = fpu_get_sti(cpu, low);
    long double st0 = fpu_get_st0(cpu);
    
    switch (mod)
    {
        case 0:
            // fadd
            cpu->Fpu.St[low_ptr].fp = sti + st0;
            break;
        case 1:
            // fmul
            cpu->Fpu.St[low_ptr].fp = sti * st0;
            break;
        case 2:
            // fcom
            fpu_fcom(cpu, sti);
            break;
        case 3:
            // fcomp
            fpu_fcom(cpu, sti);
            fpu_pop(cpu);
            break;
        case 4:
            // fsubr
            cpu->Fpu.St[low_ptr].fp = st0 - sti;
            break;
        case 5:
            // fsub
            cpu->Fpu.St[low_ptr].fp = sti - st0;
            break;
        case 6:
            // fdivr
            cpu->Fpu.St[low_ptr].fp = st0 / sti;
            break;
        case 7:
            // fdiv
            cpu->Fpu.St[low_ptr].fp = sti / st0;
            break;
        default:
            return 0;
    }
    return 1;
}

int32_t fpu_op_DC_mem(CPU* cpu, uint8_t imm8, uint32_t addr)
{
    int32_t mod = imm8 >> 3 & 7;
    double m64 = fpu_load_m64(cpu, addr);
    
    long double st0 = fpu_get_st0(cpu);
    
    switch (mod)
    {
        case 0:
            // fadd
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 + m64;
            break;
        case 1:
            // fmul
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 * m64;
            break;
        case 2:
            // fcom
            fpu_fcom(cpu, m64);
            break;
        case 3:
            // fcomp
            fpu_fcom(cpu, m64);
            fpu_pop(cpu);
            break;
        case 4:
            // fsub
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 - m64;
            break;
        case 5:
            // fsubr
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = m64 - st0;
            break;
        case 6:
            // fdiv
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 / m64;
            break;
        case 7:
            // fdivr
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = m64 / st0;
            break;
        default:
            return 0;
    }
    return 1;
}

int32_t fpu_op_DD_reg(CPU* cpu, uint8_t imm8)
{
    int32_t mod = imm8 >> 3 & 7;
    int32_t low = imm8 & 7;
    
    switch (mod)
    {
        case 2:
            // fst
            cpu->Fpu.St[cpu->Fpu.StackPtr + low & 7].fp = fpu_get_st0(cpu);
            break;
        case 3:
            // fstp
            if (low == 0)
            {
                fpu_pop(cpu);
            }
            else
            {
                cpu->Fpu.St[cpu->Fpu.StackPtr + low & 7].fp = fpu_get_st0(cpu);
                fpu_pop(cpu);
            }
            break;
        case 4:
            fpu_fucom(cpu, fpu_get_sti(cpu, low));
            break;
        case 5:
            // fucomp
            fpu_fucom(cpu, fpu_get_sti(cpu, low));
            fpu_pop(cpu);
            break;
        default:
            return 0;
    }
    return 1;
}

int32_t fpu_op_DD_mem(CPU* cpu, uint8_t imm8, uint32_t addr)
{
    int32_t mod = imm8 >> 3 & 7;
    double m64;
    
    switch (mod)
    {
        case 0:
            // fld
            m64 = fpu_load_m64(cpu, addr);
            fpu_push(cpu, (long double)m64);
            break;
        case 2:
            // fst
            fpu_store_m64(cpu, addr, 0);
            break;
        case 3:
            // fstp
            fpu_store_m64(cpu, addr, 0);
            fpu_pop(cpu);
            break;
        default:
            return 0;
    }
    return 1;
}

int32_t fpu_op_DE_reg(CPU* cpu, uint8_t imm8)
{
    int32_t mod = imm8 >> 3 & 7;
    int32_t low = imm8 & 7;
    int32_t low_ptr = cpu->Fpu.StackPtr + low & 7;
    long double sti = fpu_get_sti(cpu, low);
    long double st0 = fpu_get_st0(cpu);
    
    switch (mod)
    {
        case 0:
            // faddp
            cpu->Fpu.St[low_ptr].fp = sti + st0;
            break;
        case 1:
            // fmulp
            cpu->Fpu.St[low_ptr].fp = sti * st0;
            break;
        case 2:
            // fcomp
            fpu_fcom(cpu, sti);
            break;
        case 3:
            // fcompp
            if (low == 1)
            {
                fpu_fcom(cpu, cpu->Fpu.St[low_ptr].fp);
                fpu_pop(cpu);
                fpu_pop(cpu);
            }
            else
            {
                // not a valid encoding
                cpu_onerror(cpu, "fpu invalid instruction encoding for fcompp at 0x%08X", cpu->EIP);
            }
            break;
        case 4:
            // fsubrp
            cpu->Fpu.St[low_ptr].fp = st0 - sti;
            break;
        case 5:
            // fsubp
            cpu->Fpu.St[low_ptr].fp = sti - st0;
            break;
        case 6:
            // fdivrp
            cpu->Fpu.St[low_ptr].fp = st0 / sti;
            break;
        case 7:
            // fdivp
            cpu->Fpu.St[low_ptr].fp = sti / st0;
            break;
        default:
            return 0;
    }
    return 1;
}

int32_t fpu_op_DE_mem(CPU* cpu, uint8_t imm8, uint32_t addr)
{
    int32_t mod = imm8 >> 3 & 7;
    uint16_t m16 = cpu_readU16(cpu, addr);
    
    long double st0 = fpu_get_st0(cpu);
    
    switch (mod)
    {
        case 0:
            // fadd
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 + m16;
            break;
        case 1:
            // fmul
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 * m16;
            break;
        case 2:
            // fcom
            fpu_fcom(cpu, m16);
            break;
        case 3:
            // fcomp
            fpu_fcom(cpu, m16);
            fpu_pop(cpu);
            break;
        case 4:
            // fsub
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 - m16;
            break;
        case 5:
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = m16 - st0;
            break;
        case 6:
            // fdiv
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = st0 / m16;
            break;
        case 7:
            // fdivr
            cpu->Fpu.St[cpu->Fpu.StackPtr].fp = m16 / st0;
            break;
        default:
            return 0;
    }
    return 1;
}

int32_t fpu_op_DF_reg(CPU* cpu, uint8_t imm8)
{
    int32_t mod = imm8 >> 3 & 7;
    int32_t low = imm8 & 7;
    
    switch (mod)
    {
        case 4:
            if (imm8 == 0xE0)
            {
                // fnstsw / FSTSW AX
                cpu->Reg[REG_EAX] = (cpu->Reg[REG_EAX] & 0xFFFF0000) | fpu_load_status_word(cpu);
            }
            else
            {
                return 0;
            }
            break;
        case 5:
            // fucomip
            fpu_fucomi(cpu, fpu_get_sti(cpu, low));
            fpu_pop(cpu);
            break;
        case 6:
            // fcomip
            fpu_fcomi(cpu, fpu_get_sti(cpu, low));
            fpu_pop(cpu);
            break;
        default:
            return 0;
    }
    return 1;
}

int32_t fpu_op_DF_mem(CPU* cpu, uint8_t imm8, uint32_t addr)
{
    int32_t mod = imm8 >> 3 & 7;
    uint64_t m64;
    uint16_t m16;
    int32_t st0;
    
    switch (mod)
    {
        case 0:
            // fild
            m16 = cpu_readU16(cpu, addr);
            fpu_push(cpu, m16);
            break;
        case 2:
            // fist
            st0 = (int32_t)fpu_integer_round(cpu, fpu_get_st0(cpu));
            if (st0 <= INT16_MAX && st0 >= INT16_MIN)
            {
                cpu_writeU32(cpu, addr, (uint32_t)st0);
            }
            else
            {
                fpu_invalid_arithmatic(cpu);
                cpu_writeU32(cpu, addr, 0x8000);
            }
            break;
        case 3:
            // fistp
            st0 = (int32_t)fpu_integer_round(cpu, fpu_get_st0(cpu));
            if (st0 <= INT16_MAX && st0 >= INT16_MIN)
            {
                cpu_writeU32(cpu, addr, (uint32_t)st0);
            }
            else
            {
                fpu_invalid_arithmatic(cpu);
                cpu_writeU32(cpu, addr, 0x8000);
            }
            fpu_pop(cpu);
            break;
        case 5:
            // fild
            m64 = cpu_readU64(cpu, addr);
            fpu_push(cpu, (long double)m64);
            break;
        default:
            return 0;
    }
    return 1;
}