var binaryFileData;
var useTypedArrays = true;//typeof Uint8Array != "undefined";
var cpuInstance;
var isCompiling = false;

var compiledExe = [];

function loadBinaryFile()
{
    var fileData;
    if (isCompiling)
    {
        fileData = getTCC();
        compiledExe = [];
    }
    else
    {
        if (compiledExe.length == 0)
        {
            webcLog("The compiled exe is empty!");
            binaryFileData = null;
            return;
        }
        
        fileData = compiledExe;
    }
    if (useTypedArrays)
    {
        binaryFileData = new Uint8Array(fileData);
    }
    else
    {
        binaryFileData = fileData;
    }
}

function runBinary(cpu, buffer)
{
    var initialized = false;
    
    webcLog("Getting import map...");
    createImports(cpu);
    var importMap = getImportMap(cpu);
    webcLog("OK! (" + importMap.length + " imports)");
    
    if (getInt16(buffer, 0) == 0x5A4D)
    {
        var e_lfanew = getInt32(buffer, 0x3C);
        
        var signature = getInt32(buffer, e_lfanew);
        if (signature != 0x00004550)// IMAGE_NET_SIGNATURE
        {
            webcLog("Bad executable signature");
            return;
        }
        
        if (getInt16(buffer, e_lfanew + 4) != 0x014C)// IMAGE_FILE_MACHINE_I386
        {
            webcLog("Executable uses an unsupported architecture. Only x86 is supported. " + getInt16(binary, e_lfanew + 4));
            return;
        }        

        cpu.virtualMemoryAddress = getInt32(buffer, e_lfanew + 0x34);// ImageBase
        cpu.virtualMemoryImageSize = getInt32(buffer, e_lfanew + 0x50);// SizeOfImage
        cpu.virtualMemorySize = cpu.virtualMemoryImageSize + cpu.virtualMemoryStackSize + cpu.virtualMemoryHeapSize;
        webcLog("Creating virtual memory at 0x" + cpu.virtualMemoryAddress.toString(16) + " | " +
            "totalSize: 0x" + cpu.virtualMemorySize.toString(16) + "(" + cpu.virtualMemorySize + ") | " +
            "imageSize: 0x" + cpu.virtualMemoryImageSize.toString(16) + "(" + cpu.virtualMemoryImageSize + ") | " +
            "stackSize: 0x" + cpu.virtualMemoryStackSize.toString(16) + "(" + cpu.virtualMemoryStackSize + ") | " +
            "heapSize: 0x" + cpu.virtualMemoryHeapSize.toString(16) + "(" + cpu.virtualMemoryHeapSize + ")");
        if (useTypedArrays)
        {
            cpu.virtualMemory = new Uint8Array(cpu.virtualMemorySize);
        }
        else
        {
            cpu.virtualMemory = new Array(cpu.virtualMemorySize);
        }
        
        // Set the address for the fake import lookups    
        cpu.fakeImportsAddress = cpu.virtualMemoryAddress - (cpu.fakeImports.length * 4);
        if (cpu.virtualMemoryAddress < (cpu.fakeImports.length * 4))
        {
            webcLog("There is an overlap between the functin imports and the virtual memory. The ImageBase value needs to be larger. " + 
                "ImageBase: " + cpu.virtualMemoryAddress.toString(16) + " imports: " + cpu.fakeImports.length.toString(16));
            return;
        }
        
        cpu.virtualEntryPointAddress = cpu.virtualMemoryAddress + getInt32(buffer, e_lfanew + 0x28);
        
        var dataDirsOffset = e_lfanew + 0x78;
        var sectionsOffset = dataDirsOffset + (16 * 8);// IMAGE_DATA_DIRECTORY entries (16 of them)
        var numSections = getInt16(buffer, e_lfanew + 6);// Number of IMAGE_SECTION_HEADER entries
        var sectionsEnd = sectionsOffset + (numSections * 40);
        
        // Copy the PE header into the virtual process memory
        webcLog("Copy PE header");
        var peHeaderSize = sectionsEnd;        
        memcpy(buffer, 0, cpu.virtualMemory, 0, peHeaderSize);
        
        for (var i = 0; i < numSections; i++)
        {
            var sectionoffset = sectionsOffset + (i * 40);
            
            var sectionName = getFixedString(buffer, sectionoffset, 8);
            
            var virtualAddress = getInt32(buffer, sectionoffset + 0x0C);
            var sizeOfRawData = getInt32(buffer, sectionoffset + 0x10);
            var pointerToRawData = getInt32(buffer, sectionoffset + 0x14);

            if (sizeOfRawData > 0)
            {
                webcLog("Copy section " + sectionName + " addr:0x" + (cpu.virtualMemoryAddress + virtualAddress).toString(16) + 
                    " size:0x" + sizeOfRawData.toString(16));
                memcpy(buffer, pointerToRawData, cpu.virtualMemory, virtualAddress, sizeOfRawData);
            }
        }
                
        // Handle dll imports
        webcLog("Resolving dll imports...");
        var importDataDirOffset = dataDirsOffset + (1 * 8);// sizeof(IMAGE_DATA_DIRECTORY) == 8
        var importDataDirVAddr = getInt32(buffer, importDataDirOffset);
        var importDataDirSize = getInt32(buffer, importDataDirOffset + 4);
        var numResolvedImports = 0, numUnresolvedImports = 0;
        if (importDataDirVAddr > 0 && importDataDirSize > 0)
        {
            // (Take note that we are now using virtualMemory instead of buffer (we need to write to the virtual memory to resolve imports))
            while (getInt32(cpu.virtualMemory, importDataDirVAddr) != 0)
            {
                // Dll name is a char* (as a DWORD). So we need to read the DWORD and then the string.
                var dllName = getCString(cpu.virtualMemory, getInt32(cpu.virtualMemory, importDataDirVAddr + 12));
                if (dllName.length > 0)
                {
                    // We need a lover version for looking up functions in our import map (importMap)
                    var dllNameLower = dllName.toLowerCase();
                    
                    if (getInt32(cpu.virtualMemory, importDataDirVAddr + 8) != 0)
                    {
                        webcLog("TODO: Handle ForwarderChain in the dll imports resolver");
                    }
                    
                    // Get FirstThunk/OriginalFirstThunk, we will probe onward from these addresses (see IMAGE_THUNK_DATA)
                    var originalFirstThunk = getInt32(cpu.virtualMemory, importDataDirVAddr + 0);
                    var firstThunk = getInt32(cpu.virtualMemory, importDataDirVAddr + 16);
                    var thunkIndex = 0;
                    while (true)
                    {
                        var originalThunk = getInt32(cpu.virtualMemory, originalFirstThunk + (thunkIndex * 4));// sizeof(IMAGE_THUNK_DATA) == 4
                        if (originalThunk == 0)
                        {
                            break;
                        }
                        
                        if (originalThunk & 0x80000000)// IMAGE_ORDINAL_FLAG32
                        {
                            // Import by ordinal
                            var ordinal = originalThunk & 0xFFFF;
                            webcLog("TODO: Handle ordinal function imports " + dllName + " (" + ordinal + ")");
                        }
                        else
                        {
                            // Import by name
                            
                            // The function import name (this is a IMAGE_IMPORT_BY_NAME struct, we skip Hint)
                            var importName = getCString(cpu.virtualMemory, originalThunk + 2);
                            var importFullName = dllNameLower + ":" + importName;
                            
                            var importHandler = importMap[importFullName];
                            if (importHandler == null)
                            {
                                importHandler = cpu.unresolvedFuncImportHandler;
                                //webcLog("Unresolved import " + importFullName);
                                numUnresolvedImports++;
                            }
                            else
                            {                                
                                //webcLog("Resolved import " + importFullName);
                                numResolvedImports++;
                            }
                            
                            var importHandlerAddr = cpu.fakeImportsAddress + (importHandler.Index * 4);// pointer per fake import
                            importHandler.ThunkAddr = cpu.virtualMemoryAddress + firstThunk + (thunkIndex * 4);
                            
                            setInt32(cpu.virtualMemory, firstThunk + (thunkIndex * 4), importHandlerAddr);// sizeof(IMAGE_THUNK_DATA) == 4
                        }
                        thunkIndex++;
                    }
                }
                importDataDirVAddr += 20;// sizeof(IMAGE_IMPORT_DESCRIPTOR) == 20
            }
        }
        webcLog("OK! (resolved:" + numResolvedImports + " unresolved:" + numUnresolvedImports + ")");
        
        initialized = true;
    }
    else if (getInt32(buffer, 0) == 0x464C457F)
    {
        window.alert("TODO: Handle ELF files");
    }
    else
    {
        window.alert("Unknown executable format");
    }
    
    buffer.length = 0;
    
    if (initialized)
    {
        cpu.init();
        //cpu.set_eip(0x401000);//cpu.virtualEntryPointAddress;
        
        // Now that the cpu is fully initialized allocate any data imports that need to be allocated (and fix up the addresses)
        allocateDataImports(cpu);
        
        webcLog("\nEmulator initialized. Running code at entry point 0x" + cpu.get_eip().toString(16) + ".\n");
                
        setTimeout(stepEmulator, 150);
    }
}

var numExecutedInstructions = 0;
var instructionsPerStep = 0;//100;
var currentStepCount = 0;
var stepWait = 0;

function stepEmulator()
{
    currentStepCount = 0;
    while (!cpuInstance.complete)
    {
        if (instructionsPerStep > 0 && currentStepCount++ < instructionsPerStep)
        {
            break;
        }
        
        cpuInstance.execute_instruction();
    
        if (numExecutedInstructions++ > 18000000)//17000000)
        {
            cpuInstance.complete = true;
        }
        
        if (cpuInstance.complete)
        {
            webcLog("Emulator finished. Instructions: " + numExecutedInstructions);
            cpuInstance.destroy();
            return;
        }
    }
    
    if (!cpuInstance.complete)
    {
        webcLog("step " + numExecutedInstructions);
        setTimeout(stepEmulator, stepWait);
    }
}

function main()
{
    numExecutedInstructions = 0;
    instructionsPerStep = 0;
    currentStepCount = 0;
    stepWait = 0;
    webcLogClear();
    
    loadBinaryFile();
    if (binaryFileData)
    {
        var cpu = new CPU();
        cpuInstance = cpu;
        
        try
        {
            runBinary(cpu, binaryFileData);
        }
        catch (error)
        {
            webcLog("error " + error + " | " + error.stack);
        }
    }
}

function dumpExe()
{
    webcLogClear();
    if (compiledExe.length == 0)
    {
        webcLog("Nothing to dump!");
    }
    else
    {
        var hex = "";
        for (var i = 0; i < compiledExe.length; i++)
        {
            hex += "0x" + compiledExe[i].toString(16) + ", ";
        }
        webcLog(hex);
    }
}

function saveExe()
{
    var blob = new Blob([new Uint8Array(compiledExe)], {type: "application/exe"});
    var link = document.createElement('a');
    link.href = window.URL.createObjectURL(blob);
    var fileName = "main.exe";
    link.download = fileName;
    link.click();
}

//function mainProxy()
//{
//    domTextOutput = document.getElementById("output");
//    webcLog("Loading...");
//    setTimeout(main, 10);
//}
//
//window.onload = mainProxy;

function onLoaded()
{
    domTextOutput = document.getElementById("output");
    webcLog("Write some code, click compile, then click run.");
}

window.onload = onLoaded;