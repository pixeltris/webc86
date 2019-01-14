// A rough port of a fixed buffer memory manager written in C https://github.com/eliben/code-for-blog/tree/master/2008/memmgr

var MIN_POOL_ALLOC_QUANTAS = 16;

function MemMgr(cpu)
{
    this.cpu = cpu;
    this.blocks = new Array();
    this.baseBlock = { addr:-1, size:0, next:null };
    this.freep = null;
    
    this.poolAddr = this.cpu.virtualMemoryHeapAddress;
    this.poolFreePos = 0;
    this.poolSizeInBytes = this.cpu.virtualMemoryHeapSize;
}

MemMgr.prototype.getStats = function()
{
    var result = "";
        
    var freeBytes = 0;
    freeBytes += (this.poolSizeInBytes - this.poolFreePos);
    if (this.freep != null)
    {
        var p = this.freep;
        
        while (true)
        {
            if (p.addr > 0)
            {
                freeBytes += p.size;
            }
            
            p = p.next;
            
            if (p == this.freep || p.next == null)
            {
                break;
            }
        }
    }
    
    var size = this.poolSizeInBytes;
    var allocatedBytes = size - freeBytes;
    
    result += "Size: " + this.poolSizeInBytes + " allocated: " + allocatedBytes + " free: " + freeBytes + "\n\n";
    
    result += "Pool: freePos = " + this.poolFreePos + " (" + (this.poolSizeInBytes - this.poolFreePos) + " bytes left)\n\n";
    
    var ptr = this.poolAddr;
    while (ptr < this.poolAddr + this.poolFreePos)
    {
        var block = this.blocks[ptr];
        if (block == null)
        {
            throw "Failed to get block at address " + ptr.toString(16);
        }
        result += "  * Addr: " + block.addr.toString(16) + "; Size: " + block.size + "\n";
        ptr += block.size;
    }
    
    result += "\nFree list:\n\n"
    
    if (this.freep != null)
    {
        var p = this.freep;
        
        while (true)
        {
            result += "  * Addr: " + p.addr.toString(16) + "; Size: " + p.size + "; Next: " +
                (p.next == null ? 0 : p.next.addr).toString(16) + "\n";
            
            p = p.next;
            
            if (p == this.freep || p.next == null)
            {
                break;
            }
        }
    }
    else
    {
        result += "Empty\n";
    }
    
    result += "\n";
    return result;
};

MemMgr.prototype.getMemFromPool = function(nquantas)
{
    if (nquantas < MIN_POOL_ALLOC_QUANTAS)
    {
        nquantas = MIN_POOL_ALLOC_QUANTAS;
    }
    
    var totalReqSize = nquantas;
    
    if (this.poolFreePos + totalReqSize <= this.poolSizeInBytes)
    {
        var h = { addr:this.poolAddr + this.poolFreePos, size:nquantas, next:null };
        this.blocks[h.addr] = h;
        this.free(h.addr);
        this.poolFreePos += totalReqSize;
    }
    else
    {
        return null;
    }
    
    return this.freep;
};

MemMgr.prototype.calloc = function(num, size)
{
    var totalSize = num * size;
    var ptr = this.malloc(totalSize);
    if (ptr != 0)
    {
        memset(this.cpu.virtualMemory, ptr - this.cpu.virtualMemoryAddress, 0, totalSize);
    }
    return ptr;
};

MemMgr.prototype.realloc = function(ptr, size)
{
    //console.log("realloc " + ptr + " size " + size);
    if (ptr == 0)
    {
        return this.malloc(size);
    }
    
    var block = this.blocks[ptr];
    if (block == null)
    {
        throw "realloc memory block is null at " + ptr.toString(16);
    }
    
    if (block.size >= size)
    {
        return ptr;
    }
    
    var newPtr = this.malloc(size);
    if (newPtr == 0)
    {
        return 0;
    }
    memcpy(this.cpu.virtualMemory, ptr - this.cpu.virtualMemoryAddress,
        this.cpu.virtualMemory, newPtr - this.cpu.virtualMemoryAddress, block.size);
    this.free(ptr);
    return newPtr;
};

MemMgr.prototype.malloc = function(nbytes)
{
    var p;
    var prevp;
    
    var nquantas = nbytes;
    
    prevp = this.freep;
    if (prevp == null)
    {
        this.baseBlock.next = this.freep = prevp = this.baseBlock;
        this.baseBlock.size = 0;
    }
    
    for (p = prevp.next; ; prevp = p, p = p.next)
    {
        if (p != null && p.size >= nquantas)
        {
            if (p.size == nquantas)
            {
                prevp.next = p.next;
            }
            else
            {
                p.size -= nquantas;
                
                var result = { addr:p.addr+p.size, size:nquantas, next:null };
                this.blocks[result.addr] = result;
                p = result;
            }
            
            this.freep = prevp;
            return p.addr;
        }
        else if (p == null || p.addr == this.freep.addr)
        {
            p = this.getMemFromPool(nquantas);
            if (p == null)
            {
                // Memory allocation failed
                return 0;
            }
        }
    }
};

MemMgr.prototype.free = function(ptr)
{
    if (ptr == 0)
    {
        return;
    }
    
    var p;
    
    var block = this.blocks[ptr];
    if (block == null)
    {
        throw "Invalid free() call. There isn't a known block allocated at this address " + ptr.toString(16);
    }
    
    var firstLoop = true;
    for (p = this.freep; !(block.addr > p.addr && block.addr < p.next.addr); p = p.next)
    {
        if (p.addr >= p.next.addr && (block.addr > p.addr || block.addr < p.next.addr))
        {
            break;
        }
        if (p == this.freep)
        {
            if (firstLoop)
            {
                firstLoop = false;
            }
            else
            {
                throw "Possible double free() at address " + ptr.toString(16);
            }
        }
    }
    
    if (p == null)
    {
        throw "MemMgr.free() - freep is null";
    }
    
    if (block.next != null && block.addr + block.size == block.next.addr)
    {
        if (p.next == null)
        {
            throw "MemMgr.free() - p.next is null";
        }
        
        block.size += p.next.size;
        block.next = p.next.next;
    }
    else
    {
        block.next = p.next;
    }
    
    if (p.addr + p.size == block.addr)
    {
        delete this.blocks[block.addr];
        
        p.size += block.size;
        p.next = block.next;
    }
    else
    {
        p.next = block;
    }
    
    this.freep = p;
};