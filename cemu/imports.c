#include "cpu.h"
#include "crt.h"

const char* import_get_unresolved_name(CPU* cpu)
{

    return NULL;
}

void import_unresolved(CPU* cpu)
{
    // 1) Look for CALL XXXXX (where the target is expected to be a JMP instruction)
    // 2) Look for JMP DWORD PTR DS:[XXXXXX] (where the target is the 'thunk' address of the import)
    char* importName = NULL;
    uint32_t addr;
    if (cpu_readU8(cpu, cpu->EIP - 5) == 0xE8 && cpu_is_valid_address(cpu, addr = cpu->EIP + cpu_readU32(cpu, cpu->EIP - 4), 6))
    {
        if (cpu_readU16(cpu, addr) == 0x25FF)// JMP DWORD PTR DS:[XXXXXX]
        {
            uint32_t thunkAddr = cpu_readU32(cpu, addr + 2);// Skip the opcode
            char thunkAddrStr[12];
            sprintf(thunkAddrStr, "%u", thunkAddr);
            char** ptr = map_get(&cpu->UnresolvedImportNames, thunkAddrStr);
            if (ptr != NULL)
            {
                importName = *ptr;
            }
        }
    }
    if (importName != NULL)
    {
        printf("[WARNING] Unresolved dll import called. '%s' ret: 0x%08X\n", importName, cpu->EIP);
    }
    else
    {
        printf("[WARNING] Unresolved dll import called. ret: 0x%08X\n", cpu->EIP);
    }
}

void import_ignore(CPU* cpu)
{
    // Ignored import
    CPU_STACK_RETURN(cpu, 0);
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

void cpu_init_user_imports(CPU* cpu, int32_t* counter)
{
    cpu_define_import(cpu, counter, "webc86.dll", "wc86_assert", import_wc86_assert);
    cpu_define_import(cpu, counter, "webc86.dll", "wc86_assertI32", import_wc86_assertI32);
    cpu_define_import(cpu, counter, "webc86.dll", "wc86_assertU32", import_wc86_assertU32);
}

void cpu_init_imports(CPU* cpu)
{
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
        
        // User defined imports / internal emulator related imports
        cpu_init_user_imports(cpu, &funcsCounter);
        
        // Default dll implementations (msvcrt, kernel32, etc)
        crt_init_imports(cpu, &funcsCounter);
        
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
    
    char fullFuncName[MAX_FUNC_NAME_LEN];
    for (int32_t i = 1; i < cpu->NumImports; i++)
    {
        fullFuncName[0] = 0;
        for (int j = 0; ; j++)
        {
            char c = cpu->Imports[i].DllName[j];
            if (!c || c == '.')
            {
                fullFuncName[j] = '_';
                fullFuncName[j + 1] = 0;
                break;
            }
            fullFuncName[j] = tolower(c);
        }
        strcat(fullFuncName, cpu->Imports[i].Name);
        map_set(&cpu->ImportsByName, fullFuncName, &cpu->Imports[i]);
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
        
        crt_allocate_data_imports(cpu);
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

ImportInfo* cpu_find_import(CPU* cpu, const char* fullFuncName)
{
    ImportInfo** result = map_get(&cpu->ImportsByName, fullFuncName);
    return result ? *result : NULL;
}