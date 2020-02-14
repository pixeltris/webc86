#include <string.h>

__declspec(dllexport) char* wc86dll_msvcrt_strcpy(char* dst, const char* src)
{
    char* result = dst;
    while ((*dst++ = *src++));
    return result;
}