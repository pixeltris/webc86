#include <string.h>
#include <stdint.h>

__declspec(dllexport) void* wc86dll_msvcrt_memmove(void* dst, const void* src, size_t count)
{
    char* d = dst;
    const char* s = src;
    
    if (d == s) return d;
    if ((uintptr_t)s - (uintptr_t)d - count <= -2 * count) return memcpy(d, s, count);
    
    if (d < s)
    {
        for (; count; count--) *d++ = *s++;
    }
    else
    {
        while (count) count--, d[count] = s[count];
    }
    
    return dst;
}