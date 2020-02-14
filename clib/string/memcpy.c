#include <string.h>

__declspec(dllexport) void* wc86dll_msvcrt_memcpy(void* dst, const void* src, size_t count)
{
    char* d = dst;
    const char* s = src;
    while (count--) *d++ = *s++;
    return dst;
}