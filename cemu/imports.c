#include "cpu.h"
#include <string.h>

#define SIZEOF_FILE 32
#define IOB_NUM 20

void import_unresolved(CPU* cpu)
{
    printf("[WARNING] Unresolved dll import called! ret: 0x%08X\n", cpu->EIP);
}

void import_ignore(CPU* cpu)
{
    // Ignored import
}

void import_exit(CPU* cpu)
{
    cpu->Complete = 1;
}

void import_wc86_assert(CPU* cpu)
{
    char* test = (char*)cpu_get_real_address(cpu, cpu_readU32(cpu, cpu_get_esp(cpu, 0)));
    int32_t passed = cpu_readI32(cpu, cpu_get_esp(cpu, 4)) != 0;
    
    printf("pass:%d ESP:0x%08X test:%s\n", passed, cpu->Reg[REG_ESP], test);
}

void import_wc86_assertI32(CPU* cpu)
{
    char* test = (char*)cpu_get_real_address(cpu, cpu_readU32(cpu, cpu_get_esp(cpu, 0)));
    int32_t val = cpu_readI32(cpu, cpu_get_esp(cpu, 4));
    int32_t expected = cpu_readI32(cpu, cpu_get_esp(cpu, 8));
    
    printf("pass:%d ESP:0x%08X val:%d expected:%d test:%s\n", (val == expected), cpu->Reg[REG_ESP], val, expected, test);
}

void import_wc86_assertU32(CPU* cpu)
{
    char* test = (char*)cpu_get_real_address(cpu, cpu_readU32(cpu, cpu_get_esp(cpu, 0)));
    uint32_t val = cpu_readU32(cpu, cpu_get_esp(cpu, 4));
    uint32_t expected = cpu_readU32(cpu, cpu_get_esp(cpu, 8));
    
    printf("pass:%d ESP:0x%08X val:%u expected:%u test:%s\n", (val == expected), cpu->Reg[REG_ESP], val, expected, test);
}

void import_getmainargs(CPU* cpu)
{
    uint32_t argcAddr = cpu_readU32(cpu, cpu_get_esp(cpu, 0));
    uint32_t argvAddr = cpu_readU32(cpu, cpu_get_esp(cpu, 4));
    uint32_t envAddr = cpu_readU32(cpu, cpu_get_esp(cpu, 8));
    int32_t doWildCard = cpu_readI32(cpu, cpu_get_esp(cpu, 12));
    uint32_t startInfoAddr = cpu_readU32(cpu, cpu_get_esp(cpu, 16));
    
    char* args[3];
    args[0] = "binary_path.exe";
    args[1] = "arg1";
    args[2] = "arg2";
    int32_t numArgs = (int32_t)(sizeof(args) / sizeof(args[0]));
    
    int32_t argsBufferLen = 0;
    for (int32_t i = 0; i < numArgs; i++)
    {
        argsBufferLen += strlen(args[i]) + 1;
    }
    
    char* argsBuffer = (char*)memmgr_alloc(&cpu->Memory, argsBufferLen);
    uint32_t* argsPointers = (uint32_t*)memmgr_alloc(&cpu->Memory, numArgs * 4);
    if (argsBuffer == NULL || argsPointers == NULL)
    {
        cpu_onerror(cpu, "Failed to allocate memory for __getmainargs\n");
    }
    
    char* ptr = argsBuffer;
    for (int32_t i = 0; i < numArgs; i++)
    {
        cpu_writeU32(cpu, cpu_get_virtual_address(cpu, argsPointers + i), cpu_get_virtual_address(cpu, ptr));
        int32_t argLen = strlen(args[i]) + 1;
        memcpy(ptr, args[i], argLen);
        ptr += argLen;
    }
    
    cpu_writeI32(cpu, argcAddr, numArgs);
    cpu_writeU32(cpu, argvAddr, cpu_get_virtual_address(cpu, argsPointers));
    cpu_writeU32(cpu, envAddr, 0);
    
    cpu->Reg[REG_EAX] = 0;
}

void import_fwrite(CPU* cpu)
{
    const void* ptr = (const void*)cpu_get_real_address(cpu, cpu_readU32(cpu, cpu_get_esp(cpu, 0)));
    size_t size = (size_t)cpu_readU32(cpu, cpu_get_esp(cpu, 4));
    size_t count = (size_t)cpu_readU32(cpu, cpu_get_esp(cpu, 8));
    void** stream = (void**)cpu_get_real_address(cpu, cpu_readU32(cpu, cpu_get_esp(cpu, 12)));
    
    cpu_validate_real_address(cpu, ptr);
    cpu_validate_real_address(cpu, stream);
    
    if (*stream)
    {
        cpu->Reg[REG_EAX] = fwrite(ptr, size, count, (FILE*)*stream);
    }
    else
    {
        cpu->Reg[REG_EAX] = 0;
    }
}

void cpu_init_user_defined_imports(CPU* cpu, int32_t* counter)
{
}

void cpu_init_common_imports(CPU* cpu, int32_t* counter)
{
    cpu_define_import(cpu, counter, "msvcrt.dll", "__set_app_type", import_ignore);
    cpu_define_import(cpu, counter, "msvcrt.dll", "_controlfp", import_ignore);
    cpu_define_import(cpu, counter, "msvcrt.dll", "exit", import_exit);
    cpu_define_import(cpu, counter, "msvcrt.dll", "__getmainargs", import_getmainargs);
    cpu_define_import(cpu, counter, "msvcrt.dll", "fwrite", import_fwrite);
    
    cpu_define_data_import(cpu, counter, "msvcrt.dll", "__argc", 4);
    cpu_define_data_import(cpu, counter, "msvcrt.dll", "__argv", 4);
    cpu_define_data_import(cpu, counter, "msvcrt.dll", "_environ", 4);
    cpu->Import_IOB = cpu_define_data_import(cpu, counter, "msvcrt.dll", "_iob", SIZEOF_FILE * IOB_NUM);// 20 FILE entries
    
    cpu_define_import(cpu, counter, "webc86.dll", "wc86_assert", import_wc86_assert);
    cpu_define_import(cpu, counter, "webc86.dll", "wc86_assertI32", import_wc86_assertI32);
    cpu_define_import(cpu, counter, "webc86.dll", "wc86_assertU32", import_wc86_assertU32);
}

void cpu_init_imports(CPU* cpu)
{
    cpu->NumImports = 0;
    
    // The first pass is used to determine how much memory to allocate
    for (int i = 0; i < 2; i++)
    {
        // Reserve an entry for handling unresolved imports
        int32_t funcsCounter = 1;
        
        if (i == 1)
        {
            // Allocate the function imports memory (with a 1 item padding which we will leave empty)
            size_t memSize = (cpu->NumImports + 1) * sizeof(ImportInfo);
            cpu->Imports = (ImportInfo*)memmgr_calloc(&cpu->Memory, cpu->NumImports + 1, sizeof(ImportInfo));
            if (cpu->Imports == NULL)
            {
                cpu_onerror(cpu, "Failed to allocate memory for imports\n");
                return;
            }
            cpu->ImportsBeginAddress = cpu_get_virtual_address(cpu, cpu->Imports);
            cpu->ImportsEndAddress = cpu_get_virtual_address(cpu, (void*)(((size_t)cpu->Imports) + memSize));
            
            // Set up the handler for unhandled imports
            cpu->Imports[0].DllName = NULL;
            cpu->Imports[0].Name = NULL;
            cpu->Imports[0].Callback = import_unresolved;
            cpu->Imports[0].DataSize = 0;
            cpu->Imports[0].DataAddress = 0;
            cpu->Imports[0].ThunkAddress = 0;
            cpu->UnhandledFunctionImport = &cpu->Imports[0];
        }
        
        // Common function imports
        cpu_init_common_imports(cpu, &funcsCounter);
        
        // User defined function imports
        cpu_init_user_defined_imports(cpu, &funcsCounter);
        
        if (i == 0)
        {
            cpu->NumImports = funcsCounter;
        }
        else if (cpu->NumImports != funcsCounter)
        {
            cpu_onerror(cpu, "cpu->NumImports is doesn't match up to the second pass counter\n");
            return;
        }
    }
}

void cpu_allocate_data_imports(CPU* cpu)
{
    if (cpu->Imports != NULL)
    {
        for (int32_t i = 0; i < cpu->NumImports; i++)
        {
            if (cpu->Imports[i].DataSize > 0 && cpu->Imports[i].ThunkAddress > 0)
            {
                void* ptr = memmgr_calloc(&cpu->Memory, 1, cpu->Imports[i].DataSize);
                if (ptr == NULL)
                {
                    cpu_onerror(cpu, "Failed to allocate memory for import\n");
                    return;
                }
                cpu->Imports[i].DataAddress = cpu_get_virtual_address(cpu, ptr);
                cpu_writeU32(cpu, cpu->Imports[i].ThunkAddress, cpu->Imports[i].DataAddress);
            }
        }
        
        if (cpu->Import_IOB)
        {
            size_t data = cpu_get_real_address(cpu, (uint32_t)cpu->Import_IOB->DataAddress);
            *(void**)(data + (SIZEOF_FILE * 0)) = (void*)stdin;
            *(void**)(data + (SIZEOF_FILE * 1)) = (void*)stdout;
            *(void**)(data + (SIZEOF_FILE * 2)) = (void*)stderr;
        }
    }
}

ImportInfo* cpu_define_import_ex(CPU* cpu, int32_t* counter, const char* dllName, const char* name, FuncImportCallbackSig function, uint32_t dataSize)
{
    if (dllName != NULL && name != NULL)
    {
        int32_t index = *counter;
        *counter += 1;
        
        if (cpu->Imports != NULL && index < cpu->NumImports)
        {
            cpu->Imports[index].DllName = dllName;
            cpu->Imports[index].Name = name;
            cpu->Imports[index].Callback = function;
            cpu->Imports[index].DataSize = dataSize;
            cpu->Imports[index].DataAddress = 0;
            cpu->Imports[index].ThunkAddress = 0;
            return &cpu->Imports[index];
        }
    }
    return NULL;
}

ImportInfo* cpu_define_import(CPU* cpu, int32_t* counter, const char* dllName, const char* name, FuncImportCallbackSig function)
{
    return cpu_define_import_ex(cpu, counter, dllName, name, function, 0);
}

ImportInfo* cpu_define_data_import(CPU* cpu, int32_t* counter, const char* dllName, const char* name, uint32_t dataSize)
{
    return cpu_define_import_ex(cpu, counter, dllName, name, NULL, dataSize);
}

ImportInfo* cpu_find_import(CPU* cpu, const char* dllName, const char* name)
{
    // TODO: Use our own version of _stricmp as this wont compile outside of Windows
    // TODO: Switch to using a hashmap lookup
    
    // Find a matching import
    // NOTE: Index 0 is reserved for unhandled exports
    for (int32_t i = 1; i < cpu->NumImports; i++)
    {
        if (_stricmp(cpu->Imports[i].DllName, dllName) == 0 &&
            strcmp(cpu->Imports[i].Name, name) == 0)
        {
            return &cpu->Imports[i];
        }
    }
    
    return NULL;
}