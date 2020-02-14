#include <stdio.h>
#include <string.h>

__declspec(dllexport) int wc86dll_msvcrt_fputs(const char *restrict s, FILE *restrict f)
{
    size_t l = strlen(s);
    return (fwrite(s, 1, l, f)==l) - 1;
}