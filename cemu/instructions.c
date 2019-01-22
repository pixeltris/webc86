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
    switch (opcode)
    {
        default:
            cpu_unknown_opcode(cpu, opcode, -1);
            break;
    }
}

void cpu_execute_instruction_t16(CPU* cpu)
{
    uint8_t opcode = cpu_fetchU8(cpu);
    switch (opcode)
    {
        default:
            cpu_execute_instruction_t(cpu, opcode);
            break;
    }
}

void cpu_execute_instruction_t32(CPU* cpu)
{
    uint8_t opcode = cpu_fetchU8(cpu);
    switch (opcode)
    {
        case 0x01:
            cpu_fetch_modrm(cpu);
            //cpu_write_e32(cpu_add32(cpu_read_write_e16(cpu), cpu_read_g32s(cpu)));
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