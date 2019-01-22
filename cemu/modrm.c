#include "cpu.h"

int32_t cpu_modrm_resolve(CPU* cpu, uint8_t modrm)
{
    cpu_dbg_assert(cpu, cpu->ModRM < 0xC0, NULL);
    switch (modrm)
    {
        default:
            cpu_onerror(cpu, "Unknown modrm 0x%02X\n", modrm);
            return 0;
    }
}