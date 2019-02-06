#include "cpu.h"
#include <math.h>

int8_t cpu_add8(CPU* cpu, int8_t dest, int8_t src)
{
    return (int8_t)cpu_add(cpu, dest, src, OPSIZE_8);
}

int16_t cpu_add16(CPU* cpu, int16_t dest, int16_t src)
{
    return (int16_t)cpu_add(cpu, dest, src, OPSIZE_16);
}

int32_t cpu_add32(CPU* cpu, int32_t dest, int32_t src)
{
    return cpu_add(cpu, dest, src, OPSIZE_32);
}

int8_t cpu_adc8(CPU* cpu, int8_t dest, int8_t src)
{
    return (int8_t)cpu_adc(cpu, dest, src, OPSIZE_8);
}

int16_t cpu_adc16(CPU* cpu, int16_t dest, int16_t src)
{
    return (int16_t)cpu_adc(cpu, dest, src, OPSIZE_16);
}

int32_t cpu_adc32(CPU* cpu, int32_t dest, int32_t src)
{
    return cpu_adc(cpu, dest, src, OPSIZE_32);
}

int8_t cpu_sub8(CPU* cpu, int8_t dest, int8_t src)
{
    return (int8_t)cpu_sub(cpu, dest, src, OPSIZE_8);
}

int16_t cpu_sub16(CPU* cpu, int16_t dest, int16_t src)
{
    return (int16_t)cpu_sub(cpu, dest, src, OPSIZE_16);
}

int32_t cpu_sub32(CPU* cpu, int32_t dest, int32_t src)
{
    return cpu_sub(cpu, dest, src, OPSIZE_32);
}

int8_t cpu_cmp8(CPU* cpu, int8_t dest, int8_t src)
{
    return (int8_t)cpu_sub(cpu, dest, src, OPSIZE_8);
}

int16_t cpu_cmp16(CPU* cpu, int16_t dest, int16_t src)
{
    return (int16_t)cpu_sub(cpu, dest, src, OPSIZE_16);
}

int32_t cpu_cmp32(CPU* cpu, int32_t dest, int32_t src)
{
    return cpu_sub(cpu, dest, src, OPSIZE_32);
}

int8_t cpu_sbb8(CPU* cpu, int8_t dest, int8_t src)
{
    return (int8_t)cpu_sbb(cpu, dest, src, OPSIZE_8);
}

int16_t cpu_sbb16(CPU* cpu, int16_t dest, int16_t src)
{
    return (int16_t)cpu_sbb(cpu, dest, src, OPSIZE_16);
}

int32_t cpu_sbb32(CPU* cpu, int32_t dest, int32_t src)
{
    return cpu_sbb(cpu, dest, src, OPSIZE_32);
}

int32_t cpu_add(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t op_size)
{
    cpu->LastOp1 = dest_operand;
    cpu->LastOp2 = source_operand;
    cpu->LastAddResult = cpu->LastResult = dest_operand + source_operand;
    
    cpu->LastOpSize = op_size;
    cpu->FlagsChanged = FLAGS_ALL;
    
    return cpu->LastResult;
}

int32_t cpu_adc(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t op_size)
{
    int32_t cf = cpu_getcf(cpu);
    cpu->LastOp1 = dest_operand;
    cpu->LastOp2 = source_operand;
    cpu->LastAddResult = cpu->LastResult = (dest_operand + source_operand) + cf;
    
    cpu->LastOpSize = op_size;
    cpu->FlagsChanged = FLAGS_ALL;
    
    return cpu->LastResult;
}

int32_t cpu_sub(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t op_size)
{
    cpu->LastAddResult = dest_operand;
    cpu->LastOp2 = source_operand;
    cpu->LastOp1 = cpu->LastResult = dest_operand - source_operand;
    
    cpu->LastOpSize = op_size;
    cpu->FlagsChanged = FLAGS_ALL;
    
    return cpu->LastResult;
}

int32_t cpu_sbb(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t op_size)
{
    int32_t cf = cpu_getcf(cpu);
    cpu->LastAddResult = dest_operand;
    cpu->LastOp2 = source_operand;
    cpu->LastOp1 = cpu->LastResult = dest_operand - source_operand - cf;
    cpu->LastOpSize = op_size;
    
    cpu->FlagsChanged = FLAGS_ALL;
    
    return cpu->LastResult;
}

int8_t cpu_inc8(CPU* cpu, int8_t dest)
{
    return (int8_t)cpu_inc(cpu, dest, OPSIZE_8);
}

int16_t cpu_inc16(CPU* cpu, int16_t dest)
{
    return (int16_t)cpu_inc(cpu, dest, OPSIZE_16);
}

int32_t cpu_inc32(CPU* cpu, int32_t dest)
{
    return cpu_inc(cpu, dest, OPSIZE_32);
}

int8_t cpu_dec8(CPU* cpu, int8_t dest)
{
    return (int8_t)cpu_dec(cpu, dest, OPSIZE_8);
}

int16_t cpu_dec16(CPU* cpu, int16_t dest)
{
    return (int16_t)cpu_dec(cpu, dest, OPSIZE_16);
}

int32_t cpu_dec32(CPU* cpu, int32_t dest)
{
    return cpu_dec(cpu, dest, OPSIZE_32);
}

int32_t cpu_inc(CPU* cpu, int32_t dest_operand, int32_t op_size)
{
    cpu->Flags = (cpu->Flags & ~1) | cpu_getcf(cpu);
    cpu->LastOp1 = dest_operand;
    cpu->LastOp2 = 1;
    cpu->LastAddResult = cpu->LastResult = dest_operand + 1;
    cpu->LastOpSize = op_size;
    
    cpu->FlagsChanged = FLAGS_ALL & ~1;
    
    return cpu->LastResult;
}

int32_t cpu_dec(CPU* cpu, int32_t dest_operand, int32_t op_size)
{
    cpu->Flags = (cpu->Flags & ~1) | cpu_getcf(cpu);
    cpu->LastAddResult = dest_operand;
    cpu->LastOp2 = 1;
    cpu->LastOp1 = cpu->LastResult = dest_operand - 1;
    cpu->LastOpSize = op_size;
    
    cpu->FlagsChanged = FLAGS_ALL & ~1;
    
    return cpu->LastResult;
}

int8_t cpu_neg8(CPU* cpu, int8_t dest)
{
    return (int8_t)cpu_neg(cpu, dest, OPSIZE_8);
}

int16_t cpu_neg16(CPU* cpu, int16_t dest)
{
    return (int16_t)cpu_neg(cpu, dest, OPSIZE_16);
}

int32_t cpu_neg32(CPU* cpu, int32_t dest)
{
    return cpu_neg(cpu, dest, OPSIZE_32);
}

int32_t cpu_neg(CPU* cpu, int32_t dest_operand, int32_t op_size)
{
    cpu->LastOp1 = cpu->LastResult = -dest_operand;
    
    cpu->FlagsChanged = FLAGS_ALL;
    cpu->LastAddResult = 0;
    cpu->LastOp2 = dest_operand;
    cpu->LastOpSize = op_size;
    
    return cpu->LastResult;
}

void cpu_mul8(CPU* cpu, uint8_t source_operand)
{
    int32_t result = source_operand * cpu_getReg8(cpu, REG_EAX);
    
    cpu_setReg16(cpu, REG_EAX, result);
    cpu->LastResult = result & 0xFF;
    cpu->LastOpSize = OPSIZE_8;
    
    if (result < 0x100)
    {
        cpu->Flags = cpu->Flags & ~1 & ~FLAG_OVERFLOW;
    }
    else
    {
        cpu->Flags = cpu->Flags | 1 | FLAG_OVERFLOW;
    }
    
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
}

void cpu_imul8(CPU* cpu, int8_t source_operand)
{
    int32_t result = source_operand * cpu_getReg8s(cpu, REG_EAX);
    
    cpu_setReg16(cpu, REG_EAX, result);
    cpu->LastResult = result & 0xFF;
    cpu->LastOpSize = OPSIZE_8;
    
    if (result > INT8_MAX || result < INT8_MIN)
    {
        cpu->Flags = cpu->Flags | 1 | FLAG_OVERFLOW;
    }
    else
    {
        cpu->Flags = cpu->Flags & ~1 & ~FLAG_OVERFLOW;
    }
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
}

void cpu_mul16(CPU* cpu, uint16_t source_operand)
{
    int32_t result = source_operand * cpu_getReg16(cpu, REG_EAX);
    int32_t high_result = result >> 16;
    
    cpu_setReg16(cpu, REG_EAX, result);
    cpu_setReg16(cpu, REG_EDX, high_result);
    
    cpu->LastResult = result & 0xFFFF;
    cpu->LastOpSize = OPSIZE_16;
    
    if (high_result == 0)
    {
        cpu->Flags &= ~1 & ~FLAG_OVERFLOW;
    }
    else
    {
        cpu->Flags |= 1 | FLAG_OVERFLOW;
    }
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
}

void cpu_imul16(CPU* cpu, int16_t source_operand)
{
    int32_t result = source_operand * cpu_getReg16s(cpu, REG_EAX);
    
    cpu_setReg16(cpu, REG_EAX, result);
    cpu_setReg16(cpu, REG_EDX, result >> 16);
    
    cpu->LastResult = result & 0xFFFF;
    cpu->LastOpSize = OPSIZE_16;
    
    if (result > INT16_MAX || result < INT16_MIN)
    {
        cpu->Flags &= ~1 & ~FLAG_OVERFLOW;
    }
    else
    {
        cpu->Flags |= 1 | FLAG_OVERFLOW;
    }
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
}

int16_t cpu_imul_reg16(CPU* cpu, int16_t operand1, int16_t operand2)
{
    int32_t result = operand1 * operand2;
    
    cpu->LastResult = result & 0xFFFF;
    cpu->LastOpSize = OPSIZE_16;
    
    if (result > INT16_MAX || result < INT16_MIN)
    {
        cpu->Flags |= 1 | FLAG_OVERFLOW;
    }
    else
    {
        cpu->Flags &= ~1 & ~FLAG_OVERFLOW;
    }
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    
    return result;
}

void cpu_mul32(CPU* cpu, uint32_t source_operand)
{
    int64_t result = (int64_t)source_operand * (int64_t)cpu_getReg32s(cpu, REG_EAX);
    int32_t high_result = (int32_t)(result >> 32);
    
    cpu_setReg32s(cpu, REG_EAX, (int32_t)result);
    cpu_setReg32s(cpu, REG_EDX, high_result);
    
    cpu->LastResult = result & 0xFFFFFFFF;
    cpu->LastOpSize = OPSIZE_32;
    
    if (high_result == 0)
    {
        cpu->Flags &= ~1 & ~FLAG_OVERFLOW;
    }
    else
    {
        cpu->Flags |= 1 | FLAG_OVERFLOW;
    }
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
}

void cpu_imul32(CPU* cpu, int32_t source_operand)
{
    int64_t result = (int64_t)source_operand * (int64_t)cpu_getReg32s(cpu, REG_EAX);
    
    cpu_setReg32s(cpu, REG_EAX, (int32_t)result);
    cpu_setReg32s(cpu, REG_EDX, (int32_t)(result >> 32));
    
    cpu->LastResult = result & 0xFFFFFFFF;
    cpu->LastOpSize = OPSIZE_32;
    
    if (result > INT32_MAX || result < INT32_MIN)
    {
        cpu->Flags &= ~1 & ~FLAG_OVERFLOW;
    }
    else
    {
        cpu->Flags |= 1 | FLAG_OVERFLOW;
    }
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
}

int32_t cpu_imul_reg32(CPU* cpu, int32_t operand1, int32_t operand2)
{
    int64_t result = (int64_t)operand1 * (int64_t)operand2;
    
    cpu->LastResult = result & 0xFFFFFFFF;
    cpu->LastOpSize = OPSIZE_32;
    
    if (result > INT32_MAX || result < INT32_MIN)
    {
        cpu->Flags |= 1 | FLAG_OVERFLOW;
    }
    else
    {
        cpu->Flags &= ~1 & ~FLAG_OVERFLOW;
    }
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    
    return (int32_t)result;
}

void cpu_div8(CPU* cpu, uint8_t source_operand)
{
    if (source_operand == 0)
    {
        cpu_trigger_de(cpu);
        return;
    }
    
    uint32_t target_operand = cpu_getReg16(cpu, REG_EAX);
    uint32_t result = target_operand / source_operand;
    
    if (result > UINT8_MAX)
    {
        cpu_trigger_de(cpu);
    }
    else
    {
        cpu_setReg16(cpu, REG_EAX, result | ((target_operand % source_operand) << 8));
    }
}

void cpu_idiv8(CPU* cpu, int8_t source_operand)
{
    if (source_operand == 0)
    {
        cpu_trigger_de(cpu);
        return;
    }
    
    int32_t target_operand = cpu_getReg16s(cpu, REG_EAX);
    int32_t result = target_operand / source_operand;
    
    if (result > INT16_MAX || result < INT16_MIN)
    {
        cpu_trigger_de(cpu);
    }
    else
    {
        cpu_setReg16(cpu, REG_EAX, result | ((target_operand % source_operand) << 8));
    }
}

void cpu_div16(CPU* cpu, uint16_t source_operand)
{
    if (source_operand == 0)
    {
        cpu_trigger_de(cpu);
        return;
    }
    
    uint32_t target_operand = (cpu_getReg16(cpu, REG_EAX) | cpu_getReg16(cpu, REG_EDX) << 16);
    uint32_t result = target_operand / source_operand;
    
    if (result > UINT16_MAX || result < 0)
    {
        cpu_trigger_de(cpu);
    }
    else
    {
        cpu_setReg16(cpu, REG_EAX, result);
        cpu_setReg16(cpu, REG_EDX, target_operand % source_operand);
    }
}

void cpu_idiv16(CPU* cpu, int16_t source_operand)
{
    if (source_operand == 0)
    {
        cpu_trigger_de(cpu);
        return;
    }
    
    int32_t target_operand = cpu_getReg16(cpu, REG_EAX) | (cpu_getReg16(cpu, REG_EDX) << 16);
    int32_t result = target_operand / source_operand;
    
    if(result > INT16_MAX || result < INT16_MIN)
    {
        cpu_trigger_de(cpu);
    }
    else
    {
        cpu_setReg16(cpu, REG_EAX, result);
        cpu_setReg16(cpu, REG_EDX, target_operand % source_operand);
    }
}

void cpu_div32(CPU* cpu, uint32_t source_operand)
{
    if (source_operand == 0)
    {
        cpu_trigger_de(cpu);
        return;
    }
    
    uint64_t target_operand = ((uint64_t)cpu_getReg32(cpu, REG_EAX) | (uint64_t)cpu_getReg32(cpu, REG_EDX) << 32);
    uint64_t result = target_operand / source_operand;
    
    if (result > UINT32_MAX || result < 0)
    {
        cpu_trigger_de(cpu);
    }
    else
    {
        cpu_setReg32(cpu, REG_EAX, result);
        cpu_setReg32(cpu, REG_EDX, target_operand % source_operand);
    }
}

void cpu_idiv32(CPU* cpu, int32_t source_operand)
{
    if (source_operand == 0)
    {
        cpu_trigger_de(cpu);
        return;
    }
    
    int64_t target_operand = (int64_t)cpu_getReg32(cpu, REG_EAX) | ((int64_t)cpu_getReg32(cpu, REG_EDX) << 32);
    int64_t result = target_operand / source_operand;
    
    if(result > INT32_MAX || result < INT32_MIN)
    {
        cpu_trigger_de(cpu);
    }
    else
    {
        cpu_setReg32(cpu, REG_EAX, result);
        cpu_setReg32(cpu, REG_EDX, target_operand % source_operand);
    }
}

int8_t cpu_and8(CPU* cpu, int8_t dest, int8_t src)
{
    return (int8_t)cpu_and(cpu, dest, src, OPSIZE_8);
}

int16_t cpu_and16(CPU* cpu, int16_t dest, int16_t src)
{
    return (int16_t)cpu_and(cpu, dest, src, OPSIZE_16);
}

int32_t cpu_and32(CPU* cpu, int32_t dest, int32_t src)
{
    return cpu_and(cpu, dest, src, OPSIZE_32);
}

int32_t cpu_test8(CPU* cpu, int8_t dest, int8_t src)
{
    return cpu_and(cpu, dest, src, OPSIZE_8);
}

int32_t cpu_test16(CPU* cpu, int16_t dest, int16_t src)
{
    return cpu_and(cpu, dest, src, OPSIZE_16);
}

int32_t cpu_test32(CPU* cpu, int32_t dest, int32_t src)
{
    return cpu_and(cpu, dest, src, OPSIZE_32);
}

int8_t cpu_or8(CPU* cpu, int8_t dest, int8_t src)
{
    return (int8_t)cpu_or(cpu, dest, src, OPSIZE_8);
}

int16_t cpu_or16(CPU* cpu, int16_t dest, int16_t src)
{
    return (int16_t)cpu_or(cpu, dest, src, OPSIZE_16);
}

int32_t cpu_or32(CPU* cpu, int32_t dest, int32_t src)
{
    return cpu_or(cpu, dest, src, OPSIZE_32);
}

int8_t cpu_xor8(CPU* cpu, int8_t dest, int8_t src)
{
    return (int8_t)cpu_xor(cpu, dest, src, OPSIZE_8);
}

int16_t cpu_xor16(CPU* cpu, int16_t dest, int16_t src)
{
    return (int16_t)cpu_xor(cpu, dest, src, OPSIZE_16);
}

int32_t cpu_xor32(CPU* cpu, int32_t dest, int32_t src)
{
    return cpu_xor(cpu, dest, src, OPSIZE_32);
}

int32_t cpu_and(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t op_size)
{
    cpu->LastResult = dest_operand & source_operand;
    
    cpu->LastOpSize = op_size;
    cpu->Flags &= ~1 & ~FLAG_OVERFLOW & ~FLAG_ADJUST;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW & ~FLAG_ADJUST;
    
    return cpu->LastResult;
}

int32_t cpu_or(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t op_size)
{
    cpu->LastResult = dest_operand | source_operand;
    
    cpu->LastOpSize = op_size;
    cpu->Flags &= ~1 & ~FLAG_OVERFLOW & ~FLAG_ADJUST;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW & ~FLAG_ADJUST;
    
    return cpu->LastResult;
}

int32_t cpu_xor(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t op_size)
{
    cpu->LastResult = dest_operand ^ source_operand;
    
    cpu->LastOpSize = op_size;
    cpu->Flags &= ~1 & ~FLAG_OVERFLOW & ~FLAG_ADJUST;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW & ~FLAG_ADJUST;
    
    return cpu->LastResult;
}

int8_t cpu_rol8(CPU* cpu, int8_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    count &= 7;
    
    int32_t result = dest_operand << count | dest_operand >> (8 - count);
    
    cpu->FlagsChanged &= ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (result & 1) | (result << 11 ^ result << 4) & FLAG_OVERFLOW;
    
    return (int8_t)result;
}

int16_t cpu_rol16(CPU* cpu, int16_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    count &= 15;
    
    int32_t result = dest_operand << count | dest_operand >> (16 - count);
    
    cpu->FlagsChanged &= ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (result & 1) | (result << 11 ^ result >> 4) & FLAG_OVERFLOW;
    
    return (int16_t)result;
}

int32_t cpu_rol32(CPU* cpu, int32_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    int32_t result = dest_operand << count | dest_operand >> (32 - count);
    
    cpu->FlagsChanged &= ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (result & 1) | (result << 11 ^ result >> 20) & FLAG_OVERFLOW;
    
    return result;
}

int8_t cpu_rcl8(CPU* cpu, int8_t dest_operand, int32_t count)
{
    count %= 9;
    if (count == 0)
    {
        return dest_operand;
    }
    
    int32_t result = dest_operand << count | cpu_getcf(cpu) << (count - 1) | dest_operand >> (9 - count);
    
    cpu->FlagsChanged &= ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (result >> 8 & 1) | (result << 3 ^ result << 4) & FLAG_OVERFLOW;
    
    return (int8_t)result;
}

int16_t cpu_rcl16(CPU* cpu, int16_t dest_operand, int32_t count)
{
    count %= 17;
    if (count == 0)
    {
        return dest_operand;
    }
    
    int32_t result = dest_operand << count | cpu_getcf(cpu) << (count - 1) | dest_operand >> (17 - count);
    
    cpu->FlagsChanged &= ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (result >> 16 & 1) | (result >> 5 ^ result >> 4) & FLAG_OVERFLOW;
    
    return (int16_t)result;
}

int32_t cpu_rcl32(CPU* cpu, int32_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    int32_t result = dest_operand << count | cpu_getcf(cpu) << (count - 1);
    
    if (count > 1)
    {
        result |= dest_operand >> (33 - count);
    }
    
    cpu->FlagsChanged &= ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (dest_operand >> (32 - count) & 1);
    cpu->Flags |= (cpu->Flags << 11 ^ result >> 20) & FLAG_OVERFLOW;
    
    return result;
}

int8_t cpu_ror8(CPU* cpu, int8_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    count &= 7;
    int32_t result = dest_operand >> count | dest_operand << (8 - count);
    
    cpu->FlagsChanged &= ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (result >> 7 & 1) | (result << 4 ^ result << 5) & FLAG_OVERFLOW;
    
    return (int8_t)result;
}

int16_t cpu_ror16(CPU* cpu, int16_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    count &= 15;
    int32_t result = dest_operand >> count | dest_operand << (16 - count);
    
    cpu->FlagsChanged &= ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (result >> 15 & 1) | (result >> 4 ^ result >> 3) & FLAG_OVERFLOW;
    
    return (int16_t)result;
}

int32_t cpu_ror32(CPU* cpu, int32_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    int32_t result = dest_operand >> count | dest_operand << (32 - count);
    
    cpu->FlagsChanged &= ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (result >> 31 & 1) | (result >> 20 ^ result >> 19) & FLAG_OVERFLOW;
    
    return result;
}

int8_t cpu_rcr8(CPU* cpu, int8_t dest_operand, int32_t count)
{
    count %= 9;
    if (count == 0)
    {
        return dest_operand;
    }
    
    int32_t result = dest_operand >> count | cpu_getcf(cpu) << (8 - count) | dest_operand << (9 - count);
    
    cpu->FlagsChanged &= ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (result >> 8 & 1) | (result << 4 ^ result << 5) & FLAG_OVERFLOW;
    
    return (int8_t)result;
}

int16_t cpu_rcr16(CPU* cpu, int16_t dest_operand, int32_t count)
{
    count %= 17;
    if (count == 0)
    {
        return dest_operand;
    }
    
    int32_t result = dest_operand >> count | cpu_getcf(cpu) << (16 - count) | dest_operand << (17 - count);
    
    cpu->FlagsChanged &= ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (result >> 16 & 1) | (result >> 4 ^ result >> 3) & FLAG_OVERFLOW;
    
    return (int16_t)result;
}

int32_t cpu_rcr32(CPU* cpu, int32_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    int32_t result = dest_operand >> count | cpu_getcf(cpu) << (32 - count);
    
    if (count > 1)
    {
        result |= dest_operand << (33 - count);
    }
    
    cpu->FlagsChanged &= ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (dest_operand >> (count - 1) & 1) | (result >> 20 ^ result >> 19) & FLAG_OVERFLOW;
    
    return result;
}

uint8_t cpu_shl8(CPU* cpu, uint8_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    cpu->LastResult = dest_operand << count;
    
    cpu->LastOpSize = OPSIZE_8;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (cpu->LastResult >> 8 & 1) | (cpu->LastResult << 3 ^ cpu->LastResult << 4) & FLAG_OVERFLOW;
    
    return (uint8_t)cpu->LastResult;
}

uint16_t cpu_shl16(CPU* cpu, uint16_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    cpu->LastResult = dest_operand << count;
    
    cpu->LastOpSize = OPSIZE_16;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (cpu->LastResult >> 16 & 1) | (cpu->LastResult >> 5 ^ cpu->LastResult >> 4) & FLAG_OVERFLOW;
    
    return (uint16_t)cpu->LastResult;
}

uint32_t cpu_shl32(CPU* cpu, uint32_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    cpu->LastResult = dest_operand << count;
    
    cpu->LastOpSize = OPSIZE_32;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    // test this
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (dest_operand >> (32 - count) & 1);
    cpu->Flags |= ((cpu->Flags & 1) ^ (cpu->LastResult >> 31 & 1)) << 11 & FLAG_OVERFLOW;
    
    return cpu->LastResult;
}

uint8_t cpu_shr8(CPU* cpu, uint8_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    cpu->LastResult = dest_operand >> count;
    
    cpu->LastOpSize = OPSIZE_8;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (dest_operand >> (count - 1) & 1) | (dest_operand >> 7 & 1) << 11 & FLAG_OVERFLOW;
    
    return (uint8_t)cpu->LastResult;
}

uint16_t cpu_shr16(CPU* cpu, uint16_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    cpu->LastResult = dest_operand >> count;
    
    cpu->LastOpSize = OPSIZE_16;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (dest_operand >> (count - 1) & 1) | (dest_operand >> 4) & FLAG_OVERFLOW;
    
    return (uint16_t)cpu->LastResult;
}

uint32_t cpu_shr32(CPU* cpu, uint32_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    cpu->LastResult = dest_operand >> count;
    
    cpu->LastOpSize = OPSIZE_32;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (dest_operand >> (count - 1) & 1) | (dest_operand >> 20) & FLAG_OVERFLOW;
    
    return cpu->LastResult;
}

int8_t cpu_sar8(CPU* cpu, int8_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    if (count < 8)
    {
        cpu->LastResult = (dest_operand << 24) >> (count + 24);
        // of is zero
        cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (dest_operand >> (count - 1) & 1);
    }
    else
    {
        cpu->LastResult = dest_operand << 24 >> 31;
        cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (cpu->LastResult & 1);
    }
    
    cpu->LastOpSize = OPSIZE_8;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    
    return (int8_t)cpu->LastResult;
}

int16_t cpu_sar16(CPU* cpu, int16_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    if (count < 16)
    {
        cpu->LastResult = (dest_operand << 16) >> (count + 16);
        cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (dest_operand >> (count - 1) & 1);
    }
    else
    {
        cpu->LastResult = dest_operand << 16 >> 31;
        cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (cpu->LastResult & 1);
    }
    
    cpu->LastOpSize = OPSIZE_16;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    
    return (int16_t)cpu->LastResult;
}

int32_t cpu_sar32(CPU* cpu, int32_t dest_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    cpu->LastResult = dest_operand >> count;
    
    cpu->LastOpSize = OPSIZE_32;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1 & ~FLAG_OVERFLOW) | (dest_operand >> (count - 1) & 1);
    
    return cpu->LastResult;
}

int16_t cpu_shrd16(CPU* cpu, int16_t dest_operand, int16_t source_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    if (count <= 16)
    {
        cpu->LastResult = dest_operand >> count | source_operand << (16 - count);
        cpu->Flags = (cpu->Flags & ~1) | (dest_operand >> (count - 1) & 1);
    }
    else
    {
        cpu->LastResult = dest_operand << (32 - count) | source_operand >> (count - 16);
        cpu->Flags = (cpu->Flags & ~1) | (source_operand >> (count - 17) & 1);
    }
    
    cpu->LastOpSize = OPSIZE_16;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~FLAG_OVERFLOW) | ((cpu->LastResult ^ dest_operand) >> 4 & FLAG_OVERFLOW);
    
    return (int16_t)cpu->LastResult;
}

int32_t cpu_shrd32(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    cpu->LastResult = dest_operand >> count | source_operand << (32 - count);
    
    cpu->LastOpSize = OPSIZE_32;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1) | (dest_operand >> (count - 1) & 1);
    cpu->Flags = (cpu->Flags & ~FLAG_OVERFLOW) | ((cpu->LastResult ^ dest_operand) >> 20 & FLAG_OVERFLOW);
    
    return cpu->LastResult;
}

int16_t cpu_shld16(CPU* cpu, int16_t dest_operand, int16_t source_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    if (count <= 16)
    {
        cpu->LastResult = dest_operand << count | source_operand >> (16 - count);
        cpu->Flags = (cpu->Flags & ~1) | (dest_operand >> (16 - count) & 1);
    }
    else
    {
        cpu->LastResult = dest_operand >> (32 - count) | source_operand << (count - 16);
        cpu->Flags = (cpu->Flags & ~1) | (source_operand >> (32 - count) & 1);
    }
    
    cpu->LastOpSize = OPSIZE_16;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~FLAG_OVERFLOW) | ((cpu->Flags & 1) ^ (cpu->LastResult >> 15 & 1)) << 11;
    
    return (int16_t)cpu->LastResult;
}

int32_t cpu_shld32(CPU* cpu, int32_t dest_operand, int32_t source_operand, int32_t count)
{
    if (count == 0)
    {
        return dest_operand;
    }
    
    cpu->LastResult = dest_operand << count | source_operand >> (32 - count);
    
    cpu->LastOpSize = OPSIZE_32;
    cpu->FlagsChanged = FLAGS_ALL & ~1 & ~FLAG_OVERFLOW;
    cpu->Flags = (cpu->Flags & ~1) | (dest_operand >> (32 - count) & 1);
    
    if (count == 1)
    {
        cpu->Flags = (cpu->Flags & ~FLAG_OVERFLOW) | ((cpu->Flags & 1) ^ (cpu->LastResult >> 31 & 1)) << 11;
    }
    else
    {
        cpu->Flags &= ~FLAG_OVERFLOW;
    }
    
    return cpu->LastResult;
}

int64_t cpu_integer_round(CPU* cpu, long double f, int32_t rc)
{
    if (rc == 0)
    {
        return llroundl(f);
    }
    else if (rc == 1 || (rc == 3 && f > 0))
    {
        return (int32_t)floorl(f);
    }
    else
    {
        return (int64_t)ceill(f);
    }
}

uint8_t cpu_int_log2_table[256];
void cpu_init_int_log2_table()
{
    for (int32_t i = 0, b = -2; i < 256; i++)
    {
        if (!(i & i - 1))
        {
            b++;
        }
        
        cpu_int_log2_table[i] = (uint8_t)b;
    }
}

uint8_t cpu_int_log2_byte(uint8_t x)
{
    return cpu_int_log2_table[x];
}

uint32_t cpu_int_log2(uint32_t x)
{
    uint32_t tt = x >> 16;
    uint32_t t = tt >> 8;
    
    if (tt)
    {
        if (t)
        {
            return 24 + cpu_int_log2_table[t];
        }
        else
        {
            return 16 + cpu_int_log2_table[tt];
        }
    }
    else
    {
        if (t)
        {
            return 8 + cpu_int_log2_table[t];
        }
        else
        {
            return cpu_int_log2_table[tt];
        }
    }
    return 0;
}

uint16_t cpu_bsr16(CPU* cpu, uint16_t old, uint16_t bit_base)
{
    cpu->FlagsChanged = FLAGS_ALL & ~FLAG_ZERO;
    cpu->LastOpSize = OPSIZE_16;

    if (bit_base == 0)
    {
        cpu->Flags |= FLAG_ZERO;
        cpu->LastResult = bit_base;
        return old;
    }
    else
    {
        cpu->Flags &= ~FLAG_ZERO;
        return (uint16_t)(cpu->LastResult = cpu_int_log2(bit_base));
    }
}

uint32_t cpu_bsr32(CPU* cpu, uint32_t old, uint32_t bit_base)
{
    cpu->FlagsChanged = FLAGS_ALL & ~FLAG_ZERO;
    cpu->LastOpSize = OPSIZE_32;

    if (bit_base == 0)
    {
        cpu->Flags |= FLAG_ZERO;
        cpu->LastResult = bit_base;
        return old;
    }
    else
    {
        cpu->Flags &= ~FLAG_ZERO;
        return (uint32_t)(cpu->LastResult = cpu_int_log2(bit_base));
    }
}