#include "cpu.h"

void cpu_unknown_opcode(CPU* cpu, int32_t op1, int32_t op2)
{
    if (op2 != -1)
    {
        cpu_onerror(cpu, "Unknown opcode %02X %02X\n", op1, op2);
    }
    else
    {
        cpu_onerror(cpu, "Unknown opcode %02X\n", op1);
    }
}

void cpu_execute_instruction_t(CPU* cpu, uint8_t opcode)
{
    uint8_t tempU8;
    
    switch (opcode)
    {
        case 0x64:
            tempU8 = cpu_readU8(cpu, cpu->EIP);
            switch (tempU8)
            {
                case 0xA1:
                case 0xA3:
                    // Skip to the next instruction to avoid invalid reads/writes
                    cpu->EIP += 5;
                    break;
                default:
                    cpu_onerror(cpu, "TODO: Gracefully handle FS prefix (redirect memory to a garbage read/write memory block)\n");
                    break;
            }
            break;
        case 0x66:
            // Operand-size override prefix
            cpu->Prefixes |= PREFIX_MASK_OPSIZE;
            cpu_execute_prefix_instruction(cpu);
            cpu->Prefixes = 0;
            break;
        case 0x70:
            cpu_jmpcc8(cpu, cpu_test_o(cpu));
            break;
        case 0x71:
            cpu_jmpcc8(cpu, !cpu_test_o(cpu));
            break;
        case 0x72:
            cpu_jmpcc8(cpu, cpu_test_b(cpu));
            break;
        case 0x73:
            cpu_jmpcc8(cpu, !cpu_test_b(cpu));
            break;
        case 0x74:
            cpu_jmpcc8(cpu, cpu_test_z(cpu));
            break;
        case 0x75:
            cpu_jmpcc8(cpu, !cpu_test_z(cpu));
            break;
        case 0x76:
            cpu_jmpcc8(cpu, cpu_test_be(cpu));
            break;
        case 0x77:
            cpu_jmpcc8(cpu, !cpu_test_be(cpu));
            break;
        case 0x78:
            cpu_jmpcc8(cpu, cpu_test_s(cpu));
            break;
        case 0x79:
            cpu_jmpcc8(cpu, !cpu_test_s(cpu));
            break;
        case 0x7A:
            cpu_jmpcc8(cpu, cpu_test_p(cpu));
            break;
        case 0x7B:
            cpu_jmpcc8(cpu, !cpu_test_p(cpu));
            break;
        case 0x7C:
            cpu_jmpcc8(cpu, cpu_test_l(cpu));
            break;
        case 0x7D:
            cpu_jmpcc8(cpu, !cpu_test_l(cpu));
            break;
        case 0x7E:
            cpu_jmpcc8(cpu, cpu_test_le(cpu));
            break;
        case 0x7F:
            cpu_jmpcc8(cpu, !cpu_test_le(cpu));
            break;
        case 0x80:
            cpu_fetch_modrm(cpu);
            switch (cpu->ModRM >> 3 & 7)
            {
                case 0: cpu_write_e8(cpu, cpu_add8(cpu, cpu_read_write_e8(cpu), cpu_read_op8(cpu))); break;
                case 1: cpu_write_e8(cpu, cpu_or8(cpu, cpu_read_write_e8(cpu), cpu_read_op8(cpu))); break;
                case 2: cpu_write_e8(cpu, cpu_adc8(cpu, cpu_read_write_e8(cpu), cpu_read_op8(cpu))); break;
                case 3: cpu_write_e8(cpu, cpu_sbb8(cpu, cpu_read_write_e8(cpu), cpu_read_op8(cpu))); break;
                case 4: cpu_write_e8(cpu, cpu_and8(cpu, cpu_read_write_e8(cpu), cpu_read_op8(cpu))); break;
                case 5: cpu_write_e8(cpu, cpu_sub8(cpu, cpu_read_write_e8(cpu), cpu_read_op8(cpu))); break;
                case 6: cpu_write_e8(cpu, cpu_xor8(cpu, cpu_read_write_e8(cpu), cpu_read_op8(cpu))); break;
                case 7: cpu_cmp8(cpu, cpu_read_e8(cpu), cpu_read_op8(cpu)); break;
            }
            break;
        case 0x88:
            cpu_fetch_modrm(cpu);
            cpu_set_e8(cpu, cpu_read_g8(cpu));
            break;
        case 0x90:
            break;
        case 0xA0:
            tempU8 = cpu_readU8(cpu, cpu_read_moffs(cpu));
            cpu_setReg8(cpu, REG_EAX, tempU8);
            break;
        case 0xA2:
            cpu_writeU8(cpu, cpu_read_moffs(cpu), cpu_getReg8(cpu, REG_EAX));
            break;
        default:
            cpu_unknown_opcode(cpu, opcode, -1);
            break;
    }
}

void cpu_execute_instruction_t16(CPU* cpu)
{
    int16_t tempI16, tempI16_2, tempI16_3;
    uint16_t tempU16;
    
    uint8_t opcode = cpu_fetchU8(cpu);
    switch (opcode)
    {
        case 0x01:
            cpu_fetch_modrm(cpu);
            cpu_write_e16(cpu, cpu_add16(cpu_read_write_e16(cpu), cpu_read_g16(cpu)));
            break;
        case 0x09:
            cpu_fetch_modrm(cpu);
            cpu_write_e16(cpu, cpu_or16(cpu, cpu_read_write_e16(cpu), cpu_read_g16(cpu)));
            break;
        case 0x0F:
            cpu_execute_instruction_0F_t16(cpu);
            break;
        case 0x11:
            cpu_fetch_modrm(cpu);
            cpu_write_e16(cpu, cpu_adc16(cpu, cpu_read_write_e16(cpu), cpu_read_g16(cpu)));
            break;
        case 0x19:
            cpu_fetch_modrm(cpu);
            cpu_write_e16(cpu, cpu_sbb16(cpu, cpu_read_write_e16(cpu), cpu_read_g16(cpu)));
            break;
        case 0x1B:
            cpu_fetch_modrm(cpu);
            cpu_write_g16(cpu, cpu_sbb16(cpu, cpu_read_g16(cpu), cpu_read_e16(cpu)));
            break;
        case 0x21:
            cpu_fetch_modrm(cpu);
            cpu_write_e16(cpu, cpu_and16(cpu, cpu_read_write_e16(cpu), cpu_read_g16(cpu)));
            break;
        case 0x29:
            cpu_fetch_modrm(cpu);
            cpu_write_e16(cpu, cpu_sub16(cpu, cpu_read_write_e16(cpu), cpu_read_g16(cpu)));
            break;
        case 0x2B:
            cpu_fetch_modrm(cpu);
            cpu_write_g16(cpu, cpu_sub16(cpu, cpu_read_g16(cpu), cpu_read_e16(cpu)));
            break;
        case 0x2D:
            cpu_setReg16(cpu, REG_EAX, cpu_sub16(cpu, cpu_getReg16(cpu, REG_EAX), cpu_read_op16(cpu)));
            break;
        case 0x31:
            cpu_fetch_modrm(cpu);
            cpu_write_e16(cpu, cpu_xor16(cpu, cpu_read_write_e16(cpu), cpu_read_g16(cpu)));
            break;
        case 0x39:
            cpu_fetch_modrm(cpu);
            cpu_cmp16(cpu, cpu_read_e16(cpu), cpu_read_g16(cpu));
            break;
        case 0x3D:
            cpu_cmp16(cpu, cpu_getReg16(cpu, REG_EAX), cpu_read_op16(cpu));
            break;
        case 0x40:
            cpu_setReg16(cpu, REG_EAX, cpu_inc16(cpu, cpu_getReg16(cpu, REG_EAX)));
            break;
        case 0x41:
            cpu_setReg16(cpu, REG_ECX, cpu_inc16(cpu, cpu_getReg16(cpu, REG_ECX)));
            break;
        case 0x42:
            cpu_setReg16(cpu, REG_EDX, cpu_inc16(cpu, cpu_getReg16(cpu, REG_EDX)));
            break;
        case 0x48:
            cpu_setReg16(cpu, REG_EAX, cpu_dec16(cpu, cpu_getReg16(cpu, REG_EAX)));
            break;
        case 0x49:
            cpu_setReg16(cpu, REG_ECX, cpu_dec16(cpu, cpu_getReg16(cpu, REG_ECX)));
            break;
        case 0x4A:
            cpu_setReg16(cpu, REG_EDX, cpu_dec16(cpu, cpu_getReg16(cpu, REG_EDX)));
            break;
        case 0x50:
            cpu_push16(cpu, cpu_getReg16(cpu, REG_EAX));
            break;
        case 0x51:
            cpu_push16(cpu, cpu_getReg16(cpu, REG_ECX));
            break;
        case 0x52:
            cpu_push16(cpu, cpu_getReg16(cpu, REG_EDX));
            break;
        case 0x55:
            cpu_push16(cpu, cpu_getReg16(cpu, REG_EBP));
            break;
        case 0x5D:
            cpu_setReg16(cpu, REG_EBP, cpu_pop16(cpu));
            break;
        case 0x6A:
            cpu_push16(cpu, cpu_read_op8s(cpu));
            break;
        case 0x81:
            cpu_fetch_modrm(cpu);
            switch (cpu->ModRM >> 3 & 7)
            {
                case 0: cpu_write_e16(cpu, cpu_add16(cpu, cpu_read_write_e16(cpu), cpu_read_op16(cpu))); break;
                case 1: cpu_write_e16(cpu, cpu_or16(cpu, cpu_read_write_e16(cpu), cpu_read_op16(cpu))); break;
                case 2: cpu_write_e16(cpu, cpu_adc16(cpu, cpu_read_write_e16(cpu), cpu_read_op16(cpu))); break;
                case 3: cpu_write_e16(cpu, cpu_sbb16(cpu, cpu_read_write_e16(cpu), cpu_read_op16(cpu))); break;
                case 4: cpu_write_e16(cpu, cpu_and16(cpu, cpu_read_write_e16(cpu), cpu_read_op16(cpu))); break;
                case 5: cpu_write_e16(cpu, cpu_sub16(cpu, cpu_read_write_e16(cpu), cpu_read_op16(cpu))); break;
                case 6: cpu_write_e16(cpu, cpu_xor16(cpu, cpu_read_write_e16(cpu), cpu_read_op16(cpu))); break;
                case 7: cpu_cmp16(cpu, cpu_read_e16(cpu), cpu_read_op16(cpu)); break;
            }
            break;
        case 0x83:
            cpu_fetch_modrm(cpu);
            switch (cpu->ModRM >> 3 & 7)
            {
                case 0: cpu_write_e16(cpu, cpu_add16(cpu, cpu_read_write_e16(cpu), cpu_read_op8s(cpu))); break;
                case 1: cpu_write_e16(cpu, cpu_or16(cpu, cpu_read_write_e16(cpu), cpu_read_op8s(cpu))); break;
                case 2: cpu_write_e16(cpu, cpu_adc16(cpu, cpu_read_write_e16(cpu), cpu_read_op8s(cpu))); break;
                case 3: cpu_write_e16(cpu, cpu_sbb16(cpu, cpu_read_write_e16(cpu), cpu_read_op8s(cpu))); break;
                case 4: cpu_write_e16(cpu, cpu_and16(cpu, cpu_read_write_e16(cpu), cpu_read_op8s(cpu))); break;
                case 5: cpu_write_e16(cpu, cpu_sub16(cpu, cpu_read_write_e16(cpu), cpu_read_op8s(cpu))); break;
                case 6: cpu_write_e16(cpu, cpu_xor16(cpu, cpu_read_write_e16(cpu), cpu_read_op8s(cpu))); break;
                case 7: cpu_cmp16(cpu, cpu_read_e16(cpu), cpu_read_op8s(cpu)); break;
            }
            break;
        case 0x85:
            cpu_fetch_modrm(cpu);
            tempU16 = cpu_read_e16(cpu);
            cpu_test16(cpu, tempU16, cpu_read_g16(cpu));
            break;
        case 0x87:
            cpu_fetch_modrm(cpu);
            tempU16 = cpu_read_write_e16(cpu);
            cpu_write_e16(cpu, cpu_xchg16(cpu, tempU16, cpu->ModRM));
            break;
        case 0x89:
            cpu_fetch_modrm(cpu);
            cpu_set_e16(cpu, cpu_read_g16(cpu));
            break;
        case 0x8B:
            cpu_fetch_modrm(cpu);
            tempU16 = cpu_read_e16(cpu);
            cpu_write_g16(cpu, tempU16);
            break;
        case 0x8D:
            cpu_fetch_modrm(cpu);
            if (cpu->ModRM >= 0xC0)
            {
                //cpu_dbg_log("lea #ud", LOG_CPU);
                cpu_trigger_ud(cpu);
            }
            tempU16 = (uint16_t)(cpu->ModRM >> 3 & 7);
            
            // override prefix, so modrm_resolve does not return the segment part
            cpu->Prefixes |= SEG_PREFIX_ZERO;
            cpu_setReg16(cpu, tempU16, cpu_modrm_resolve(cpu, cpu->ModRM));
            cpu->Prefixes = 0;
            break;
        case 0x99:
            cpu_setReg16(cpu, REG_EDX, cpu_getReg16s(cpu, REG_EAX) >> 15);
            break;
        case 0xA1:
            tempU16 = cpu_readU16(cpu, cpu_read_moffs(cpu));
            cpu_setReg16(cpu, REG_EAX, tempU16);
            break;
        case 0xA3:
            cpu_writeU16(cpu, cpu_read_moffs(cpu), cpu_getReg16(cpu, REG_EAX));
            break;
        case 0xB8:
            cpu_setReg16(cpu, REG_EAX, cpu_read_op16(cpu));
            break;
        case 0xB9:
            cpu_setReg16(cpu, REG_ECX, cpu_read_op16(cpu));
            break;
        case 0xBA:
            cpu_setReg16(cpu, REG_EDX, cpu_read_op16(cpu));
            break;
        case 0xC1:
            cpu_fetch_modrm(cpu);
            tempI16 = (int16_t)(cpu_read_write_e16(cpu));
            tempI16_2 = (int16_t)(cpu_read_op8(cpu) & 31);
            tempI16_3 = 0;
            switch (cpu->ModRM >> 3 & 7)
            {
                case 0: tempI16_3 = cpu_rol16(cpu, tempI16, tempI16_2); break;
                case 1: tempI16_3 = cpu_ror16(cpu, tempI16, tempI16_2); break;
                case 2: tempI16_3 = cpu_rcl16(cpu, tempI16, tempI16_2); break;
                case 3: tempI16_3 = cpu_rcr16(cpu, tempI16, tempI16_2); break;
                case 4: tempI16_3 = cpu_shl16(cpu, tempI16, tempI16_2); break;
                case 5: tempI16_3 = cpu_shr16(cpu, tempI16, tempI16_2); break;
                case 6: tempI16_3 = cpu_shl16(cpu, tempI16, tempI16_2); break;
                case 7: tempI16_3 = cpu_sar16(cpu, tempI16, tempI16_2); break;
            }
            cpu_write_e16(cpu, (uint16_t)tempI16_3);
            break;
        default:
            cpu_execute_instruction_t(cpu, opcode);
            break;
    }
}

void cpu_execute_instruction_t32(CPU* cpu)
{
    int16_t tempI16;
    int32_t tempI32, tempI32_2, tempI32_3;
    uint32_t tempU32;
    
    uint8_t opcode = cpu_fetchU8(cpu);
    switch (opcode)
    {
        case 0x01:
            cpu_fetch_modrm(cpu);
            cpu_write_e32(cpu, cpu_add32(cpu_read_write_e16(cpu), cpu_read_g32s(cpu)));
            break;
        case 0x09:
            cpu_fetch_modrm(cpu);
            cpu_write_e32(cpu, cpu_or32(cpu, cpu_read_write_e32(cpu), cpu_read_g32s(cpu)));
            break;
        case 0x0F:
            cpu_execute_instruction_0F_t32(cpu);
            break;
        case 0x11:
            cpu_fetch_modrm(cpu);
            cpu_write_e32(cpu, cpu_adc32(cpu, cpu_read_write_e32(cpu), cpu_read_g32s(cpu)));
            break;
        case 0x19:
            cpu_fetch_modrm(cpu);
            cpu_write_e32(cpu, cpu_sbb32(cpu, cpu_read_write_e32(cpu), cpu_read_g32s(cpu)));
            break;
        case 0x1B:
            cpu_fetch_modrm(cpu);
            cpu_write_g32(cpu, cpu_sbb32(cpu, cpu_read_g32s(cpu), cpu_read_e32s(cpu)));
            break;
        case 0x21:
            cpu_fetch_modrm(cpu);
            cpu_write_e32(cpu, cpu_and32(cpu, cpu_read_write_e32(cpu), cpu_read_g32s(cpu)));
            break;
        case 0x29:
            cpu_fetch_modrm(cpu);
            cpu_write_e32(cpu, cpu_sub32(cpu, cpu_read_write_e32(cpu), cpu_read_g32s(cpu)));
            break;
        case 0x2B:
            cpu_fetch_modrm(cpu);
            cpu_write_g32(cpu, cpu_sub32(cpu, cpu_read_g32s(cpu), cpu_read_e32s(cpu)));
            break;
        case 0x2D:
            cpu_setReg32s(cpu, REG_EAX, cpu_sub32(cpu, cpu_getReg32s(cpu, REG_EAX), cpu_read_op32s(cpu)));
            break;
        case 0x31:
            cpu_fetch_modrm(cpu);
            cpu_write_e32(cpu, cpu_xor32(cpu, cpu_read_write_e32(cpu), cpu_read_g32s(cpu)));
            break;
        case 0x39:
            cpu_fetch_modrm(cpu);
            cpu_cmp32(cpu, cpu_read_e32s(cpu), cpu_read_g32s(cpu));
            break;
        case 0x3D:
            cpu_cmp32(cpu, cpu_getReg32s(cpu, REG_EAX), cpu_read_op32s(cpu));
            break;
        case 0x40:
            cpu_setReg32s(cpu, REG_EAX, cpu_inc32(cpu, cpu_getReg32s(cpu, REG_EAX)));
            break;
        case 0x41:
            cpu_setReg32s(cpu, REG_ECX, cpu_inc32(cpu, cpu_getReg32s(cpu, REG_ECX)));
            break;
        case 0x42:
            cpu_setReg32s(cpu, REG_EDX, cpu_inc32(cpu, cpu_getReg32s(cpu, REG_EDX)));
            break;
        case 0x48:
            cpu_setReg32s(cpu, REG_EAX, cpu_dec32(cpu, cpu_getReg32s(cpu, REG_EAX)));
            break;
        case 0x49:
            cpu_setReg32s(cpu, REG_ECX, cpu_dec32(cpu, cpu_getReg32s(cpu, REG_ECX)));
            break;
        case 0x4A:
            cpu_setReg32s(cpu, REG_EDX, cpu_dec32(cpu, cpu_getReg32s(cpu, REG_EDX)));
            break;
        case 0x50:
            cpu_push32(cpu, cpu_getReg32s(cpu, REG_EAX));
            break;
        case 0x51:
            cpu_push32(cpu, cpu_getReg32s(cpu, REG_ECX));
            break;
        case 0x52:
            cpu_push32(cpu, cpu_getReg32s(cpu, REG_EDX));
            break;
        case 0x55:
            cpu_push32(cpu, cpu_getReg32s(cpu, REG_EBP));
            break;
        case 0x5D:
            cpu_setReg32s(cpu, REG_EBP, cpu_pop32s(cpu));
            break;
        case 0x6A:
            cpu_push32(cpu, cpu_read_op8s(cpu));
            break;
        case 0x81:
            cpu_fetch_modrm(cpu);
            switch (cpu->ModRM >> 3 & 7)
            {
                case 0: cpu_write_e32(cpu, cpu_add32(cpu, cpu_read_write_e32(cpu), cpu_read_op32s(cpu))); break;
                case 1: cpu_write_e32(cpu, cpu_or32(cpu, cpu_read_write_e32(cpu), cpu_read_op32s(cpu))); break;
                case 2: cpu_write_e32(cpu, cpu_adc32(cpu, cpu_read_write_e32(cpu), cpu_read_op32s(cpu))); break;
                case 3: cpu_write_e32(cpu, cpu_sbb32(cpu, cpu_read_write_e32(cpu), cpu_read_op32s(cpu))); break;
                case 4: cpu_write_e32(cpu, cpu_and32(cpu, cpu_read_write_e32(cpu), cpu_read_op32s(cpu))); break;
                case 5: cpu_write_e32(cpu, cpu_sub32(cpu, cpu_read_write_e32(cpu), cpu_read_op32s(cpu))); break;
                case 6: cpu_write_e32(cpu, cpu_xor32(cpu, cpu_read_write_e32(cpu), cpu_read_op32s(cpu))); break;
                case 7: cpu_cmp32(cpu, cpu_read_e32s(cpu), cpu_read_op32s(cpu)); break;
            }
            break;
        case 0x83:
            cpu_fetch_modrm(cpu);
            switch (cpu->ModRM >> 3 & 7)
            {
                case 0: cpu_write_e32(cpu, cpu_add32(cpu, cpu_read_write_e32(cpu), cpu_read_op8s(cpu))); break;
                case 1: cpu_write_e32(cpu, cpu_or32(cpu, cpu_read_write_e32(cpu), cpu_read_op8s(cpu))); break;
                case 2: cpu_write_e32(cpu, cpu_adc32(cpu, cpu_read_write_e32(cpu), cpu_read_op8s(cpu))); break;
                case 3: cpu_write_e32(cpu, cpu_sbb32(cpu, cpu_read_write_e32(cpu), cpu_read_op8s(cpu))); break;
                case 4: cpu_write_e32(cpu, cpu_and32(cpu, cpu_read_write_e32(cpu), cpu_read_op8s(cpu))); break;
                case 5: cpu_write_e32(cpu, cpu_sub32(cpu, cpu_read_write_e32(cpu), cpu_read_op8s(cpu))); break;
                case 6: cpu_write_e32(cpu, cpu_xor32(cpu, cpu_read_write_e32(cpu), cpu_read_op8s(cpu))); break;
                case 7: cpu_cmp32(cpu, cpu_read_e32s(cpu), cpu_read_op8s(cpu)); break;
            }
            break;
        case 0x85:
            cpu_fetch_modrm(cpu);
            tempI32 = cpu_read_e32s(cpu);
            cpu_test32(cpu, tempI32, cpu_read_g32s(cpu));
            break;
        case 0x87:
            cpu_fetch_modrm(cpu);
            tempU32 = cpu_read_write_e32(cpu);
            cpu_write_e32(cpu, cpu_xchg32(cpu, tempU32, cpu->ModRM));
            break;
        case 0x89:
            cpu_fetch_modrm(cpu);
            cpu_set_e32(cpu, cpu_read_g32s(cpu));
            break;
        case 0x8B:
            cpu_fetch_modrm(cpu);
            tempI32 = cpu_read_e32s(cpu);
            cpu_write_g32(cpu, tempI32);
            break;
        case 0x8D:
            cpu_fetch_modrm(cpu);
            if (cpu->ModRM >= 0xC0)
            {
                //cpu_dbg_log("lea #ud", LOG_CPU);
                cpu_trigger_ud(cpu);
            }
            tempI32 = cpu->ModRM >> 3 & 7;
            
            cpu->Prefixes |= SEG_PREFIX_ZERO;
            cpu_setReg32s(cpu, tempI32, cpu_modrm_resolve(cpu, cpu->ModRM));
            cpu->Prefixes = 0;
            break;
        case 0x99:
            cpu_setReg32s(cpu, REG_EDX, cpu_getReg32s(cpu, REG_EAX) >> 31);
            break;
        case 0xA1:
            tempI32 = cpu_readI32(cpu, cpu_read_moffs(cpu));
            cpu_setReg32s(cpu, REG_EAX, tempI32);
            break;
        case 0xA3:
            cpu_writeU32(cpu, cpu_read_moffs(cpu), cpu_getReg32s(cpu, REG_EAX));
            break;
        case 0xB8:
            cpu_setReg32s(cpu, REG_EAX, cpu_read_op32s(cpu));
            break;
        case 0xB9:
            cpu_setReg32s(cpu, REG_ECX, cpu_read_op32s(cpu));
            break;
        case 0xBA:
            cpu_setReg32s(cpu, REG_EDX, cpu_read_op32s(cpu));
            break;
        case 0xC1:
            cpu_fetch_modrm(cpu);
            tempI32 = (int32_t)(cpu_read_write_e32(cpu));
            tempI32_2 = (int32_t)(cpu_read_op8(cpu) & 31);
            tempI32_3 = 0;
            switch (cpu->ModRM >> 3 & 7)
            {
                case 0: tempI32_3 = cpu_rol32(cpu, tempI32, tempI32_2); break;
                case 1: tempI32_3 = cpu_ror32(cpu, tempI32, tempI32_2); break;
                case 2: tempI32_3 = cpu_rcl32(cpu, tempI32, tempI32_2); break;
                case 3: tempI32_3 = cpu_rcr32(cpu, tempI32, tempI32_2); break;
                case 4: tempI32_3 = cpu_shl32(cpu, tempI32, tempI32_2); break;
                case 5: tempI32_3 = cpu_shr32(cpu, tempI32, tempI32_2); break;
                case 6: tempI32_3 = cpu_shl32(cpu, tempI32, tempI32_2); break;
                case 7: tempI32_3 = cpu_sar32(cpu, tempI32, tempI32_2); break;
            }
            cpu_write_e32(cpu, (uint32_t)tempI32_3);
            break;
        default:
            cpu_execute_instruction_t(cpu, opcode);
            break;
    }
}

void cpu_execute_instruction_0F_t(CPU* cpu, uint8_t opcode)
{
    switch (opcode)
    {
        default:
            cpu_unknown_opcode(cpu, opcode, -1);
            break;
    }
}

void cpu_execute_instruction_0F_t16(CPU* cpu)
{
    uint8_t opcode = cpu_fetchU8(cpu);
    switch (opcode)
    {
        default:
            cpu_execute_instruction_0F_t(cpu, opcode);
            break;
    }
}

void cpu_execute_instruction_0F_t32(CPU* cpu)
{
    uint8_t opcode = cpu_fetchU8(cpu);
    switch (opcode)
    {
        default:
            cpu_execute_instruction_0F_t(cpu, opcode);
            break;
    }
}