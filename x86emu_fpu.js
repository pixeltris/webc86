// FPU code is taken from https://github.com/copy/v86/blob/master/src/fpu.js

var fpu_indefinite_nan = NaN;

function FPU(cpu)
{
    this.fpu_init();
    this.cpu = cpu;
}

FPU.prototype.fpu_init = function()
{
    if (!useTypedArrays)
    {
        return;
    }
    
    this.fpu_st = new Float64Array(8);
    this.fpu_stack_empty = 0xFF;
    this.fpu_stack_ptr = 0;
    this.fpu_control_word = 0x37F;
    this.fpu_status_word = 0xFF;

    this.fpu_float32 = new Float32Array(1);
    this.fpu_float32_byte = new Uint8Array(this.fpu_float32.buffer);
    this.fpu_float32_int = new Int32Array(this.fpu_float32.buffer);
    this.fpu_float64 = new Float64Array(1);
    this.fpu_float64_byte = new Uint8Array(this.fpu_float64.buffer);
    this.fpu_float64_int = new Int32Array(this.fpu_float64.buffer);
};

FPU.prototype.fpu_readFloat = function(addr)
{
    if (!useTypedArrays)
    {
        return 0;
    }
    return this.fpu_load_m32(addr);
};

FPU.prototype.fpu_readDouble = function(addr)
{
    if (!useTypedArrays)
    {
        return 0;
    }
    return this.fpu_load_m64(addr);
};

FPU.prototype.fpu_fetchFloat = function()
{
    if (!useTypedArrays)
    {
        return 0;
    }
    var addr = this.cpu.get_eip();
    var result = this.fpu_load_m32(addr);
    this.cpu.mem_eip += 4;
    return result;
};

FPU.prototype.fpu_fetchDouble = function()
{
    if (!useTypedArrays)
    {
        return 0;
    }
    var addr = this.cpu.get_eip();
    var result = this.fpu_load_m64(addr);
    this.cpu.mem_eip += 8;
    return result;
};

FPU.prototype.fpu_load_status_word = function()
{
    return this.fpu_status_word & ~(7 << 11) | this.fpu_stack_ptr << 11;
};

FPU.prototype.fpu_set_status_word = function(sw)
{
    this.fpu_status_word = sw & ~(7 << 11);
    this.fpu_stack_ptr = sw >> 11 & 7;
};

FPU.prototype.fpu_stack_fault = function()
{
    this.fpu_status_word |= (1 << 6) | (1 << 0);//FPU_EX_SF | FPU_EX_I;
    throw "fpu_stack_fault";
};

FPU.prototype.fpu_invalid_arithmatic = function()
{
    this.fpu_status_word |= (1 << 0);//FPU_EX_I;
};

FPU.prototype.fpu_fcom = function(y)
{
    var x = this.fpu_get_st0();
    
    this.fpu_status_word &= ~(0x100 | 0x200 | 0x400 | 0x4000);//~FPU_RESULT_FLAGS;
    
    if (x > y)
    {
    }
    else if (y > x)
    {
        this.fpu_status_word |= 0x100;//FPU_C0;        
    }
    else if (x === y)
    {
        this.fpu_status_word |= 0x4000;//FPU_C3;
    }
    else
    {
        this.fpu_status_word |= (0x100 | 0x400 | 0x4000);//(FPU_C0 | FPU_C2 | FPU_C3);
    }
};

FPU.prototype.fpu_fucom = function(y)
{
    // TODO
    this.fpu_fcom(y);
};

FPU.prototype.fpu_fcomi = function(y)
{
    var x = this.fpu_st[this.fpu_stack_ptr];
    
    this.cpu.cpu_eflags &= ~(1 | 4 | 64);//~(1 | flag_parity | flag_zero);
    if (x > y)
    {        
    }
    else if (y > x)
    {
        this.cpu.cpu_eflags |= 1;
    }
    else if (x === y)
    {
        this.cpu.cpu_eflags |= 64;//64
    }
    else
    {
        this.cpu.cpu_eflags |= (1 | 4 | 64);//1 | flag_parity | flag_zero;
    }
};

FPU.prototype.fpu_fucomi = function(y)
{
    // TODO
    this.fpu_fcomi(y);
};

FPU.prototype.fpu_ftst = function(x)
{
    this.fpu_status_word &= ~(0x100 | 0x200 | 0x400 | 0x4000);//~FPU_RESULT_FLAGS;
    
    if (isNaN(x))
    {
        this.fpu_status_word |= (0x4000 | 0x400 | 0x100);//FPU_C3 | FPU_C2 | FPU_C0;
    }
    else if (x === 0)
    {
        this.fpu_status_word |= 0x4000;//FPU_C3;
    }
    else if (x < 0)
    {
        this.fpu_status_word |= 0x100;
    }
    
    // TODO: unordered (x is nan, etc)
};

FPU.prototype.fpu_integer_round = function(f)
{
    var rc = this.fpu_control_word >> 10 & 3;
    //return this.cpu.integer_round(f, rc);
    
    if (rc === 0)
    {
        // Round to nearest, or even if equidistant
        var rounded = Math.round(f);
        
        if(rounded - f === 0.5 && (rounded % 2))
        {
            // Special case: Math.round rounds to positive infinity
            // if equidistant
            rounded--;
        }
        
        return rounded;
    }
    // rc=3 is truncate -> floor for positive numbers
    else if(rc === 1 || (rc === 3 && f > 0))        
    {
        return Math.floor(f);
    }
    else
    {
        return Math.ceil(f);
    }
};

FPU.prototype.fpu_truncate = function(x)
{
    return x > 0 ? Math.floor(x) : Math.ceil(x);
};

FPU.prototype.fpu_push = function(x)
{
    this.fpu_stack_ptr = this.fpu_stack_ptr - 1 & 7;
    
    if (this.fpu_stack_empty >> this.fpu_stack_ptr & 1)
    {
        this.fpu_status_word &= ~0x200;//~FPU_C1;
        this.fpu_stack_empty &= ~(1 << this.fpu_stack_ptr);
        this.fpu_st[this.fpu_stack_ptr] = x;
    }
    else
    {
        this.fpu_status_word |= 0x200;//FPU_C1;
        this.fpu_stack_fault();
        this.fpu_st[this.fpu_stack_ptr] = fpu_indefinite_nan;
    }
};

FPU.prototype.fpu_pop = function()
{
    this.fpu_stack_empty |= 1 << this.fpu_stack_ptr;
    this.fpu_stack_ptr = this.fpu_stack_ptr + 1 & 7;
};

FPU.prototype.fpu_get_sti = function(i)
{
    //dbg_assert(typeof i === "number" && i >= 0 && i < 8);
    
    i = i + this.fpu_stack_ptr & 7;
    
    if (this.fpu_stack_empty >> i & 1)
    {
        this.fpu_status_word &= ~0x200;//~FPU_C1;
        this.fpu_stack_fault();
        return fpu_indefinite_nan;
    }
    else
    {
        return this.fpu_st[i];
    }
};

FPU.prototype.fpu_get_st0 = function()
{
    if (this.fpu_stack_empty >> this.fpu_stack_ptr & 1)
    {
        this.fpu_status_word &= ~0x200;//~FPU_C1;
        this.fpu_stack_fault();
        return fpu_indefinite_nan;
    }
    else
    {
        return this.fpu_st[this.fpu_stack_ptr];
    }
};

FPU.prototype.fpu_load_m80 = function(addr)
{
    var exponent = this.cpu.readI16(addr + 8),
        sign,
        low = this.cpu.readI32(addr) >>> 0,
        high = this.cpu.readI32(addr + 4) >>> 0;
    
    sign = exponent >> 15;
    exponent &= ~0x8000;
    
    if (exponent === 0)
    {
        // TODO: denormal numbers
        return 0;
    }
    
    if (exponent < 0x7FFF)
    {
        exponent -= 0x3FFF;
    }
    else
    {
        // TODO: NaN, Infinity
        //dbg_log("Load m80 TODO", LOG_FPU);
        this.fpu_float64_byte[7] = 0x7F | sign << 7;
        this.fpu_float64_byte[6] = 0xF0 | high >> 30 << 3 & 0x08;
        
        this.fpu_float64_byte[5] = 0;
        this.fpu_float64_byte[4] = 0;
        
        this.fpu_float64_int[0] = 0;
        
        return this.fpu_float64[0];
    }
    
    // Note: some bits might be lost at this point
    var mantissa = low + 0x100000000 * high;
    
    if (sign)
    {
        mantissa = -mantissa;
    }
    
     return mantissa * Math.pow(2, exponent - 63);
};

FPU.prototype.fpu_store_m80 = function(addr, n)
{
    this.fpu_float64[0] = n;
    
    var sign = this.fpu_float64_byte[7] & 0x80,
        exponent = (this.fpu_float64_byte[7] & 0x7f) << 4 | this.fpu_float64_byte[6] >> 4,
        low,
        high;
    
    if (exponent === 0x7FF)
    {
        // all bits set (NaN and infinity)
        exponent = 0x7FFF;
        low = 0;
        high = 0x80000000 | (this.fpu_float64_int[1] & 0x80000) << 11;
    }
    else if (exponent === 0)
    {
        // zero and denormal numbers
        // Just assume zero for now
        low = 0;
        high = 0;        
    }
    else
    {
        exponent += 0x3FFF - 0x3FF;
        
        // does the mantissa need to be adjusted?
        low = this.fpu_float64_int[0] << 11;
        high = 0x80000000 | (this.fpu_float64_int[1] & 0xFFFFF) << 11 | (this.fpu_float64_int[0] >>> 21);
    }
    
    //dbg_assert(exponent >= 0 && exponent < 0x8000);
    
    this.cpu.writeU32(addr, low);
    this.cpu.writeU32(addr + 4, high);
    
    this.cpu.writeU16(addr + 8, sign << 8 | exponent);
};

FPU.prototype.fpu_load_m64 = function(addr)
{
    var low = this.cpu.readI32(addr),
        high = this.cpu.readI32(addr + 4);
    
    this.fpu_float64_int[0] = low;
    this.fpu_float64_int[1] = high;
    
    return this.fpu_float64[0];
};

FPU.prototype.fpu_store_m64 = function(addr, i)
{
    if (!this.cpu.isValidAddress(addr, 8))
    {
        throw "fpu_store_m64 invalid fpu write at " + addr.toString(16);
    }
    
    this.fpu_float64[0] = this.fpu_get_sti(i);
    
    this.cpu.writeU32(addr, this.fpu_float64_int[0]);
    this.cpu.writeU32(addr + 4, this.fpu_float64_int[1]);
};

FPU.prototype.fpu_load_m32 = function(addr)
{
    this.fpu_float32_int[0] = this.cpu.readI32(addr);
    return this.fpu_float32[0];
};

FPU.prototype.fpu_store_m32 = function(addr, x)
{
    this.fpu_float32[0] = x;
    this.cpu.writeU32(addr, this.fpu_float32_int[0]);
};

FPU.prototype.fpu_op_D8_reg = function(imm8)
{
    var mod = imm8 >> 3 & 7,
        low = imm8 & 7;
        sti = this.fpu_get_sti(low);
        st0 = this.fpu_get_st0();
    
    switch (mod)
    {
        case 0:
            // fadd
            this.fpu_st[this.fpu_stack_ptr] = st0 + sti;
            break;
        case 1:
            // fmul
            this.fpu_st[this.fpu_stack_ptr] = st0 * sti;
            break;
        case 2:
            // fcom
            this.fpu_fcom(sti);
            break;
        case 3:
            // fcomp
            this.fpu_fcom(sti);
            this.fpu_pop();
            break;
        case 4:
            // fsub
            this.fpu_st[this.fpu_stack_ptr] = st0 - sti;
            break;
        case 5:
            // fsubr
            this.fpu_st[this.fpu_stack_ptr] = sti - st0;
            break;
        case 6:
            // fdiv
            this.fpu_st[this.fpu_stack_ptr] = st0 / sti;
            break;
        case 7:
            // fdivr
            this.fpu_st[this.fpu_stack_ptr] = sti / st0;
            break;
        default:
            return 0;
    }
    return 1;
};

FPU.prototype.fpu_op_D8_mem = function(imm8, addr)
{
    var mod = imm8 >> 3 & 7,
        m32 = this.fpu_load_m32(addr);
    
    var st0 = this.fpu_get_st0();
    
    switch (mod)
    {
        case 0:
            // fadd
            this.fpu_st[this.fpu_stack_ptr] = st0 + m32;
            break;
        case 1:
            // fmul
            this.fpu_st[this.fpu_stack_ptr] = st0 * m32;
            break;
        case 2:
            // fcom
            this.fpu_fcom(m32);
            break;
        case 3:
            // fcomp
            this.fpu_fcom(m32);
            this.fpu_pop();
            break;
        case 4:
            // fsub
            this.fpu_st[this.fpu_stack_ptr] = st0 - m32;
            break;
        case 5:
            // fsubr
            this.fpu_st[this.fpu_stack_ptr] = m32 - st0;
            break;
        case 6:
            // fdiv
            this.fpu_st[this.fpu_stack_ptr] = st0 / m32;
            break;
        case 7:
            // fdivr
            this.fpu_st[this.fpu_stack_ptr] = m32 / st0;
            break;
        default:
            return 0;
    }
    return 1;
};

FPU.prototype.fpu_op_D9_reg = function(imm8)
{
    var mod = imm8 >> 3 & 7,
        low = imm8 & 7;
    
    switch (mod)
    {
        case 0:
            // fld
            var sti = this.fpu_get_sti(low);
            this.fpu_push(sti);
            break;
        case 1:
            // fxch
            var sti = this.fpu_get_sti(low);
            
            this.fpu_st[this.fpu_stack_ptr + low & 7] = this.fpu_get_st0();
            this.fpu_st[this.fpu_stack_ptr] = sti;
            break;
        default:
            return 0;
    }
    return 1;
};

FPU.prototype.fpu_op_D9_mem = function(imm8, addr)
{
    var mod = imm8 >> 3 & 7;
    
    switch (mod)
    {
        case 0:
            // fld
            var data = this.fpu_load_m32(addr);
            this.fpu_push(data);
            break;
        case 2:
            // fst
            this.fpu_store_m32(addr, this.fpu_get_st0());
            break;
        case 3:
            // fstp
            this.fpu_store_m32(addr, this.fpu_get_st0());
            this.fpu_pop();
            break;
        default:
            return 0;
    }
    return 1;
};

FPU.prototype.fpu_op_DA_reg = function(imm8)
{
    var mod = imm8 >> 3 & 7,
        low = imm8 & 7;
    
    switch (mod)
    {
        case 5:
            if (low === 1)
            {
                // fucompp
                this.fpu_fcom(this.fpu_get_sti(1));
                this.fpu_pop();
                this.fpu_pop();
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
};

FPU.prototype.fpu_op_DB_mem = function(imm8, addr)
{
    var mod = imm8 >> 3 & 7;
    
    switch (mod)
    {
        case 0:
            // fild
            var int32 = this.cpu.readI32(addr);
            this.fpu_push(int32);
            break;
        case 2:
            // fist
            var st0 = this.fpu_integer_round(this.fpu_get_st0());
            if (st0 <= 0x7FFFFFFF && st0 >= -0x80000000)
            {
                // TODO: Invalid operation
                this.cpu.writeU32(addr, st0);
            }
            else
            {
                this.fpu_invalid_arithmatic();
                this.cpu.writeU32(addr, 0x80000000|0);
            }
            break;
        case 3:
            // fistp
            var st0 = this.fpu_integer_round(this.fpu_get_st0());
            if (st0 <= 0x7FFFFFFF && st0 >= -0x80000000)
            {
                // TODO: Invalid operation
                this.cpu.writeU32(addr, st0);
            }
            else
            {
                this.fpu_invalid_arithmatic();
                this.cpu.writeU32(addr, 0x80000000|0);
            }
            fpu_pop();
            break;
        case 5:
            // fld
            this.fpu_push(this.fpu_load_m80(addr));
            break;
        case 7:
            // fstp
            if (!this.cpu.isValidAddress(addr, 10))
            {
                throw "fpu_op_DB_mem:fstp invalid fpu write at " + addr.toString(16);
            }
            this.fpu_store_m80(addr, this.fpu_get_st0());
            this.fpu_pop();
            break;
        default:
            return 0;
    }
    return 1;
};

FPU.prototype.fpu_op_DC_reg = function(imm8)
{
    var mod = imm8 >> 3 & 7,
        low = imm8 & 7,
        low_ptr = this.fpu_stack_ptr + low & 7,
        sti = this.fpu_get_sti(low),
        st0 = this.fpu_get_st0();
    
    switch (mod)
    {
        case 0:
            // fadd
            this.fpu_st[this.low_ptr] = sti + st0;
            break;
        case 1:
            // fmul
            this.fpu_st[this.low_ptr] = sti * st0;
            break;
        case 2:
            // fcom
            this.fpu_fcom(sti);
            break;
        case 3:
            // fcomp
            this.fpu_fcom(sti);
            this.fpu_pop();
            break;
        case 4:
            // fsubr
            this.fpu_st[this.low_ptr] = st0 - sti;
            break;
        case 5:
            // fsub
            this.fpu_st[this.low_ptr] = sti - st0;
            break;
        case 6:
            // fdivr
            this.fpu_st[this.low_ptr] = st0 / sti;
            break;
        case 7:
            // fdiv
            this.fpu_st[this.low_ptr] = sti / st0;
            break;
        default:
            return 0;
    }
    return 1;
};

FPU.prototype.fpu_op_DC_mem = function(imm8, addr)
{
    var mod = imm8 >> 3 & 7,
        m64 = this.fpu_load_m64(addr);
    
    var st0 = this.fpu_get_st0();
    
    switch (mod)
    {
        case 0:
            // fadd
            this.fpu_st[this.fpu_stack_ptr] = st0 + m64;
            break;
        case 1:
            // fmul
            this.fpu_st[this.fpu_stack_ptr] = st0 * m64;
            break;
        case 2:
            // fcom
            this.fpu_fcom(m64);
            break;
        case 3:
            // fcomp
            this.fpu_fcom(m64);
            this.fpu_pop();
            break;
        case 4:
            // fsub
            this.fpu_st[this.fpu_stack_ptr] = st0 - m64;
            break;
        case 5:
            // fsubr
            this.fpu_st[this.fpu_stack_ptr] = m64 - st0;
            break;
        case 6:
            // fdiv
            this.fpu_st[this.fpu_stack_ptr] = st0 / m64;
            break;
        case 7:
            // fdivr
            this.fpu_st[this.fpu_stack_ptr] = m64 / st0;
            break;
        default:
            return 0;
    }
    return 1;
};

FPU.prototype.fpu_op_DD_reg = function(imm8)
{
    var mod = imm8 >> 3 & 7,
        low = imm8 & 7;
    
    switch (mod)
    {
        case 2:
            // fst
            this.fpu_st[this.fpu_stack_ptr + low & 7] = this.fpu_get_st0();
            break;
        case 3:
            // fstp
            if (low === 0)
            {
                this.fpu_pop();
            }
            else
            {
                this.fpu_st[this.fpu_stack_ptr + low & 7] = this.fpu_get_st0();
                this.fpu_pop();
            }
            break;
        case 4:
            this.fpu_fucom(this.fpu_get_sti(low));
            break;
        case 5:
            // fucomp
            this.fpu_fucom(this.cpu_get_sti(low));
            this.fpu_pop();
            break;
        default:
            return 0;
    }
    return 1;
};

FPU.prototype.fpu_op_DD_mem = function(imm8, addr)
{
    var mod = imm8 >> 3 & 7;
    
    switch (mod)
    {
        case 0:
            // fld
            var data = this.fpu_load_m64(addr);
            this.fpu_push(data);
            break;
        case 2:
            // fst
            this.fpu_store_m64(addr, 0);
            break;
        case 3:
            // fstp
            this.fpu_store_m64(addr, 0);
            this.fpu_pop();
            break;
        default:
            return 0;
    }
    return 1;
};

FPU.prototype.fpu_op_DE_reg = function(imm8)
{
    var mod = imm8 >> 3 & 7,
        low = imm8 & 7,
        low_ptr = this.fpu_stack_ptr + low & 7,
        sti = this.fpu_get_sti(low),
        st0 = this.fpu_get_st0();
    
    switch (mod)
    {
        case 0:
            // faddp
            this.fpu_st[low_ptr] = sti + st0;
            break;
        case 1:
            // fmulp
            this.fpu_st[low_ptr] = sti * st0;
            break;
        case 2:
            // fcomp
            this.fpu_fcom(sti);
            break;
        case 3:
            // fcompp
            if (low === 1)
            {
                this.fpu_fcom(this.fpu_st[low_ptr]);
                this.fpu_pop();
            }
            else
            {
                // not a valid encoding
                throw "fpu invalid instruction encoding for fcompp at " + addr.toString(16);
            }
            break;
        case 4:
            // fsubrp
            this.fpu_st[low_ptr] = st0 - sti;
            break;
        case 5:
            // fsubp
            this.fpu_st[low_ptr] = sti - st0;
            break;
        case 6:
            // fdivrp
            this.fpu_st[low_ptr] = st0 / sti;
            break;
        case 7:
            // fdivp
            this.fpu_st[low_ptr] = sti / st0;
            break;
        default:
            return 0;
    }
    return 1;
};

FPU.prototype.fpu_op_DE_mem = function(imm8, addr)
{
    var mod = imm8 >> 3 & 7,
        m16 = this.cpu.readI16(addr) << 16 >> 16;
    
    var st0 = fpu_get_st0();
    
    switch (mod)
    {
        case 0:
            // fadd
            this.fpu_st[this.fpu_stack_ptr] = st0 + m16;
            break;
        case 1:
            // fmul
            this.fpu_st[this.fpu_stack_ptr] = st0 * m16;
            break;
        case 2:
            // fcom
            this.fpu_fcom(m16);
            break;
        case 3:
            // fcomp
            this.fpu_fcom(m16);
            this.fpu_pop();
            break;
        case 4:
            // fsub
            this.fpu_st[this.fpu_stack_ptr] = st0 - m16;
            break;
        case 5:
            // fsubr
            this.fpu_st[this.fpu_stack_ptr] = m16 - st0;
            break;
        case 6:
            // fdiv
            this.fpu_st[this.fpu_stack_ptr] = st0 / m16;
            break;
        case 7:
            // fdivr
            this.fpu_st[this.fpu_stack_ptr] = m16 / st0;
            break;
        default:
            return 0;
    }
    return 1;
};

FPU.prototype.fpu_op_DF_reg = function(imm8)
{
    var mod = imm8 >> 3 & 7,
        low = imm8 & 7;
    
    switch (mod)
    {
        case 4:
            if (imm8 === 0xE0)
            {
                // fnstsw / FSTSW AX
                this.cpu.reg32[reg_eax] = (this.cpu.reg32[reg_eax] & 0xFFFF0000) | this.fpu_load_status_word();
            }
            else
            {
                return 0;
            }
            break;
        case 5:
            // fucomip
            this.fpu_fucomi(this.fpu_get_sti(low));
            this.fpu_pop();
            break;
        case 6:
            // fcomip
            this.fpu_fcomi(this.fpu_get_sti(low));
            this.fpu_pop();
            break;
        default:
            return 0;
    }
    return 1;
};

FPU.prototype.fpu_op_DF_mem = function(imm8, addr)
{
    var mod = imm8 >> 3 & 7;
    
    switch (mod)
    {
        case 0:
            // fild
            var m16 = this.cpu.readI16(addr) << 16 >> 16;
            this.fpu_push(m16);
            break;
        case 2:
            // fist
            var st0 = this.fpu_integer_round(fpu_get_st0());
            if (st0 <= 0x7FFF && st0 >= -0x8000)
            {
                this.cpu.writeU32(addr, st0);
            }
            else
            {
                this.fpu_invalid_arithmatic();
                this.cpu.writeU32(addr, 0x8000);
            }
            break;
        case 3:
            // fistp
            var st0 = this.fpu_integer_round(this.fpu_get_st0());
            if (st0 <= 0x7FFF && st0 >= -0x8000)
            {
                this.cpu.writeU32(addr, st0);
            }
            else
            {
                this.fpu_invalid_arithmatic();
                this.cpu.writeU32(addr, 0x8000);
            }
            this.fpu_pop();
            break;
        case 5:
            // fild
            var low = this.cpu.readI32(addr) >>> 0,
                high = this.cpu.readI32(addr + 4);
            var m64 = low + 0x100000000 * high;
            this.fpu_push(m64);
            break;
        default:
            return 0;
    }
    return 1;
};

FPU.prototype.exec_fpu = function()
{
    if (!useTypedArrays)
    {
        //throw "Cannot handle FPU opcode " + this.cpu.opcode.toString(16) + " as typed arrays are either disabled or not supported";
        
        this.cpu.fetchOperands(this.cpu.addrInfoDst, this.cpu.addrInfoSrc);
        //console.log("Ignore FPU opcode " + this.cpu.opcode.toString(16) + " at EIP " + this.cpu.get_eip().toString(16));
        return 1;
    }
    
    var modrm = this.cpu.virtualMemory[this.cpu.mem_eip];
    this.cpu.fetchOperands(this.cpu.addrInfoDst, this.cpu.addrInfoSrc);
    var addr = this.cpu.addrInfoSrc.addr;
    var subop = this.cpu.addrInfoDst.addr;
    //if source.type == TYPE_REG  ==> modrm >= 0xC0
    //   modrm == 0xC0 | (dest.addr << 3) | source.addr
    //else
    //   dest.addr is the subop
    //   and source.addr is the address of the operand
    
    var isMem = modrm < 0xC0;
    var handled = true;
    
    switch (this.cpu.opcode)
    {
        case 0xD8:
            if (isMem)
            {
                handled = this.fpu_op_D8_mem(modrm, addr);
            }
            else
            {
                handled = this.fpu_op_D8_reg(modrm);
            }
            break;
        case 0xD9:
            if (isMem)
            {
                handled = this.fpu_op_D9_mem(modrm, addr);
            }
            else
            {
                handled = this.fpu_op_D9_reg(modrm);
            }
            break;
        case 0xDA:
            if (isMem)
            {
                handled = false;
            }
            else
            {
                handled = this.fpu_op_DA_reg(modrm);
            }
            break;
        case 0xDB:
            if (isMem)
            {
                handled = this.fpu_op_DB_mem(modrm, addr);
            }
            else
            {
                handled = false;
            }
            break;
        case 0xDC:
            if (isMem)
            {
                handled = this.fpu_op_DC_mem(modrm, addr);
            }
            else
            {
                handled = this.fpu_op_DC_reg(modrm);
            }
            break;
        case 0xDD:
            if (isMem)
            {
                handled = this.fpu_op_DD_mem(modrm, addr);
            }
            else
            {
                handled = this.fpu_op_DD_reg(modrm);
            }
            break;
        case 0xDE:
            if (isMem)
            {
                handled = this.fpu_op_DE_mem(modrm, addr);
            }
            else
            {
                handled = this.fpu_op_DE_reg(modrm);
            }
            break;
        case 0xDF:
            if (isMem)
            {
                handled = this.fpu_op_DF_mem(modrm, addr);
            }
            else
            {
                handled = this.fpu_op_DF_reg(modrm);
            }
            break;
        default:
            handled = false;
            break;
    }
    
    if (!handled)
    {
        throw "exec_fpu unhandled opcode " + this.cpu.opcode.toString(16) + " at EIP " + this.cpu.get_eip().toString(16);
    }
    
    return 1;
};