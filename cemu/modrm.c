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

int32_t cpu_sib_resolve(CPU* cpu, uint8_t mod)
{
    switch (mod)
    {
        default:
            cpu_onerror(cpu, "Unknown mod for cpu_sib_resolve 0x%02X\n", mod);
            return 0;
    }
}