#include <string.h>

__declspec(dllexport) size_t strlen(const char* str)
{
    const char* s;
    for (s = str; *s; ++s);
    return(s - str);
}