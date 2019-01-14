CPU.prototype.shared_ctor = function()
{
    // Memory layout: [IMAGE MEMORY][STACK MEMORY][HEAP MEMORY] (all one array / typed array)
    // - A few things currently depend on this layout (search for MEM_IMAGE_STACK_HEAP)
    //   - memEIP also depends on ImageBase being at virtualMemoryAddress ([IMAGE MEMORY]) in various places
    // - TODO: We may get a performance gain if we used seperate arrays for each memory type on browsers without typed arrays
    this.virtualMemory = null;// The virtual memory of our virtual process
    this.virtualMemoryAddress = 0;// The base address of the virtual memory
    this.virtualMemoryImageSize = 0;// The fixed size of the executable in memory
    this.virtualMemorySize = 0;// Image memory size + stack memory size + heap memory size
    this.virtualMemoryStackAddress = 0;// This address comes directly after the "image" memory (memAddr+imageSize)
    this.virtualMemoryStackSize = 0x100000;//1MB //8192;//1024;
    this.virtualMemoryHeapAddress = 0;// This address comes directly after the stack memory
    this.virtualMemoryHeapSize = 0xA00000;//10MB //8192;//1024;
    this.virtualMemoryHeapEndAddress = 0;// The address where the heap ends (virtualMemoryHeapAddress + virtualMemoryHeapSize)
    this.virtualEntryPointAddress = 0;// The virtual address of the first instruction to execute
    
    // Defines the fake imports (functions/data defined in external modules/dlls such as printf, CreateFile, sleep/Sleep, etc)
    // NOTE: Lookups are done based on int32 size. Values gets multiplied or divided by 4 for lookups (see import resolving/exec_Call)
    //       This gives the feeling that imports are correctly spaced apart rather than seemingly being 1 byte (aka indice) apart.
    this.fakeImports = null;
    this.fakeImportsAddress = 0;
    this.unresolvedFuncImportHandler = null;
    
    // An offset into the virtualMemory array so that we don't have to keep calculating the offset each time EIP modified
    // Use get_eip() / set_eip() to access the regular version of the EIP
    this.mem_eip = 0;
    
    this.complete = false;
    this.initialized = false;
};

CPU.prototype.shared_init = function()
{
    if (this.initialized)
    {
        throw "emulator init called twice";
    }
    
    // webc86.js sets up the following for us:
    // virtualMemory, virtualMemoryAddress, virtualMemoryImageSize, virtualEntryPointAddress
    
    // Zero the stack / heap?
    this.virtualMemoryStackAddress = this.virtualMemoryAddress + this.virtualMemoryImageSize;
    this.virtualMemoryHeapAddress = this.virtualMemoryStackAddress + this.virtualMemoryStackSize;
    this.virtualMemoryHeapEndAddress = this.virtualMemoryHeapAddress + this.virtualMemoryHeapSize;
    
    this.heap = new MemMgr(this);
    
    this.reg32[0]=this.reg32[1]=this.reg32[2]=this.reg32[3]=this.reg32[4]=this.reg32[5]=this.reg32[6]=this.reg32[7]=0;
    this.reg32[reg_esp] = this.virtualMemoryStackAddress + this.virtualMemoryStackSize;
    this.set_eip(this.virtualEntryPointAddress);
    
    this.complete = false;
    this.initialized = true;
};

CPU.prototype.shared_destroy = function()
{
    this.complete = false;
    this.initialized = false;
    this.virtualMemory = null;
    this.virtualMemoryAddress = 0;
    this.fakeImports = null;
    this.fakeImportsAddress = 0;
    this.unresolvedFuncImportHandler = null;
};

CPU.prototype.getCallstack = function(maxFrames)
{
    var result = "";
    var ebp = this.reg32[reg_ebp];
    for (var i = 0; i < maxFrames; i++)
    {
        var eip = this.safe_read32(ebp + 4);
        if (!this.isValidAddress(eip, 1))
        {
            break;
        }
        ebp = this.safe_read32(ebp + 0);
        
        if (result.length != 0)
        {
            result += ", ";
        }
        result += "0x" + eip.toString(16);
    }
    return result;
};

///////////////////////////////////////////////////////////////////////
// ESP validation, call this before (or after) executing each instruction
///////////////////////////////////////////////////////////////////////

CPU.prototype.check_stack_memory = function()
{
    // MEM_IMAGE_STACK_HEAP
    var esp = this.reg32[reg_esp];
    if (esp < this.virtualMemoryStackAddress)
    {
        webcLog("overflow");
        throw "Emulator encountered a stack overflow (" + (this.virtualMemoryStackAddress - esp) + " byte(s))";
    }
    else if (esp > this.virtualMemoryHeapAddress)
    {
        throw "Emulator encountered a stack underflow (" + (esp - this.virtualMemoryHeapAddress) + " byte(s))";
    }
};

///////////////////////////////////////////////////////////////////////
// CALL / JMP functions for handling API function calls
///////////////////////////////////////////////////////////////////////

CPU.prototype.exec_call = function(addr)
{
    // MEM_IMAGE_STACK_HEAP
    if (addr >= this.virtualMemoryAddress)
    {
        if (addr < this.virtualMemoryHeapEndAddress)
        {
            this.push32(this.get_eip());
            this.set_eip(addr | 0);
            return;
        }
    }
    else if (addr >= this.fakeImportsAddress)
    {
        var fakeImportOffset = (addr - this.fakeImportsAddress) / 4;
        //webcLog("call " + this.fakeImports[fakeImportOffset].Callback.name + " ret: " + this.get_eip().toString(16));
        this.fakeImports[fakeImportOffset].Callback(this);
        return;
    }
    throw "exec_call invalid function address " + addr.toString(16) + " EIP: " + get_eip().toString(16);
};

CPU.prototype.exec_check_jump_function = function(addr)
{
    // MEM_IMAGE_STACK_HEAP
    if (addr >= this.virtualMemoryAddress)
    {
        if (addr < this.virtualMemoryHeapEndAddress)
        {
            return 0;
        }
    }
    else if (addr >= this.fakeImportsAddress)
    {
        // Get the return address into eip
        this.set_eip(this.pop32s());
        
        var fakeImportOffset = (addr - this.fakeImportsAddress) / 4;
        //webcLog("call " + this.fakeImports[fakeImportOffset].Callback.name + " ret: " + this.get_eip().toString(16));
        this.fakeImports[fakeImportOffset].Callback(this);
        return 1;
    }
    throw "exec_check_jump_function invalid address " + addr.toString(16) + " EIP: " + get_eip().toString(16);
    return 0;

};

///////////////////////////////////////////////////////////////////////
// Conversion between virtual and real addresses
///////////////////////////////////////////////////////////////////////

CPU.prototype.addressToOffset = function(addr)
{
    return addr - this.virtualMemoryAddress;
};

CPU.prototype.offsetToAddress = function(offset)
{
    return this.virtualMemoryAddress + offset;
};

CPU.prototype.translate_address_read = function(addr)
{
    return addr - this.virtualMemoryAddress;
};

CPU.prototype.translate_address_write = function(addr)
{
    return addr - this.virtualMemoryAddress;
};

///////////////////////////////////////////////////////////////////////
// Register getter/setter helpers
///////////////////////////////////////////////////////////////////////

CPU.prototype.get_stack_reg = function()
{
    return this.reg32[reg_esp];
};

CPU.prototype.set_stack_reg = function(value)
{
    this.reg32[reg_esp] = value;
};

CPU.prototype.adjust_stack_reg = function(value)
{
    this.reg32[reg_esp] += value;
};

CPU.prototype.get_esp = function(mod)
{
    return (this.reg32[reg_esp] | 0) + (mod | 0);
};

CPU.prototype.set_esp = function(value)
{
    this.reg32[reg_esp] = value;
};

CPU.prototype.get_eip = function()
{
    return this.mem_eip + this.virtualMemoryAddress;
};

CPU.prototype.set_eip = function(addr)
{
    this.mem_eip = addr - this.virtualMemoryAddress;
};

CPU.prototype.get_eax = function()
{
    return this.reg32[reg_eax];
};

CPU.prototype.set_eax = function(value)
{
    this.reg32[reg_eax] = value;
};

CPU.prototype.set_edx = function(value)
{
    this.reg32[reg_edx] = value;
};

///////////////////////////////////////////////////////////////////////
// read / write functions
// (these operate on offsets into the virtual memory array)
///////////////////////////////////////////////////////////////////////

/**
 * @param {number} addr
 */
CPU.prototype.read8 = function(addr)
{
    return this.virtualMemory[addr];
};

/**
 * @param {number} addr
 */
CPU.prototype.read16 = function(addr)
{
    return this.virtualMemory[addr] | this.virtualMemory[addr + 1 | 0] << 8;
};

/**
 * @param {number} addr
 */
CPU.prototype.read32s = function(addr)
{
    return this.virtualMemory[addr] | this.virtualMemory[addr + 1 | 0] << 8 | this.virtualMemory[addr + 2 | 0] << 16 | this.virtualMemory[addr + 3 | 0] << 24;
};

/**
 * @param {number} addr
 * @param {number} value
 */
CPU.prototype.write8 = function(addr, value)
{
    this.virtualMemory[addr] = (value & 0xFF);
};

/**
 * @param {number} addr
 * @param {number} value
 */
CPU.prototype.write16 = function(addr, value)
{
    this.virtualMemory[addr] = (value & 0xFF);
    this.virtualMemory[addr + 1 | 0] = (value >> 8) & 0xFF;
};

/**
 * @param {number} addr
 * @param {number} value
 */
CPU.prototype.write32 = function(addr, value)
{
    this.virtualMemory[addr] = (value & 0xFF);
    this.virtualMemory[addr + 1 | 0] = (value >> 8) & 0xFF;
    this.virtualMemory[addr + 2 | 0] = (value >> 16) & 0xFF;
    this.virtualMemory[addr + 3 | 0] = (value >> 24) & 0xFF;
};

///////////////////////////////////////////////////////////////////////
// safe_read, safe_write
// read or write byte, word or dword to the given *virtual* address
///////////////////////////////////////////////////////////////////////

CPU.prototype.validateAddress = function(addr, size)
{
    if (!this.isValidAddress(addr, size))
    {
        throw "Invalid address accessed " + addr.toString(16);
    }
};

CPU.prototype.isValidAddress = function(addr, size)
{
    // MEM_IMAGE_STACK_HEAP
    return addr >= this.virtualMemoryAddress && addr + (size | 0) < this.virtualMemoryHeapEndAddress;
};

CPU.prototype.isValidAddressEx = function(addr, size)
{
    // This version also includes the fake import address range (which should come directly before virtualMemory)
    
    // MEM_IMAGE_STACK_HEAP
    if (addr >= this.virtualMemoryAddress)
    {
        return addr + (size | 0) < this.virtualMemoryHeapEndAddress;
    }
    return addr >= this.fakeImportsAddress;
};

CPU.prototype.safe_read_string = function(addr)
{
    // MEM_IMAGE_STACK_HEAP
    addr = this.translate_address_read(addr);
    var addrEnd = this.virtualMemoryHeapEndAddress;
    
    var result = "";
    for (var i = addr; i < addrEnd; i++)
    {
        var val = this.virtualMemory[i];
        if (val == 0)
        {
            break;
        }
        result += String.fromCharCode(val);
    }
    return result;
};

CPU.prototype.safe_read8 = function(addr)
{
    dbg_assert(addr < 0x80000000);
    return this.read8(this.translate_address_read(addr));
};

CPU.prototype.safe_read16 = function(addr)
{
    return this.read16(this.translate_address_read(addr));
};

CPU.prototype.safe_read32s = function(addr)
{
    return this.read32s(this.translate_address_read(addr));
};

CPU.prototype.safe_read32 = function(addr)
{
    return this.read32s(this.translate_address_read(addr)) >>> 0;
};

CPU.prototype.safe_write8 = function(addr, value)
{
    dbg_assert(addr < 0x80000000);
    this.write8(this.translate_address_write(addr), value);
};

CPU.prototype.safe_write16 = function(addr, value)
{
    this.write16(this.translate_address_write(addr), value);
};

CPU.prototype.safe_write32 = function(addr, value)
{
    this.write32(this.translate_address_write(addr), value);
};

CPU.prototype.safe_write64 = function(addr, low, high)
{
    this.writable_or_pagefault(addr, 8);
    this.safe_write32(addr, low);
    this.safe_write32(addr + 4 | 0, high);
};

CPU.prototype.safe_write128 = function(addr, d0, d1, d2, d3)
{
    this.writable_or_pagefault(addr, 16);
    this.safe_write32(addr, d0);
    this.safe_write32(addr + 4 | 0, d1);
    this.safe_write32(addr + 8 | 0, d2);
    this.safe_write32(addr + 12 | 0, d3);
};

CPU.prototype.writable_or_pagefault = function(addr, size)
{
    dbg_assert(addr >= this.virtualMemoryAddress && addr < this.virtualMemoryHeapEndAddress);
};

///////////////////////////////////////////////////////////////////////
// Register getters
///////////////////////////////////////////////////////////////////////

CPU.prototype.getReg8 = function(reg)
{
    return this.reg32[reg] & SIZE_MASK_8;
};

CPU.prototype.getReg8s = function(reg)
{
    return (this.reg32[reg] << 24) >> 24;
};

CPU.prototype.getReg16 = function(reg)
{
    return this.reg32[reg] & SIZE_MASK_16;
};

CPU.prototype.getReg16s = function(reg)
{
    return (this.reg32[reg] << 16) >> 16;
};

CPU.prototype.getReg32 = function(reg)
{
    return this.reg32[reg] >>> 0;
};

CPU.prototype.getReg32s = function(reg)
{
    return this.reg32[reg] | 0;
};

///////////////////////////////////////////////////////////////////////
// Register setters (always store unsigned?)
///////////////////////////////////////////////////////////////////////

CPU.prototype.setReg8 = function(reg, val)
{
    this.reg32[reg] = val & SIZE_MASK_8;
};

CPU.prototype.setReg8s = function(reg, val)
{
    this.reg32[reg] = (val << 8) >> 8;
};

CPU.prototype.setReg16 = function(reg, val)
{
    this.reg32[reg] = val & SIZE_MASK_16;
};

CPU.prototype.setReg16s = function(reg, val)
{
    this.reg32[reg] = (val << 16) >> 16;
};

CPU.prototype.setReg32 = function(reg, val)
{
    this.reg32[reg] = val;
};

CPU.prototype.setReg32s = function(reg, val)
{
    this.reg32[reg] = val;
};

///////////////////////////////////////////////////////////////////////
// Push / pop functions
///////////////////////////////////////////////////////////////////////

CPU.prototype.push_X = function(val, size)
{
    switch (size)
    {
        case 1: this.push_8(val); break;
        case 2: this.push_16(val); break;
        case 4: this.push_32(val); break;
        default: throw "Invalid stack push size " + size;
    }
};

CPU.prototype.push_8 = function(val)
{
    this.virtualMemory[(--this.reg32[reg_esp]) - this.virtualMemoryAddress] = (val & 0xFF);
};

CPU.prototype.push_16 = function(val)
{
    var addr = (this.reg32[reg_esp] -= 2) - this.virtualMemoryAddress;
    this.virtualMemory[addr] = (val & 0xFF);
    this.virtualMemory[addr + 1] = (val >> 8) & 0xFF;
};

CPU.prototype.push_32 = function(val)
{
    var addr = (this.reg32[reg_esp] -= 4) - this.virtualMemoryAddress;
    this.virtualMemory[addr] = (val & 0xFF);
    this.virtualMemory[addr + 1] = (val >> 8) & 0xFF;
    this.virtualMemory[addr + 2] = (val >> 16) & 0xFF;
    this.virtualMemory[addr + 3] = (val >> 24) & 0xFF;
};

CPU.prototype.popI = function(size)
{
    switch (size)
    {
        case 1: return this.popI8();
        case 2: return this.popI16();
        case 4: return this.popI32();
        default: throw "Invalid stack pop size " + size;
    }
};

CPU.prototype.popI8 = function()
{
    var result = ((((this.virtualMemory[this.reg32[reg_esp] - this.virtualMemoryAddress]) << 24) >> 24) | 0);
    this.reg32[reg_esp]++;
    return result;
};

CPU.prototype.popI16 = function()
{
    var addr = this.reg32[reg_esp] - this.virtualMemoryAddress;
    var result = ((((this.virtualMemory[addr] + (this.virtualMemory[addr + 1] << 8)) << 16) >> 16) | 0);
    this.reg32[reg_esp] += 2;
    return result;
};

CPU.prototype.popI32 = function()
{
    var addr = this.reg32[reg_esp] - this.virtualMemoryAddress;
    var result = ((this.virtualMemory[addr] + (this.virtualMemory[addr + 1] << 8) + (this.virtualMemory[addr + 2] << 16) + (this.virtualMemory[addr + 3] << 24)) | 0);
    this.reg32[reg_esp] += 4;
    return result;
};

CPU.prototype.popU = function(size)
{
    switch (size)
    {
        case 1: return this.popU8();
        case 2: return this.popU16();
        case 4: return this.popU32();
        default: throw "Invalid stack pop size " + size;
    }
};

CPU.prototype.popU8 = function()
{
    var result = (this.virtualMemory[this.reg32[reg_esp] - this.virtualMemoryAddress]) >>> 0;
    this.reg32[reg_esp]++;
    return result;
};

CPU.prototype.popU16 = function()
{
    var addr = this.reg32[reg_esp] - this.virtualMemoryAddress;
    var result = (this.virtualMemory[addr] + (this.virtualMemory[addr + 1] << 8)) >>> 0;
    this.reg32[reg_esp] += 2;
    return result;
};

CPU.prototype.popU32 = function()
{
    var addr = this.reg32[reg_esp] - this.virtualMemoryAddress;
    var result = (this.virtualMemory[addr] + (this.virtualMemory[addr + 1] << 8) + (this.virtualMemory[addr + 2] << 16) + (this.virtualMemory[addr + 3] << 24)) >>> 0;
    this.reg32[reg_esp] += 4;
    return result;
};

CPU.prototype.push16 = function(imm16)
{
    var sp = this.get_esp(-2);

    this.safe_write16(sp, imm16);
    this.adjust_stack_reg(-2);
};

CPU.prototype.push32 = function(imm32)
{
    var sp = this.get_esp(-4);

    this.safe_write32(sp, imm32);
    this.adjust_stack_reg(-4);
};

CPU.prototype.pop16 = function()
{
    var sp = this.get_stack_reg() | 0;
    var result = this.safe_read16(sp);

    this.adjust_stack_reg(2);
    return result;
};

CPU.prototype.pop32s = function()
{
    var sp = this.get_stack_reg() | 0;
    var result = this.safe_read32s(sp);

    this.adjust_stack_reg(4);
    return result;
};

///////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////

/**
 * @define {boolean}
 * Overridden for production by closure compiler
 */
var DEBUG = true;

/**
 * @const
 * How many cycles the CPU does at a time before running hardware timers
 */
var LOOP_COUNTER = 11001;

var
// flags register bitflags
/** @const */ flag_carry = 1,
/** @const */ flag_parity = 4,
/** @const */ flag_adjust = 16,
/** @const */ flag_zero = 64,
/** @const */ flag_sign = 128,
/** @const */ flag_trap = 256,
/** @const */ flag_interrupt = 512,
/** @const */ flag_direction = 1024,
/** @const */ flag_overflow = 2048,
/** @const */ flag_iopl = 1 << 12 | 1 << 13,
/** @const */ flag_nt = 1 << 14,
/** @const */ flag_rf = 1 << 16,
/** @const */ flag_vm = 1 << 17,
/** @const */ flag_ac = 1 << 18,
/** @const */ flag_vif = 1 << 19,
/** @const */ flag_vip = 1 << 20,
/** @const */ flag_id = 1 << 21,

/**
 * default values of reserved flags bits
 * @const
 */
flags_default = 1 << 1,

/**
 * bitmask to select non-reserved flags bits
 * @const
 */
flags_mask =
    flag_carry | flag_parity | flag_adjust | flag_zero | flag_sign | flag_trap | flag_interrupt |
    flag_direction | flag_overflow | flag_iopl | flag_nt | flag_rf | flag_vm | flag_ac |
    flag_vif | flag_vip | flag_id,


/**
 * all arithmetic flags
 * @const
 */
flags_all = flag_carry | flag_parity | flag_adjust | flag_zero | flag_sign | flag_overflow;

/**
 * opsizes used by get flag functions
 *
 * @const
 */
var OPSIZE_8 = 7,
/** @const */
OPSIZE_16 = 15,
/** @const */
OPSIZE_32 = 31;

var
/** @const */ reg_eax = 0,
/** @const */ reg_ecx = 1,
/** @const */ reg_edx = 2,
/** @const */ reg_ebx = 3,
/** @const */ reg_esp = 4,
/** @const */ reg_ebp = 5,
/** @const */ reg_esi = 6,
/** @const */ reg_edi = 7,

/** @const */ reg_es = 0,
/** @const */ reg_cs = 1,
/** @const */ reg_ss = 2,
/** @const */ reg_ds = 3,
/** @const */ reg_fs = 4,
/** @const */ reg_gs = 5;

// Segment prefixes must not collide with reg_*s variables
// _ZERO is a special zero offset segment
var
    /** @const */
    SEG_PREFIX_NONE = -1,

    /** @const */
    SEG_PREFIX_ZERO = 7;

/** @const */
var PREFIX_MASK_REP = 0x18;//0b11000;
/** @const */
var PREFIX_REPZ = 0x8;//0b01000;
/** @const */
var PREFIX_REPNZ = 0x10;//0b10000;

/** @const */
var PREFIX_MASK_SEGMENT = 0x7;//0b111;

/** @const */
var PREFIX_MASK_OPSIZE = 0x20;//0b100000;
/** @const */
var PREFIX_MASK_ADDRSIZE = 0x40;//0b1000000;

/** @const */
var PREFIX_F2 = PREFIX_REPNZ; // alias
/** @const */
var PREFIX_F3 = PREFIX_REPZ; // alias
/** @const */
var PREFIX_66 = PREFIX_MASK_OPSIZE; // alias

/** @const */
var SIZE_MASK_8 = 0xFF;
/** @const */
var SIZE_MASK_16 = 0xFFFF;
/** @const */
var SIZE_MASK_32 = 0xFFFFFFFF;

///////////////////////////////////////////////////////////////////////
// Logging functions
///////////////////////////////////////////////////////////////////////

function do_the_log(message)
{
}

/**
 * @type {function((string|number), number=)}
 * @const
 */
function dbg_log()
{
}

/**
 * @param {number=} level
 */
function dbg_trace(level)
{
}

/**
 * console.assert is fucking slow
 * @param {string=} msg
 * @param {number=} level
 */
function dbg_assert(cond, msg, level)
{
    if (!cond)
    {
        dbg_assert_failed(msg);
    }
}

function dbg_assert_failed(msg)
{
    if(msg)
    {
        throw "Assert failed: " + msg;
    }
    else
    {
        throw "Assert failed";
    }
}