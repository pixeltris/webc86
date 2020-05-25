#include <stdio.h>
#include <string.h>
#include "image.h"
#include "memmgr.h"
#include "cpu.h"

int try_file_seek(FILE* file, unsigned long offset, int origin)
{
    // NOTE: This expects the file to be larger than the target offset (offset can't be the file size).
    if (fseek(file, offset, origin) != 0)
    {
        return -1;
    }
    char temp;
    if (fread(&temp, 1, 1, file) != 1)
    {
        return -2;
    }
    if (feof(file) != 0 || ferror(file) != 0)
    {
        return -3;
    }
    if (fseek(file, -1, SEEK_CUR) != 0)
    {
        return -4;
    }
    return 0;
}

int try_get_file_size(FILE* file, size_t* outFileSize)
{
    if (outFileSize == NULL)
    {
        return -1;
    }
    if (fseek(file, 0, SEEK_END) != 0 || ferror(file) != 0)
    {
        *outFileSize = 0;
        return -2;
    }
    *outFileSize = ftell(file);
    if (*outFileSize < 0)
    {
        return -3;
    }
    if (fseek(file, 0, SEEK_SET) != 0 || ferror(file) != 0)
    {
        return -4;
    }
    return 0;
}

// (LPE = load portable executable)
#define LPE_LOG(...) printf(__VA_ARGS__)

typedef struct
{
    int ErrorCode;
    size_t Address;// The address of the exe (real address)
    size_t AddressOfEntryPoint;// The address of the first instruction to execute (real address, only used for reference, use the virtual address)
    DWORD VirtualAddress;// The virtual address of the loaded exe (should be ntHeader.OptionalHeader.ImageBase)
    DWORD VirtualAddressOfEntryPoint;// Virtual address of the first instruction to execute
    DWORD ImageSize;// ntHeader.OptionalHeader.SizeOfImage
} ExeLoadResult;

ExeLoadResult* close_win32_exe(ExeLoadResult* result, FILE* file, int errorCode)
{
    result->ErrorCode = errorCode;
    if (file)
    {
        if (fclose(file) != 0 && errorCode == 0)
        {
            result->ErrorCode = -1;
        }
    }
    return result;
}

ExeLoadResult* load_win32_exe_internal(CPU* cpu, ExeLoadResult* result, const char* fileName, ModuleInfo* moduleInfo);
ExeLoadResult* load_win32_exe(CPU* cpu, ExeLoadResult* result, const char* fileName, ModuleInfo** outModuleInfo)
{
    // Check if the module is already loaded
    {
        char moduleNameToLower[MAX_MODULE_NAME_LEN];
        strcpylower(moduleNameToLower, path_to_short_path(fileName));
        ModuleInfo** existingModule = map_get(&cpu->Modules, moduleNameToLower);
        if (existingModule != NULL)
        {
            if (outModuleInfo != NULL)
            {
                *outModuleInfo = NULL;
            }
            result->ErrorCode = -3;
            return result;
        }
    }
    
    ModuleInfo* module = (ModuleInfo*)calloc(1, sizeof(ModuleInfo));
    if (outModuleInfo != NULL)
    {
        *outModuleInfo = module;
    }
    if (module == NULL)
    {
        result->ErrorCode = -2;
    }
    else
    {
        load_win32_exe_internal(cpu, result, fileName, module);
        if (result->ErrorCode == 0)
        {
            module->IsLoaded = 1;
        }
    }
    return result;
}
CPU_SIZE_T cpu_loaddll(CPU* cpu, const char* fileName)
{
    ExeLoadResult loadResult;
    ModuleInfo* moduleInfo = NULL;
    load_win32_exe(cpu, &loadResult, fileName, &moduleInfo);
    if (moduleInfo != NULL)
    {
        return (CPU_SIZE_T)moduleInfo->VirtualAddress;
    }
    return 0;
}

ExeLoadResult* load_win32_exe_internal(CPU* cpu, ExeLoadResult* result, const char* fileName, ModuleInfo* moduleInfo)
{
    // Error codes (LPE = load portable executable)
    enum
    {
        LPE_NONE,
        LPE_LOAD_FILE_FAILED,
        LPE_READ_IMAGE_DOS_HEADER_FAILED,
        LPE_UNSUPPORTED_IMAGE_DOS_HEADER_SIGNATURE,
        LPE_BAD_LFA_NEW_OFFSET,
        LPE_READ_IMAGE_NT_HEADER_FAILED,
        LPE_UNSUPPORTED_IMAGE_NT_HEADER_SIGNATURE,
        LPE_UNSUPPORTED_ARCHITECTURE,
        LPE_READ_IMAGE_OPTIONAL_HEADER_FAILED,
        LPE_BAD_NUMBER_OF_IMAGE_SECTION_HEADER,
        LPE_READ_IMAGE_SECTION_HEADER_FAILED,
        LPE_BAD_IMAGE_SECTION_HEADER_DATA_RANGE,
        LPE_BAD_IMAGE_SECTION_HEADER_VIRTUAL_ADDRESS,
        LPE_BAD_IMAGE_DATA_DIRECTORY_VIRTUAL_ADDRESS,
        LPE_GET_FILE_SIZE_FAILED,
        LPE_BASE_VIRTUAL_ADDRESS_ALREADY_ASSIGNED,
        LPE_ALIGN_MODULE_FAILED,
        LPE_ALLOCATE_VIRTUAL_MEMORY_FAILED,
        LPE_INVALID_VIRTUAL_MEMORY,
        LPE_SEEK_FILE_START_FAILED,
        LPE_COPY_PE_HEADER_FAILED,
        LPE_READ_IMAGE_SECTION_HEADER_DATA_FAILED,
        LPE_BAD_IMAGE_EXPORT_DIRECTORY_DLL_NAME,
        LPE_BAD_IMAGE_EXPORT_DIRECTORY_NAME,
        LPE_BAD_IMAGE_EXPORT_DIRECTORY_ORDINAL,
        LPE_BAD_IMAGE_EXPORT_DIRECTORY_ADDRESS,
        LPE_BAD_IMAGE_IMPORT_DESCRIPTOR_NAME,
        LPE_BAD_IMAGE_IMPORT_DESCRIPTOR_VIRTUAL_ADDRESS,
        LPE_BAD_IMAGE_IMPORT_BY_NAME
    };
    
    int32_t isMainModule = moduleInfo == cpu->MainModule;

    memset(result, 0, sizeof(ExeLoadResult));

#pragma warning(push)
#pragma warning(disable:4996)// disable warning about using fopen instead of fopen_s
    FILE* file = fopen(fileName, "rb");
#pragma warning(pop)
    if (file == NULL)
    {
        result->ErrorCode = LPE_LOAD_FILE_FAILED;
        return result;
    }

    IMAGE_DOS_HEADER dosHeader;
    IMAGE_NT_HEADERS32 ntHeader;

    if (fread(&dosHeader, sizeof(dosHeader), 1, file) != 1)
    {
        LPE_LOG("Failed to read IMAGE_DOS_HEADER\n");
        return close_win32_exe(result, file, LPE_READ_IMAGE_DOS_HEADER_FAILED);
    }

    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
    {
        LPE_LOG("Unsupported IMAGE_DOS_HEADER signature (%d)", dosHeader.e_magic);
        return close_win32_exe(result, file, LPE_UNSUPPORTED_IMAGE_DOS_HEADER_SIGNATURE);
    }

    if (try_file_seek(file, dosHeader.e_lfanew, SEEK_SET) != 0)
    {
        LPE_LOG("Bad e_lfanew offset (long file address for the new exe header (IMAGE_NT_HEADERS))\n");
        return close_win32_exe(result, file, LPE_BAD_LFA_NEW_OFFSET);
    }

    if (fread(&ntHeader, sizeof(ntHeader), 1, file) != 1)
    {
        LPE_LOG("Failed to read IMAGE_NT_HEADERS\n");
        return close_win32_exe(result, file, LPE_READ_IMAGE_NT_HEADER_FAILED);
    }

    if (ntHeader.Signature != IMAGE_NT_SIGNATURE)
    {
        LPE_LOG("Unsupported IMAGE_NT_HEADERS signature (%d)\n", ntHeader.Signature);
        return close_win32_exe(result, file, LPE_UNSUPPORTED_IMAGE_NT_HEADER_SIGNATURE);
    }

    if (ntHeader.FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
    {
        LPE_LOG("Only x86 executables are supported (%04hX)\n", ntHeader.FileHeader.Machine);
        return close_win32_exe(result, file, LPE_UNSUPPORTED_ARCHITECTURE);
    }

    // Seek to the end of the IMAGE_OPTIONAL_HEADER
    size_t optionalHeaderEnd = dosHeader.e_lfanew + sizeof(ntHeader.Signature) +
        sizeof(ntHeader.FileHeader) + ntHeader.FileHeader.SizeOfOptionalHeader;
    if (try_file_seek(file, optionalHeaderEnd, SEEK_SET) != 0)
    {
        LPE_LOG("Failed to read IMAGE_OPTIONAL_HEADER\n");
        return close_win32_exe(result, file, LPE_READ_IMAGE_OPTIONAL_HEADER_FAILED);
    }

    // There is a hard limit of 96 sections according to the documentation
    IMAGE_SECTION_HEADER sections[96] = {0};
    if (ntHeader.FileHeader.NumberOfSections > 96)
    {
        LPE_LOG("Bad number of sections (IMAGE_SECTION_HEADER) (limit is 96, found %d)\n", ntHeader.FileHeader.NumberOfSections);
        return close_win32_exe(result, file, LPE_BAD_NUMBER_OF_IMAGE_SECTION_HEADER);
    }

    if (fread(sections, sizeof(IMAGE_SECTION_HEADER), ntHeader.FileHeader.NumberOfSections, file) != ntHeader.FileHeader.NumberOfSections)
    {
        LPE_LOG("Failed to read section headers (IMAGE_SECTION_HEADER)\n");
        return close_win32_exe(result, file, LPE_READ_IMAGE_SECTION_HEADER_FAILED);
    }

    IMAGE_SECTION_HEADER* entryPointSection = NULL;
    
    // This is where the section data ends (in terms of file data). It will either point to 1 byte beyond the end of the file or the
    // start of the file padding / hidden data.
    unsigned int sectionsDataEndOnDisk = 0;

    for (int i = 0; i < ntHeader.FileHeader.NumberOfSections; i++)
    {
        IMAGE_SECTION_HEADER* section = &sections[i];

        // Use SizeOfRawData-1 as try_file_seek will fail on the last section if perfectly aligned with the file
        if (section->PointerToRawData != 0 && try_file_seek(file, section->PointerToRawData + (section->SizeOfRawData - 1), SEEK_SET) != 0)
        {
            LPE_LOG("Bad IMAGE_SECTION_HEADER offset / size (points beyond the size of the binary). Offset: %d size: %d",
                section->PointerToRawData, section->SizeOfRawData);
            return close_win32_exe(result, file, LPE_BAD_IMAGE_SECTION_HEADER_DATA_RANGE);
        }

        // Ensure the virtual address doesn't point outside of OptionalHeader.SizeOfImage (as we depend on this in a memcpy below)
        // NOTE: Is this even correct? Couldn't we have crazy virtual addresses. Maybe change moduleMemory to be initialized
        //      to the largest section->VirtualAddress + section->SizeOfRawData.
        if (section->VirtualAddress + section->SizeOfRawData > ntHeader.OptionalHeader.SizeOfImage)
        {
            LPE_LOG("IMAGE_SECTION_HEADER has a bad VirtualAddress pointing outside of OptionalHeader.SizeOfImage\n");
            return close_win32_exe(result, file, LPE_BAD_IMAGE_SECTION_HEADER_VIRTUAL_ADDRESS);
        }

        // Find where all of the sections end, this is useful to check for additional file padding / hidden data
        unsigned int sectionEnd = section->PointerToRawData + section->SizeOfRawData;
        if (sectionEnd > sectionsDataEndOnDisk)
        {
            sectionsDataEndOnDisk = sectionEnd;
        }

        if (ntHeader.OptionalHeader.AddressOfEntryPoint >= section->VirtualAddress &&
            ntHeader.OptionalHeader.AddressOfEntryPoint < section->VirtualAddress + section->Misc.VirtualSize)
        {
            entryPointSection = section;
            //LPE_LOG("Entry point section: %.*s\n", sizeof(section->Name), section->Name);
        }
    }

    // Make sure all of the IMAGE_DATA_DIRECTORY entries point within the binary (use the same check as in the "sections" code)
    for (int i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++)
    {
        IMAGE_DATA_DIRECTORY* dataDir = &ntHeader.OptionalHeader.DataDirectory[i];
        if (dataDir->VirtualAddress + dataDir->Size > ntHeader.OptionalHeader.SizeOfImage)
        {
            LPE_LOG("IMAGE_DATA_DIRECTORY has a bad VirtualAddress pointing outside of OptionalHeader.SizeOfImage\n");
            return close_win32_exe(result, file, LPE_BAD_IMAGE_DATA_DIRECTORY_VIRTUAL_ADDRESS);
        }
    }
    
    size_t fileSize;
    if (try_get_file_size(file, &fileSize) != 0)
    {
        LPE_LOG("Failed to get the exe file size\n");
        return close_win32_exe(result, file, LPE_GET_FILE_SIZE_FAILED);
    }

    if (isMainModule)
    {
        // Set the virtual address which we will use to map virtual addresses to/from our real memory addresses
        if (memmgr_get_base_virtual_address(&cpu->Memory) != 0)
        {
            LPE_LOG("Base virtual address already set in memmgr\n");
            return close_win32_exe(result, file, LPE_BASE_VIRTUAL_ADDRESS_ALREADY_ASSIGNED);
        }
        memmgr_set_base_virtual_address(&cpu->Memory, ntHeader.OptionalHeader.ImageBase);
    }

    // We have validated the file enough so that we can now set up the memory for the module
    size_t moduleAlign = 0x1000;
    uint8_t* moduleMemory = memmgr_alloc(&cpu->Memory, ntHeader.OptionalHeader.SizeOfImage + moduleAlign);
    if (moduleMemory == NULL)
    {
        LPE_LOG("Failed to allocate memory for the module '%s'. isMainModule: %d\n", fileName, isMainModule);
        return close_win32_exe(result, file, LPE_ALLOCATE_VIRTUAL_MEMORY_FAILED);
    }
    // Align the module address (don't do this for the main module, as we will be loading directly at the desired address)
    size_t moduleMemoryVAddr = memmgr_get_virtual_address(&cpu->Memory, (size_t)moduleMemory);
    if (!isMainModule && moduleMemoryVAddr % moduleAlign != 0)
    {
        moduleMemory += moduleAlign - (moduleMemoryVAddr % moduleAlign);
    }
    uint8_t* moduleMemoryEnd = moduleMemory + ntHeader.OptionalHeader.SizeOfImage;

    // This HAS to be our first allocation to memmgr as this address will be our base address for mapping virtual
    // memory to real memory addresses and as such this pointer must point to the first byte in the memmgr data
    if (isMainModule && !memmgr_is_first_pointer(&cpu->Memory, moduleMemory))
    {
        LPE_LOG("mmgr is broken or something used the allocator before loading the binary. The binary must be the "
            "first entry in the virtual memory so that we can properly validate memory bounds.\n");
        return close_win32_exe(result, file, LPE_INVALID_VIRTUAL_MEMORY);
    }
    
    // Copy the PE header into the virtual process memory
    size_t peHeaderSize = optionalHeaderEnd + ntHeader.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
    if (try_file_seek(file, 0, SEEK_SET) != 0)
    {
        LPE_LOG("Failed to seek to the start of the file\n");
        return close_win32_exe(result, file, LPE_SEEK_FILE_START_FAILED);
    }
    if (fread(moduleMemory, 1, peHeaderSize, file) != peHeaderSize)
    {
        LPE_LOG("Failed to copy the Portable Executable header to the virtual process memory\n");
        return close_win32_exe(result, file, LPE_COPY_PE_HEADER_FAILED);
    }

    // Copy the data of each section into the virtual process memory
    for (int i = 0; i < ntHeader.FileHeader.NumberOfSections; i++)
    {
        IMAGE_SECTION_HEADER* section = &sections[i];

        if (section->VirtualAddress == 0)
        {
            LPE_LOG("IMAGE_SECTION_HEADER virtual address will wipe over the PE header. This isn't allowed.\n");
            return close_win32_exe(result, file, LPE_BAD_IMAGE_SECTION_HEADER_VIRTUAL_ADDRESS);
        }

        if (section->SizeOfRawData > 0)
        {
            if (try_file_seek(file, section->PointerToRawData, SEEK_SET) != 0)
            {
                LPE_LOG("Failed to seek to a IMAGE_SECTION_HEADER data offset\n");
                return close_win32_exe(result, file, LPE_READ_IMAGE_SECTION_HEADER_DATA_FAILED);
            }

            if (fread(moduleMemory + section->VirtualAddress, 1, section->SizeOfRawData, file) != section->SizeOfRawData)
            {
                LPE_LOG("Failed to read the data of a IMAGE_SECTION_HEADER\n");
                return close_win32_exe(result, file, LPE_READ_IMAGE_SECTION_HEADER_DATA_FAILED);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // We need to fix up some entries in ntHeader.OptionalHeader.DataDirectory (imports / base relocations)
    // Dll injectors which handle manual dll loading are a good resource for this
    // http://www.rohitab.com/discuss/topic/40761-manual-dll-injection/ - handles base relocation / dll imports
    // http://blog.sevagas.com/?PE-injection-explained - slightly more descriptive base relocations

    // We don't need to do base relocation fixups if the virtual address matches up to ImageBase. If we ever support dll loading we will.
    // For now this code doesn't serve any real purpose as the check *should* always fail (and as such the following code is untested).
    IMAGE_DATA_DIRECTORY baseRelocDataDir = ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    size_t loadedImageBaseAddress = memmgr_get_virtual_address(&cpu->Memory, (size_t)moduleMemory);
    if (baseRelocDataDir.VirtualAddress > 0 && baseRelocDataDir.Size > 0 && loadedImageBaseAddress != ntHeader.OptionalHeader.ImageBase)
    {
        if (isMainModule)
        {
            LPE_LOG("[WARNING] Virtual address of the exe in memory is different from the virtual address in the file\n");
        }
        
        // Get the difference between OptionalHeader.ImageBase and the actual address of the exe/dll in memory
        size_t imageBaseDifference = loadedImageBaseAddress - ntHeader.OptionalHeader.ImageBase;

        IMAGE_BASE_RELOCATION* reloc = (IMAGE_BASE_RELOCATION*)(moduleMemory + baseRelocDataDir.VirtualAddress);
        while (reloc->VirtualAddress != 0)
        {
            // Make sure the current block contains enough data to hold relocation descriptors
            if (reloc->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION))
            {
                DWORD numRelocDescriptors = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                LPWORD relocDescriptors = (LPWORD)((LPBYTE)reloc + sizeof(IMAGE_BASE_RELOCATION));                
                for (DWORD i = 0; i < numRelocDescriptors; i++)
                {
                    if (relocDescriptors[i] > 0)
                    {
                        // Get the address in memory and update the offset based on the ImageBase difference
                        PDWORD ptr = (PDWORD)((LPBYTE)moduleMemory + (reloc->VirtualAddress + (relocDescriptors[i] & 0xFFF)));
                        if ((size_t)ptr + sizeof(DWORD) >= (size_t)moduleMemoryEnd)
                        {
                            LPE_LOG("[WARNING] IMAGE_BASE_RELOCATION descriptor points outside the bounds of the binary\n");
                        }
                        else
                        {
                            *ptr += imageBaseDifference;
                        }
                    }
                }
            }            
            reloc = (IMAGE_BASE_RELOCATION*)((uint8_t*)reloc + reloc->SizeOfBlock);
        }
    }
    
    if (isMainModule)
    {
        // Initialize the API imports (dll functions used by the target binary, which we need to provide implementations for)
        cpu_init_imports(cpu);
    }

    // Handle dll exports
    IMAGE_DATA_DIRECTORY exportDataDir = ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (exportDataDir.VirtualAddress > 0 && exportDataDir.Size > 0)
    {
        IMAGE_EXPORT_DIRECTORY* exportDir = (IMAGE_EXPORT_DIRECTORY*)(moduleMemory + exportDataDir.VirtualAddress);
        
        DWORD moduleNameAddr = exportDir->Name;
        if (moduleMemory + moduleNameAddr >= moduleMemoryEnd)
        {
            LPE_LOG("IMAGE_EXPORT_DIRECTORY dll name pointer points outside the bounds of the binary.\n");
            return close_win32_exe(result, file, LPE_BAD_IMAGE_EXPORT_DIRECTORY_DLL_NAME);
        }
        strcpy(moduleInfo->Name, (char*)(moduleMemory + moduleNameAddr));
        
        DWORD* functionTable = (DWORD*)(moduleMemory + exportDir->AddressOfFunctions);
        DWORD* nameTable = (DWORD*)(moduleMemory + exportDir->AddressOfNames);
        WORD* ordinalTable = (WORD*)(moduleMemory + exportDir->AddressOfNameOrdinals);
        for (DWORD i = 0; i < exportDir->NumberOfNames; i++)
        {
            DWORD exportNameAddr = nameTable[i];
            if (moduleMemory + exportNameAddr >= moduleMemoryEnd)
            {
                LPE_LOG("IMAGE_EXPORT_DIRECTORY dll export name points outside the bounds of the binary.\n");
                return close_win32_exe(result, file, LPE_BAD_IMAGE_EXPORT_DIRECTORY_NAME);
            }
            
            char* exportName = (char*)(moduleMemory + exportNameAddr);
            WORD ordinal = ordinalTable[i];
            if (ordinal >= exportDir->NumberOfFunctions)
            {
                LPE_LOG("IMAGE_EXPORT_DIRECTORY export ordinal index is larger than the number of functions.\n");
                return close_win32_exe(result, file, LPE_BAD_IMAGE_EXPORT_DIRECTORY_ORDINAL);
            }
            
            DWORD exportAddr = functionTable[ordinal];
            if (exportAddr >= exportDataDir.VirtualAddress && exportAddr < exportDataDir.VirtualAddress + exportDataDir.Size)
            {
                // TODO: Handle dll export forwarders (these point within IMAGE_DIRECTORY_ENTRY_EXPORT)
                // This should be in the format "MyDllName.MyFunctionName" (or "MyDllName.#1", where #XXX is an ordinal)
                char* forwardedExport = (char*)(moduleMemory + exportAddr);
            }
            else
            {
                if (moduleMemory + exportAddr >= moduleMemoryEnd)
                {
                    LPE_LOG("IMAGE_EXPORT_DIRECTORY export address points outside the bounds of the binary.\n");
                    return close_win32_exe(result, file, LPE_BAD_IMAGE_EXPORT_DIRECTORY_ADDRESS);
                }
                
                char targetFuncName[MAX_FUNC_NAME_LEN];
                targetFuncName[0] = 0;
                int32_t isExternalApiFunc = 0;
                if (strlen(exportName) > 8 && strncmp(exportName, "wc86dll_", 8) == 0)
                {
                    strcpy(targetFuncName, exportName + 8);
                }
                else
                {
                    // Append the module name (without the dll extension)
                    for (int i = 0; ; i++)
                    {
                        char c = moduleInfo->Name[i];
                        if (!c || c == '.')
                        {
                            targetFuncName[i] = '_';
                            break;
                        }
                        targetFuncName[i] = tolower(c);
                    }
                    strcat(targetFuncName, exportName);
                }
                map_set(&cpu->ModuleExportsMap, targetFuncName, (uint32_t)memmgr_get_virtual_address(&cpu->Memory, (size_t)(moduleMemory + exportAddr)));
            }
            //printf("FuncExport: %s %s\n", moduleInfo->Name, exportName);
        }
        /*const char* key;
        map_iter_t iter = map_iter(&cpu->ModuleExportsMap);
        while ((key = map_next(&cpu->ModuleExportsMap, &iter)))
        {
            printf("%s -> %x\n", key, memmgr_get_virtual_address(&cpu->Memory, (size_t)*map_get(&cpu->ModuleExportsMap, key)));
        }*/
    }
    else
    {
        // There isn't an export table, but we still want to get the name of the module (without file path info).
        strcpy(moduleInfo->Name, path_to_short_path(fileName));
    }
    
    // Add the module to modules collection
    {
        char moduleNameToLower[MAX_MODULE_NAME_LEN];
        strcpylower(moduleNameToLower, moduleInfo->Name);
        map_set(&cpu->Modules, moduleNameToLower, moduleInfo);
    }
    
    // Handle dll imports
    map_int_t modulesToLoad;
    map_init(&modulesToLoad);
    IMAGE_DATA_DIRECTORY importDescriptorDataDir = ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDescriptorDataDir.VirtualAddress > 0 && importDescriptorDataDir.Size > 0)
    {
        IMAGE_IMPORT_DESCRIPTOR* importDescriptor = (IMAGE_IMPORT_DESCRIPTOR*)(moduleMemory + importDescriptorDataDir.VirtualAddress);
        IMAGE_IMPORT_DESCRIPTOR emptyEmportDescriptor = { 0 };
        while (memcmp(importDescriptor, &emptyEmportDescriptor, sizeof(IMAGE_IMPORT_DESCRIPTOR)) != 0)
        {
            if (moduleMemory + importDescriptor->Name >= moduleMemoryEnd)
            {
                LPE_LOG("IMAGE_IMPORT_DESCRIPTOR dll name points outside the bounds of the binary.\n");
                return close_win32_exe(result, file, LPE_BAD_IMAGE_IMPORT_DESCRIPTOR_NAME);
            }

            // Get the name of the dll
            char* importDllName = (char*)(moduleMemory + importDescriptor->Name);

            if (strlen(importDllName) == 0)
            {
                LPE_LOG("Referenced dll has an empty name. The imports cannot be resolved for this dll.\n");
            }
            else if (importDescriptor->FirstThunk == 0 && importDescriptor->OriginalFirstThunk == 0)
            {
                // This would be a bit strange but it's probably possible. Remove this warning if it shows up on valid binaries.
                LPE_LOG("Referenced dll %s has no imports.\n", importDllName);
            }
            else if (importDescriptor->FirstThunk == 0 || importDescriptor->OriginalFirstThunk == 0)
            {
                // I *think* OriginalFirstThunk CAN be 0 in the file. We would need to allocate space for these and fill them in.
                LPE_LOG("Need to handle OriginalFirstThunk / FirstThunk 0 values in IMAGE_IMPORT_DESCRIPTOR? (%s).\n", importDllName);
            }
            else if (moduleMemory + importDescriptor->FirstThunk + sizeof(size_t) >= moduleMemoryEnd ||
                moduleMemory + importDescriptor->OriginalFirstThunk + sizeof(size_t) >= moduleMemoryEnd)
            {
                LPE_LOG("IMAGE_IMPORT_DESCRIPTOR has virtual addresses which points outside the bounds of the binary.\n");
                return close_win32_exe(result, file, LPE_BAD_IMAGE_IMPORT_DESCRIPTOR_VIRTUAL_ADDRESS);
            }
            else
            {
                // Add the module name to the list of modules to load
                char importDllNameToLower[MAX_MODULE_NAME_LEN];
                strcpylower(importDllNameToLower, importDllName);
                int* importDllLoadState = map_get(&modulesToLoad, importDllNameToLower);
                if (importDllLoadState == NULL)
                {
                    // The value doesn't matter, we are just using it as a hashset
                    map_set(&modulesToLoad, importDllNameToLower, 0);
                }
                
                // NOTE: The following currently assumes that BOTH FirstThunk and OriginalFirstThunk are assigned in the file.
                // These are really IMAGE_THUNK_DATA*
                DWORD firstThunk = importDescriptor->FirstThunk;
                DWORD* thunkData = (DWORD*)(moduleMemory + importDescriptor->FirstThunk);
                DWORD* originalThunkData = (DWORD*)(moduleMemory + importDescriptor->OriginalFirstThunk);
                
                char mappedImportName[MAX_FUNC_NAME_LEN];
                size_t mappedImportNameDllIndex = 0;
                for (int i = 0; ; i++)
                {
                    char c = importDllName[i];
                    if (!c || c == '.')
                    {
                        mappedImportName[i] = '_';
                        mappedImportNameDllIndex = i + 1;
                        break;
                    }
                    mappedImportName[i] = tolower(c);
                }

                while ((size_t)originalThunkData + sizeof(DWORD) < (size_t)moduleMemoryEnd &&
                    (size_t)thunkData + sizeof(DWORD) < (size_t)moduleMemoryEnd &&
                    *originalThunkData != 0)
                {
                    // For each func reset the internal import name to just the dll name (no extension)
                    mappedImportName[mappedImportNameDllIndex] = '\0';
                    
                    if (*originalThunkData & IMAGE_ORDINAL_FLAG32)
                    {
                        // Import by ordinal
                        DWORD ordinal = *originalThunkData & 0xFFFF;
                        LPE_LOG("[WARNING] TODO: Handle ordinal function imports %s (%d)\n", importDllName, ordinal);
                    }
                    else
                    {
                        // Import by name

                        // The virtual address of the function name string
                        DWORD originalThunkNameVA = *originalThunkData;

                        if (moduleMemory + originalThunkNameVA + sizeof(IMAGE_IMPORT_BY_NAME) >= moduleMemoryEnd)
                        {   
                            LPE_LOG("IMAGE_IMPORT_BY_NAME string points outside of the bounds of the binary\n");
                            return close_win32_exe(result, file, LPE_BAD_IMAGE_IMPORT_BY_NAME);
                        }
                        
                        IMAGE_IMPORT_BY_NAME* importByName = (IMAGE_IMPORT_BY_NAME*)(moduleMemory + originalThunkNameVA);
                        const char* importName = (const char*)importByName->Name;

                        // Try finding the import from mapped modules, otherwise look at the manually implemented imports
                        strcat(mappedImportName, importName);
                        uint32_t* mappedImportAddr = map_get(&cpu->ModuleExportsMap, mappedImportName);
                        if (mappedImportAddr != NULL)
                        {
                            *thunkData = (DWORD)(*mappedImportAddr);
                            printf("Resolved import (mapped) %s %s\n", importDllName, importName);
                        }
                        else
                        {
                            ImportInfo* import = cpu_find_import(cpu, mappedImportName);
                            if (import != NULL)
                            {
                                import->ThunkAddress = (uint32_t)cpu_get_virtual_address(cpu, thunkData);
                                printf("Resolved import %s %s\n", importDllName, importName);
                            }
                            else
                            {
                                import = cpu->UnhandledFunctionImport;
                                printf("Unresolved import %s %s\n", importDllName, importName);
                                
                                // Add the unresolved import to a map so we can look it up if it gets called
                                char thunkAddressStr[12];
                                sprintf(thunkAddressStr, "%u", cpu_get_virtual_address(cpu, thunkData));
                                map_set(&cpu->UnresolvedImportNames, thunkAddressStr, (char*)importByName->Name);
                            }
                            *thunkData = (DWORD)cpu_get_virtual_address(cpu, import);
                        }
                    }
                    
                    thunkData++;
                    originalThunkData++;
                }
            }
            importDescriptor++;
        }
    }
    
    while (1)
    {
        map_iter_t iter = map_iter(modulesToLoad);
        const char* key = map_next(&modulesToLoad, &iter);
        if (key == NULL)
        {
            break;
        }
        ModuleInfo** existingModule = map_get(&cpu->Modules, key);
        if (existingModule == NULL)
        {
            ExeLoadResult dllLoadResult;
            ModuleInfo* dllModuleInfo;
            load_win32_exe(cpu, &dllLoadResult, key, &dllModuleInfo);
            if (dllModuleInfo != NULL)
            {
                map_set(&cpu->Modules, key, dllModuleInfo);
            }
        }
        map_remove(&modulesToLoad, key);
    }

    // Now that the exe is loaded into memory we need to initialize some of the following things
    // NOTE: As these are required to be used in conjunction with segment registers we will set these up
    //       inside the emulator. This might need to be generalized if we support multiple emulators.
    // Process Environment Block (PEB)
    // Thread Environment Block (TEB)
    // Global Descriptor Table (GDT)
    // Local Descriptor Table (LDT)

    if (isMainModule)
    {
        result->ErrorCode = 0;
        result->Address = (size_t)moduleMemory;
        result->AddressOfEntryPoint = result->Address + ntHeader.OptionalHeader.AddressOfEntryPoint;
        result->VirtualAddress = ntHeader.OptionalHeader.ImageBase;
        result->VirtualAddressOfEntryPoint = result->VirtualAddress + ntHeader.OptionalHeader.AddressOfEntryPoint;
        result->ImageSize = ntHeader.OptionalHeader.SizeOfImage;
    }

    return close_win32_exe(result, file, 0);
}

int main()
{
    const char* fileName = "main_file_tests.exe";//"C:\\main.exe";
    
    CPU cpu;
    cpu_init(&cpu);
    
    const size_t heapSize = 0x1000000;// 16MB - Heap size (the stack size gets added to this to produce the final memory size)
    const size_t stackSize  = 0x100000;// 1MB - Total number of bytes in the stack
    const size_t totalMemorySize = heapSize + stackSize;
    if (memmgr_init(&cpu.Memory, totalMemorySize) != 0)
    {
        printf("Failed to initialize the emulator memory of size %08x\n", totalMemorySize);
        cpu_destroy(&cpu);
        getchar();
        return;
    }
    printf("Total memory size: %08x (%llu)\n", totalMemorySize, (uint64_t)totalMemorySize);
    
    ExeLoadResult loadResult;
    load_win32_exe(&cpu, &loadResult, fileName, &cpu.MainModule);
    if (loadResult.ErrorCode != 0 || cpu.ErrorCode != 0)
    {
        printf("Load exe failed.\n");
        cpu_destroy(&cpu);
        getchar();
        return 1;
    }
    
    printf("EOP: 0x%08X\n", loadResult.VirtualAddressOfEntryPoint);
    
    cpu_init_state(&cpu, (uint32_t)loadResult.VirtualAddress, (uint32_t)loadResult.VirtualAddressOfEntryPoint, (uint32_t)loadResult.ImageSize, (uint32_t)heapSize, (uint32_t)stackSize);
    if (cpu.ErrorCode != 0)
    {
        printf("cpu_init_state failed.\n");
        cpu_destroy(&cpu);
        getchar();
        return 2;
    }
    
    if (setjmp(cpu.JmpBuf) == 0 && cpu.ErrorCode == 0)
    {
        cpu.JmpBufInitialized = 1;
        while (!cpu.Complete)
        {
            cpu_execute_instruction(&cpu);
        }
    }
    
    cpu_destroy(&cpu);
    
    if (cpu.ErrorCode == 0)
    {
        printf("done\n");
    }
    else
    {
        printf("done (with errors)\n");
    }
    getchar();
    return 0;
}