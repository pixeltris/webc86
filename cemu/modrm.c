#include "cpu.h"

int32_t cpu_modrm_resolve(CPU* cpu, uint8_t modrm)
{
    cpu_dbg_assert(cpu, cpu->ModRM < 0xC0, NULL);
    switch (modrm)
    {
        case 0:
        case 8:
        case 16:
        case 24:
        case 32:
        case 40:
        case 48:
        case 56:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX));
        
        case 64:
        case 72:
        case 80:
        case 88:
        case 96:
        case 104:
        case 112:
        case 120:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX)) + cpu_read_disp8s(cpu);
        
        case 128:
        case 136:
        case 144:
        case 152:
        case 160:
        case 168:
        case 176:
        case 184:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX)) + cpu_read_disp32s(cpu);
        
        case 1:
        case 9:
        case 17:
        case 25:
        case 33:
        case 41:
        case 49:
        case 57:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX));
        
        case 65:
        case 73:
        case 81:
        case 89:
        case 97:
        case 105:
        case 113:
        case 121:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX)) + cpu_read_disp8s(cpu);
        
        case 129:
        case 137:
        case 145:
        case 153:
        case 161:
        case 169:
        case 177:
        case 185:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX)) + cpu_read_disp32s(cpu);
        
        case 2:
        case 10:
        case 18:
        case 26:
        case 34:
        case 42:
        case 50:
        case 58:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX));
        
        case 66:
        case 74:
        case 82:
        case 90:
        case 98:
        case 106:
        case 114:
        case 122:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX)) + cpu_read_disp8s(cpu);
        
        case 130:
        case 138:
        case 146:
        case 154:
        case 162:
        case 170:
        case 178:
        case 186:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX)) + cpu_read_disp32s(cpu);
        
        case 3:
        case 11:
        case 19:
        case 27:
        case 35:
        case 43:
        case 51:
        case 59:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX));
        
        case 67:
        case 75:
        case 83:
        case 91:
        case 99:
        case 107:
        case 115:
        case 123:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX)) + cpu_read_disp8s(cpu);
        
        case 131:
        case 139:
        case 147:
        case 155:
        case 163:
        case 171:
        case 179:
        case 187:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX)) + cpu_read_disp32s(cpu);
        
        case 4:// special case
        case 12:
        case 20:
        case 28:
        case 36:
        case 44:
        case 52:
        case 60:
            return cpu_sib_resolve(cpu, 0);
        
        case 68:// special case
        case 76:
        case 84:
        case 92:
        case 100:
        case 108:
        case 116:
        case 124:
            return cpu_sib_resolve(cpu, 1) + cpu_read_disp8s(cpu);
        
        case 132:// special case
        case 140:
        case 148:
        case 156:
        case 164:
        case 172:
        case 180:
        case 188:
            return cpu_sib_resolve(cpu, 1) + cpu_read_disp32s(cpu);
        
        case 5:// special case (override?)
        case 13:
        case 21:
        case 29:
        case 37:
        case 45:
        case 53:
        case 61:
            return cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu);
        
        case 69:
        case 77:
        case 85:
        case 93:
        case 101:
        case 109:
        case 117:
        case 125:
            return (cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP)) + cpu_read_disp8s(cpu);
        
        case 133:
        case 141:
        case 149:
        case 157:
        case 165:
        case 173:
        case 181:
        case 189:
            return (cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP)) + cpu_read_disp32s(cpu);
        
        case 6:// special case (override?)
        case 14:
        case 22:
        case 30:
        case 38:
        case 46:
        case 54:
        case 62:
            return cpu_get_seg_prefix_ds(cpu) + cpu_read_disp16(cpu);
        
        case 70:
        case 78:
        case 86:
        case 94:
        case 102:
        case 110:
        case 118:
        case 126:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI)) + cpu_read_disp8s(cpu);
        
        case 134:
        case 142:
        case 150:
        case 158:
        case 166:
        case 174:
        case 182:
        case 190:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI)) + cpu_read_disp32s(cpu);
        
        case 7:
        case 15:
        case 23:
        case 31:
        case 39:
        case 47:
        case 55:
        case 63:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI));
        
        case 71:
        case 79:
        case 87:
        case 95:
        case 103:
        case 111:
        case 119:
        case 127:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI)) + cpu_read_disp8s(cpu);
        
        case 135:
        case 143:
        case 151:
        case 159:
        case 167:
        case 175:
        case 183:
        case 191:
            return (cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI)) + cpu_read_disp32s(cpu);
        
        default:
            cpu_onerror(cpu, "Unknown modrm 0x%02X at EIP 0x%08X\n", modrm, cpu->EIP);
            return 0;
    }
}

int32_t cpu_sib_resolve(CPU* cpu, uint8_t mod)
{
    uint8_t sib = cpu_read_sib(cpu);
    switch (sib)
    {
        case 0: return (cpu_getReg32s(cpu, REG_EAX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 1: return (cpu_getReg32s(cpu, REG_EAX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 2: return (cpu_getReg32s(cpu, REG_EAX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 3: return (cpu_getReg32s(cpu, REG_EAX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 4: return (cpu_getReg32s(cpu, REG_EAX)) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 5: return (cpu_getReg32s(cpu, REG_EAX)) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 6: return (cpu_getReg32s(cpu, REG_EAX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 7: return (cpu_getReg32s(cpu, REG_EAX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 64: return (cpu_getReg32s(cpu, REG_EAX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 65: return (cpu_getReg32s(cpu, REG_EAX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 66: return (cpu_getReg32s(cpu, REG_EAX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 67: return (cpu_getReg32s(cpu, REG_EAX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 68: return (cpu_getReg32s(cpu, REG_EAX) << 1) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 69: return (cpu_getReg32s(cpu, REG_EAX) << 1) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 70: return (cpu_getReg32s(cpu, REG_EAX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 71: return (cpu_getReg32s(cpu, REG_EAX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 128: return (cpu_getReg32s(cpu, REG_EAX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 129: return (cpu_getReg32s(cpu, REG_EAX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 130: return (cpu_getReg32s(cpu, REG_EAX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 131: return (cpu_getReg32s(cpu, REG_EAX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 132: return (cpu_getReg32s(cpu, REG_EAX) << 2) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 133: return (cpu_getReg32s(cpu, REG_EAX) << 2) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 134: return (cpu_getReg32s(cpu, REG_EAX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 135: return (cpu_getReg32s(cpu, REG_EAX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 192: return (cpu_getReg32s(cpu, REG_EAX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 193: return (cpu_getReg32s(cpu, REG_EAX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 194: return (cpu_getReg32s(cpu, REG_EAX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 195: return (cpu_getReg32s(cpu, REG_EAX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 196: return (cpu_getReg32s(cpu, REG_EAX) << 3) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 197: return (cpu_getReg32s(cpu, REG_EAX) << 3) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 198: return (cpu_getReg32s(cpu, REG_EAX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 199: return (cpu_getReg32s(cpu, REG_EAX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 8: return (cpu_getReg32s(cpu, REG_ECX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 9: return (cpu_getReg32s(cpu, REG_ECX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 10: return (cpu_getReg32s(cpu, REG_ECX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 11: return (cpu_getReg32s(cpu, REG_ECX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 12: return (cpu_getReg32s(cpu, REG_ECX)) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 13: return (cpu_getReg32s(cpu, REG_ECX)) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 14: return (cpu_getReg32s(cpu, REG_ECX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 15: return (cpu_getReg32s(cpu, REG_ECX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 72: return (cpu_getReg32s(cpu, REG_ECX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 73: return (cpu_getReg32s(cpu, REG_ECX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 74: return (cpu_getReg32s(cpu, REG_ECX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 75: return (cpu_getReg32s(cpu, REG_ECX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 76: return (cpu_getReg32s(cpu, REG_ECX) << 1) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 77: return (cpu_getReg32s(cpu, REG_ECX) << 1) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 78: return (cpu_getReg32s(cpu, REG_ECX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 79: return (cpu_getReg32s(cpu, REG_ECX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 136: return (cpu_getReg32s(cpu, REG_ECX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 137: return (cpu_getReg32s(cpu, REG_ECX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 138: return (cpu_getReg32s(cpu, REG_ECX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 139: return (cpu_getReg32s(cpu, REG_ECX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 140: return (cpu_getReg32s(cpu, REG_ECX) << 2) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 141: return (cpu_getReg32s(cpu, REG_ECX) << 2) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 142: return (cpu_getReg32s(cpu, REG_ECX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 143: return (cpu_getReg32s(cpu, REG_ECX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 200: return (cpu_getReg32s(cpu, REG_ECX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 201: return (cpu_getReg32s(cpu, REG_ECX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 202: return (cpu_getReg32s(cpu, REG_ECX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 203: return (cpu_getReg32s(cpu, REG_ECX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 204: return (cpu_getReg32s(cpu, REG_ECX) << 3) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 205: return (cpu_getReg32s(cpu, REG_ECX) << 3) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 206: return (cpu_getReg32s(cpu, REG_ECX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 207: return (cpu_getReg32s(cpu, REG_ECX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 16: return (cpu_getReg32s(cpu, REG_EDX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 17: return (cpu_getReg32s(cpu, REG_EDX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 18: return (cpu_getReg32s(cpu, REG_EDX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 19: return (cpu_getReg32s(cpu, REG_EDX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 20: return (cpu_getReg32s(cpu, REG_EDX)) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 21: return (cpu_getReg32s(cpu, REG_EDX)) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 22: return (cpu_getReg32s(cpu, REG_EDX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 23: return (cpu_getReg32s(cpu, REG_EDX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 80: return (cpu_getReg32s(cpu, REG_EDX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 81: return (cpu_getReg32s(cpu, REG_EDX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 82: return (cpu_getReg32s(cpu, REG_EDX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 83: return (cpu_getReg32s(cpu, REG_EDX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 84: return (cpu_getReg32s(cpu, REG_EDX) << 1) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 85: return (cpu_getReg32s(cpu, REG_EDX) << 1) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 86: return (cpu_getReg32s(cpu, REG_EDX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 87: return (cpu_getReg32s(cpu, REG_EDX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 144: return (cpu_getReg32s(cpu, REG_EDX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 145: return (cpu_getReg32s(cpu, REG_EDX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 146: return (cpu_getReg32s(cpu, REG_EDX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 147: return (cpu_getReg32s(cpu, REG_EDX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 148: return (cpu_getReg32s(cpu, REG_EDX) << 2) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 149: return (cpu_getReg32s(cpu, REG_EDX) << 2) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 150: return (cpu_getReg32s(cpu, REG_EDX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 151: return (cpu_getReg32s(cpu, REG_EDX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 208: return (cpu_getReg32s(cpu, REG_EDX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 209: return (cpu_getReg32s(cpu, REG_EDX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 210: return (cpu_getReg32s(cpu, REG_EDX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 211: return (cpu_getReg32s(cpu, REG_EDX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 212: return (cpu_getReg32s(cpu, REG_EDX) << 3) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 213: return (cpu_getReg32s(cpu, REG_EDX) << 3) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 214: return (cpu_getReg32s(cpu, REG_EDX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 215: return (cpu_getReg32s(cpu, REG_EDX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 24: return (cpu_getReg32s(cpu, REG_EBX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 25: return (cpu_getReg32s(cpu, REG_EBX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 26: return (cpu_getReg32s(cpu, REG_EBX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 27: return (cpu_getReg32s(cpu, REG_EBX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 28: return (cpu_getReg32s(cpu, REG_EBX)) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 29: return (cpu_getReg32s(cpu, REG_EBX)) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 30: return (cpu_getReg32s(cpu, REG_EBX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 31: return (cpu_getReg32s(cpu, REG_EBX)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 88: return (cpu_getReg32s(cpu, REG_EBX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 89: return (cpu_getReg32s(cpu, REG_EBX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 90: return (cpu_getReg32s(cpu, REG_EBX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 91: return (cpu_getReg32s(cpu, REG_EBX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 92: return (cpu_getReg32s(cpu, REG_EBX) << 1) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 93: return (cpu_getReg32s(cpu, REG_EBX) << 1) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 94: return (cpu_getReg32s(cpu, REG_EBX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 95: return (cpu_getReg32s(cpu, REG_EBX) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 152: return (cpu_getReg32s(cpu, REG_EBX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 153: return (cpu_getReg32s(cpu, REG_EBX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 154: return (cpu_getReg32s(cpu, REG_EBX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 155: return (cpu_getReg32s(cpu, REG_EBX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 156: return (cpu_getReg32s(cpu, REG_EBX) << 2) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 157: return (cpu_getReg32s(cpu, REG_EBX) << 2) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 158: return (cpu_getReg32s(cpu, REG_EBX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 159: return (cpu_getReg32s(cpu, REG_EBX) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 216: return (cpu_getReg32s(cpu, REG_EBX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 217: return (cpu_getReg32s(cpu, REG_EBX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 218: return (cpu_getReg32s(cpu, REG_EBX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 219: return (cpu_getReg32s(cpu, REG_EBX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 220: return (cpu_getReg32s(cpu, REG_EBX) << 3) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 221: return (cpu_getReg32s(cpu, REG_EBX) << 3) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 222: return (cpu_getReg32s(cpu, REG_EBX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 223: return (cpu_getReg32s(cpu, REG_EBX) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 32: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 33: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 34: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 35: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 36: return cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 37: return (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 38: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 39: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 96: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 97: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 98: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 99: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 100: return cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 101: return (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 102: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 103: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 160: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 161: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 162: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 163: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 164: return cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 165: return (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 166: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 167: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 224: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 225: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 226: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 227: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 228: return cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 229: return (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 230: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 231: return cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 40: return (cpu_getReg32s(cpu, REG_EBP)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 41: return (cpu_getReg32s(cpu, REG_EBP)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 42: return (cpu_getReg32s(cpu, REG_EBP)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 43: return (cpu_getReg32s(cpu, REG_EBP)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 44: return (cpu_getReg32s(cpu, REG_EBP)) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 45: return (cpu_getReg32s(cpu, REG_EBP)) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 46: return (cpu_getReg32s(cpu, REG_EBP)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 47: return (cpu_getReg32s(cpu, REG_EBP)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 104: return (cpu_getReg32s(cpu, REG_EBP) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 105: return (cpu_getReg32s(cpu, REG_EBP) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 106: return (cpu_getReg32s(cpu, REG_EBP) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 107: return (cpu_getReg32s(cpu, REG_EBP) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 108: return (cpu_getReg32s(cpu, REG_EBP) << 1) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 109: return (cpu_getReg32s(cpu, REG_EBP) << 1) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 110: return (cpu_getReg32s(cpu, REG_EBP) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 111: return (cpu_getReg32s(cpu, REG_EBP) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 168: return (cpu_getReg32s(cpu, REG_EBP) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 169: return (cpu_getReg32s(cpu, REG_EBP) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 170: return (cpu_getReg32s(cpu, REG_EBP) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 171: return (cpu_getReg32s(cpu, REG_EBP) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 172: return (cpu_getReg32s(cpu, REG_EBP) << 2) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 173: return (cpu_getReg32s(cpu, REG_EBP) << 2) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 174: return (cpu_getReg32s(cpu, REG_EBP) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 175: return (cpu_getReg32s(cpu, REG_EBP) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 232: return (cpu_getReg32s(cpu, REG_EBP) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 233: return (cpu_getReg32s(cpu, REG_EBP) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 234: return (cpu_getReg32s(cpu, REG_EBP) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 235: return (cpu_getReg32s(cpu, REG_EBP) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 236: return (cpu_getReg32s(cpu, REG_EBP) << 3) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 237: return (cpu_getReg32s(cpu, REG_EBP) << 3) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 238: return (cpu_getReg32s(cpu, REG_EBP) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 239: return (cpu_getReg32s(cpu, REG_EBP) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 48: return (cpu_getReg32s(cpu, REG_ESI)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 49: return (cpu_getReg32s(cpu, REG_ESI)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 50: return (cpu_getReg32s(cpu, REG_ESI)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 51: return (cpu_getReg32s(cpu, REG_ESI)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 52: return (cpu_getReg32s(cpu, REG_ESI)) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 53: return (cpu_getReg32s(cpu, REG_ESI)) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 54: return (cpu_getReg32s(cpu, REG_ESI)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 55: return (cpu_getReg32s(cpu, REG_ESI)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 112: return (cpu_getReg32s(cpu, REG_ESI) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 113: return (cpu_getReg32s(cpu, REG_ESI) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 114: return (cpu_getReg32s(cpu, REG_ESI) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 115: return (cpu_getReg32s(cpu, REG_ESI) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 116: return (cpu_getReg32s(cpu, REG_ESI) << 1) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 117: return (cpu_getReg32s(cpu, REG_ESI) << 1) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 118: return (cpu_getReg32s(cpu, REG_ESI) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 119: return (cpu_getReg32s(cpu, REG_ESI) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 176: return (cpu_getReg32s(cpu, REG_ESI) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 177: return (cpu_getReg32s(cpu, REG_ESI) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 178: return (cpu_getReg32s(cpu, REG_ESI) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 179: return (cpu_getReg32s(cpu, REG_ESI) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 180: return (cpu_getReg32s(cpu, REG_ESI) << 2) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 181: return (cpu_getReg32s(cpu, REG_ESI) << 2) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 182: return (cpu_getReg32s(cpu, REG_ESI) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 183: return (cpu_getReg32s(cpu, REG_ESI) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 240: return (cpu_getReg32s(cpu, REG_ESI) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 241: return (cpu_getReg32s(cpu, REG_ESI) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 242: return (cpu_getReg32s(cpu, REG_ESI) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 243: return (cpu_getReg32s(cpu, REG_ESI) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 244: return (cpu_getReg32s(cpu, REG_ESI) << 3) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 245: return (cpu_getReg32s(cpu, REG_ESI) << 3) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 246: return (cpu_getReg32s(cpu, REG_ESI) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 247: return (cpu_getReg32s(cpu, REG_ESI) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 56: return (cpu_getReg32s(cpu, REG_EDI)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 57: return (cpu_getReg32s(cpu, REG_EDI)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 58: return (cpu_getReg32s(cpu, REG_EDI)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 59: return (cpu_getReg32s(cpu, REG_EDI)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 60: return (cpu_getReg32s(cpu, REG_EDI)) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 61: return (cpu_getReg32s(cpu, REG_EDI)) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 62: return (cpu_getReg32s(cpu, REG_EDI)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 63: return (cpu_getReg32s(cpu, REG_EDI)) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 120: return (cpu_getReg32s(cpu, REG_EDI) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 121: return (cpu_getReg32s(cpu, REG_EDI) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 122: return (cpu_getReg32s(cpu, REG_EDI) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 123: return (cpu_getReg32s(cpu, REG_EDI) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 124: return (cpu_getReg32s(cpu, REG_EDI) << 1) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 125: return (cpu_getReg32s(cpu, REG_EDI) << 1) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 126: return (cpu_getReg32s(cpu, REG_EDI) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 127: return (cpu_getReg32s(cpu, REG_EDI) << 1) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 184: return (cpu_getReg32s(cpu, REG_EDI) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 185: return (cpu_getReg32s(cpu, REG_EDI) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 186: return (cpu_getReg32s(cpu, REG_EDI) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 187: return (cpu_getReg32s(cpu, REG_EDI) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 188: return (cpu_getReg32s(cpu, REG_EDI) << 2) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 189: return (cpu_getReg32s(cpu, REG_EDI) << 2) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 190: return (cpu_getReg32s(cpu, REG_EDI) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 191: return (cpu_getReg32s(cpu, REG_EDI) << 2) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        case 248: return (cpu_getReg32s(cpu, REG_EDI) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EAX);
        case 249: return (cpu_getReg32s(cpu, REG_EDI) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ECX);
        case 250: return (cpu_getReg32s(cpu, REG_EDI) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDX);
        case 251: return (cpu_getReg32s(cpu, REG_EDI) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EBX);
        case 252: return (cpu_getReg32s(cpu, REG_EDI) << 3) + cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_ESP);
        case 253: return (cpu_getReg32s(cpu, REG_EDI) << 3) + (mod ? cpu_get_seg_prefix_ss(cpu) + cpu_getReg32s(cpu, REG_EBP) : cpu_get_seg_prefix_ds(cpu) + cpu_read_disp32s(cpu));
        case 254: return (cpu_getReg32s(cpu, REG_EDI) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_ESI);
        case 255: return (cpu_getReg32s(cpu, REG_EDI) << 3) + cpu_get_seg_prefix_ds(cpu) + cpu_getReg32s(cpu, REG_EDI);
        
        default:
            cpu_onerror(cpu, "Unknown mod for cpu_sib_resolve 0x%02X at EIP 0x%08X\n", mod, cpu->EIP);
            return 0;
    }
}