"use strict";

function genTables(cpu)
{
    var t = [];
    var t16 = [];
    var t32 = [];
    
    t16[0x01] = function() { cpu.read_modrm_byte(); cpu.write_e16(cpu.add16(cpu.read_write_e16(), cpu.read_g16())); };
    t32[0x01] = function() { cpu.read_modrm_byte(); cpu.write_e32(cpu.add32(cpu.read_write_e32(), cpu.read_g32s())); };
    t16[0x09] = function() { cpu.read_modrm_byte(); cpu.write_e16(cpu.or16(cpu.read_write_e16(), cpu.read_g16())); };
    t32[0x09] = function() { cpu.read_modrm_byte(); cpu.write_e32(cpu.or32(cpu.read_write_e32(), cpu.read_g32s())); };
    t16[0x0F] = function() {
        cpu.table0F_16[cpu.read_op0F()](cpu);
    };
    t32[0x0F] = function() {
        cpu.table0F_32[cpu.read_op0F()](cpu);
    };
    
    t16[0x11] = function() { cpu.read_modrm_byte(); cpu.write_e16(cpu.adc16(cpu.read_write_e16(), cpu.read_g16())); };
    t32[0x11] = function() { cpu.read_modrm_byte(); cpu.write_e32(cpu.adc32(cpu.read_write_e32(), cpu.read_g32s())); };

    t16[0x19] = function() { cpu.read_modrm_byte(); cpu.write_e16(cpu.sbb16(cpu.read_write_e16(), cpu.read_g16())); };
    t32[0x19] = function() { cpu.read_modrm_byte(); cpu.write_e32(cpu.sbb32(cpu.read_write_e32(), cpu.read_g32s())); };
    t16[0x1b] = function() { cpu.read_modrm_byte(); cpu.write_g16(cpu.sbb16(cpu.read_g16(), cpu.read_e16())); };
    t32[0x1b] = function() { cpu.read_modrm_byte(); cpu.write_g32(cpu.sbb32(cpu.read_g32s(), cpu.read_e32s())); };

    t16[0x21] = function() { cpu.read_modrm_byte(); cpu.write_e16(cpu.and16(cpu.read_write_e16(), cpu.read_g16())); };
    t32[0x21] = function() { cpu.read_modrm_byte(); cpu.write_e32(cpu.and32(cpu.read_write_e32(), cpu.read_g32s())); };

    t16[0x29] = function() { cpu.read_modrm_byte(); cpu.write_e16(cpu.sub16(cpu.read_write_e16(), cpu.read_g16())); };
    t32[0x29] = function() { cpu.read_modrm_byte(); cpu.write_e32(cpu.sub32(cpu.read_write_e32(), cpu.read_g32s())); };
    t16[0x2b] = function() { cpu.read_modrm_byte(); cpu.write_g16(cpu.sub16(cpu.read_g16(), cpu.read_e16())); };
    t32[0x2b] = function() { cpu.read_modrm_byte(); cpu.write_g32(cpu.sub32(cpu.read_g32s(), cpu.read_e32s())); };
    t16[0x2d] = function() { cpu.setReg16(reg_eax, cpu.sub16(cpu.getReg16(reg_eax), cpu.read_op16())); };
    t32[0x2d] = function() { cpu.setRead32s(reg_eax, cpu.sub32(cpu.getReg32s(reg_eax), cpu.read_op32s())); };

    t16[0x31] = function() { cpu.read_modrm_byte(); cpu.write_e16(cpu.xor16(cpu.read_write_e16(), cpu.read_g16())); };
    t32[0x31] = function() { cpu.read_modrm_byte(); cpu.write_e32(cpu.xor32(cpu.read_write_e32(), cpu.read_g32s())); };

    t16[0x39] = function() { cpu.read_modrm_byte(); cpu.cmp16(cpu.read_e16(), cpu.read_g16()); };
    t32[0x39] = function() { cpu.read_modrm_byte(); cpu.cmp32(cpu.read_e32s(), cpu.read_g32s()); };
    t16[0x3D] = function() { cpu.cmp16(cpu.getReg16(reg_eax), cpu.read_op16()); };
    t32[0x3D] = function() { cpu.cmp32(cpu.getReg32s(reg_eax), cpu.read_op32s()); };

    t16[0x40] = function() { cpu.setReg16(reg_eax, cpu.inc16(cpu.getReg16(reg_eax))); };
    t32[0x40] = function() { cpu.setReg32s(reg_eax, cpu.inc32(cpu.getReg32s(reg_eax))); };
    t16[0x41] = function() { cpu.setReg16(reg_ecx, cpu.inc16(cpu.getReg16(reg_ecx))); };
    t32[0x41] = function() { cpu.setReg32s(reg_ecx, cpu.inc32(cpu.getReg32s(reg_ecx))); };
    t16[0x42] = function() { cpu.setReg16(reg_edx, cpu.inc16(cpu.getReg16(reg_edx))); };
    t32[0x42] = function() { cpu.setReg32s(reg_edx, cpu.inc32(cpu.getReg32s(reg_edx))); };
    
    t16[0x48] = function() { cpu.setReg16(reg_eax, cpu.dec16(cpu.getReg16(reg_eax))); };
    t32[0x48] = function() { cpu.setReg32s(reg_eax, cpu.dec32(cpu.getReg32s(reg_eax))); };
    t16[0x49] = function() { cpu.setReg16(reg_ecx, cpu.dec16(cpu.getReg16(reg_ecx))); };
    t32[0x49] = function() { cpu.setReg32s(reg_ecx, cpu.dec32(cpu.getReg32s(reg_ecx))); };
    t16[0x4A] = function() { cpu.setReg16(reg_edx, cpu.dec16(cpu.getReg16(reg_edx))); };
    t32[0x4A] = function() { cpu.setReg32s(reg_edx, cpu.dec32(cpu.getReg32s(reg_edx))); };
    
    t16[0x50] = function() { cpu.push16(cpu.getReg16(reg_eax)); };
    t32[0x50] = function() { cpu.push32(cpu.getReg32s(reg_eax)); };
    t16[0x51] = function() { cpu.push16(cpu.getReg16(reg_ecx)); };
    t32[0x51] = function() { cpu.push32(cpu.getReg32s(reg_ecx)); };
    t16[0x52] = function() { cpu.push16(cpu.getReg16(reg_edx)); };
    t32[0x52] = function() { cpu.push32(cpu.getReg32s(reg_edx)); };
    t16[0x55] = function() { cpu.push16(cpu.getReg16(reg_ebp)); };
    t32[0x55] = function() { cpu.push32(cpu.getReg32s(reg_ebp)); };
    
    t16[0x5D] = function() { cpu.setReg16(reg_ebp, cpu.pop16()); };
    t32[0x5D] = function() { cpu.setReg32s(reg_ebp, cpu.pop32s()); };
    
    t[0x64] = function() {
        switch (cpu.virtualMemory[cpu.mem_eip])
        {
            case 0xA1:
            case 0xA3:
                // Skip to the next instruction to avoid invalid reads/writes
                cpu.mem_eip += 5;
                break;
            default:
                throw "TODO: Gracefully handle FS prefix (redirect memory to a garbage read/write memory block)";
        }        
        //cpu.segment_prefix_op(reg_fs);
    };
    
    t[0x66] = function() {
        // Operand-size override prefix
        cpu.prefixes |= PREFIX_MASK_OPSIZE;
        cpu.run_prefix_instruction();
        cpu.prefixes = 0;
    };
    
    t16[0x6A] = function() { cpu.push16(cpu.read_op8s()); };
    t32[0x6A] = function() { cpu.push32(cpu.read_op8s()); };
    
    t[0x70] = function() { cpu.jmpcc8( cpu.test_o()); };
    t[0x71] = function() { cpu.jmpcc8(!cpu.test_o()); };
    t[0x72] = function() { cpu.jmpcc8( cpu.test_b()); };
    t[0x73] = function() { cpu.jmpcc8(!cpu.test_b()); };
    t[0x74] = function() { cpu.jmpcc8( cpu.test_z()); };
    t[0x75] = function() { cpu.jmpcc8(!cpu.test_z()); };
    t[0x76] = function() { cpu.jmpcc8( cpu.test_be()); };
    t[0x77] = function() { cpu.jmpcc8(!cpu.test_be()); };
    t[0x78] = function() { cpu.jmpcc8( cpu.test_s()); };
    t[0x79] = function() { cpu.jmpcc8(!cpu.test_s()); };
    t[0x7A] = function() { cpu.jmpcc8( cpu.test_p()); };
    t[0x7B] = function() { cpu.jmpcc8(!cpu.test_p()); };
    t[0x7C] = function() { cpu.jmpcc8( cpu.test_l()); };
    t[0x7D] = function() { cpu.jmpcc8(!cpu.test_l()); };
    t[0x7E] = function() { cpu.jmpcc8( cpu.test_le()); };
    t[0x7F] = function() { cpu.jmpcc8(!cpu.test_le()); };
    
    t[0x80] = function() { cpu.read_modrm_byte();
        switch(cpu.modrm_byte >> 3 & 7)
        {
            case 0: cpu.write_e8(cpu.add8(cpu.read_write_e8(), cpu.read_op8())); break;
            case 1: cpu.write_e8(cpu. or8(cpu.read_write_e8(), cpu.read_op8())); break;
            case 2: cpu.write_e8(cpu.adc8(cpu.read_write_e8(), cpu.read_op8())); break;
            case 3: cpu.write_e8(cpu.sbb8(cpu.read_write_e8(), cpu.read_op8())); break;
            case 4: cpu.write_e8(cpu.and8(cpu.read_write_e8(), cpu.read_op8())); break;
            case 5: cpu.write_e8(cpu.sub8(cpu.read_write_e8(), cpu.read_op8())); break;
            case 6: cpu.write_e8(cpu.xor8(cpu.read_write_e8(), cpu.read_op8())); break;
            case 7: cpu.cmp8(cpu.read_e8(), cpu.read_op8()); break;
        }
    };
    t16[0x81] = function() { cpu.read_modrm_byte();
        switch(cpu.modrm_byte >> 3 & 7)
        {
            case 0: cpu.write_e16(cpu.add16(cpu.read_write_e16(), cpu.read_op16())); break;
            case 1: cpu.write_e16(cpu. or16(cpu.read_write_e16(), cpu.read_op16())); break;
            case 2: cpu.write_e16(cpu.adc16(cpu.read_write_e16(), cpu.read_op16())); break;
            case 3: cpu.write_e16(cpu.sbb16(cpu.read_write_e16(), cpu.read_op16())); break;
            case 4: cpu.write_e16(cpu.and16(cpu.read_write_e16(), cpu.read_op16())); break;
            case 5: cpu.write_e16(cpu.sub16(cpu.read_write_e16(), cpu.read_op16())); break;
            case 6: cpu.write_e16(cpu.xor16(cpu.read_write_e16(), cpu.read_op16())); break;
            case 7: cpu.cmp16(cpu.read_e16(), cpu.read_op16()); break;
        }
    };
    t32[0x81] = function() { cpu.read_modrm_byte();
        switch(cpu.modrm_byte >> 3 & 7)
        {
            case 0: cpu.write_e32(cpu.add32(cpu.read_write_e32(), cpu.read_op32s())); break;
            case 1: cpu.write_e32(cpu. or32(cpu.read_write_e32(), cpu.read_op32s())); break;
            case 2: cpu.write_e32(cpu.adc32(cpu.read_write_e32(), cpu.read_op32s())); break;
            case 3: cpu.write_e32(cpu.sbb32(cpu.read_write_e32(), cpu.read_op32s())); break;
            case 4: cpu.write_e32(cpu.and32(cpu.read_write_e32(), cpu.read_op32s())); break;
            case 5: cpu.write_e32(cpu.sub32(cpu.read_write_e32(), cpu.read_op32s())); break;
            case 6: cpu.write_e32(cpu.xor32(cpu.read_write_e32(), cpu.read_op32s())); break;
            case 7: cpu.cmp32(cpu.read_e32s(), cpu.read_op32s()); break;
        }
    };
    //t[0x82] = t[0x80]; // alias
    t16[0x83] = function() { cpu.read_modrm_byte();
        switch(cpu.modrm_byte >> 3 & 7)
        {
            case 0: cpu.write_e16(cpu.add16(cpu.read_write_e16(), cpu.read_op8s())); break;
            case 1: cpu.write_e16(cpu. or16(cpu.read_write_e16(), cpu.read_op8s())); break;
            case 2: cpu.write_e16(cpu.adc16(cpu.read_write_e16(), cpu.read_op8s())); break;
            case 3: cpu.write_e16(cpu.sbb16(cpu.read_write_e16(), cpu.read_op8s())); break;
            case 4: cpu.write_e16(cpu.and16(cpu.read_write_e16(), cpu.read_op8s())); break;
            case 5: cpu.write_e16(cpu.sub16(cpu.read_write_e16(), cpu.read_op8s())); break;
            case 6: cpu.write_e16(cpu.xor16(cpu.read_write_e16(), cpu.read_op8s())); break;
            case 7: cpu.cmp16(cpu.read_e16(), cpu.read_op8s()); break;
        }
    };
    t32[0x83] = function() { cpu.read_modrm_byte();
        switch(cpu.modrm_byte >> 3 & 7)
        {
            case 0: cpu.write_e32(cpu.add32(cpu.read_write_e32(), cpu.read_op8s())); break;
            case 1: cpu.write_e32(cpu. or32(cpu.read_write_e32(), cpu.read_op8s())); break;
            case 2: cpu.write_e32(cpu.adc32(cpu.read_write_e32(), cpu.read_op8s())); break;
            case 3: cpu.write_e32(cpu.sbb32(cpu.read_write_e32(), cpu.read_op8s())); break;
            case 4: cpu.write_e32(cpu.and32(cpu.read_write_e32(), cpu.read_op8s())); break;
            case 5: cpu.write_e32(cpu.sub32(cpu.read_write_e32(), cpu.read_op8s())); break;
            case 6: cpu.write_e32(cpu.xor32(cpu.read_write_e32(), cpu.read_op8s())); break;
            case 7: cpu.cmp32(cpu.read_e32s(), cpu.read_op8s()); break;
        }
    };
    
    //t[0x84] = function() { cpu.read_modrm_byte(); var data = cpu.read_e8(); cpu.test8(data, cpu.read_g8()); };
    t16[0x85] = function() { cpu.read_modrm_byte(); var data = cpu.read_e16(); cpu.test16(data, cpu.read_g16()); };
    t32[0x85] = function() { cpu.read_modrm_byte(); var data = cpu.read_e32s(); cpu.test32(data, cpu.read_g32s()); };

    //t[0x86] = function() { cpu.read_modrm_byte(); var data = cpu.read_write_e8(); cpu.write_e8(cpu.xchg8(data, cpu.modrm_byte)); };
    t16[0x87] = function() { cpu.read_modrm_byte();
        var data = cpu.read_write_e16(); cpu.write_e16(cpu.xchg16(data, cpu.modrm_byte));
    };
    t32[0x87] = function() { cpu.read_modrm_byte();
        var data = cpu.read_write_e32(); cpu.write_e32(cpu.xchg32(data, cpu.modrm_byte));
    };
    
    t[0x88] = function() { cpu.read_modrm_byte(); cpu.set_e8(cpu.read_g8()); };
    t16[0x89] = function() { cpu.read_modrm_byte(); cpu.set_e16(cpu.read_g16()); };
    t32[0x89] = function() { cpu.read_modrm_byte(); cpu.set_e32(cpu.read_g32s()); };
    
    t16[0x8B] = function() { cpu.read_modrm_byte();
        var data = cpu.read_e16();
        cpu.write_g16(data);
    };
    t32[0x8B] = function() { cpu.read_modrm_byte();
        var data = cpu.read_e32s();
        cpu.write_g32(data);
    };
    
    t16[0x8D] = function() { cpu.read_modrm_byte();
        // lea
        if(cpu.modrm_byte >= 0xC0)
        {
            dbg_log("lea #ud", LOG_CPU);
            cpu.trigger_ud();
        }
        var mod = cpu.modrm_byte >> 3 & 7;

        // override prefix, so modrm_resolve does not return the segment part
        cpu.prefixes |= SEG_PREFIX_ZERO;
        cpu.setReg16(mod, cpu.modrm_resolve(cpu.modrm_byte));
        cpu.prefixes = 0;
    };
    t32[0x8D] = function() { cpu.read_modrm_byte();
        if(cpu.modrm_byte >= 0xC0)
        {
            dbg_log("lea #ud", LOG_CPU);
            cpu.trigger_ud();
        }
        var mod = cpu.modrm_byte >> 3 & 7;

        cpu.prefixes |= SEG_PREFIX_ZERO;
        cpu.setReg32s(mod, cpu.modrm_resolve(cpu.modrm_byte));
        cpu.prefixes = 0;
    };
    
    t[0x90] = function() { };

    t16[0x99] = function() { /* cwd */ cpu.setReg16(reg_edx, cpu.getReg16s(reg_eax) >> 15); };
    t32[0x99] = function() { /* cdq */ cpu.setReg32s(reg_edx, cpu.getReg32s(reg_eax) >> 31); };
    
    t[0xA0] = function() {
        // mov
        var data = cpu.safe_read8(cpu.read_moffs());
        cpu.setReg8(reg_eax, data);//reg_al
    };
    t16[0xA1] = function() {
        // mov
        var data = cpu.safe_read16(cpu.read_moffs());
        cpu.setReg16(reg_eax, data);//reg_ax
    };
    t32[0xA1] = function() {
        var data = cpu.safe_read32s(cpu.read_moffs());
        cpu.setReg32s(reg_eax, data);
    };
    t[0xA2] = function() {
        // mov
        cpu.safe_write8(cpu.read_moffs(), cpu.getReg8(reg_eax));//reg_al
    };
    t16[0xA3] = function() {
        // mov
        cpu.safe_write16(cpu.read_moffs(), cpu.getReg16(reg_eax));//reg_ax
    };
    t32[0xA3] = function() {
        cpu.safe_write32(cpu.read_moffs(), cpu.getReg32s(reg_eax));
    };

    t16[0xB8] = function() { cpu.setReg16(reg_eax, cpu.read_op16()); };
    t32[0xB8] = function() { cpu.setReg32s(reg_eax, cpu.read_op32s()); };
    t16[0xB9] = function() { cpu.setReg16(reg_ecx, cpu.read_op16()); };
    t32[0xB9] = function() { cpu.setReg32s(reg_ecx, cpu.read_op32s()); };
    t16[0xBA] = function() { cpu.setReg16(reg_edx, cpu.read_op16()); };
    t32[0xBA] = function() { cpu.setReg32s(reg_edx, cpu.read_op32s()); };
    
    t16[0xC1] = function() { cpu.read_modrm_byte();
        var op1 = cpu.read_write_e16();
        var op2 = cpu.read_op8() & 31;
        var result = 0;
        switch(cpu.modrm_byte >> 3 & 7)
        {
            case 0: result = cpu.rol16(op1, op2); break;
            case 1: result = cpu.ror16(op1, op2); break;
            case 2: result = cpu.rcl16(op1, op2); break;
            case 3: result = cpu.rcr16(op1, op2); break;
            case 4: result = cpu.shl16(op1, op2); break;
            case 5: result = cpu.shr16(op1, op2); break;
            case 6: result = cpu.shl16(op1, op2); break;
            case 7: result = cpu.sar16(op1, op2); break;
        }
        cpu.write_e16(result);
    };
    t32[0xC1] = function() { cpu.read_modrm_byte();
        var op1 = cpu.read_write_e32();
        var op2 = cpu.read_op8() & 31;
        var result = 0;
        switch(cpu.modrm_byte >> 3 & 7)
        {
            case 0: result = cpu.rol32(op1, op2); break;
            case 1: result = cpu.ror32(op1, op2); break;
            case 2: result = cpu.rcl32(op1, op2); break;
            case 3: result = cpu.rcr32(op1, op2); break;
            case 4: result = cpu.shl32(op1, op2); break;
            case 5: result = cpu.shr32(op1, op2); break;
            case 6: result = cpu.shl32(op1, op2); break;
            case 7: result = cpu.sar32(op1, op2); break;
        }
        cpu.write_e32(result);
    };
    
    t16[0xC2] = function() {
        // retn
        var imm16 = cpu.read_op16();

        cpu.set_eip(cpu.pop32s());//cpu.pop16() | 0);//cpu.mem_eip = cpu.get_seg(reg_cs) + cpu.pop16() | 0;
        dbg_assert(cpu.is_asize_32() || cpu.get_eip() < 0x10000);
        cpu.adjust_stack_reg(imm16);
        cpu.diverged();
    };
    t32[0xC2] = function() {
        // retn
        var imm16 = cpu.read_op16();
        var ip = cpu.pop32s();

        dbg_assert(cpu.is_asize_32() || ip < 0x10000);
        cpu.set_eip(ip | 0);//cpu.mem_eip = cpu.get_seg(reg_cs) + ip | 0;
        cpu.adjust_stack_reg(imm16);
        cpu.diverged();
    };
    t16[0xC3] = function() {
        // retn
        cpu.set_eip(cpu.pop32s());//cpu.pop16() | 0);//cpu.mem_eip = cpu.get_seg(reg_cs) + cpu.pop16() | 0;
        cpu.diverged();
    };
    t32[0xC3] = function() {
        // retn
        var ip = cpu.pop32s();
        dbg_assert(cpu.is_asize_32() || ip < 0x10000);
        cpu.set_eip(ip | 0);//cpu.mem_eip = cpu.get_seg(reg_cs) + ip | 0;
        cpu.diverged();
    };

    t16[0xC9] = function() {
        // leave
        var old_vbp = cpu.stack_size_32 ? cpu.getReg32s(reg_ebp) : cpu.getReg16(reg_ebp);
        var new_bp = cpu.safe_read16(cpu.get_seg(reg_ss) + old_vbp | 0);
        cpu.set_stack_reg(old_vbp + 2 | 0);
        cpu.setReg16(reg_ebp, new_bp);
    };
    t32[0xC9] = function() {
        var old_vbp = cpu.stack_size_32 ? cpu.getReg32s(reg_ebp) : cpu.getReg16(reg_ebp);
        var new_ebp = cpu.safe_read32s(cpu.get_seg(reg_ss) + old_vbp | 0);
        cpu.set_stack_reg(old_vbp + 4 | 0);
        cpu.setReg32s(reg_ebp, new_ebp);
    };
    
    t16[0xD3] = function() { cpu.read_modrm_byte();
        var op1 = cpu.read_write_e16();
        var op2 = cpu.getReg8(reg_ecx) & 31;
        var result = 0;
        switch(cpu.modrm_byte >> 3 & 7)
        {
            case 0: result = cpu.rol16(op1, op2); break;
            case 1: result = cpu.ror16(op1, op2); break;
            case 2: result = cpu.rcl16(op1, op2); break;
            case 3: result = cpu.rcr16(op1, op2); break;
            case 4: result = cpu.shl16(op1, op2); break;
            case 5: result = cpu.shr16(op1, op2); break;
            case 6: result = cpu.shl16(op1, op2); break;
            case 7: result = cpu.sar16(op1, op2); break;
        }
        cpu.write_e16(result);
    };
    t32[0xD3] = function() { cpu.read_modrm_byte();
        var op1 = cpu.read_write_e32();
        var op2 = cpu.getReg8(reg_ecx) & 31;
        var result = 0;
        switch(cpu.modrm_byte >> 3 & 7)
        {
            case 0: result = cpu.rol32(op1, op2); break;
            case 1: result = cpu.ror32(op1, op2); break;
            case 2: result = cpu.rcl32(op1, op2); break;
            case 3: result = cpu.rcr32(op1, op2); break;
            case 4: result = cpu.shl32(op1, op2); break;
            case 5: result = cpu.shr32(op1, op2); break;
            case 6: result = cpu.shl32(op1, op2); break;
            case 7: result = cpu.sar32(op1, op2); break;
        }
        cpu.write_e32(result);
    };
    
    t[0xD8] = function() { cpu.read_modrm_byte();
        //cpu.task_switch_test();
        if(cpu.modrm_byte < 0xC0)
            cpu.fpu.op_D8_mem(cpu.modrm_byte, cpu.modrm_resolve(cpu.modrm_byte));
        else
            cpu.fpu.op_D8_reg(cpu.modrm_byte);
    };
    t[0xD9] = function() { cpu.read_modrm_byte();
        //cpu.task_switch_test();
        if(cpu.modrm_byte < 0xC0)
            cpu.fpu.op_D9_mem(cpu.modrm_byte, cpu.modrm_resolve(cpu.modrm_byte));
        else
            cpu.fpu.op_D9_reg(cpu.modrm_byte);
    };
    t[0xDA] = function() { cpu.read_modrm_byte();
        //cpu.task_switch_test();
        if(cpu.modrm_byte < 0xC0)
            cpu.fpu.op_DA_mem(cpu.modrm_byte, cpu.modrm_resolve(cpu.modrm_byte));
        else
            cpu.fpu.op_DA_reg(cpu.modrm_byte);
    };
    t[0xDB] = function() { cpu.read_modrm_byte();
        //cpu.task_switch_test();
        if(cpu.modrm_byte < 0xC0)
            cpu.fpu.op_DB_mem(cpu.modrm_byte, cpu.modrm_resolve(cpu.modrm_byte));
        else
            cpu.fpu.op_DB_reg(cpu.modrm_byte);
    };
    t[0xDC] = function() { cpu.read_modrm_byte();
        //cpu.task_switch_test();
        if(cpu.modrm_byte < 0xC0)
            cpu.fpu.op_DC_mem(cpu.modrm_byte, cpu.modrm_resolve(cpu.modrm_byte));
        else
            cpu.fpu.op_DC_reg(cpu.modrm_byte);
    };
    t[0xDD] = function() { cpu.read_modrm_byte();
        //cpu.task_switch_test();
        if(cpu.modrm_byte < 0xC0)
            cpu.fpu.op_DD_mem(cpu.modrm_byte, cpu.modrm_resolve(cpu.modrm_byte));
        else
            cpu.fpu.op_DD_reg(cpu.modrm_byte);
    };
    t[0xDE] = function() { cpu.read_modrm_byte();
        //cpu.task_switch_test();
        if(cpu.modrm_byte < 0xC0)
            cpu.fpu.op_DE_mem(cpu.modrm_byte, cpu.modrm_resolve(cpu.modrm_byte));
        else
            cpu.fpu.op_DE_reg(cpu.modrm_byte);
    };
    t[0xDF] = function() { cpu.read_modrm_byte();
        //cpu.task_switch_test();
        if(cpu.modrm_byte < 0xC0)
            cpu.fpu.op_DF_mem(cpu.modrm_byte, cpu.modrm_resolve(cpu.modrm_byte));
        else
            cpu.fpu.op_DF_reg(cpu.modrm_byte);
    };
    
    t16[0xE8] = function() {
        // call
        var imm16 = cpu.read_op16();
        cpu.exec_call(cpu.get_eip() + imm16);
        cpu.diverged();
    };
    t32[0xE8] = function() {
        // call
        var imm32s = cpu.read_op32s();
        cpu.exec_call(cpu.get_eip() + imm32s);
        cpu.diverged();
    };
    t16[0xE9] = function() {
        // jmp
        var imm16 = cpu.read_op16();
        mem_eip = mem_eip + imm16 | 0;//cpu.jmp_rel16(imm16);
        cpu.diverged();
    };
    t32[0xE9] = function() {
        // jmp
        var imm32s = cpu.read_op32s();
        cpu.mem_eip = cpu.mem_eip + imm32s | 0;
        dbg_assert(cpu.is_asize_32() || cpu.get_eip() < 0x10000);
        cpu.diverged();
    };
    
    t[0xEB] = function() {
        // jmp near
        var imm8 = cpu.read_op8s();
        cpu.mem_eip = cpu.mem_eip + imm8 | 0;
        dbg_assert(cpu.is_asize_32() || cpu.get_eip() < 0x10000);
        cpu.diverged();
    };
    
    t[0xF6] = function() { cpu.read_modrm_byte();
        switch(cpu.modrm_byte >> 3 & 7)
        {
            case 0:
                var data = cpu.read_e8(); cpu.test8(data, cpu.read_op8());
                break;
            case 1:
                var data = cpu.read_e8(); cpu.test8(data, cpu.read_op8());
                break;
            case 2:
                var data = cpu.read_write_e8(); cpu.write_e8(~(data));
                break;
            case 3:
                var data = cpu.read_write_e8(); cpu.write_e8(cpu.neg8(data));
                break;
            case 4:
                var data = cpu.read_e8(); cpu.mul8(data);
                break;
            case 5:
                var data = cpu.read_e8s(); cpu.imul8(data);
                break;
            case 6:
                var data = cpu.read_e8(); cpu.div8(data);
                break;
            case 7:
                var data = cpu.read_e8s(); cpu.idiv8(data);
                break;
        }
    };
    
    t16[0xF7] = function() { cpu.read_modrm_byte();
        switch(cpu.modrm_byte >> 3 & 7)
        {
            case 0:
                var data = cpu.read_e16(); cpu.test16(data, cpu.read_op16());
                break;
            case 1:
                var data = cpu.read_e16(); cpu.test16(data, cpu.read_op16());
                break;
            case 2:
                var data = cpu.read_write_e16(); cpu.write_e16(~(data));
                break;
            case 3:
                var data = cpu.read_write_e16(); cpu.write_e16(cpu.neg16(data));
                break;
            case 4:
                var data = cpu.read_e16(); cpu.mul16(data);
                break;
            case 5:
                var data = cpu.read_e16s(); cpu.imul16(data);
                break;
            case 6:
                var data = cpu.read_e16(); cpu.div16(data);
                break;
            case 7:
                var data = cpu.read_e16s(); cpu.idiv16(data);
                break;
        }
    };
    t32[0xF7] = function() { cpu.read_modrm_byte();
        switch(cpu.modrm_byte >> 3 & 7)
        {
            case 0:
                var data = cpu.read_e32s(); cpu.test32(data, cpu.read_op32s());
                break;
            case 1:
                var data = cpu.read_e32s(); cpu.test32(data, cpu.read_op32s());
                break;
            case 2:
                var data = cpu.read_write_e32(); cpu.write_e32(~(data));
                break;
            case 3:
                var data = cpu.read_write_e32(); cpu.write_e32(cpu.neg32(data));
                break;
            case 4:
                var data = cpu.read_e32(); cpu.mul32(data);
                break;
            case 5:
                var data = cpu.read_e32s(); cpu.imul32(data);
                break;
            case 6:
                var data = cpu.read_e32(); cpu.div32(data);
                break;
            case 7:
                var data = cpu.read_e32s(); cpu.idiv32(data);
                break;
        }
    };
    
    t16[0xFF] = function() { cpu.read_modrm_byte();
        switch(cpu.modrm_byte >> 3 & 7)
        {
            case 0:
                var data = cpu.read_write_e16(); cpu.write_e16(cpu.inc16(data));
                break;
            case 1:
                var data = cpu.read_write_e16(); cpu.write_e16(cpu.dec16(data));
                break;
            case 2:
                // 2, call near
                cpu.exec_call(cpu.read_e16());
                cpu.diverged();
                break;
            /*case 3:
                // 3, callf
                if(cpu.modrm_byte >= 0xC0)
                {
                    dbg_log("callf #ud", LOG_CPU);
                    cpu.trigger_ud();
                    dbg_assert(false, "unreachable");
                }

                var virt_addr = cpu.modrm_resolve(cpu.modrm_byte);
                var new_ip = cpu.safe_read16(virt_addr);
                var new_cs = cpu.safe_read16(virt_addr + 2 | 0);

                cpu.far_jump(new_ip, new_cs, true);
                dbg_assert(cpu.is_asize_32() || cpu.get_eip() < 0x10000);
                cpu.diverged();
                break;*/
            case 4:
                // 4, jmp near
                var data = cpu.read_e16();
                if (!cpu.exec_check_jump_function(data))
                {
                    cpu.set_eip(data | 0);
                }
                cpu.diverged();
                break;
            /*case 5:
                // 5, jmpf
                if(cpu.modrm_byte >= 0xC0)
                {
                    dbg_log("jmpf #ud", LOG_CPU);
                    cpu.trigger_ud();
                    dbg_assert(false, "unreachable");
                }

                var virt_addr = cpu.modrm_resolve(cpu.modrm_byte);
                var new_ip = cpu.safe_read16(virt_addr);
                var new_cs = cpu.safe_read16(virt_addr + 2 | 0);

                cpu.far_jump(new_ip, new_cs, false);
                dbg_assert(cpu.is_asize_32() || cpu.get_eip() < 0x10000);
                cpu.diverged();
                break;*/
            case 6:
                // 6, push
                var data = cpu.read_e16();
                cpu.push16(data);
                break;
            case 7:
                cpu.todo();
        }
    };
    t32[0xFF] = function() { cpu.read_modrm_byte();
        switch(cpu.modrm_byte >> 3 & 7)
        {
            case 0:
                var data = cpu.read_write_e32(); cpu.write_e32(cpu.inc32(data));
                break;
            case 1:
                var data = cpu.read_write_e32(); cpu.write_e32(cpu.dec32(data));
                break;
            case 2:
                // 2, call near
                cpu.exec_call(cpu.read_e32s());
                cpu.diverged();
                break;
            /*case 3:
                // 3, callf
                if(cpu.modrm_byte >= 0xC0)
                {
                    dbg_log("callf #ud", LOG_CPU);
                    cpu.trigger_ud();
                    dbg_assert(false, "unreachable");
                }

                var virt_addr = cpu.modrm_resolve(cpu.modrm_byte);
                var new_ip = cpu.safe_read32s(virt_addr);
                var new_cs = cpu.safe_read16(virt_addr + 4 | 0);

                if(!cpu.protected_mode || cpu.vm86_mode())
                {
                    if(new_ip & 0xFFFF0000)
                    {
                        throw cpu.debug.unimpl("#GP handler");
                    }
                }

                cpu.far_jump(new_ip, new_cs, true);
                dbg_assert(cpu.is_asize_32() || new_ip < 0x10000);
                cpu.diverged();
                break;*/
            case 4:
                // 4, jmp near
                var data = cpu.read_e32s();
                if (!cpu.exec_check_jump_function(data))
                {
                    cpu.set_eip(data | 0);
                }
                cpu.diverged();
                break;
            /*case 5:
                // 5, jmpf
                if(cpu.modrm_byte >= 0xC0)
                {
                    dbg_log("jmpf #ud", LOG_CPU);
                    cpu.trigger_ud();
                    dbg_assert(false, "unreachable");
                }

                var virt_addr = cpu.modrm_resolve(cpu.modrm_byte);
                var new_ip = cpu.safe_read32s(virt_addr);
                var new_cs = cpu.safe_read16(virt_addr + 4 | 0);

                if(!cpu.protected_mode || cpu.vm86_mode())
                {
                    if(new_ip & 0xFFFF0000)
                    {
                        throw cpu.debug.unimpl("#GP handler");
                    }
                }

                cpu.far_jump(new_ip, new_cs, false);
                dbg_assert(cpu.is_asize_32() || new_ip < 0x10000);
                cpu.diverged();
                break;*/
            case 6:
                // push
                var data = cpu.read_e32s();
                cpu.push32(data);
                break;
            case 7:
                cpu.todo();
        }
    };
    
    var table16 = [];
    var table32 = [];
    cpu.table16 = table16;
    cpu.table32 = table32;
    
    for(var i = 0; i < 256; i++)
    {
        if(t[i])
        {
            //dbg_assert(!t16[i]);
            //dbg_assert(!t32[i]);
            table16[i] = table32[i] = t[i];
        }
        else if(t16[i])
        {
            //dbg_assert(!t[i]);
            //dbg_assert(t32[i]);
            table16[i] = t16[i];
            table32[i] = t32[i];
        }
    }
    
    t = [];
    t16 = [];
    t32 = [];
    
    // 0F ops start here

    // jmpcc
    t16[0x80] = function() { cpu.jmpcc16( cpu.test_o()); };
    t32[0x80] = function() { cpu.jmpcc32( cpu.test_o()); };
    t16[0x81] = function() { cpu.jmpcc16(!cpu.test_o()); };
    t32[0x81] = function() { cpu.jmpcc32(!cpu.test_o()); };
    t16[0x82] = function() { cpu.jmpcc16( cpu.test_b()); };
    t32[0x82] = function() { cpu.jmpcc32( cpu.test_b()); };
    t16[0x83] = function() { cpu.jmpcc16(!cpu.test_b()); };
    t32[0x83] = function() { cpu.jmpcc32(!cpu.test_b()); };
    t16[0x84] = function() { cpu.jmpcc16( cpu.test_z()); };
    t32[0x84] = function() { cpu.jmpcc32( cpu.test_z()); };
    t16[0x85] = function() { cpu.jmpcc16(!cpu.test_z()); };
    t32[0x85] = function() { cpu.jmpcc32(!cpu.test_z()); };
    t16[0x86] = function() { cpu.jmpcc16( cpu.test_be()); };
    t32[0x86] = function() { cpu.jmpcc32( cpu.test_be()); };
    t16[0x87] = function() { cpu.jmpcc16(!cpu.test_be()); };
    t32[0x87] = function() { cpu.jmpcc32(!cpu.test_be()); };
    t16[0x88] = function() { cpu.jmpcc16( cpu.test_s()); };
    t32[0x88] = function() { cpu.jmpcc32( cpu.test_s()); };
    t16[0x89] = function() { cpu.jmpcc16(!cpu.test_s()); };
    t32[0x89] = function() { cpu.jmpcc32(!cpu.test_s()); };
    t16[0x8A] = function() { cpu.jmpcc16( cpu.test_p()); };
    t32[0x8A] = function() { cpu.jmpcc32( cpu.test_p()); };
    t16[0x8B] = function() { cpu.jmpcc16(!cpu.test_p()); };
    t32[0x8B] = function() { cpu.jmpcc32(!cpu.test_p()); };
    t16[0x8C] = function() { cpu.jmpcc16( cpu.test_l()); };
    t32[0x8C] = function() { cpu.jmpcc32( cpu.test_l()); };
    t16[0x8D] = function() { cpu.jmpcc16(!cpu.test_l()); };
    t32[0x8D] = function() { cpu.jmpcc32(!cpu.test_l()); };
    t16[0x8E] = function() { cpu.jmpcc16( cpu.test_le()); };
    t32[0x8E] = function() { cpu.jmpcc32( cpu.test_le()); };
    t16[0x8F] = function() { cpu.jmpcc16(!cpu.test_le()); };
    t32[0x8F] = function() { cpu.jmpcc32(!cpu.test_le()); };
    
    // setcc
    t[0x90] = function() { cpu.read_modrm_byte(); cpu.setcc( cpu.test_o()); };
    t[0x91] = function() { cpu.read_modrm_byte(); cpu.setcc(!cpu.test_o()); };
    t[0x92] = function() { cpu.read_modrm_byte(); cpu.setcc( cpu.test_b()); };
    t[0x93] = function() { cpu.read_modrm_byte(); cpu.setcc(!cpu.test_b()); };
    t[0x94] = function() { cpu.read_modrm_byte(); cpu.setcc( cpu.test_z()); };
    t[0x95] = function() { cpu.read_modrm_byte(); cpu.setcc(!cpu.test_z()); };
    t[0x96] = function() { cpu.read_modrm_byte(); cpu.setcc( cpu.test_be()); };
    t[0x97] = function() { cpu.read_modrm_byte(); cpu.setcc(!cpu.test_be()); };
    t[0x98] = function() { cpu.read_modrm_byte(); cpu.setcc( cpu.test_s()); };
    t[0x99] = function() { cpu.read_modrm_byte(); cpu.setcc(!cpu.test_s()); };
    t[0x9A] = function() { cpu.read_modrm_byte(); cpu.setcc( cpu.test_p()); };
    t[0x9B] = function() { cpu.read_modrm_byte(); cpu.setcc(!cpu.test_p()); };
    t[0x9C] = function() { cpu.read_modrm_byte(); cpu.setcc( cpu.test_l()); };
    t[0x9D] = function() { cpu.read_modrm_byte(); cpu.setcc(!cpu.test_l()); };
    t[0x9E] = function() { cpu.read_modrm_byte(); cpu.setcc( cpu.test_le()); };
    t[0x9F] = function() { cpu.read_modrm_byte(); cpu.setcc(!cpu.test_le()); };
    
    t16[0xAF] = function() { cpu.read_modrm_byte();
        var data = cpu.read_e16s();
        cpu.write_g16(cpu.imul_reg16(cpu.read_g16s(), data));
    };
    t32[0xAF] = function() { cpu.read_modrm_byte();
        var data = cpu.read_e32s();
        cpu.write_g32(cpu.imul_reg32(cpu.read_g32s(), data));
    };
    
    t16[0xB6] = function() { cpu.read_modrm_byte();
        // movzx
        var data = cpu.read_e8();
        cpu.write_g16(data);
    };
    t32[0xB6] = function() { cpu.read_modrm_byte();
        var data = cpu.read_e8();
        cpu.write_g32(data);
    };
    
    t16[0xB7] = function() { cpu.read_modrm_byte();
        // movzx
        dbg_assert(false, "Possibly invalid encoding");
        var data = cpu.read_e16();
        cpu.write_g16(data);
    };
    t32[0xB7] = function() { cpu.read_modrm_byte();
        var data = cpu.read_e16();
        cpu.write_g32(data);
    };
    
    t16[0xBD] = function() { cpu.read_modrm_byte();
        var data = cpu.read_e16();
        cpu.write_g16(cpu.bsr16(cpu.read_g16(), data));
    };
    t32[0xBD] = function() { cpu.read_modrm_byte();
        var data = cpu.read_e32s();
        cpu.write_g32(cpu.bsr32(cpu.read_g32s(), data));
    };
    
    t16[0xBE] = function() { cpu.read_modrm_byte();
        // movsx
        var data = cpu.read_e8s();
        cpu.write_g16(data);
    };
    t32[0xBE] = function() { cpu.read_modrm_byte();
        var data = cpu.read_e8s();
        cpu.write_g32(data);
    };
    
    t16[0xBF] = function() { cpu.read_modrm_byte();
        // movsx
        dbg_assert(false, "Possibly invalid encoding");
        var data = cpu.read_e16();
        cpu.write_g16(data);
    };
    
    t32[0xBF] = function() { cpu.read_modrm_byte();
        var data = cpu.read_e16s();
        cpu.write_g32(data);
    };
    
    var table0F_16 = [];
    var table0F_32 = [];
    cpu.table0F_16 = table0F_16;
    cpu.table0F_32 = table0F_32;
    
    for(i = 0; i < 256; i++)
    {
        if(t[i])
        {
            //dbg_assert(!t16[i]);
            //dbg_assert(!t32[i]);
            table0F_16[i] = table0F_32[i] = t[i];
        }
        else if(t16[i])
        {
            //dbg_assert(!t[i]);
            //dbg_assert(t32[i]);
            table0F_16[i] = t16[i];
            table0F_32[i] = t32[i];
        }
    }
}