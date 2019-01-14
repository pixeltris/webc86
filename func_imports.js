function import_OnUnresolvedImportCalled(cpu)
{
    webcLog("[WARNING] Unresolved dll import function called! ret: " + cpu.get_eip().toString(16));
}

function import_ignore(cpu)
{
}

function copyFilePathMem(cpu, path, fileNameAddr, fileNameSize)
{
    var copiedBytes = 0;
    var offset = cpu.addressToOffset(fileNameAddr);
    for (var i = 0; i < path.length + 1 && i < fileNameSize; i++)
    {
        var charCode = 0;
        if (i != path.length && i != fileNameSize - 1)
        {
            charCode = path.charCodeAt(i);
            copiedBytes++;
        }
        cpu.virtualMemory[offset + i] = charCode;
    }
    
    cpu.set_eax(copiedBytes);
}

function import_GetModuleFileNameA(cpu)
{
    var hModule = cpu.safe_read32(cpu.get_esp(0));
    var lpFilenameAddr = cpu.safe_read32(cpu.get_esp(4));
    var nSize = cpu.safe_read32(cpu.get_esp(8));
    
    if (hModule != 0)
    {
        throw "Unsupported GetModuleFileNameA call. Only the current module file name is supported.";
    }
    
    // NOTE:
    // It's important that a fully qualified path is used otherwise tcc_set_lib_path() / tcc_lib_path is initialized to
    // an empty string which causes tcc_split_path()/cstr_cat() to fail resulting in an invalid memmove call
    var path = "C:/webc86/tcc.exe";// TODO: Use some sort of virtual file system?
    
    copyFilePathMem(cpu, path, lpFilenameAddr, nSize);
}

function import_GetSystemDirectoryA(cpu)
{
    var lpBufferAddr = cpu.safe_read32(cpu.get_esp(0));
    var uSize = cpu.safe_read32(cpu.get_esp(4));
    
    // Used by tcc_add_systemdir()
    var path = "C:/Windows/System32";
    
    copyFilePathMem(cpu, path, lpBufferAddr, uSize);
}

function import_exit(cpu)
{
    cpu.complete = true;
}

function import_setjmp(cpu)
{
    webcLog("Ignore setjmp()");
    cpu.set_eax(0);
}

function import_snprintf(cpu)
{
    snprintf_x86(cpu);
}

function import_sprintf(cpu)
{
    sprintf_x86(cpu);
}

function import_strlwr(cpu)
{
    var strAddr = cpu.safe_read32(cpu.get_esp(0));
    cpu.validateAddress(strAddr, 1);
    var str = getCString(cpu.virtualMemory, cpu.addressToOffset(strAddr));
    var strLower = str.toLocaleString();
    if (str.length != strLower.length)
    {
        throw "TODO: strlwr length missmatch";
    }
    
    var strOffset = cpu.addressToOffset(strAddr);
    for (var i = 0; i < str.length; i++)
    {
        cpu.virtualMemory[strOffset + i] = strLower.charCodeAt(i);
    }
}

function import_vsnprintf(cpu)
{
    vsnprintf_x86(cpu);
}

function import_printf(cpu)
{
    printf_x86(cpu);
}

function import_atoi(cpu)
{
    var strAddr = cpu.safe_read32(cpu.get_esp(0));
    cpu.validateAddress(strAddr, 1);
    var str = getCString(cpu.virtualMemory, cpu.addressToOffset(strAddr));
    cpu.set_eax(str | 0);
}

function import_close(cpu)
{
    var fd = cpu.safe_read32(cpu.get_esp(0));
    cpu.set_eax(fileSystem.CloseFile(fd));
}

function import_dup(cpu)
{
    var fd = cpu.safe_read32(cpu.get_esp(0));
    cpu.set_eax(fileSystem.Duplicate(fd));
}

function import_calloc(cpu)
{
    var num = cpu.safe_read32(cpu.get_esp(0));
    var size = cpu.safe_read32(cpu.get_esp(4));
    var ptr = cpu.heap.calloc(num, size);
    cpu.set_eax(ptr);
}

function import_fclose(cpu)
{
    var stream = cpu.safe_read32(cpu.get_esp(0));
    
    var result = fileSystem.fclose(cpu, stream);
    cpu.set_eax(result);
}

function import_fdopen(cpu)
{
    var fd = cpu.safe_read32(cpu.get_esp(0));
    var modeAddr = cpu.safe_read32(cpu.get_esp(4));
    
    cpu.validateAddress(modeAddr, 1);
    
    var mode = getCString(cpu.virtualMemory, cpu.addressToOffset(modeAddr));
    var result = fileSystem.FdOpen(cpu, fd, mode);
    cpu.set_eax(result);
}

function import_fopen(cpu)
{
    var filenameAddr = cpu.safe_read32(cpu.get_esp(0));
    var modeAddr = cpu.safe_read32(cpu.get_esp(4));
    
    cpu.validateAddress(filenameAddr, 1);
    cpu.validateAddress(modeAddr, 1);
    
    var filename = getCString(cpu.virtualMemory, cpu.addressToOffset(filenameAddr));
    var mode = getCString(cpu.virtualMemory, cpu.addressToOffset(modeAddr));
    var result = fileSystem.fopen(cpu, filename, mode);
    cpu.set_eax(result);
}

function import_fprintf(cpu)
{
    var espStart = cpu.get_esp();//cdecl
    
    var stream = cpu.popU32();
    var fmt = cpu.safe_read_string(cpu.popU32());
    var result = sprintf_x86_internal(cpu, fmt);
    webcLog("fprintf: " + result);
    
    cpu.set_esp(espStart);
    cpu.set_eax(result.length);
}

function import_fgets(cpu)
{
    var strAddr = cpu.safe_read32(cpu.get_esp(0));
    var n = cpu.safe_read32(cpu.get_esp(4));
    var streamAddr = cpu.safe_read32(cpu.get_esp(8));
    
    var result = fileSystem.fgets(cpu, n, streamAddr);
    if (result == -1)
    {
        cpu.set_eax(0);
    }
    else
    {
        if (result.length > 0)
        {
            memcpy(result, 0, cpu.virtualMemory, cpu.addressToOffset(strAddr), result.length);
        }
        cpu.virtualMemory[cpu.addressToOffset(strAddr + result.length)] = 0;
        cpu.set_eax(strAddr);
    }
}

function import_free(cpu)
{
    var ptrAddr = cpu.safe_read32(cpu.get_esp(0));
    cpu.heap.free(ptrAddr);
}

function import_fflush(cpu)
{
    // Ignore for now
}

function import_fputc(cpu)
{
    var character = cpu.safe_read32s(cpu.get_esp(0));
    var streamAddr = cpu.safe_read32(cpu.get_esp(4));
    
    var result = fileSystem.fputc(cpu, character, streamAddr);
    cpu.set_eax(result);
}

function import_fseek(cpu)
{
    var streamAddr = cpu.safe_read32(cpu.get_esp(0));
    var offset = cpu.safe_read32(cpu.get_esp(4));
    var origin = cpu.safe_read32s(cpu.get_esp(8));
    
    var result = fileSystem.fseek(cpu, streamAddr, offset, origin);
    cpu.set_eax(result);
}

function import_ftell(cpu)
{
    var streamAddr = cpu.safe_read32(cpu.get_esp(0));
    
    var result = fileSystem.ftell(cpu, streamAddr);
    cpu.set_eax(result);
}

function import_fwrite(cpu)
{
    var ptr = cpu.safe_read32(cpu.get_esp(0));
    var size = cpu.safe_read32(cpu.get_esp(4));
    var count = cpu.safe_read32(cpu.get_esp(8));
    var streamAddr = cpu.safe_read32(cpu.get_esp(12));
    
    var result = fileSystem.fwrite(cpu, ptr, size, count, streamAddr);
    cpu.set_eax(result);
}

function import_getenv(cpu)
{
    var nameAddr = cpu.safe_read32(cpu.get_esp(0));
    cpu.validateAddress(nameAddr, 1);
    var name = getCString(cpu.virtualMemory, cpu.addressToOffset(nameAddr));
    
    //webcLog("TODO: getenv (" + name + ")");
    cpu.set_eax(0);
}

function import_getmainargs(cpu)
{
    var argcAddr = cpu.safe_read32(cpu.get_esp(0));
    var argvAddr = cpu.safe_read32(cpu.get_esp(4));
    var envAddr = cpu.safe_read32(cpu.get_esp(8));
    var doWildCard = cpu.safe_read32s(cpu.get_esp(12));
    var startInfoAddr = cpu.safe_read32(cpu.get_esp(16));
    
    var args = ["binary_path.exe", "main.c"];
    
    var argsBufferLen = 0;
    for (var i = 0; i < args.length; i++)
    {
        argsBufferLen += args[i].length + 1;
    }
    var argsBuffer = cpu.heap.malloc(argsBufferLen);
    var argsPointers = cpu.heap.malloc(args.length * 4);
    if (argsBuffer == 0 || argsPointers == 0)
    {
        throw "Failed to allocate memory for main args";
    }
    
    var ptr = argsBuffer;
    for (var i = 0; i < args.length; i++)
    {
        cpu.safe_write32(argsPointers + (i * 4), ptr);
        
        for (var j = 0; j < args[i].length; j++)
        {
            cpu.virtualMemory[cpu.addressToOffset(ptr)] = args[i].charCodeAt(j);
            ptr++;
        }
        cpu.virtualMemory[cpu.addressToOffset(ptr)] = 0;
        ptr++;
    }
    
    cpu.safe_write32(argcAddr, args.length);
    cpu.safe_write32(argvAddr, argsPointers);
    cpu.safe_write32(envAddr, 0);
    
    cpu.set_eax(0);
}

function import_islower(cpu)
{
    var character = cpu.safe_read32s(cpu.get_esp(0));
    var c = string.fromCharCode(character);
    cpu.set_eax(c == c.toLowerCase() ? 1 : 0);
}

function import_isspace(cpu)
{
    var character = cpu.safe_read32s(cpu.get_esp(0));
    var c = string.fromCharCode(character);
    
    var isSpace = c == ' ' || c == '\t';
    cpu.set_eax(isSpace ? 1 : 0);
}

function import_isupper(cpu)
{
    var character = cpu.safe_read32s(cpu.get_esp(0));
    var c = string.fromCharCode(character);
    cpu.set_eax(c == c.toUpperCase() ? 1 : 0);
}

function import_ldexp(cpu)
{
    webcLog("TODO: ldexp");
}

function import_localtime(cpu)
{
    webcLog("TODO: localtime");
}

function import_longjmp(cpu)
{
    webcLog("longjmp called. tcc_error()?");
    cpu.complete = true;
}

function import_lseek(cpu)
{
    var fd = cpu.safe_read32s(cpu.get_esp(0));
    var offset = cpu.safe_read32(cpu.get_esp(4));
    var origin = cpu.safe_read32s(cpu.get_esp(8));
    
    var result = fileSystem.Seek(fd, offset, origin);
    cpu.set_eax(result);
}

function import_malloc(cpu)
{
    var size = cpu.safe_read32(cpu.get_esp(0));
    var result = cpu.heap.malloc(size);
    cpu.set_eax(result);
}

function import_memcmp(cpu)
{
    var ptr1Addr = cpu.safe_read32(cpu.get_esp(0));
    var ptr2Addr = cpu.safe_read32(cpu.get_esp(4));
    var num = cpu.safe_read32(cpu.get_esp(8));
    
    var bytes1 = [];
    var bytes2 = [];
    for (var i = 0; i < num; i++)
    {
        bytes1[i] = cpu.virtualMemory[cpu.addressToOffset(ptr1Addr + i)];
        bytes2[i] = cpu.virtualMemory[cpu.addressToOffset(ptr2Addr + i)];
    }
    
    var result = memcmp(cpu.virtualMemory, cpu.addressToOffset(ptr1Addr), cpu.virtualMemory, cpu.addressToOffset(ptr2Addr), num);
    cpu.set_eax(result);
}

function import_memcpy(cpu)
{
    var destinationAddr = cpu.safe_read32(cpu.get_esp(0));
    var sourceAddr = cpu.safe_read32(cpu.get_esp(4));
    var num = cpu.safe_read32(cpu.get_esp(8));
    
    cpu.validateAddress(destinationAddr, num);
    cpu.validateAddress(sourceAddr, num);
    
    memcpy(cpu.virtualMemory, cpu.addressToOffset(sourceAddr), cpu.virtualMemory, cpu.addressToOffset(destinationAddr), num);
    cpu.set_eax(destinationAddr);
}

function import_memmove(cpu)
{
    var destinationAddr = cpu.safe_read32(cpu.get_esp(0));
    var sourceAddr = cpu.safe_read32(cpu.get_esp(4));
    var num = cpu.safe_read32(cpu.get_esp(8));
    
    cpu.validateAddress(destinationAddr, num);
    cpu.validateAddress(sourceAddr, num);
    
    memmove(cpu.virtualMemory, cpu.addressToOffset(sourceAddr), cpu.virtualMemory, cpu.addressToOffset(destinationAddr), num);
    cpu.set_eax(destinationAddr);
}

function import_memset(cpu)
{
    var bufferAddr = cpu.safe_read32(cpu.get_esp(0));
    var value = cpu.safe_read32s(cpu.get_esp(4));
    var count = cpu.safe_read32(cpu.get_esp(8));
    
    cpu.validateAddress(bufferAddr, count);
    
    memset(cpu.virtualMemory, cpu.addressToOffset(bufferAddr), value, count);
    cpu.set_eax(bufferAddr);
}

function import_open(cpu)
{
    var filenameAddr = cpu.safe_read32(cpu.get_esp(0));
    var flags = cpu.safe_read32s(cpu.get_esp(4));
    
    cpu.validateAddress(filenameAddr, 1);
    
    var filename = getCString(cpu.virtualMemory, cpu.addressToOffset(filenameAddr));
    var fd = fileSystem.OpenFile(filename, flags);
    cpu.set_eax(fd);
}

function import_putchar(cpu)
{
    var character = cpu.safe_read32s(cpu.get_esp(0));
    webcLogRaw(String.fromCharCode(character));
    cpu.set_eax(character);
}

function import_puts(cpu)
{
    var strAddr = cpu.safe_read32(cpu.get_esp(0));
    
    cpu.validateAddress(strAddr, 1);
    
    var str = getCString(cpu.virtualMemory, cpu.addressToOffset(strAddr));
    webcLog(str);
    cpu.set_eax(0);
}

function import_qsort(cpu)
{
    webcLog("TODO: qsort");
}

function import_read(cpu)
{
    var fd = cpu.safe_read32s(cpu.get_esp(0));
    var bufferAddr = cpu.safe_read32(cpu.get_esp(4));
    var count = cpu.safe_read32(cpu.get_esp(8));
    
    var bytes = fileSystem.Read(fd, count);
    if (bufferAddr != 0 && bytes)
    {
        if (bytes == -1)
        {
            cpu.set_eax(0);
        }
        else
        {
            memcpy(bytes, 0, cpu.virtualMemory, cpu.addressToOffset(bufferAddr), Math.min(count, bytes.length));
            cpu.set_eax(bytes.length);
        }
    }
    else
    {
        cpu.set_eax(-1);
    }
}

function import_realloc(cpu)
{
    var ptrAddr = cpu.safe_read32(cpu.get_esp(0));
    var size = cpu.safe_read32(cpu.get_esp(4));
    
    var result = cpu.heap.realloc(ptrAddr, size);
    cpu.set_eax(result);
}

function import_sscanf(cpu)
{
    //webcLog("TODO: sscanf");
    cpu.set_eax(-1);
}

function import_strcat(cpu)
{
    var destinationAddr = cpu.safe_read32(cpu.get_esp(0));
    var sourceAddr = cpu.safe_read32(cpu.get_esp(4));
    
    cpu.validateAddress(destinationAddr, 1);
    cpu.validateAddress(sourceAddr, 1);
    
    var dstOffset = cpu.addressToOffset(destinationAddr);
    var srcOffset = cpu.addressToOffset(sourceAddr);
    
    while (cpu.virtualMemory[dstOffset] != 0)
    {
        dstOffset++;
    }
    
    while (true)
    {
        if ((cpu.virtualMemory[dstOffset] = cpu.virtualMemory[srcOffset]) == 0)
        {
            break;
        }
        dstOffset++;
        srcOffset++;
    }
    
    cpu.set_eax(destinationAddr);
}

function import_strchr(cpu)
{
    var strAddr = cpu.safe_read32(cpu.get_esp(0));
    var character = cpu.safe_read32s(cpu.get_esp(4));
    
    cpu.validateAddress(strAddr, 1);
    
    var strOffset = cpu.addressToOffset(strAddr);
    var charAddr = 0;
    
    for (var i = 0; ; i++)
    {
        var c = cpu.virtualMemory[strOffset + i];
        if (c == character)
        {
            charAddr = strAddr + i;
            break;
        }
        else if (c == 0)
        {
            break;
        }
    }
    
    cpu.set_eax(charAddr);
}

function import_strcmp(cpu, ignoreCase)
{
    var str1Addr = cpu.safe_read32(cpu.get_esp(0));
    var str2Addr = cpu.safe_read32(cpu.get_esp(4));
    //webcLog("cmp " + getCString(cpu.virtualMemory, cpu.addressToOffset(str1Addr)) + " " + getCString(cpu.virtualMemory, cpu.addressToOffset(str2Addr)));
    
    cpu.validateAddress(str1Addr, 1);
    cpu.validateAddress(str2Addr, 1);
    
    var str1Offset = cpu.addressToOffset(str1Addr);
    var str2Offset = cpu.addressToOffset(str2Addr);
    
    var result = 0;
        
    for (var i = 0; ; i++)
    {
        var c1 = cpu.virtualMemory[str1Offset + i];
        var c2 = cpu.virtualMemory[str2Offset + i];
        if (ignoreCase)
        {
            c1 = String.fromCharCode(c1).toUpperCase().charCodeAt(0);
            c2 = String.fromCharCode(c2).toUpperCase().charCodeAt(0);
        }
        if (c1 != c2)
        {
            result = c1 < c2 ? -1 : 1;
            break;
        }
        else if (c1 == 0)
        {
            break;
        }
    }
    
    cpu.set_eax(result);
}

function import_stricmp(cpu)
{
    import_strcmp(cpu, true);
}

function import_strcpy(cpu)
{
    var destinationAddr = cpu.safe_read32(cpu.get_esp(0));
    var sourceAddr = cpu.safe_read32(cpu.get_esp(4));
    
    cpu.validateAddress(destinationAddr, 1);
    cpu.validateAddress(sourceAddr, 1);
    
    var dstOffset = cpu.addressToOffset(destinationAddr);
    var srcOffset = cpu.addressToOffset(sourceAddr);
    
    var c = 1;
    while (c != 0)
    {
        c = cpu.virtualMemory[srcOffset];
        cpu.virtualMemory[dstOffset] = c;
        
        srcOffset++;
        dstOffset++;
    }
    
    cpu.set_eax(destinationAddr);
}

function import_strerror(cpu)
{
    webcLog("TODO: strerror");
    var errnum = cpu.safe_read32s(cpu.get_esp(0));
    cpu.set_eax(0);
}

function import_strlen(cpu)
{
    var strAddr = cpu.safe_read32(cpu.get_esp(0));
    cpu.validateAddress(strAddr, 1);
    
    var strOffset = cpu.addressToOffset(strAddr);
    
    var index = 0;
    while (true)
    {
        if (cpu.virtualMemory[strOffset + index] == 0)
        {
            break;
        }
        index++;
    }
    cpu.set_eax(index);
}

function import_strncmp(cpu, ignoreCase)
{
    var str1Addr = cpu.safe_read32(cpu.get_esp(0));
    var str2Addr = cpu.safe_read32(cpu.get_esp(4));
    var num = cpu.safe_read32(cpu.get_esp(8));
    //webcLog("cmp " + getCString(cpu.virtualMemory, cpu.addressToOffset(str1Addr)) + " - " + getCString(cpu.virtualMemory, cpu.addressToOffset(str2Addr)));
    
    cpu.validateAddress(str1Addr, 1);
    cpu.validateAddress(str2Addr, 1);
    
    var str1Offset = cpu.addressToOffset(str1Addr);
    var str2Offset = cpu.addressToOffset(str2Addr);
    
    var result = 0;
    
    for (var i = 0; i < num; i++)
    {
        var c1 = cpu.virtualMemory[str1Offset + i];
        var c2 = cpu.virtualMemory[str2Offset + i];
        if (ignoreCase)
        {
            c1 = String.fromCharCode(c1).toUpperCase().charCodeAt(0);
            c2 = String.fromCharCode(c2).toUpperCase().charCodeAt(0);
        }
        if (c1 != c2)
        {
            result = c1 < c2 ? -1 : 1;
            break;
        }
        else if (c1 == 0)
        {
            break;
        }
    }
    
    cpu.set_eax(result);
}

function import_strnicmp(cpu)
{
    import_strncmp(cpu, true);
}

function import_strncpy(cpu)
{
    var destinationAddr = cpu.safe_read32(cpu.get_esp(0));
    var sourceAddr = cpu.safe_read32(cpu.get_esp(4));
    var num = cpu.safe_read32(cpu.get_esp(8));
    
    cpu.validateAddress(destinationAddr, num);
    cpu.validateAddress(sourceAddr, num);
    
    var dstOffset = cpu.addressToOffset(destinationAddr);
    var srcOffset = cpu.addressToOffset(sourceAddr);
    
    for (var i = 0; i < num; i++)
    {
        var srcVal = cpu.virtualMemory[srcOffset + i];
        cpu.virtualMemory[dstOffset + i] = srcVal;
        
        if (srcVal == 0)
        {
            // Zero fill after the null terminator in the source string
            for (; i < num; i++)
            {
                cpu.virtualMemory[dstOffset + i] = 0;
            }
        }
    }
    
    cpu.set_eax(destinationAddr);
}

function import_strrchr(cpu)
{
    var strAddr = cpu.safe_read32(cpu.get_esp(0));
    var character = cpu.safe_read32s(cpu.get_esp(4));
    
    cpu.validateAddress(strAddr, 1);
    
    var str = getCString(cpu.virtualMemory, cpu.addressToOffset(strAddr));
    var lastIndex = str.lastIndexOf(String.fromCharCode(character));
    if (lastIndex >= 0)
    {
        cpu.set_eax((strAddr + lastIndex) >>> 0);
    }
    else
    {
        cpu.set_eax(0);
    }
}

function import_strtod(cpu)
{
    webcLog("TODO: strtod");
    var strAddr = cpu.safe_read32(cpu.get_esp(0));
    var endPtr = cpu.safe_read32(cpu.get_esp(4));
    
    cpu.validateAddress(strAddr, 1);
    
    var str = getCStringNumber(cpu.virtualMemory, cpu.addressToOffset(strAddr), -1);
    if (str.length > 0)
    {
    }
    else
    {
    }
}

function import_strtol(cpu, unsigned)
{
    var strAddr = cpu.safe_read32(cpu.get_esp(0));
    var endPtr = cpu.safe_read32(cpu.get_esp(4));
    var radix = cpu.safe_read32s(cpu.get_esp(8));
    
    cpu.validateAddress(strAddr, 1);
    
    var str = getCStringNumber(cpu.virtualMemory, cpu.addressToOffset(strAddr), radix);
    if (str.length > 0)
    {
        var buffer = unsigned ? uint64FromString(str, radix) : int64FromString(str, radix);
        cpu.set_eax(getInt32(buffer, 0));
        cpu.set_edx(getInt32(buffer, 4));
        
        if (endPtr != 0)
        {
            cpu.validateAddress(endPtr, 1);
            cpu.safe_write32(endPtr, strAddr + str.length);
        }
    }
    else
    {
        cpu.set_eax(0);
        cpu.set_edx(0);
    }
}

function import_strtoul(cpu)
{
    import_strtol(cpu, true);
}

function import_time(cpu)
{
    var time = (new Date().getTime() / 1000) >>> 0;
    
    var timeAddr = cpu.safe_read32(cpu.get_esp(0));
    if (timeAddr != 0)
    {
        cpu.validateAddress(timeAddr, 4);
        cpu.safe_write32(timeAddr, time);
    }
    
    cpu.set_eax(time);
}

function import_wc86_assert(cpu)
{
    var test = cpu.safe_read_string(cpu.safe_read32(cpu.get_esp(0)));
    var passed = cpu.safe_read32(cpu.get_esp(4)) != 0;
    
    webcLog("pass:" + passed + " ESP:" + cpu.get_esp().toString(16) + " test:" + test);
}

function import_wc86_assertI32(cpu)
{
    var test = cpu.safe_read_string(cpu.safe_read32(cpu.get_esp(0)));
    var val = cpu.safe_read32s(cpu.get_esp(4));
    var expected = cpu.safe_read32s(cpu.get_esp(8));
    
    webcLog("pass:" + (val == expected) + " ESP:" + cpu.get_esp().toString(16) + " val:" + val + " expected:" + expected + " test:" + test);
}

function import_wc86_assertU32(cpu)
{
    var test = cpu.safe_read_string(cpu.safe_read32(cpu.get_esp(0)));
    var val = cpu.safe_read32(cpu.get_esp(4));
    var expected = cpu.safe_read32(cpu.get_esp(8));
    
    webcLog("pass:" + (val == expected) + " ESP:" + cpu.get_esp().toString(16) + " val:" + val + " expected:" + expected + " test:" + test);
}

function createImports(cpu)
{
    cpu.fakeImports = new Array();
    cpu.fakeImportsAddress = 0;
    
    // Add an unresolved function import handler for logging when unresolved functions are called
    cpu.unresolvedFuncImportHandler = addFuncImport(cpu, import_OnUnresolvedImportCalled, "UNRESOLVED_IMPORTS", "-");
    
    // Ignore a few functions called by go_winmain to avoid unresolved import warnings
    addFuncImport(cpu, import_ignore, "msvcrt.dll", "__set_app_type");
    addFuncImport(cpu, import_ignore, "msvcrt.dll", "_controlfp");
    
    addFuncImport(cpu, import_GetModuleFileNameA, "kernel32.dll", "GetModuleFileNameA");
    addFuncImport(cpu, import_GetSystemDirectoryA, "kernel32.dll", "GetSystemDirectoryA");
    
    addDataImport(cpu, 4, "msvcrt.dll", "__argc");
    addDataImport(cpu, 4, "msvcrt.dll", "__argv");
    addDataImport(cpu, 4, "msvcrt.dll", "_environ");
    addDataImport(cpu, 32 * 20, "msvcrt.dll", "_iob");// 20 FILE entries
    
    addFuncImport(cpu, import_strlwr, "msvcrt.dll", "_strlwr");
    addFuncImport(cpu, import_vsnprintf, "msvcrt.dll", "_vsnprintf");
    addFuncImport(cpu, import_exit, "msvcrt.dll", "exit");
    addFuncImport(cpu, import_exit, "msvcrt.dll", "abort");
    addFuncImport(cpu, import_atoi, "msvcrt.dll", "atoi");
    addFuncImport(cpu, import_close, "msvcrt.dll", "_close");
    addFuncImport(cpu, import_dup, "msvcrt.dll", "_dup");
    addFuncImport(cpu, import_fclose, "msvcrt.dll", "fclose");
    addFuncImport(cpu, import_fdopen, "msvcrt.dll", "_fdopen");
    addFuncImport(cpu, import_fopen, "msvcrt.dll", "fopen");
    addFuncImport(cpu, import_fprintf, "msvcrt.dll", "fprintf");
    addFuncImport(cpu, import_fgets, "msvcrt.dll", "fgets");
    addFuncImport(cpu, import_free, "msvcrt.dll", "free");
    addFuncImport(cpu, import_fflush, "msvcrt.dll", "fflush");
    addFuncImport(cpu, import_fputc, "msvcrt.dll", "fputc");
    addFuncImport(cpu, import_fseek, "msvcrt.dll", "fseek");
    addFuncImport(cpu, import_ftell, "msvcrt.dll", "ftell");
    addFuncImport(cpu, import_fwrite, "msvcrt.dll", "fwrite");
    addFuncImport(cpu, import_getenv, "msvcrt.dll", "getenv");
    addFuncImport(cpu, import_getmainargs, "msvcrt.dll", "__getmainargs");
    addFuncImport(cpu, import_islower, "msvcrt.dll", "islower");
    addFuncImport(cpu, import_isspace, "msvcrt.dll", "isspace");
    addFuncImport(cpu, import_isupper, "msvcrt.dll", "isupper");
    addFuncImport(cpu, import_ldexp, "msvcrt.dll", "ldexp");
    addFuncImport(cpu, import_localtime, "msvcrt.dll", "localtime");
    addFuncImport(cpu, import_longjmp, "msvcrt.dll", "longjmp");
    addFuncImport(cpu, import_lseek, "msvcrt.dll", "_lseek");
    addFuncImport(cpu, import_malloc, "msvcrt.dll", "malloc");
    addFuncImport(cpu, import_memcmp, "msvcrt.dll", "memcmp");
    addFuncImport(cpu, import_memcpy, "msvcrt.dll", "memcpy");
    addFuncImport(cpu, import_memmove, "msvcrt.dll", "memmove");
    addFuncImport(cpu, import_memset, "msvcrt.dll", "memset");
    addFuncImport(cpu, import_open, "msvcrt.dll", "_open");
    addFuncImport(cpu, import_printf, "msvcrt.dll", "printf");
    addFuncImport(cpu, import_putchar, "msvcrt.dll", "putchar");
    addFuncImport(cpu, import_puts, "msvcrt.dll", "puts");
    addFuncImport(cpu, import_qsort, "msvcrt.dll", "qsort");
    addFuncImport(cpu, import_read, "msvcrt.dll", "_read");
    addFuncImport(cpu, import_realloc, "msvcrt.dll", "realloc");
    addFuncImport(cpu, import_setjmp, "msvcrt.dll", "_setjmp");
    addFuncImport(cpu, import_snprintf, "msvcrt.dll", "_snprintf");
    addFuncImport(cpu, import_sprintf, "msvcrt.dll", "sprintf");
    addFuncImport(cpu, import_sscanf, "msvcrt.dll", "sscanf");
    addFuncImport(cpu, import_strcat, "msvcrt.dll", "strcat");
    addFuncImport(cpu, import_strchr, "msvcrt.dll", "strchr");
    addFuncImport(cpu, import_strcmp, "msvcrt.dll", "strcmp");
    addFuncImport(cpu, import_stricmp, "msvcrt.dll", "_stricmp");
    addFuncImport(cpu, import_strcpy, "msvcrt.dll", "strcpy");
    addFuncImport(cpu, import_strerror, "msvcrt.dll", "strerror");
    addFuncImport(cpu, import_strlen, "msvcrt.dll", "strlen");
    addFuncImport(cpu, import_strncmp, "msvcrt.dll", "strncmp");
    addFuncImport(cpu, import_strnicmp, "msvcrt.dll", "_strnicmp");
    addFuncImport(cpu, import_strncpy, "msvcrt.dll", "strncpy");
    addFuncImport(cpu, import_strrchr, "msvcrt.dll", "strrchr");
    addFuncImport(cpu, import_strtod, "msvcrt.dll", "strtod");
    addFuncImport(cpu, import_strtol, "msvcrt.dll", "strtol");
    addFuncImport(cpu, import_strtoul, "msvcrt.dll", "strtoul");
    addFuncImport(cpu, import_time, "msvcrt.dll", "time");
    
    addFuncImport(cpu, import_wc86_assert, "webc86.dll", "wc86_assert");
    addFuncImport(cpu, import_wc86_assertI32, "webc86.dll", "wc86_assertI32");
    addFuncImport(cpu, import_wc86_assertU32, "webc86.dll", "wc86_assertU32");
}

function addFuncImport(cpu, funcCallback, dllName, name, dataSize)
{
    var fullName = dllName.toLowerCase() + ":" + name;
    var importIndex = cpu.fakeImports.length;
    var result;
    if (funcCallback)
    {
        result = { Callback: funcCallback, Name: fullName, Index: importIndex, ThunkAddr: 0 };
    }
    else
    {
        result = { Name: fullName, Index: importIndex, ThunkAddr: 0, DataSize: dataSize ? dataSize : 0, DataAddr: 0 };
    }
    cpu.fakeImports[importIndex] = result;
    return result;
}

function addDataImport(cpu, dataSize, dllName, name)
{
    addFuncImport(cpu, null, dllName, name, dataSize);
}

function allocateDataImports(cpu)
{
    for (var i = 0; i < cpu.fakeImports.length; i++)
    {
        var fakeImport = cpu.fakeImports[i];
        if (fakeImport.DataSize > 0)
        {
            fakeImport.DataAddr = cpu.heap.calloc(1, fakeImport.DataSize);
            if (fakeImport.DataAddr == 0)
            {
                throw "Failed to allocate memory for import " + fakeImport.Name;
            }
            cpu.safe_write32(fakeImport.ThunkAddr, fakeImport.DataAddr);
        }
    }
}

function getImportMap(cpu)
{
    var length = cpu.fakeImports.length;
    var result = new Array(length);
    for (var i = 0; i < length; i++)
    {
        var fakeImport = cpu.fakeImports[i];
        result[fakeImport.Name] = fakeImport;
    }
    return result;
}