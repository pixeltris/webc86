#include <string.h>

__declspec(dllexport) size_t wc86dll_msvcrt_strlen(const char* str)
{
    const char* s;
    for (s = str; *s; ++s);
    return(s - str);
}