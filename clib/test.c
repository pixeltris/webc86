#include <stdio.h>
#include <string.h>

__declspec(dllexport) void wc86dll_kernel32_GetProcAddress(void* hModule, char* lpProcName)
{
    //printf("!! %s %s !!\n", lpProcName, "hi2");
    //vfprintf(stdout, "%s", lpProcName);
    fputs("hello world\n", stdout);
}