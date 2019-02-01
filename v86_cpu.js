/** @constructor */
function CPU()
{
    // current operand/address size
    /** @type {boolean} */
    this.is_32 = false;
    
    /** @type {number} */
    this.esp_phys = 0;
    
    /** @type {number} */
    this.prefixes = 0;
    
    /** @type {number} */
    this.flags = 0;
    
    /**
     * bitmap of flags which are not updated in the flags variable
     * changed by arithmetic instructions, so only relevant to arithmetic flags
     * @type {number}
     */
    this.flags_changed = 0;
    
    /**
     * the last 2 operators and the result and size of the last arithmetic operation
     * @type {number}
     */
    this.last_op1 = 0;
    /** @type {number} */
    this.last_op2 = 0;
    /** @type {number} */
    this.last_op_size = 0;
    
    /** @type {number} */
    this.last_add_result = 0;

    /** @type {number} */
    this.last_result = 0;
    
    if (useTypedArrays)
    {
        this.mul32_result = new Int32Array(2);
        this.div32_result = new Float64Array(2);
    }
    else
    {
        this.mul32_result = [0, 0];
        this.div32_result = [0, 0];
    }
    
    /** @type {number} */
    this.modrm_byte = 0;
    
    /** @type {number} */
    this.phys_addr = 0;
    
    /** @type {number} */
    this.phys_addr_high = 0;
    
    this.table = [];
    
    /** @type {number} */
    this.previous_mem_eip = 0;
    
    /**
     * @type {number}
     */
    this.timestamp_counter = 0;
    
    // registers
    this.reg32 = [0, 0, 0, 0, 0, 0, 0, 0];
    
    this.fpu = undefined;
    
    this.table16 = undefined;
    this.table32 = undefined;
    this.table0F_16 = undefined;
    this.table0F_32 = undefined;
    genTables(this);
    
    dbg_assert(this.table16 && this.table32);    
    dbg_assert(this.table0F_16 && this.table0F_32);
    
    this.update_operand_size();
    
    this.shared_ctor();
}

CPU.prototype.reset = function()
{
    for(var i = 0; i < 8; i++)
    {
        this.reg32[i] = 0;
    }

    this.is_32 = true;//false;
    this.stack_size_32 = true;//false;
    this.prefixes = 0;

    this.update_operand_size();

    this.timestamp_counter = 0;
    this.previous_mem_eip = 0;

    this.flags = flags_default;
    this.flags_changed = 0;

    this.last_result = 0;
    this.last_add_result = 0;
    this.last_op1 = 0;
    this.last_op2 = 0;
    this.last_op_size = 0;

    this.mem_eip = 0xFFFF0;
};

CPU.prototype.init = function()
{
    this.reset();
    this.shared_init();
    
    this.fpu = new FPU(this);
};

CPU.prototype.destroy = function()
{
    this.shared_destroy();
};

/**
 * execute a single instruction cycle on the cpu
 * this includes reading all prefixes and the whole instruction
 */
CPU.prototype.execute_instruction = function()
{
    this.check_stack_memory();
    
    this.previous_mem_eip = this.mem_eip;

    this.timestamp_counter++;

    var opcode = this.read_imm8();
    //webcLog(this.get_eip().toString(16) + " opcode:" + opcode.toString(16) + " esp:" + this.get_esp());

    try
    {
        // call the instruction
        this.table[opcode](this);
    }
    catch (error)
    {
        if (opcode === undefined)
        {
            throw "Failed to read next instruction at EIP " + this.get_eip().toString(16);
        }
        else if (this.table[opcode] === undefined)
        {
            throw "Unhandled opcode " + opcode.toString(16);
        }
        else
        {
            throw "An error occured whilst processing an instruction. opcode: " + 
                opcode.toString(16) + " EIP: " + this.get_eip() + " error: " + error;
        }
    }
};

CPU.prototype.segment_prefix_op = function(sreg)
{
    dbg_assert(sreg <= 5);
    this.prefixes |= sreg + 1;
    this.run_prefix_instruction();
    this.prefixes = 0;
};

CPU.prototype.run_prefix_instruction = function()
{
    if(this.is_osize_32())
    {
        this.table32[this.read_imm8()](this);
    }
    else
    {
        this.table16[this.read_imm8()](this);
    }
};

CPU.prototype.clear_prefixes = function()
{
    this.prefixes = 0;
};

CPU.prototype.read_imm8 = function()
{
    var data8 = this.read8(this.mem_eip);
    this.mem_eip = this.mem_eip + 1 | 0;

    return data8;
};

CPU.prototype.read_imm8s = function()
{
    return (this.read_imm8() << 24) >> 24;
};

CPU.prototype.read_imm16 = function()
{
    var data16 = this.read16(this.mem_eip);
    this.mem_eip = this.mem_eip + 2 | 0;

    return data16;
};

CPU.prototype.read_imm32s = function()
{
    var data32 = this.read32s(this.mem_eip);
    this.mem_eip = this.mem_eip + 4 | 0;

    return data32;
};

CPU.prototype.read_modrm_byte = function()
{
    this.modrm_byte = this.read_imm8();
};

CPU.prototype.read_op0F = CPU.prototype.read_imm8;
CPU.prototype.read_sib = CPU.prototype.read_imm8;
CPU.prototype.read_op8 = CPU.prototype.read_imm8;
CPU.prototype.read_op8s = CPU.prototype.read_imm8s;
CPU.prototype.read_op16 = CPU.prototype.read_imm16;
CPU.prototype.read_op32s = CPU.prototype.read_imm32s;
CPU.prototype.read_disp8 = CPU.prototype.read_imm8;
CPU.prototype.read_disp8s = CPU.prototype.read_imm8s;
CPU.prototype.read_disp16 = CPU.prototype.read_imm16;
CPU.prototype.read_disp32s = CPU.prototype.read_imm32s;

CPU.prototype.init2 = function () {};
CPU.prototype.branch_taken = function () {};
CPU.prototype.branch_not_taken = function () {};
CPU.prototype.diverged = function () {};

CPU.prototype.modrm_resolve = function(modrm_byte)
{
    dbg_assert(modrm_byte < 0xC0);

    return (this.is_asize_32() ? this.modrm_table32 : this.modrm_table16)[modrm_byte](this);
};

CPU.prototype.sib_resolve = function(mod)
{
    return this.sib_table[this.read_sib()](this, mod);
};

// read 2 or 4 byte from ip, depending on address size attribute
CPU.prototype.read_moffs = function()
{
    if (this.is_asize_32())
    {
        return this.get_seg_prefix(reg_ds) + this.read_op32s() | 0;
    }
    else
    {
        return this.get_seg_prefix(reg_ds) + this.read_op16() | 0;
    }
};

CPU.prototype.get_eflags = function()
{
    return (this.flags & ~flags_all) | !!this.getcf() | !!this.getpf() << 2 | !!this.getaf() << 4 |
                                  !!this.getzf() << 6 | !!this.getsf() << 7 | !!this.getof() << 11;
};

/**
 * Update the flags register depending on iopl and cpl
 */
CPU.prototype.update_eflags = function(new_flags)
{
    var dont_update = flag_rf | flag_vm | flag_vip | flag_vif,
        clear = ~flag_vip & ~flag_vif & flags_mask;

    this.flags = (new_flags ^ ((this.flags ^ new_flags) & dont_update)) & clear | flags_default;

    this.flags_changed = 0;
};

CPU.prototype.trigger_de = function()
{
    throw "trigger_de";
};

CPU.prototype.trigger_ud = function()
{
    throw "trigger_ud";
};

CPU.prototype.trigger_nm = function()
{
    throw "trigger_nm";
};

CPU.prototype.trigger_ts = function(code)
{
    throw "trigger_ts";
};

CPU.prototype.trigger_gp = function(code)
{
    throw "trigger_gp";
};

CPU.prototype.trigger_np = function(code)
{
    throw "trigger_np";
};

CPU.prototype.trigger_ss = function(code)
{
    throw "trigger_ss";
};

CPU.prototype.todo = function()
{
    if(DEBUG)
    {
        dbg_trace();
        throw "TODO";
    }

    this.trigger_ud();
};

CPU.prototype.undefined_instruction = function()
{
    dbg_assert(false, "Possible fault: undefined instruction");
    this.trigger_ud();
};

CPU.prototype.get_seg_prefix_ds = function()
{
    return this.get_seg_prefix(reg_ds);
};

CPU.prototype.get_seg_prefix_ss = function()
{
    return this.get_seg_prefix(reg_ss);
};

CPU.prototype.get_seg_prefix_cs = function()
{
    return this.get_seg_prefix(reg_cs);
};

/**
 * Get segment base by prefix or default
 * @param {number} default_segment
 */
CPU.prototype.get_seg_prefix = function(default_segment /*, offset*/)
{
    return 0;
};

/**
 * Get segment base
 * @param {number} segment
 */
CPU.prototype.get_seg = function(segment /*, offset*/)
{
    return 0;
};

CPU.prototype.get_reg8 = function(index)
{
    if (index >= 4)
    {
        // AH, CH, DH, BH
        return (this.reg32[index - 4] >> 8) & SIZE_MASK_8;
    }
    return this.reg32[index] & SIZE_MASK_8;
};

CPU.prototype.set_reg8 = function(index, value)
{
    if (index >= 4)
    {
        // AH, CH, DH, BH
        this.reg32[index - 4] &= ~0x0000FF00;// H_MASK
        this.reg32[index - 4] |= ((value >> 8) & SIZE_MASK_8);
    }
    else
    {
        this.reg32[index] &= ~0x000000FF;
        this.reg32[index] |= (value & SIZE_MASK_8);
    }
};

////////////////////////////////////////////////////

CPU.prototype.read_e8 = function()
{
    if(this.modrm_byte < 0xC0) {
        return this.safe_read8(this.modrm_resolve(this.modrm_byte));
    } else {
        return this.get_reg8(this.modrm_byte & 7);
    }
};

CPU.prototype.read_e8s = function()
{
    return this.read_e8() << 24 >> 24;
};

CPU.prototype.read_e16 = function()
{
    if(this.modrm_byte < 0xC0) {
        return this.safe_read16(this.modrm_resolve(this.modrm_byte));
    } else {
        return this.reg32[this.modrm_byte & 7] & SIZE_MASK_16;
    }
};

CPU.prototype.read_e16s = function()
{
    return this.read_e16() << 16 >> 16;
};

CPU.prototype.read_e32s = function()
{
    if(this.modrm_byte < 0xC0) {
        return this.safe_read32s(this.modrm_resolve(this.modrm_byte));
    } else {
        return this.reg32[this.modrm_byte & 7] | 0;
    }
};

CPU.prototype.read_e32 = function()
{
    return this.read_e32s() >>> 0;
};

////////////////////////////////////////////////////

CPU.prototype.set_e8 = function(value)
{
    if(this.modrm_byte < 0xC0) {
        var addr = this.modrm_resolve(this.modrm_byte);
        this.safe_write8(addr, value);
    } else {
        this.set_reg8(this.modrm_byte & 7, value);
    }
};

CPU.prototype.set_e16 = function(value)
{
    if(this.modrm_byte < 0xC0) {
        var addr = this.modrm_resolve(this.modrm_byte);
        this.safe_write16(addr, value);
    } else {
        this.reg32[this.modrm_byte & 7] = value & SIZE_MASK_16;
    }
};

CPU.prototype.set_e32 = function(value)
{
    if(this.modrm_byte < 0xC0) {
        var addr = this.modrm_resolve(this.modrm_byte);
        this.safe_write32(addr, value);
    } else {
        this.reg32[this.modrm_byte & 7] = value;
    }        
};

////////////////////////////////////////////////////

CPU.prototype.read_write_e8 = function()
{
    if(this.modrm_byte < 0xC0) {
        var virt_addr = this.modrm_resolve(this.modrm_byte);
        this.phys_addr = this.translate_address_write(virt_addr);
        return this.read8(this.phys_addr);
    } else {
        return this.get_reg8(this.modrm_byte & 7);
    }
};

CPU.prototype.write_e8 = function(value)
{
    if(this.modrm_byte < 0xC0) {
        this.write8(this.phys_addr, value);
    } else {
        this.set_reg8(this.modrm_byte & 7, value);
    }
};

CPU.prototype.read_write_e16 = function()
{
    if(this.modrm_byte < 0xC0) {
        var virt_addr = this.modrm_resolve(this.modrm_byte);
        this.phys_addr = this.translate_address_write(virt_addr);
        return this.read16(this.phys_addr);
    } else {
        return this.reg32[this.modrm_byte & 7] & SIZE_MASK_16;
    }
};

CPU.prototype.write_e16 = function(value)
{
    if(this.modrm_byte < 0xC0) {
        this.write16(this.phys_addr, value);
    } else {
        this.reg32[this.modrm_byte & 7] = value & SIZE_MASK_16;
    }
};

CPU.prototype.read_write_e32 = function()
{
    if(this.modrm_byte < 0xC0) {
        var virt_addr = this.modrm_resolve(this.modrm_byte);
        this.phys_addr = this.translate_address_write(virt_addr);
        return this.read32s(this.phys_addr);
    } else {
        return this.reg32[this.modrm_byte & 7] | 0;
    }        
};

CPU.prototype.write_e32 = function(value)
{
    if(this.modrm_byte < 0xC0) {
        this.write32(this.phys_addr, value);
    } else {
        this.reg32[this.modrm_byte & 7] = value;
    }
};

////////////////////////////////////////////////////

CPU.prototype.read_reg_e16 = function()
{
    return (this.reg32[this.modrm_byte & 7] << 16) >> 16;
};

CPU.prototype.write_reg_e16 = function(value)
{
    this.reg32[this.modrm_byte & 7] = value & SIZE_MASK_16;
};

CPU.prototype.read_reg_e32s = function()
{
    return this.reg32[this.modrm_byte & 7] | 0;
};

CPU.prototype.write_reg_e32 = function(value)
{
    this.reg32[this.modrm_byte & 7] = value;
};

////////////////////////////////////////////////////

CPU.prototype.read_modrm_byte = function()
{
    this.modrm_byte = this.read_imm8();
};

CPU.prototype.read_g8 = function()
{
    return this.get_reg8(this.modrm_byte >> 3 & 7);
};

CPU.prototype.write_g8 = function(value)
{
    this.set_reg8(this.modrm_byte >> 3 & 7, value);
};

CPU.prototype.read_g16 = function()
{
    return this.reg32[this.modrm_byte >> 3 & 7] & SIZE_MASK_16;
};

CPU.prototype.read_g16s = function()
{
    return (this.reg32[this.modrm_byte >> 3 & 7] << 16) >> 16;
};

CPU.prototype.write_g16 = function(value)
{
    this.reg32[this.modrm_byte >> 3 & 7] = value & SIZE_MASK_16;
};

CPU.prototype.read_g32s = function()
{
    return this.reg32[this.modrm_byte >> 3 & 7] | 0;
};

CPU.prototype.write_g32 = function(value)
{
    this.reg32[this.modrm_byte >> 3 & 7] = value;
};

////////////////////////////////////////////////////

CPU.prototype.update_operand_size = function()
{
    if(this.is_32)
    {
        this.table = this.table32;
    }
    else
    {
        this.table = this.table16;
    }
};

CPU.prototype.is_osize_32 = function()
{
    return this.is_32 !== ((this.prefixes & PREFIX_MASK_OPSIZE) === PREFIX_MASK_OPSIZE);
};

CPU.prototype.is_asize_32 = function()
{
    return this.is_32 !== ((this.prefixes & PREFIX_MASK_ADDRSIZE) === PREFIX_MASK_ADDRSIZE);
};

CPU.prototype.get_reg_asize = function(reg)
{
    dbg_assert(reg === reg_ecx || reg === reg_esi || reg === reg_edi);
    var r = this.reg32[reg];

    if(this.is_asize_32())
    {
        return r;
    }
    else
    {
        return r & SIZE_MASK_16;
    }
};

CPU.prototype.set_ecx_asize = function(value)
{
    if(this.is_asize_32())
    {
        this.reg32[reg_ecx] = value & SIZE_MASK_32;
    }
    else
    {
        this.reg32[reg_ecx] = value & SIZE_MASK_16;
    }
};

CPU.prototype.add_reg_asize = function(reg, value)
{
    dbg_assert(reg === reg_ecx || reg === reg_esi || reg === reg_edi);
    if(this.is_asize_32())
    {
        this.reg32[reg] = (this.reg32[reg] + (value & SIZE_MASK_32)) & SIZE_MASK_32;
    }
    else
    {
        this.reg32[reg] = ((this.reg32[reg] & SIZE_MASK_16) + (value & SIZE_MASK_16)) & SIZE_MASK_16;
    }
};

CPU.prototype.decr_ecx_asize = function()
{
    if(this.is_asize_32())
    {
        this.reg32[reg] = (this.reg32[reg] + 1) & SIZE_MASK_32;
    }
    else
    {
        this.reg32[reg] = ((this.reg32[reg] & SIZE_MASK_16) + 1) & SIZE_MASK_16;
    }
};

///////////////////////////////////////////////////////////////////////
// misc_instr.js merged into this file as there isnt't much code
///////////////////////////////////////////////////////////////////////

CPU.prototype.jmpcc8 = function(condition)
{
    var imm8 = this.read_op8s();
    if(condition)
    {
        this.mem_eip = this.mem_eip + imm8 | 0;
        this.branch_taken();
    }
    else
    {
        this.branch_not_taken();
    }
};

CPU.prototype.jmp_rel16 = function(rel16)
{
    var current_cs = this.get_seg(reg_cs);

    // limit ip to 16 bit
    // ugly
    this.mem_eip -= current_cs;
    this.mem_eip = (this.mem_eip + rel16) & 0xFFFF;
    this.mem_eip = this.mem_eip + current_cs | 0;
};

CPU.prototype.jmpcc16 = function(condition)
{
    var imm16 = this.read_op16();
    if(condition)
    {
        this.jmp_rel16(imm16);
        this.branch_taken();
    }
    else
    {
        this.branch_not_taken();
    }
};

CPU.prototype.jmpcc32 = function(condition)
{
    var imm32s = this.read_op32s();
    if(condition)
    {
        // don't change to `this.mem_eip += this.read_op32s()`,
        //   since read_op32s modifies mem_eip

        this.mem_eip = this.mem_eip + imm32s | 0;
        this.branch_taken();
    }
    else
    {
        this.branch_not_taken();
    }
};

CPU.prototype.setcc = function(condition)
{
    this.set_e8(condition ? 1 : 0)
};

/**
 * @return {number}
 * @const
 */
CPU.prototype.getcf = function()
{
    if(this.flags_changed & 1)
    {
        return (this.last_op1 ^ (this.last_op1 ^ this.last_op2) & (this.last_op2 ^ this.last_add_result)) >>> this.last_op_size & 1;
    }
    else
    {
        return this.flags & 1;
    }
};

/** @return {number} */
CPU.prototype.getpf = function()
{
    if(this.flags_changed & flag_parity)
    {
        // inverted lookup table
        return 0x9669 << 2 >> ((this.last_result ^ this.last_result >> 4) & 0xF) & flag_parity;
    }
    else
    {
        return this.flags & flag_parity;
    }
};

/** @return {number} */
CPU.prototype.getaf = function()
{
    if(this.flags_changed & flag_adjust)
    {
        return (this.last_op1 ^ this.last_op2 ^ this.last_add_result) & flag_adjust;
    }
    else
    {
        return this.flags & flag_adjust;
    }
};

/** @return {number} */
CPU.prototype.getzf = function()
{
    if(this.flags_changed & flag_zero)
    {
        return (~this.last_result & this.last_result - 1) >>> this.last_op_size & 1;
    }
    else
    {
        return this.flags & flag_zero;
    }
};

/** @return {number} */
CPU.prototype.getsf = function()
{
    if(this.flags_changed & flag_sign)
    {
        return this.last_result >>> this.last_op_size & 1;
    }
    else
    {
        return this.flags & flag_sign;
    }
};

/** @return {number} */
CPU.prototype.getof = function()
{
    if(this.flags_changed & flag_overflow)
    {
        return ((this.last_op1 ^ this.last_add_result) & (this.last_op2 ^ this.last_add_result)) >>> this.last_op_size & 1;
    }
    else
    {
        return this.flags & flag_overflow;
    }
};

CPU.prototype.test_o = CPU.prototype.getof;
CPU.prototype.test_b = CPU.prototype.getcf;
CPU.prototype.test_z = CPU.prototype.getzf;
CPU.prototype.test_s = CPU.prototype.getsf;
CPU.prototype.test_p = CPU.prototype.getpf;

CPU.prototype.test_be = function()
{
    // Idea:
    //    return this.last_op1 <= this.last_op2;
    return this.getcf() || this.getzf();
};

CPU.prototype.test_l = function()
{
    // Idea:
    //    return this.last_add_result < this.last_op2;
    return !this.getsf() !== !this.getof();
};

CPU.prototype.test_le = function()
{
    // Idea:
    //    return this.last_add_result <= this.last_op2;
    return this.getzf() || !this.getsf() !== !this.getof();
};

CPU.prototype.xchg16 = function(memory_data, modrm_byte)
{
    var mod = modrm_byte >> 3 & 7,
        tmp = this.reg32[mod] & SIZE_MASK_16;
    
    this.reg32[mod] = memory_data & SIZE_MASK_16;
    
    return tmp;
};

CPU.prototype.xchg32 = function(memory_data, modrm_byte)
{
    var mod = modrm_byte >> 3 & 7,
        tmp = this.reg32[mod];
    
    this.reg32[mod] = memory_data;
    
    return tmp;
};