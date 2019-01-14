// This is a subset of https://github.com/cseagle/x86emu with bits of https://github.com/copy/v86

// Masks to clear out bytes appropriate to the sizes above
var SIZE_MASKS = [ 0, 0x000000FF, 0x0000FFFF, 0, 0xFFFFFFFF ];

// Masks to limit bit rotation amount in rotation instructionSize
var ROTATE_SIZE_MASKS = [ 0, 7, 0xF, 0, 0x1F ];

// Masks to clear out bytes appropriate to the sizes above
var SIGN_BITS = [ 0, 0x00000080, 0x00008000, 0, 0x80000000 ];

// Masks to clear out bytes appropriate to the sizes above
var CARRY_BITS = [ 0, 0x00000100, 0x00010000, 0, 0x100000000 ];

var BITS_BITS = [ 0, 8, 16, 0, 32 ];

var parityValues = [
1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
];

var TYPE_REG = 1;
var TYPE_IMM = 2;
var TYPE_MEM = 4;

function CPU()
{
    this.reg32 = [ 0, 0, 0, 0, 0, 0, 0, 0 ];
    this.cpu_eflags = 0;
    
    // Struct to describe an instruction being decoded
    this.addrInfoSrc = { addr: 0, type: 0, modrm: 0 };
    this.addrInfoDst = { addr: 0, type: 0, modrm: 0 };
    this.opsize = 0;// Operand size for this instruction
    this.opcode = 0;// Opcode, first or second unsigned char (if first == 0x0F)
    
    this.fpu = null;
    
    this.shared_ctor();
}

CPU.prototype.init = function()
{
    this.shared_init();
    
    if (useTypedArrays)
    {
        this.fpu = new FPU(this);
        this.fpu.fpu_init();
    }
    
    this.cpu_eflags = /*xIF*/0x200 | 2;
};

CPU.prototype.destroy = function()
{
    this.shared_destroy();
    
    this.addrInfoSrc.addr = this.addrInfoSrc.type = this.addrInfoSrc.modrm = 0;
    this.addrInfoDst.addr = this.addrInfoDst.type = this.addrInfoDst.modrm = 0;
};

//////////////////////////////////////////////////////////////////////////
// Arithmatic functions from v86
//////////////////////////////////////////////////////////////////////////

// If the dividend is too large, the division cannot be done precisely using
// JavaScript's double floating point numbers. Run simple long divsion until
// the dividend is small enough
CPU.prototype.arith_do_div32 = function(div_low, div_high, quot, idiv)
{
    var is_neg;
    var div_is_neg;
    if (idiv)
    {
        div_low = div_low | 0;
        
        if (quot < 0)
        {
            is_neg = true;
            quot = -quot;
        }
        else
        {
            div_is_neg = false;
        }
        
        if (div_high < 0)
        {
            div_is_neg = true;
            is_neg = !is_neg;
            div_low = -div_low >>> 0;
            div_high = ~div_high + !div_low;
        }
        else
        {
            div_is_neg = false;
        }
    }
    else
    {
        div_low = div_low >>> 0;
        div_high = div_high >>> 0;
    }
    
    //if (div_high >= quot || quot === 0)
    //{
    //    throw "Bad div32 " + div_low + " " + div_high + " " + quot;
    //}
    
    var result = 0;
    
    if (div_high > 0x100000)
    {
        var m = 0;
        var i = 32;
        var q = quot;
        while (q > div_high)
        {
            q >>>= 1;
            i--;
        }
        while (div_high > 0x100000)
        {
            if (div_high >= q)
            {
                div_high -= q;
                var sub = quot << i >>> 0;
                if (sub > div_low)
                {
                    div_high--;
                }
                div_low = div_low - sub >>> 0;
                result |= 1 << i
            }
            i--;
            q >>= 1;
        }
        result >>>= 0;
    }
    
    var div = div_low + div_high * 0x100000000;
    var mod = div % quot;
    result += div / quot | 0;
    
    if (idiv)
    {
        if (is_neg)
        {
            result = -result | 0;
        }
        
        if (div_is_neg)
        {
            mod = -mod | 0;
        }
        
        if (result >= 0x80000000 || result <= -0x80000001)
        {
            throw "Bad idiv32 result " + result;
        }
    }
    else if (result >= 0x100000000)
    {
        throw "Bad div32 result " + result;
    }
    
    this.reg32[reg_eax] = result;
    this.reg32[reg_edx] = mod;
};

CPU.prototype.arith_do_mul32 = function(a, b, imul)
{
    var is_neg = false;
    if (imul)
    {
        if (a < 0)
        {
            is_neg = true;
            a = -a | 0;
        }
        if (b < 0)
        {
            is_neg = !is_neg;
            b = -b | 0;
        }
    }
    
    var a00 = a & 0xFFFF;
    var a16 = a >>> 16;
    var b00 = b & 0xFFFF;
    var b16 = b >>> 16;
    var low_result = a00 * b00;
    var mid = (low_result >>> 16) + (a16 * b00 | 0) | 0;
    var high_result = mid >>> 16;
    mid = (mid & 0xFFFF) + (a00 * b16 | 0) | 0;
    
    var mul32_result0 = (mid << 16) | low_result & 0xFFFF;
    var mul32_result1 = ((mid >>> 16) + (a16 * b16 | 0) | 0) + high_result | 0;
    
    if (imul)
    {
        if (is_neg)
        {
            mul32_result0 = -mul32_result0 | 0;
            ~mul32_result1 + !mul32_result0 | 0;
        }
        
        if (mul32_result1 === (mul32_result0 >> 31))
        {
            // clear overflow
            
        }
        else
        {
            // set overflow
        }
    }
    else
    {
        if (mul32_result1 == 0)
        {
            // clear overflow
        }
        else
        {
            // set overflow
        }
    }
    
    this.reg32[reg_eax] = mul32_result0;
    this.reg32[reg_edx] = mul32_result1;
};

//////////////////////////////////////////////////////////////////////////
// Sign extension functions
//////////////////////////////////////////////////////////////////////////

// unsigned char->unsigned short
CPU.prototype.se_bw = function(val)
{
    return ((val << 24) >> 24) >>> 0;
    //return val;
};

// unsigned short->unsigned int
CPU.prototype.se_wd = function(val)
{
    return ((val << 16) >> 16) >>> 0;
    //return val;
};

// unsigned char->unsigned int
CPU.prototype.se_bd = function(val)
{
    return ((val << 24) >> 24) >>> 0;
    //return val;
};

CPU.prototype.se_dq = function(val)
{
    throw "sedq not implemented";
};

//////////////////////////////////////////////////////////////////////////
// Operand and flag functions
//////////////////////////////////////////////////////////////////////////

// deal with sign, zero, and parity flags
CPU.prototype.setEflags = function(val, size)
{
    val &= SIZE_MASKS[size];// mask off upper bytes    
    if (val) this.cpu_eflags &= ~0x40;// xZF/xZERO
    else this.cpu_eflags |= 0x40;
    if (val & SIGN_BITS[size]) this.cpu_eflags |= 0x80;// xSF/xSIGN
    else this.cpu_eflags &= ~0x80;
    //if (parityValues[val & 0xFF]) this.cpu_eflags |= 0x4;// xPF/xPARITY
    //else this.cpu_eflags &= ~0x4;
};

// Kris Kaspersky pointed out that the AF flag did not
// function properly for normal adds and subtracts
CPU.prototype.checkAuxCarry = function(op1, op2, result)
{
    var aux = ((op1 ^ pop2) & 0x10) != (result & 0x10);
    if (aux) this.cpu_eflags |= 0x10;// xAF/xAUX_CARRY
    else this.cpu_eflags &= ~0x10;
};

CPU.prototype.hasAddOverflow = function(op1, op2, sum)
{
    var mask = SIGN_BITS[this.opsize];
    if ((op1 & op2 & ~sum & mask) || (~op1 & ~op2 & sum & mask)) return true;
    else return false;
};

CPU.prototype.checkAddOverflowCarry = function(op1, op2, sum, sizeMask)
{
    // NOTE: This overflow / carry check slows down every operation even when the carry/overflow isn't checked.
    // Really this should define what the current operation is and then calculate the carry when needed (similar to jslinux/v86)
    // - For TCC we might be able to get away with only doing overflow/carry for op_cmp, and ignore it elsewhere?
    
    switch (this.opsize)
    {
        // TODO: Improve
        case 1:
        case 2:
            if (sum & CARRY_BITS[this.opsize]) this.cpu_eflags |= 0x1;// xCF/xCARRY
            else this.cpu_eflags &= ~0x1;
            break;
        case 4:
            if (((op1 & sizeMask) >>> 0) + ((op2 & sizeMask) >>> 0) > 0xFFFFFFFF)
            {
                this.cpu_eflags |= 0x1;// xCF/xCARRY
            }
            else
            {
                this.cpu_eflags &= ~0x1;
            }
            break;
    }
    
    var mask = SIGN_BITS[this.opsize];
    if ((op1 & op2 & ~sum & mask) || (~op1 & ~op2 & sum & mask)) this.cpu_eflags |= 0x800;// xOF/xOVERFLOW
    else this.cpu_eflags &= ~0x800;
};

CPU.prototype.op_add = function(op1, op2)
{
    var mask = SIZE_MASKS[this.opsize];
    var result = (op1 & mask) + (op2 & mask);
    this.checkAddOverflowCarry(op1, op2, result, mask);
    this.setEflags(result, this.opsize);
    //this.checkAuxCarry(op1, op2, result);
    return result & mask;
};

CPU.prototype.op_adc = function(op1, op2)
{
    var mask = SIZE_MASKS[this.opsize];
    var result = (op1 & mask) + (op2 & mask) + (this.cpu_eflags & 0x1);// xC/xB
    this.checkAddOverflowCarry(op1, op2, result, mask);
    this.setEflags(result, this.opsize);
    //this.checkAuxCarry(op1, op2, result);
    return result & mask;
};

CPU.prototype.hasSubOverflow = function(op1, op2, diff)
{
    var mask = SIGN_BITS[this.opsize];
    if ((op1 & ~op2 & ~diff & mask) || (~op1 & op2 & diff & mask)) return false;
    else return false;
};

/*CPU.prototype.checkSubOverflow = function(op1, op2, diff)
{
    var mask = SIGN_BITS[this.opsize];
    if ((op1 & ~op2 & ~diff & mask) || (~op1 & op2 & diff & mask)) this.cpu_eflags |= 0x800;// xOF/xOVERFLOW
    else this.cpu_eflags &= ~0x800;
};*/

CPU.prototype.checkSubOverflowCarry = function(op1, op2, diff, sizeMask)
{
    // NOTE: This overflow / carry check slows down every operation even when the carry/overflow isn't checked.
    // Really this should define what the current operation is and then calculate the carry when needed (similar to jslinux/v86)
    // - For TCC we might be able to get away with only doing overflow/carry for op_cmp, and ignore it elsewhere?
    
    switch (this.opsize)
    {
        // TODO: Improve
        case 1:
        case 2:
            if (diff & CARRY_BITS[this.opsize]) this.cpu_eflags |= 0x1;// xCF/xCARRY
            else this.cpu_eflags &= ~0x1;
            break;
        case 4:
            if (((op1 & sizeMask) >>> 0) - ((op2 & sizeMask) >>> 0) < 0)
            {
                this.cpu_eflags |= 0x1;// xCF/xCARRY
            }
            else
            {
                this.cpu_eflags &= ~0x1;
            }
            break;
    }
    
    var mask = SIGN_BITS[this.opsize];
    if ((op1 & ~op2 & ~diff & mask) || (~op1 & op2 & diff & mask)) this.cpu_eflags |= 0x800;// xOF/xOVERFLOW
    else this.cpu_eflags &= ~0x800;
};

CPU.prototype.op_sub = function(op1, op2)
{
    var mask = SIZE_MASKS[this.opsize];
    var result = (op1 & mask) - (op2 & mask);
    this.checkSubOverflowCarry(op1, op2, result, mask);
    this.setEflags(result, this.opsize);
    //this.checkAuxCarry(op1, op2, result);
    return result & mask;
};

CPU.prototype.op_sbb = function(op1, op2)
{
    var mask = SIZE_MASKS[this.opsize];
    var result = (op1 & mask) - (op2 & mask) - (this.cpu_eflags & 0x1);// xC/xB
    this.checkSubOverflowCarry(op1, op2, result, mask);
    this.setEflags(result, this.opsize);
    //this.checkAuxCarry(op1, op2, result);
    return result & mask;
};

CPU.prototype.op_AND = function(op1, op2)
{
    var mask = SIZE_MASKS[this.opsize];
    var result = (op1 & mask) & (op2 & mask);
    this.cpu_eflags &= ~(0x1 | 0x800);// xCF | xOF
    this.setEflags(result, this.opsize);
    return result & mask;
};

CPU.prototype.op_OR = function(op1, op2)
{
    var mask = SIZE_MASKS[this.opsize];
    var result = (op1 & mask) | (op2 & mask);
    this.cpu_eflags &= ~(0x1 | 0x800);// xCF | xOF
    this.setEflags(result, this.opsize);
    return result & mask;
};

CPU.prototype.op_XOR = function(op1, op2)
{
    var mask = SIZE_MASKS[this.opsize];
    var result = (op1 & mask) ^ (op2 & mask);
    this.cpu_eflags &= ~(0x1 | 0x800);// xCF | xOF
    this.setEflags(result, this.opsize);
    return result & mask;
};

CPU.prototype.op_cmp = function(op1, op2)
{
    var mask = SIZE_MASKS[this.opsize];
    var result = (op1 & mask) - (op2 & mask);
    this.checkSubOverflowCarry(op1, op2, result, mask);
    this.setEflags(result, this.opsize);
};

CPU.prototype.op_inc = function(op1)
{
    var oldCarry = (this.cpu_eflags & 0x1);// xC/xB
    op1 = this.op_add(op1, 1);
    this.cpu_eflags &= ~0x1;// xCF/xCARRY
    this.cpu_eflags |= oldCarry;
    return op1;
};

CPU.prototype.op_dec = function(op1)
{
    var oldCarry = (this.cpu_eflags & 0x1);// xC/xB
    op1 = this.op_sub(op1, 1);
    this.cpu_eflags &= ~0x1;// xCF/xCARRY
    this.cpu_eflags |= oldCarry;
    return op1;
};

CPU.prototype.checkLeftOverflow = function(result, size)
{
    var msb = result & SIGN_BITS[size];
    if ((msb && (this.cpu_eflags & 0x1)) || (!msb && !(this.cpu_eflags & 0x1))) this.cpu_eflags &= ~0x800;// xC/xB  xNC/xNB  xOF/xOVERFLOW
    else this.cpu_eflags |= 0x800;
};

CPU.prototype.op_rol = function(op, amt)
{
    // remove unnecessary rotations
    amt = amt & ROTATE_SIZE_MASKS[this.opsize];
    if (amt)
    {
        op = op & SIZE_MASKS[this.opsize];
        op = (op >> (BITS_BITS[this.opsize] - amt)) | (op << amt);
        if (op & 1) this.cpu_eflags |= 0x1;// xCF/xCARRY
        else this.cpu_eflags &= ~0x1;
        if (amt == 1)
        {
            this.checkLeftOverflow(op, this.opsize);
        }
    }
    return op & SIZE_MASKS[this.opsize];
};

CPU.prototype.op_ror = function(op, amt)
{
    // remove unnecessary rotations
    amt = amt & ROTATE_SIZE_MASKS[this.opsize];
    if (amt)
    {
        op = op & SIZE_MASKS[this.opsize];
        op = (op << (BITS_BITS[this.opsize] - amt)) | (op >> amt);
        if (op & SIGN_BITS[this.opsize]) this.cpu_eflags |= 0x1;// xCF/xCARRY
        else this.cpu_eflags &= ~0x1;
        if (amt == 1)
        {
            var shift = op << 1;
            shift = (shift ^ op) & SIGN_BITS[this.opsize];
            if (shift) this.cpu_eflags |= 0x800;// xOF/xOVERFLOW
            else this.cpu_eflags &= ~0x800;
        }
    }
    return op & SIZE_MASKS[this.opsize];
};

CPU.prototype.op_rcl = function(op, amt)
{
    var error = "op_rcl not implemented";
    alert(error);
    throw error;
};

CPU.prototype.op_rcr = function(op, amt)
{
    var error = "op_rcr not implemented";
    alert(error);
    throw error;
};

CPU.prototype.op_shl = function(op, amt)
{
    if (amt)
    {
        op <<= amt;
        
        // TODO: Improve
        switch (this.opsize)
        {            
            case 1:
            case 2:
                if (op & CARRY_BITS[this.opsize]) this.cpu_eflags |= 0x1;// xCF/xCARRY
                else this.cpu_eflags &= ~0x1;
                break;
            case 4:
                if (((op << (amt - 1)) >>> 0) > (op >>> 0))
                {
                    this.cpu_eflags |= 0x1;// xCF/xCARRY
                }
                else
                {
                    this.cpu_eflags &= ~0x1;
                }
                break;
        }
        //if (op & CARRY_BITS[this.opsize]) this.cpu_eflags |= 0x1;// xCF/xCARRY
        //else this.cpu_eflags &= ~0x1;
        
        if (amt == 1)
        {
            this.checkLeftOverflow(op, this.opsize);
        }
        this.setEflags(op, this.opsize);//flags only affected when amt != 0
    }
    return op & SIZE_MASKS[this.opsize];
};

// mask op down to size before calling
CPU.prototype.shiftRight = function(op, amt, unsigned)
{
    if (amt)
    {
        // Will this work? (possibly? but not for >= 32 bit values?)
        var final_carry = 1 << (amt - 1);
        if ((op >>> 0) & final_carry) this.cpu_eflags |= 0x1;// xCF/xCARRY
        else this.cpu_eflags &= ~0x1;
        
        if (unsigned)
        {
            op >>>= amt;
        }
        else
        {
            op >>= amt;
        }
        this.setEflags(op, this.opsize);//flags only affected when amt != 0
    }
    return op;
};

CPU.prototype.op_shr = function(op, amt)
{
    if (amt == 1)
    {
        if (op & SIGN_BITS[this.opsize]) this.cpu_eflags |= 0x800;// xOF/xOVERFLOW
        else this.cpu_eflags &= ~0x800;
    }
    return this.shiftRight(op & SIZE_MASKS[this.opsize], amt, true);
};

CPU.prototype.op_sar = function(op, amt)
{
    op = op & SIZE_MASKS[this.opsize];
    // We don't need these sign conversions as we are using a signed shift in shiftRight
    /*switch (this.opsize)
    {
        case 1:
            op = this.se_dq(this.se_bd(op));
            break;
        case 2:
            op = this.se_dq(this.se_wd(op));
            break;
        case 4:
            op = this.se_dq(op);
            break;
    }*/
    if (amt == 1)
    {
        this.cpu_eflags &= ~0x800;// xOF/xOVERFLOW
    }
    return this.shiftRight(op, amt, false);
};

CPU.prototype.op_shrd = function(op1, bits, amt)
{
    var error = "op_shrd not implemented";
    alert(error);
    throw error;
};

CPU.prototype.op_shld = function(op1, bits, amt)
{
    var error = "op_shld not implemented";
    alert(error);
    throw error;
};

CPU.prototype.op_dShift = function()
{
    var error = "op_dShift not implemented";
    alert(error);
    throw error;
};

CPU.prototype.getLongShiftCount = function()
{
    var error = "getLongShiftCount not implemented";
    alert(error);
    throw error;
};

//////////////////////////////////////////////////////////////////////////
// Read / write functions
//////////////////////////////////////////////////////////////////////////

CPU.prototype.readI8 = function(addr)
{
    return (((this.virtualMemory[addr - this.virtualMemoryAddress]) << 24) >> 24);
};

CPU.prototype.readI16 = function(addr)
{
    addr -= this.virtualMemoryAddress;
    return (((this.virtualMemory[addr] + (this.virtualMemory[addr + 1] << 8)) << 16) >> 16);
};

CPU.prototype.readI32 = function(addr)
{
    addr -= this.virtualMemoryAddress;
    return ((this.virtualMemory[addr] + (this.virtualMemory[addr + 1] << 8) + (this.virtualMemory[addr + 2] << 16) + (this.virtualMemory[addr + 3] << 24)) | 0);
};

CPU.prototype.readU8 = function(addr)
{
    return (this.virtualMemory[addr - this.virtualMemoryAddress]) >>> 0;
};

CPU.prototype.readU16 = function(addr)
{
    addr -= this.virtualMemoryAddress;
    return (this.virtualMemory[addr] + (this.virtualMemory[addr + 1] << 8)) >>> 0;
};

CPU.prototype.readU32 = function(addr)
{
    addr -= this.virtualMemoryAddress;
    return (this.virtualMemory[addr] + (this.virtualMemory[addr + 1] << 8) + (this.virtualMemory[addr + 2] << 16) + (this.virtualMemory[addr + 3] << 24)) >>> 0;
};

CPU.prototype.read_8 = function(addr)
{
    return this.virtualMemory[addr - this.virtualMemoryAddress];
};

CPU.prototype.read_16 = function(addr)
{
    addr -= this.virtualMemoryAddress;
    return this.virtualMemory[addr] + (this.virtualMemory[addr + 1] << 8);
};

CPU.prototype.read_32 = function(addr)
{
    addr -= this.virtualMemoryAddress;
    return this.virtualMemory[addr] + (this.virtualMemory[addr + 1] << 8) + (this.virtualMemory[addr + 2] << 16) + (this.virtualMemory[addr + 3] << 24);
};

CPU.prototype.readMem = function(addr, size)
{
    switch (size)
    {
        case 1: return this.readU8(addr);
        case 2: return this.readU16(addr);
        case 4: return this.readU32(addr);
        default: throw "Invalid read size " + size;
    }
};

CPU.prototype.writeU8 = function(addr, val)
{
    this.virtualMemory[addr - this.virtualMemoryAddress] = (val & 0xFF);
};

CPU.prototype.writeU16 = function(addr, val)
{
    addr -= this.virtualMemoryAddress;
    this.virtualMemory[addr] = (val & 0xFF);
    this.virtualMemory[addr + 1] = (val >> 8) & 0xFF;
};

CPU.prototype.writeU32 = function(addr, val)
{
    addr -= this.virtualMemoryAddress;
    this.virtualMemory[addr] = (val & 0xFF);
    this.virtualMemory[addr + 1] = (val >> 8) & 0xFF;
    this.virtualMemory[addr + 2] = (val >> 16) & 0xFF;
    this.virtualMemory[addr + 3] = (val >> 24) & 0xFF;
};

CPU.prototype.writeMem = function(addr, val, size)
{
    switch (size)
    {
        case 1: return this.writeU8(addr, val);
        case 2: return this.writeU16(addr, val);
        case 4: return this.writeU32(addr, val);
        default: throw "Invalid write size " + size;
    }
};

//////////////////////////////////////////////////////////////////////////
// EIP read (aka fetch) functions (these advance EIP on each call)
// - Avoid using mem_eip+=fetchXXX() (use a temp var instead)
//////////////////////////////////////////////////////////////////////////

CPU.prototype.fetchIx = function(x)
{
    switch (x)
    {
        default:
        case 1: return this.fetchI8();
        case 2: return this.fetchI16();
        case 4: return this.fetchI32();
    }
};

CPU.prototype.fetchI8 = function()
{
    return ((this.virtualMemory[this.mem_eip++] << 24) >> 24);
};

CPU.prototype.fetchI16 = function()
{
    return ((this.virtualMemory[this.mem_eip++] + (this.virtualMemory[this.mem_eip++] << 8) << 16) >> 16);
};

CPU.prototype.fetchI32 = function()
{
    return ((this.virtualMemory[this.mem_eip++] + (this.virtualMemory[this.mem_eip++] << 8) + (this.virtualMemory[this.mem_eip++] << 16) + (this.virtualMemory[this.mem_eip++] << 24)) | 0);
};

CPU.prototype.fetchUx = function(x)
{
    switch (x)
    {
        default:
        case 1: return this.fetchU8();
        case 2: return this.fetchU16();
        case 4: return this.fetchU32();
    }
};

CPU.prototype.fetchU8 = function()
{
    return (this.virtualMemory[this.mem_eip++]);
};

CPU.prototype.fetchU16 = function()
{
    return (this.virtualMemory[this.mem_eip++] + (this.virtualMemory[this.mem_eip++] << 8));
};

CPU.prototype.fetchU32 = function()
{
    return (this.virtualMemory[this.mem_eip++] + (this.virtualMemory[this.mem_eip++] << 8) + (this.virtualMemory[this.mem_eip++] << 16) + (this.virtualMemory[this.mem_eip++] << 24));
};

/*CPU.prototype.fetchOperands16 = function(dst, src)
{
    var modrm = this.fetchU8();
    dst.modrm = modrm;
    var mod = modrm & 0xC0;
    var rm = modrm & 0x07;
    var disp = 0;    
    if (mod != 0xC0)// MOD_3
    {
        switch (rm)
        {
            case 0:
                src.addr = this.reg32[reg_ebx] + this.reg32[reg_esi];
                break;
            case 1:
                src.addr = this.reg32[reg_ebx] + this.reg32[reg_edi];
                break;
            case 2:
                src.addr = this.reg32[reg_ebp] + this.reg32[reg_esi];
                break;
            case 3:
                src.addr = this.reg32[reg_ebp] + this.reg32[reg_edi];
                break;
            case 4:
                src.addr = this.reg32[reg_esi];
                break;
            case 5:
                src.addr = this.reg32[reg_edi];
                break;
            case 6:
                src.addr = this.reg32[reg_ebp];
                break;
            case 7:
                src.addr = this.reg32[reg_ebx];
                break;
        }
    }
    switch (mod)
    {
        case 0:// MOD_0
            if (rm == 5)
            {
                src.addr = this.fetchI16();
            }
            src.type = TYPE_MEM;
            break;
        case 0x40:// MOD_1
            disp = this.fetchI8();
            src.type = TYPE_MEM;
            break;
        case 0x80:// MOD_2
            disp = this.fetchI16();
            src.type = TYPE_MEM;
            break;
        case 0xC0:// MOD_3
            src.type = TYPE_REG;
            src.addr = rm;
            break;
        default:
            src.type = TYPE_MEM;
            break;
    }    
    if (src.type == TYPE_MEM)
    {
        src.addr += disp;
        src.addr &= SIZE_MASKS[2];
    }    
    dst.addr = (modrm >> 3) & 0x07// REG
    dst.type = TYPE_REG;
};*/

CPU.prototype.fetchOperands = function(dst, src)
{
    // Does TCC use the 0x67 (PREFIX_ADDR) prefix?
    /*if (this.prefix & 0x400)// PREFIX_ADDR
    {
        this.fetchOperands16(dst, src);
        return;
    }*/
    
    // MOD bits: 0xC0 = 11000000
    // REG bits: 0x38 = 00111000
    // R/M bits: 0x07 = 00000111
    
    // To extract MOD / REG / RM
    // MOD: (modrm & 0xC0) >> 6
    // REG: (modrm & 0x38) >> 3
    // R/M: (modrm & 0x07)
    
    // MOD value detection (without having to shift right by 6 (>> 6))
    // 0x00 = 00000000 (MOD_0)
    // 0x40 = 10000000 (MOD_1)
    // 0x80 = 01000000 (MOD_2)
    // 0x0C = 11000000 (MOD_3)
    
    var modrm = this.fetchU8();
    dst.modrm = modrm;
    var mod = modrm & 0xC0;
    var rm = modrm & 0x07;
    var disp = 0;
    var sib = 0;// The 0x100/0xFF is just for sib detection
    if (mod != 0xC0)// MOD_3
    {
        if (rm == 4)
        {
            sib = this.fetchU8() | 0x100;
            //src.addr = 0;
        }
        else
        {
            src.addr = this.reg32[rm]
        }
    }    
    switch (mod)
    {
        case 0:// MOD_0
            if (rm == 5)
            {
                src.addr = this.fetchI32();
            }
            src.type = TYPE_MEM;
            break;
        case 0x40:// MOD_1
            disp = this.fetchI8();
            src.type = TYPE_MEM;
            break;
        case 0x80:// MOD_2
            disp = this.fetchI32();
            src.type = TYPE_MEM;
            break;
        case 0xC0:// MOD_3
            src.type = TYPE_REG;
            src.addr = rm;
            break;
        default:
            src.type = TYPE_MEM;
            break;
    }    
    if (src.type == TYPE_MEM)
    {
        src.addr += disp;
        if (sib != 0)
        {
            sib = sib & 0xFF;
            var index = (sib >> 3) & 0x07;// INDEX
            index = index == 4 ? 0 : this.reg32[index] * (1 << (sib >> 6));// SCALE
            src.addr += index;
            var base = sib & 0x07;// BASE
            if (base == 5 && mod == 0)
            {
                src.addr += this.fetchI32();
            }
            else
            {
                src.addr += this.reg32[base];
            }
        }
    }    
    dst.addr = (modrm >> 3) & 0x07// REG
    dst.type = TYPE_REG;
};

//////////////////////////////////////////////////////////////////////////
// AddrInfo helpers
//////////////////////////////////////////////////////////////////////////

CPU.prototype.A_Ix = function()
{
    this.addrInfoDst.addr = 0;
    this.addrInfoDst.type = TYPE_REG;
    this.addrInfoSrc.addr = this.fetchIx(this.opsize);
};

CPU.prototype.decodeAddressingModes = function()
{
    this.opsize = this.opcode & 1 ? this.opsize : 1;
    switch (this.opcode & 0x7)
    {
        case 0: case 1:
            this.fetchOperands(this.addrInfoSrc, this.addrInfoDst);
            break;
        case 2: case 3:
            this.fetchOperands(this.addrInfoDst, this.addrInfoSrc);
            break;
        case 4: case 5:
            this.A_Ix();
            break;
    }
};

CPU.prototype.getOperand = function(op)
{
    var mask = SIZE_MASKS[this.opsize];
    switch (op.type)
    {
        case TYPE_REG:
            if (this.opsize == 1 && op.addr >= 4)
            {
                // AH, CH, DH, BH
                return (this.reg32[op.addr - 4] >> 8) & mask;
            }
            return this.reg32[op.addr] & mask;
        case TYPE_IMM:            
            return op.addr & mask;
        case TYPE_MEM:
            //this.setSegment();
            return this.readMem(op.addr, this.opsize) & mask;
    }
    return 0;
};

CPU.prototype.storeOperand = function(op, val)
{
    var mask = SIZE_MASKS[this.opsize];
    val &= mask;
    if (op.type == TYPE_REG)
    {
        if (this.opsize == 1 && op.addr >= 4)
        {
            // AH, CH, DH, BH
            this.reg32[op.addr - 4] &= ~0x0000FF00;// H_MASK
            this.reg32[op.addr - 4] |= (val >> 8);
        }
        else
        {
            this.reg32[op.addr] &= ~SIZE_MASKS[this.opsize];
            this.reg32[op.addr] |= val;
        }
    }
    else
    {
        //this.setSegment();
        this.writeMem(op.addr, val, this.opsize);
    }
};

//////////////////////////////////////////////////////////////////////////
// Instruction handlers
//////////////////////////////////////////////////////////////////////////

CPU.prototype.exec_Set = function(cc)
{
    var set = 0;
    this.fetchOperands(this.addrInfoSrc, this.addrInfoDst);
    this.opsize = 1;
    
    // This is functionally the same as exec_7X (TODO: Merge them?)
    switch (cc)
    {
        case 0:// SO
            set = this.cpu_eflags & 0x800;// xO/xOF/xOVERFLOW
            break;
        case 1:// SNO
            set = !(this.cpu_eflags & 0x800);// xNO (!xO)
            break;
        case 2:// B/NAE/C
            set = this.cpu_eflags & 0x1;// xB/xCF
            break;
        case 3:// NB/AE/NC
            set = !(this.cpu_eflags & 0x1);// xNB/xAE/xNC (!xB)
            break;
        case 4:// E/Z
            set = this.cpu_eflags & 0x40;// xZ/xE/xZF
            break;
        case 5:// NE/NZ
            set = !(this.cpu_eflags & 0x40);// xNZ/xNE (!xZF)
            break;
        case 6:// BE/NA
            set = (this.cpu_eflags & (0x40 | 0x01));// xBE (xZF | xCF)
            break;
        case 7:// NBE/A
            set = !(this.cpu_eflags & (0x40 | 0x01));// xA/xNBE (!xBE)
            break;
        case 8:// S
            set = this.cpu_eflags & 0x80;// xS/xSF
            break;
        case 9:// NS
            set = !(this.cpu_eflags & 0x80);// xNS (!xS/!xSF)
            break;
        case 0xA:// P/PE
            set = this.cpu_eflags & 0x4;// xP/xPF;
            break;
        case 0xB:// NP/PO
            set = !(this.cpu_eflags & 0x4);// xNP (!xP/!xPF)
            break;
        case 0xC:// L/NGE
            set = (((this.cpu_eflags & (0x80 | 0x800)) == 0x80) || ((this.cpu_eflags & (0x80 | 0x800)) == 0x800));// xL
            break;
        case 0xD:// NL/GE
            set = (((this.cpu_eflags & (0x80 | 0x800)) == 0) || ((this.cpu_eflags & (0x80 | 0x800)) == (0x80 | 0x800)));// xGE
            break;
        case 0xE:// LE/NG
            set = (((this.cpu_eflags & (0x80 | 0x800)) == 0x80) || ((this.cpu_eflags & (0x80 | 0x800)) == 0x800) || (this.cpu_eflags & 0x40));// xLE
            break;
        case 0xF:// NLE/G
            set = ((((this.cpu_eflags & (0x80 | 0x800)) == 0) || ((this.cpu_eflags & (0x80 | 0x800)) == (0x80 | 0x800))) && !(this.cpu_eflags & 0x40));// xG
            break;
    }
    this.storeOperand(this.addrInfoDst, set ? 1 : 0);
    return 1;
};

// Two byte opcodes (0F XX)
CPU.prototype.exec_0F = function()
{
    this.opcode = this.virtualMemory[this.mem_eip++];
    var upper = this.opcode >> 4;
    var lower = this.opcode & 0xF;
    switch (upper)
    {
        case 8:// Jcc
            return this.exec_7X();// one unsigned char Jcc handler
        case 9:// SET
            return this.exec_Set(lower);
        case 0xA:// IMUL, SHRD, SHLD
            switch (lower)
            {
                case 0xF:// IMUL
                    this.fetchOperands(this.addrInfoDst, this.addrInfoSrc);
                    var op1 = this.getOperand(this.addrInfoSrc);
                    var op2 = this.getOperand(this.addrInfoDst);
                    var result = op1 * op2;
                    this.storeOperand(this.addrInfoDst, result);
                    this.setEflags(result, this.opsize);
                    break;
                default:
                    throw "exec_TwoByteOP unhandled opcode 0F " + this.opcode.toString(16);
            }
            break;
        case 0xB:
            switch (lower)
            {
                case 6: case 7: case 0xE: case 0xF:// MOVZX, MOVSX
                    if ((this.opcode & 7) == 6) this.opsize = 1;
                    else this.opsize = 2;
                    this.fetchOperands(this.addrInfoDst, this.addrInfoSrc);
                    var result = this.getOperand(this.addrInfoSrc);
                    if (this.opcode & 8)
                    {
                        // MOVSX
                        if (this.opsize == 1) result = this.se_bd(result);
                        else result = this.se_wd(result);
                    }
                    this.opsize = 4;
                    this.storeOperand(this.addrInfoDst, result);
                    break;
                case 0xD:// BSR
                    this.fetchOperands(this.addrInfoDst, this.addrInfoSrc);
                    var src = this.getOperand(this.addrInfoSrc);
                    if (src == 0)
                    {
                        this.cpu_eflags |= 0x40;// xZF/xZERO
                    }
                    else
                    {
                        this.cpu_eflags &= ~0x40;// xZF/xZERO
                        var result = 0;
                        var numBits = BITS_BITS[this.opsize];
                        for (var i = 0; i < numBits; i++)
                        {
                            if (src & 1)
                            {
                                this.storeOperand(this.addrInfoDst, result);
                                break;
                            }
                            src >>= 1;
                            result++;
                        }
                    }
                    break;
                default:
                    throw "exec_TwoByteOP unhandled opcode 0F " + this.opcode.toString(16);
            }
            break;
        
        default:
            throw "exec_TwoByteOP unhandled opcode 0F " + this.opcode.toString(16);
    }
    return 1;
};

CPU.prototype.exec_0X = function()
{
    var op = this.opcode & 0x0F;
    if ((op & 0x07) < 6)
    {
        this.decodeAddressingModes();
        var op1 = this.getOperand(this.addrInfoDst);
        var op2 = this.getOperand(this.addrInfoSrc);
        if (op < 8)
        {
            // ADD
            this.storeOperand(this.addrInfoDst, this.op_add(op1, op2));
        }
        else
        {
            // OR
            this.storeOperand(this.addrInfoDst, this.op_OR(op1, op2));
        }
    }
    else
    {
        // Temporarily removed 0x06, 0x07, 0x0E
        switch (op)
        {
            case 0x0F:
                return this.exec_0F();
            default:
                throw "exec_0X unhandled opcode " + this.opcode.toString(16);
        }
    }
    return 1;
};

CPU.prototype.exec_1X = function()
{
    var op = this.opcode & 0x0F;
    if ((op & 0x7) < 6)
    {
        this.decodeAddressingModes();
        var op1 = this.getOperand(this.addrInfoDst);
        var op2 = this.getOperand(this.addrInfoSrc);
        if (op < 8)
        {
            // ADC
            this.storeOperand(this.addrInfoDst, this.op_adc(op1, op2));
        }
        else
        {
            // SBB
            this.storeOperand(this.addrInfoDst, this.op_sbb(op1, op2));
        }
    }
    else
    {
        throw "exec_1X unhandled opcode " + this.opcode.toString(16);
    }
    return 1;
};

CPU.prototype.exec_2X = function()
{
    var op = this.opcode & 0x0F;
    if ((op & 0x7) < 6)
    {
        this.decodeAddressingModes();
        var op1 = this.getOperand(this.addrInfoDst);
        var op2 = this.getOperand(this.addrInfoSrc);
        if (op < 8)
        {
            // AND
            this.storeOperand(this.addrInfoDst, this.op_AND(op1, op2));
        }
        else
        {
            // SUB
            this.storeOperand(this.addrInfoDst, this.op_sub(op1, op2));
        }
    }
    else
    {
        throw "exec_2X unhandled opcode " + this.opcode.toString(16);
    }
    return 1;
};

CPU.prototype.exec_3X = function()
{
    var op = this.opcode & 0x0F;
    if ((op & 0x7) < 6)
    {
        this.decodeAddressingModes();
        var op1 = this.getOperand(this.addrInfoDst);
        var op2 = this.getOperand(this.addrInfoSrc);
        if (op < 8)
        {
            // XOR
            this.storeOperand(this.addrInfoDst, this.op_XOR(op1, op2));
        }
        else
        {
            // CMP
            this.op_cmp(op1, op2);
        }
    }
    else
    {
        throw "exec_3X unhandled opcode " + this.opcode.toString(16);
    }
    return 1;
};

CPU.prototype.exec_4X = function()
{
    var op = this.opcode & 0x0F;
    var reg = op & 7;
    var mask = SIZE_MASKS[this.opsize];
    // Skip source setup, just read the register
    var result = this.reg32[reg] & mask;
    this.addrInfoDst.type = TYPE_REG;
    this.addrInfoDst.addr = reg;
    if (op < 8)// INC
    {
        result = this.op_inc(result);
    }
    else// DEC
    {
        result = this.op_dec(result);
    }
    this.storeOperand(this.addrInfoDst, result);
    this.setEflags(result, this.opsize);
    return 1;
};

CPU.prototype.exec_5X = function()
{
    // TODO: Confirm which push/pop registers we need
    switch (this.opcode)
    {
        case 0x50:// PUSH EAX
            this.push_32(this.reg32[reg_eax]);
            break;
        case 0x51:// PUSH ECX
            this.push_32(this.reg32[reg_ecx]);
            break;
        case 0x52:// PUSH EDX
            this.push_32(this.reg32[reg_edx]);
            break;
        case 0x53:// PUSH EBX
            this.push_32(this.reg32[reg_ebx]);
            break;    
        case 0x55:// PUSH EBP
            this.push_32(this.reg32[reg_ebp]);
            break;
        case 0x56:// PUSH ESI
            this.push_32(this.reg32[reg_esi]);
            break;
        case 0x57:// PUSH EDI
            this.push_32(this.reg32[reg_edi]);
            break;            
        case 0x58:// POP EAX
            this.reg32[reg_eax] = this.popI32();
            break;
        case 0x59:// POP ECX
            this.reg32[reg_ecx] = this.popI32();
            break;
        case 0x5A:// POP EDX
            this.reg32[reg_edx] = this.popI32();
            break;
        case 0x5B:// POP EBX
            this.reg32[reg_ebx] = this.popI32();
            break;
        case 0x5C:// POP ESP
            this.reg32[reg_esp] = this.popI32();
            break;
        case 0x5D:// POP EBP
            this.reg32[reg_ebp] = this.popI32();
            break;
        case 0x5E:// POP ESI
            this.reg32[reg_esi] = this.popI32();
            break;
        case 0x5F:// POP EDI
            this.reg32[reg_edi] = this.popI32();
            break;
    }
    return 1;
};

CPU.prototype.exec_6X = function()
{
    var op = this.opcode & 0x0F;
    // Skip source setup, just setup the destination
    this.addrInfoDst.type = TYPE_REG;
    switch (op)
    {
        case 4:
            switch (this.virtualMemory[this.mem_eip])
            {
                case 0xA1:
                case 0xA3:
                    // Skip to the next instruction to avoid invalid reads/writes
                    this.mem_eip += 5;
                    return 1;
                default:
                    throw "TODO: Gracefully handle FS prefix (redirect memory to a garbage read/write memory block)";
            }
            //this.prefix |= PREFIX_FS;
            return 0;
        case 6:
            //this.prefix |= PREFIX_SIZE;
            this.opsize = 2;
            return 0;
        case 0xA:
            // not certain this should be sign extended
            this.push_X(this.fetchI8(), this.opsize);//this.push_X(this.se_bd(this.fetchI8()), this.opsize);
            break;
        default:
            throw "exec_6X unhandled opcode " + this.opcode.toString(16);
    }
    return 1;
};

CPU.prototype.exec_7X = function()
{
    var op = this.opcode & 0x0F;
    var imm = this.fetchIx(this.opsize);
    var branch = false;
    
    // This is functionally the same as exec_7X (TODO: Merge them?)
    switch (op)
    {
        case 0:// JO
            branch = this.cpu_eflags & 0x800;// xO/xOF/xOVERFLOW
            break;
        case 1:// JNO
            branch = !(this.cpu_eflags & 0x800);// xNO (!xO)
            break;
        case 2:// B/NAE/C
            branch = this.cpu_eflags & 0x1;// xB/xCF
            break;
        case 3:// NB/AE/NC
            branch = !(this.cpu_eflags & 0x1);// xNB/xAE/xNC (!xB)
            break;
        case 4:// E/Z
            branch = this.cpu_eflags & 0x40;// xZ/xE/xZF
            break;
        case 5:// NE/NZ
            branch = !(this.cpu_eflags & 0x40);// xNZ/xNE (!xZF)
            break;
        case 6:// BE/NA
            branch = (this.cpu_eflags & (0x40 | 0x01));// xBE (xZF | xCF)
            break;
        case 7:// NBE/A
            branch = !(this.cpu_eflags & (0x40 | 0x01));// xA/xNBE (!xBE)
            break;
        case 8:// S
            branch = this.cpu_eflags & 0x80;// xS/xSF
            break;
        case 9:// NS
            branch = !(this.cpu_eflags & 0x80);// xNS (!xS/!xSF)
            break;
        case 0xA:// P/PE
            branch = this.cpu_eflags & 0x4;// xP/xPF
            break;
        case 0xB:// NP/PO
            branch = !(this.cpu_eflags & 0x4);// xNP (!xP/!xPF)
            break;
        case 0xC:// L/NGE
            branch = (((this.cpu_eflags & (0x80 | 0x800)) == 0x80) || ((this.cpu_eflags & (0x80 | 0x800)) == 0x800));// xL
            break;
        case 0xD:// NL/GE
            branch = (((this.cpu_eflags & (0x80 | 0x800)) == 0) || ((this.cpu_eflags & (0x80 | 0x800)) == (0x80 | 0x800)));// xGE
            break;
        case 0xE:// LE/NG
            branch = (((this.cpu_eflags & (0x80 | 0x800)) == 0x80) || ((this.cpu_eflags & (0x80 | 0x800)) == 0x800) || (this.cpu_eflags & 0x40));// xLE
            break;
        case 0xF:// NLE/G
            branch = ((((this.cpu_eflags & (0x80 | 0x800)) == 0) || ((this.cpu_eflags & (0x80 | 0x800)) == (0x80 | 0x800))) && !(this.cpu_eflags & 0x40));// xG
            break;
    }
    if (branch)
    {
        this.mem_eip += imm;//(this.opsize == 1) ? this.se_bd(imm) : imm;
    }    
    return 1;
};

CPU.prototype.exec_8X = function()
{
    var op = this.opcode & 0x0F;
    var size = op & 1 ? this.opsize : 1;    
    switch (op)
    {
        case 0: case 1: case 2: case 3:
            // 83 is sign extended unsigned char->unsigned int
            this.opsize = size;
            this.fetchOperands(this.addrInfoSrc, this.addrInfoDst);
            var subop = ((this.addrInfoSrc.addr << 24) >> 24);// >>> 0;// Convert to uint8 (>>> 0 might not be needed here)
            var op2 = this.fetchIx((op == 1) ? this.opsize : 1);
            //if (op == 3) op2 = this.se_bd(op2);
            op1 = this.getOperand(this.addrInfoDst);
            // ADD, OR, ADC, SBB, AND, SUB, XOR, CMP
            switch (subop)
            {
                case 0:// ADD
                    this.storeOperand(this.addrInfoDst, this.op_add(op1, op2));
                    break;
                case 1:// OR
                    this.storeOperand(this.addrInfoDst, this.op_OR(op1, op2));
                    break;
                case 2:// ADC
                    this.storeOperand(this.addrInfoDst, this.op_adc(op1, op2));
                    break;
                case 3:// SBB
                    this.storeOperand(this.addrInfoDst, this.op_sbb(op1, op2));
                    break;
                case 4:// AND
                    this.storeOperand(this.addrInfoDst, this.op_AND(op1, op2));
                    break;
                case 5:// SUB
                    this.storeOperand(this.addrInfoDst, this.op_sub(op1, op2));
                    break;
                case 6: //XOR
                    this.storeOperand(this.addrInfoDst, this.op_XOR(op1, op2));
                    break;
                case 7:
                    this.op_cmp(op1, op2);
                    break;
                default:
                    throw "Unknown sub op in exec_8X:0-3 " + subop;
            }
            break;
        
        case 4: case 5: case 6: case 7:
            this.opsize = size;
            this.fetchOperands(this.addrInfoSrc, this.addrInfoDst);
            if (op < 6)
            {
                // TEST
                this.op_AND(this.getOperand(this.addrInfoSrc, this.getOperand(this.addrInfoDst)));
            }
            else
            {
                // XCHG
                var temp = this.getOperand(this.addrInfoDst);
                this.storeOperand(this.addrInfoDst, this.getOperand(this.addrInfoSrc));
                this.storeOperand(this.addrInfoSrc, temp);
            }
            break;
        
        case 8: case 9: case 0xA: case 0xB:// MOV
            this.opsize = size;
            this.decodeAddressingModes();
            this.storeOperand(this.addrInfoDst, this.getOperand(this.addrInfoSrc));
            break;
            
        case 0xD:// LEA
            this.fetchOperands(this.addrInfoDst, this.addrInfoSrc);// Generate the address
            this.storeOperand(this.addrInfoDst, this.addrInfoSrc.addr);// Store the address
            break;
        
        default:
            throw "exec_8X unhandled opcode " + this.opcode.toString(16);
    }
    return 1;
};

CPU.prototype.exec_9X = function()
{
    var op = this.opcode & 0xF;
    this.addrInfoDst.type = TYPE_REG;
    if (this.opcode == 0x90)
    {
        // 0x90 / NOP
    }
    else
    {
        switch (op)
        {
            case 8:// CBW/CWDE
                this.addrInfoDst.addr = reg_eax;
                if (this.opsize == 2) this.storeOperand(this.addrInfoDst, this.se_bw(this.reg32[reg_eax]));
                else this.storeOperand(this.addrInfoDst, this.se_wd(this.reg32[reg_eax]));
                break;
            case 9:// CWD/CDQ
                this.addrInfoDst.addr = reg_edx;
                var temp = this.reg32[reg_eax] & SIGN_BITS[this.opsize] ? 0xFFFFFFFF : 0;
                this.storeOperand(this.addrInfoDst, temp);
                break;
            default:
                throw "exec_9X unhandled opcode " + this.opcode.toString(16);
        }
    }
    return 1;
};

CPU.prototype.exec_AX = function()
{
    var op = this.opcode & 0xF;
    var override = false;//this.prefix & PREFIX_ADDR;
    switch (op)
    {
        case 0:// Segemented MOV moffs
            this.opsize = 1;
        case 1:// Segemented MOV moffs
            if (override)
            {
                this.addrInfoSrc.addr = this.fetchI16();
            }
            else
            {
                this.addrInfoSrc.addr = this.fetchI32();
            }
            this.addrInfoSrc.type = TYPE_MEM;
            this.storeOperand(this.addrInfoDst, this.getOperand(this.addrInfoSrc));
            break;
        case 2:// Segemented MOV moffs
            this.opsize = 1;
        case 3:// Segemented MOV moffs
            if (override)
            {
                this.addrInfoDst.addr = this.fetchI16();
            }
            else
            {
                this.addrInfoDst.addr = this.fetchI32();
            }
            this.addrInfoDst.type = TYPE_MEM;
            this.storeOperand(this.addrInfoDst, reg_eax);
            break;
        default:
            throw "exec_AX unhandled opcode " + this.opcode.toString(16);
    }
    return 1;
};

CPU.prototype.exec_BX = function()
{
    var op = this.opcode & 0xF;
    this.addrInfoDst.addr = op & 7;
    this.addrInfoDst.type = TYPE_REG;
    if (op < 8)
    {
        var data = this.fetchI8();
        if (op < 4)
        {
            this.opsize = 1;
            this.storeOperand(this.addrInfoDst, data);
        }
        else
        {
            this.reg32[this.addrInfoDst.addr & 3] &= ~0x0000FF00;// H_MASK
            data <<= 8;
            this.reg32[this.addrInfoDst.addr & 3] |= (data & 0x0000FF00);// H_MASK
        }
    }
    else
    {
        this.storeOperand(this.addrInfoDst, this.fetchIx(this.opsize));
    }
    return 1;
};

CPU.prototype.exec_CX = function()
{
    var op = this.opcode & 0xF;
    switch (op)
    {
        case 0:
            this.opsize = 1;
        case 1:// SHFT Group 2
            this.fetchOperands(this.addrInfoSrc, this.addrInfoDst);
            var subop = this.addrInfoSrc.addr;
            var delta = this.fetchU8() & 0x1F;// Shift amount
            if (delta)
            {
                var temp = this.getOperand(this.addrInfoDst);
                switch (subop)
                {
                    case 0:// ROL
                        this.storeOperand(this.addrInfoDst, this.op_rol(temp, delta));
                        break;
                    case 1:// ROR
                        this.storeOperand(this.addrInfoDst, this.op_ror(temp, delta));
                        break;
                    case 2:// RCL
                        this.storeOperand(this.addrInfoDst, this.op_rcl(temp, delta));
                        break;
                    case 3:// RCR
                        this.storeOperand(this.addrInfoDst, this.op_rcr(temp, delta));
                        break;
                    case 4:// SHL/SAL
                        this.storeOperand(this.addrInfoDst, this.op_shl(temp, delta));
                        break;
                    case 5:// SHR
                        this.storeOperand(this.addrInfoDst, this.op_shr(temp, delta));
                        break;
                    case 7:// SAR
                        this.storeOperand(this.addrInfoDst, this.op_sar(temp, delta));
                        break;
                    default:
                        throw "exec_CX unhandled SHFT " + subop;
                        break;
                }
            }
            break;
        case 2:// RETN Iw
            var delta = this.fetchI16();
            this.mem_eip = this.popU32() - this.virtualMemoryAddress;
            this.reg32[reg_esp] += delta;
            break;
        case 3:// RETN
            this.mem_eip = this.popU32() - this.virtualMemoryAddress;
            break;
        case 9:// LEAVE
            this.reg32[reg_esp] = this.reg32[reg_ebp];
            this.reg32[reg_ebp] = this.popU32();
            break;
        default:
            throw "exec_CX unhandled opcode " + this.opcode.toString(16);
    }
    return 1;
};

CPU.prototype.exec_DX = function()
{
    var op = this.opcode & 0x0F;
    var subop;
    if (op > 7)
    {
        if (useTypedArrays)
        {
            return this.fpu.exec_fpu();
        }
        else
        {
            throw "Typed arrays must be enabled for floating point operations to work.";
            this.fetchOperands(this.addrInfoDst, this.addrInfoSrc);
            return 1;
        }
    }
    switch (op)
    {
        case 0: case 2:
            this.opsize = 1;
            // Fall through
        case 1: case 3:// SHFT Group 2
            this.fetchOperands(this.addrInfoSrc, this.addrInfoDst);
            subop = this.addrInfoSrc.addr;
            var delta = op < 2 ? 1 : this.reg32[reg_ecx] & 0x1F;
            var temp = this.getOperand(this.addrInfoDst);
            switch (subop)
            {
                case 0:// ROL
                    this.storeOperand(this.addrInfoDst, this.op_rol(temp, delta));
                    break;
                case 1:// ROR
                    this.storeOperand(this.addrInfoDst, this.op_ror(temp, delta));
                    break;
                case 2:// RCL
                    this.storeOperand(this.addrInfoDst, this.op_rcl(temp, delta));
                    break;
                case 3:// RCR
                    this.storeOperand(this.addrInfoDst, this.op_rcr(temp, delta));
                    break;
                case 4:// SHL/SAL
                    this.storeOperand(this.addrInfoDst, this.op_shl(temp, delta));
                    break;
                case 5:// SHR
                    this.storeOperand(this.addrInfoDst, this.op_shr(temp, delta));
                    break;
                case 7:// SAR
                    this.storeOperand(this.addrInfoDst, this.op_sar(temp, delta));
                    break;
                default:
                    throw "exec_DX unhandled opcode " + this.opcode.toString(16);
            }
            break;
        default:
            throw "exec_DX unhandled opcode " + this.opcode.toString(16);
    }
    return 1;
};

CPU.prototype.exec_EX = function()
{
    var op = this.opcode & 0x0F;
    switch (op)
    {
        case 8:// CALL
            var disp = this.fetchIx(this.opsize);
            if (this.opsize == 2) disp = this.se_wd(disp);
            this.exec_call(this.get_eip() + disp);
            break;
        case 9:// JMP
            var disp = this.fetchIx(this.opsize);
            if (this.opsize == 2) disp = this.se_wd(disp);
            this.mem_eip += disp;
            break;
        case 0xB:// JMP
            var disp = this.fetchI8();//var disp = this.se_bd(this.fetchI8());
            this.mem_eip += disp;
            break;
        default:
            throw "exec_EX unhandled opcode " + this.opcode.toString(16);
    }
    return 1;
};

CPU.prototype.exec_FX = function()
{
    var op = this.opcode & 0x0F;
    if ((op & 7) > 5)// Subgroup
    {
        this.fetchOperands(this.addrInfoSrc, this.addrInfoDst);
        var subop = this.addrInfoSrc.addr;
        if (op < 8)// Unary group 3
        {
            if (op == 6) this.opsize = 1;
            switch (subop)
            {
                case 0:// TEST
                    this.op_AND(this.getOperand(this.addrInfoDst), this.fetchUx(this.opsize));                    
                    break;
                case 1:// NOT
                    this.storeOperand(this.addrInfoDst, ~this.getOperand(this.addrInfoDst));
                    break;
                case 3:// NEG
                    var temp = this.getOperand(this.addrInfoDst);
                    this.storeOperand(this.addrInfoDst, this.op_sub(0, temp));//temp >>> 0
                    if (temp) this.cpu_eflags |= 0x1;// xCF/xCARRY
                    else this.cpu_eflags &= ~0x1;
                    break;
                case 4: case 5:// MUL: IMUL: (CF/OF incorrect for IMUL
                    // NOTE: I'm not sure if this will work for javascript (at least not for 32 bit multiplication)
                    // - v86 seems to split high/low and do 16 bit multiplication and then it combines them
                    // - What values fail? The carry flag is broken for sure (>>32) but what about the multiplication?
                    this.addrInfoSrc.addr = this.addrInfoDst.addr;
                    this.addrInfoSrc.type = this.addrInfoDst.type;
                    var temp = this.getOperand(this.addrInfoSrc);
                    this.addrInfoDst.addr = reg_eax;// Change dest to EAX
                    this.addrInfoDst.type = TYPE_REG;
                    var multiplier = this.getOperand(this.addrInfoDst);
                    if (this.opsize == 1)
                    {
                        temp *= multiplier;
                        
                        this.opsize = 2;
                        this.storeOperand(this.addrInfoDst, temp);//temp >>> 0
                        temp >>= 8;
                    }
                    else if (this.opsize == 2)
                    {
                        temp *= multiplier;
                        
                        this.storeOperand(this.addrInfoDst, temp);//temp >>> 0
                        this.addrInfoDst.addr = this.reg32[reg_edx];
                        temp >>= 16;//this.opsize == 2 ? 16 : 32;
                        this.storeOperand(this.addrInfoDst, temp);//temp >>> 0
                    }
                    else
                    {
                        // 4=MUL, 5=IMUL
                        this.arith_do_mul32(temp, multiplier, subop == 5);
                        break;
                    }
                    if (temp) this.cpu_eflags |= 0x1;// xCF/xCARRY
                    else this.cpu_eflags &= ~0x1;
                    break;
                case 6: case 7:// DIV: IDIV: (does this work for IDIV?)
                    this.addrInfoSrc.addr = this.addrInfoDst.addr;
                    this.addrInfoSrc.type = this.addrInfoDst.type;                    
                    
                    var divisor = this.getOperand(this.addrInfoSrc);
                    if (divisor == 0)
                    {
                        throw "Divide by 0 error. Instruction opcode: " + this.opcode.toString(16) + " EIP: " + this.get_eip().toString(16);
                    }
                    
                    var temp;
                    if (this.opsize == 1)
                    {
                        temp = this.reg32[reg_eax] & 0xFFFF;
                    }
                    else if (this.opsize == 2)
                    {
                        temp = ((this.reg32[reg_edx] & 0xFFFF) << 16) | (this.reg32[reg_eax] & 0xFFFF);
                    }
                    else
                    {
                        //temp = this.reg32[reg_edx];
                        //temp <<= 32;
                        //temp |= this.reg32[reg_eax];
                        
                        // 6=DIV, 7=IDIV
                        this.arith_do_div32(this.reg32[reg_eax], this.reg32[reg_edx], divisor, subop == 7);
                        break;
                    }
                    
                    // I8/I16 only (I32 wont work for this)
                    this.addrInfoDst.addr = reg_eax;
                    this.addrInfoDst.type = TYPE_REG;
                    this.storeOperand(this.addrInfoDst, temp / divisor);
                    this.addrInfoDst.addr = reg_edx;
                    this.storeOperand(this.addrInfoDst, temp % divisor);
                    break;
                default:
                    throw "exec_FX unhandled opcode " + this.opcode.toString(16);
            }
        }
        else// Group 4/5
        {
            if (op == 0xE) this.opsize = 1;// Should only be a group 4
            if (subop < 2)// INC/DEC
            {
                if (subop == 0) result = this.op_inc(this.getOperand(this.addrInfoDst));
                else result = this.op_dec(this.getOperand(this.addrInfoDst));
                this.storeOperand(this.addrInfoDst, result);
            }
            else
            {
                switch (subop)
                {
                    case 2:// CALLN
                        this.exec_call(this.getOperand(this.addrInfoDst));
                        break;
                    case 4:// JMPN
                        var addr = this.getOperand(this.addrInfoDst);
                        if (!this.exec_check_jump_function(addr))
                        {
                            this.mem_eip = addr - this.virtualMemoryAddress;
                        }
                        break;
                    case 6:// PUSH
                        this.push_X(this.getOperand(this.addrInfoDst), this.opsize);
                        break;
                    default:
                        throw "exec_FX unhandled opcode " + this.opcode.toString(16);
                        break;
                }
            }
        }
    }
    else
    {
        throw "exec_FX unhandled opcode " + this.opcode.toString(16);
    }
    return 1;
};

CPU.prototype.execute_instruction = function()
{
    this.check_stack_memory();
    
    var done = false;
    
    this.addrInfoDst.addr = this.addrInfoSrc.addr = 0;//this.prefix = 0;
    this.opsize = 4;
    
    while (!done)
    {
        this.opcode = this.virtualMemory[this.mem_eip++];
        //webcLog(this.get_eip().toString(16) + " " + "opcode: " + this.opcode.toString(16));
        switch ((this.opcode >> 4) & 0x0F)
        {
            case 0:
                done = this.exec_0X();
                break;
            case 1:
                done = this.exec_1X();
                break;
            case 2:
                done = this.exec_2X();
                break;
            case 3:
                done = this.exec_3X();
                break;
            case 4:
                done = this.exec_4X();
                break;
            case 5:
                done = this.exec_5X();
                break;
            case 6:
                done = this.exec_6X();
                break;
            case 7:
                this.opsize = 1;
                done = this.exec_7X();
                break;
            case 8:
                done = this.exec_8X();
                break;
            case 9:
                done = this.exec_9X();
                break;
            case 10:
                done = this.exec_AX();
                break;
            case 11:
                done = this.exec_BX();
                break;
            case 12:
                done = this.exec_CX();
                break;
            case 13:
                done = this.exec_DX();
                break;
            case 14:
                done = this.exec_EX();
                break;
            case 15:
                done = this.exec_FX();
                break;
            default:
                throw "Unhandled opcode " + this.opcode;
        }
    }
}