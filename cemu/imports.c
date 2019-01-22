#include "cpu.h"

void OnUnresolvedImportCalled(CPU* cpu)
{
    printf("[WARNING] Unresolved dll import function called! ret: 0x%08X\n", cpu->EIP);
}

void import_exit(CPU* cpu)
{
    cpu->Complete = 1;
}

void cpu_init_user_defined_imports(CPU* cpu, int32_t* counter)
{
}

void cpu_init_common_imports(CPU* cpu, int32_t* counter)
{
    cpu_define_import(cpu, counter, NULL, "msvcrt.dll", "exit", import_exit);
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
            cpu->ImportsBeginAddress = cpu_get_virtual_address(cpu, (size_t)cpu->Imports);
            cpu->ImportsEndAddress = cpu_get_virtual_address(cpu, ((size_t)cpu->Imports) + memSize);
            
            // Set up the handler for unhandled imports
            cpu->Imports[0].TargetBinaryName = NULL;
            cpu->Imports[0].DllName = NULL;
            cpu->Imports[0].Name = NULL;
            cpu->Imports[0].Callback = OnUnresolvedImportCalled;
            cpu->Imports[0].DataSize = 0;
            cpu->Imports[0].DataAddress = 0;
            cpu->Imports[0].ThunkAddress = 0;
            cpu->UnhandledFunctionImport = &cpu->Imports[0];
        }
        
        // Common function imports
        cpu_init_common_imports(cpu, &funcsCounter);
        
        // Target binary specific function imports
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
                cpu->Imports[i].DataAddress = cpu_get_virtual_address(cpu, (size_t)ptr);
                cpu_writeU32(cpu, cpu->Imports[i].ThunkAddress, cpu->Imports[i].DataAddress);
            }
        }
    }
}

ImportInfo* cpu_define_import_ex(CPU* cpu, int32_t* counter, const char* targetName, const char* dllName, const char* name, FuncImportCallbackSig function, uint32_t dataSize, uint32_t dataAddress)
{
    if (dllName != NULL && name != NULL)
    {
        int32_t index = *counter;
        *counter += 1;
        
        if (cpu->Imports != NULL && index < cpu->NumImports)
        {
            cpu->Imports[index].TargetBinaryName = targetName;
            cpu->Imports[index].DllName = dllName;
            cpu->Imports[index].Name = name;
            cpu->Imports[index].Callback = function;
            cpu->Imports[index].DataSize = dataSize;
            cpu->Imports[index].DataAddress = dataAddress;
            cpu->Imports[index].ThunkAddress = 0;
            return &cpu->Imports[index];
        }
    }
    return NULL;
}

ImportInfo* cpu_define_import(CPU* cpu, int32_t* counter, const char* targetName, const char* dllName, const char* name, FuncImportCallbackSig function)
{
    return cpu_define_import_ex(cpu, counter, targetName, dllName, name, function, 0, 0);
}

ImportInfo* cpu_define_data_import(CPU* cpu, int32_t* counter, const char* targetName, const char* dllName, const char* name, uint32_t dataSize, uint32_t dataAddress)
{
    return cpu_define_import_ex(cpu, counter, targetName, dllName, name, NULL, dataSize, dataAddress);
}

ImportInfo* cpu_find_import(CPU* cpu, const char* targetName, const char* dllName, const char* name)
{
    // TODO: Use our own version of _stricmp as this wont compile outside of Windows
    
    // Find a matching import (for the specific target binary)
    if (targetName != NULL)
    {
        for (int32_t i = 1; i < cpu->NumImports; i++)
        {
            if (cpu->Imports[i].TargetBinaryName != NULL &&
                cpu->Imports[i].DllName != NULL &&
                cpu->Imports[i].Name != NULL &&
                _stricmp(cpu->Imports[i].TargetBinaryName, targetName) == 0 &&
                _stricmp(cpu->Imports[i].DllName, dllName) == 0 &&
                strcmp(cpu->Imports[i].Name, name) == 0)
            {
                return &cpu->Imports[i];
            }
        }
    }
    
    // Find a matching import
    for (int32_t i = 1; i < cpu->NumImports; i++)
    {
        if (cpu->Imports[i].DllName != NULL &&
            cpu->Imports[i].Name != NULL &&
            _stricmp(cpu->Imports[i].DllName, dllName) == 0 &&
            strcmp(cpu->Imports[i].Name, name) == 0)
        {
            return &cpu->Imports[i];
        }
    }
    
    return NULL;
}