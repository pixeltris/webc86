#include "crt.h"
#include <assert.h>
#include <locale.h>
#include <stdlib.h>
#include <math.h>
//#include "extra/printf.h"
#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#else
#include "extra/findfirst.h"
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#define _findfirst32 _findfirst
#define _findnext32 _findnext
#endif

#define MSVCRT_DLL "msvcrt"
#define KERNEL32_DLL "kernel32"
#define WS2_32_DLL "ws2_32"

#define ARGC_STR "__argc"
#define ARGV_STR "__argv"

#define SIZEOF_FILE 32
#define IOB_NUM 20

void cpu_set_command_line_args(CPU* cpu, char** args, int nargs)
{
    ImportInfo* importArgC = cpu_find_import(cpu, MSVCRT_DLL"_"ARGC_STR);
    ImportInfo* importArgV = cpu_find_import(cpu, MSVCRT_DLL"_"ARGV_STR);
    if (importArgC == NULL || importArgV == NULL || importArgC->DataAddress == 0 || importArgV->DataAddress == 0)
    {
        return;
    }
    
    CPU_SIZE_T argsPtr = cpu_readU32(cpu, importArgV->DataAddress);
    int32_t currentNumArgs = cpu_readI32(cpu, importArgC->DataAddress);
    if (argsPtr)
    {
        for (int i = 0; i < currentNumArgs; i++)
        {
            memmgr_free(&cpu->Memory, cpu_get_real_address(cpu, cpu_readU32(cpu, argsPtr + (i * sizeof(CPU_SIZE_T)))));
        }
        memmgr_free(&cpu->Memory, cpu_get_real_address(cpu, argsPtr));
        cpu_writeU32(cpu, importArgV->DataAddress, 0);
        cpu_writeI32(cpu, importArgC->DataAddress, 0);
    }
    if (nargs > 0)
    {
        cpu_add_command_line_args(cpu, args, nargs);
    }
}

void cpu_add_command_line_arg(CPU* cpu, char* arg)
{
    ImportInfo* importArgC = cpu_find_import(cpu, MSVCRT_DLL"_"ARGC_STR);
    ImportInfo* importArgV = cpu_find_import(cpu, MSVCRT_DLL"_"ARGV_STR);
    if (importArgC == NULL || importArgV == NULL || importArgC->DataAddress == 0 || importArgV->DataAddress == 0)
    {
        return;
    }
    
    CPU_SIZE_T argsPtr = cpu_readU32(cpu, importArgV->DataAddress);
    int32_t currentNumArgs = cpu_readI32(cpu, importArgC->DataAddress);
    if (argsPtr)
    {
        void* ptr = memmgr_realloc(&cpu->Memory, cpu_get_real_address(cpu, argsPtr), (currentNumArgs + 1) * sizeof(CPU_SIZE_T));
        if (ptr == NULL)
        {
            return;
        }
        argsPtr = cpu_get_virtual_address(cpu, ptr);
        cpu_writeU32(cpu, importArgV->DataAddress, argsPtr);
    }
    else
    {
        void* ptr = memmgr_alloc(&cpu->Memory, sizeof(CPU_SIZE_T));
        if (ptr == NULL)
        {
            return;
        }
        argsPtr = cpu_get_virtual_address(cpu, ptr);
        cpu_writeU32(cpu, importArgV->DataAddress, argsPtr);
    }
    
    int srcLen = strlen(arg);
    int dstLen = srcLen > 0 ? srcLen : 1;
    char* argCopy = memmgr_alloc(&cpu->Memory, dstLen * sizeof(char));
    if (argCopy != NULL)
    {
        if (srcLen > 0)
        {
            strcpy(argCopy, arg);
        }
        else
        {
            argCopy[0] = '\0';
        }
    }
    cpu_writeU32(cpu, argsPtr + (currentNumArgs * sizeof(CPU_SIZE_T)), cpu_get_virtual_address(cpu, argCopy));
    cpu_writeI32(cpu, importArgC->DataAddress, currentNumArgs + 1);
}

void cpu_add_command_line_args(CPU* cpu, char** args, int nargs)
{
    for (int i = 0; i < nargs; i++)
    {
        cpu_add_command_line_arg(cpu, args[i]);
    }
}

int32_t cpu_get_command_line_arg_count(CPU* cpu)
{
    ImportInfo* importArgC = cpu_find_import(cpu, MSVCRT_DLL"_"ARGC_STR);
    if (importArgC == NULL || importArgC->DataAddress == 0)
    {
        return 0;
    }
    return cpu_readU32(cpu, importArgC->DataAddress);
}

char* cpu_get_command_line_arg(CPU* cpu, int32_t index, char* str, int strLen)
{
    ImportInfo* importArgC = cpu_find_import(cpu, MSVCRT_DLL"_"ARGC_STR);
    ImportInfo* importArgV = cpu_find_import(cpu, MSVCRT_DLL"_"ARGV_STR);
    if (importArgC == NULL || importArgV == NULL || importArgC->DataAddress == 0 || importArgV->DataAddress == 0)
    {
        return NULL;
    }
    
    CPU_SIZE_T argsPtr = cpu_readU32(cpu, importArgV->DataAddress);
    int32_t numArgs = cpu_readI32(cpu, importArgC->DataAddress);
    if (!argsPtr || index >= numArgs)
    {
        return str;
    }
    
    char* arg = cpu_get_real_address(cpu, cpu_readU32(cpu, argsPtr + (index * sizeof(CPU_SIZE_T))));
    char* result = str;
    if (result == NULL)
    {
        result = (char*)malloc(strlen(arg) + 1);
        if (result == NULL)
        {
            return NULL;
        }
    }
    strcpy(result, arg);
    return result;
}

void crt_init_imports(CPU* cpu, int32_t* counter)
{
    cpu_define_import(cpu, counter, MSVCRT_DLL, "__set_app_type", import_ignore);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_controlfp", import_ignore);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "__getmainargs", crt___getmainargs);
    
    cpu_define_data_import(cpu, counter, MSVCRT_DLL, "__argc", 4);// int
    cpu_define_data_import(cpu, counter, MSVCRT_DLL, "__argv", 4);// char**
    cpu_define_data_import(cpu, counter, MSVCRT_DLL, "_environ", 4);// char**
    //cpu_define_data_import(cpu, counter, MSVCRT_DLL, "_wenviron", 4);// wchar_t**
    cpu_define_data_import(cpu, counter, MSVCRT_DLL, "_iob", SIZEOF_FILE * IOB_NUM);// 20 FILE entries
    
    /////////////////////////////////
    // assert.h
    /////////////////////////////////
    
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_assert", crt__assert);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "_wassert", crt__wassert);
    
    /////////////////////////////////
    // ctype.h
    /////////////////////////////////

    cpu_define_import(cpu, counter, MSVCRT_DLL, "isalpha", crt_isalpha);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "isupper", crt_isupper);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "islower", crt_islower);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "isdigit", crt_isdigit);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "isxdigit", crt_isxdigit);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "isspace", crt_isspace);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "ispunct", crt_ispunct);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "isalnum", crt_isalnum);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "isprint", crt_isprint);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "isgraph", crt_isgraph);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "iscntrl", crt_iscntrl);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "toupper", crt_toupper);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "tolower", crt_tolower);
    
    /////////////////////////////////
    // direct.h
    /////////////////////////////////
    
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_getcwd", crt__getcwd);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_getdcwd", crt__getdcwd);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_chdir", crt__chdir);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_mkdir", crt__mkdir);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_rmdir", crt__rmdir);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_chdrive", crt__chdrive);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_getdrive", crt__getdrive);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_getdrives", crt__getdrives);
    
    /////////////////////////////////
    // errno.h
    /////////////////////////////////
    
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_errno", crt__errno);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_set_errno", crt__set_errno);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_get_errno", crt__get_errno);

    /////////////////////////////////
    // io.h
    /////////////////////////////////

    cpu_define_import(cpu, counter, MSVCRT_DLL, "_findfirst", crt__findfirst);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_findnext", crt__findnext);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_findclose", crt__findclose);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_access", crt__access);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_chmod", crt__chmod);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_chsize", crt__chsize);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_close", crt__close);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_commit", crt__commit);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_creat", crt__creat);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_dup", crt__dup);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_dup2", crt__dup2);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_eof", crt__eof);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_filelength", crt__filelength);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_isatty", crt__isatty);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_locking", crt__locking);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_lseek", crt__lseek);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_mktemp", crt__mktemp);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_open", crt__open);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_read", crt__read);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_setmode", crt__setmode);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_sopen", crt__sopen);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_tell", crt__tell);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_umask", crt__umask);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_write", crt__write);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_unlink", crt__unlink);
    
    /////////////////////////////////
    // locale.h
    /////////////////////////////////
    
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "setlocale", crt_setlocale);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "localeconv", crt_localeconv);
    
    /////////////////////////////////
    // math.h
    /////////////////////////////////
    
    cpu_define_import(cpu, counter, MSVCRT_DLL, "acos", crt_acos);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "asin", crt_asin);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "atan", crt_atan);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "atan2", crt_atan2);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_copysign", crt__copysign);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_chgsign", crt__chgsign);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "cos", crt_cos);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "cosh", crt_cosh);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "exp", crt_exp);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fabs", crt_fabs);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fmod", crt_fmod);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "log", crt_log);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "log10", crt_log10);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "pow", crt_pow);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "sin", crt_sin);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "sinh", crt_sinh);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "tan", crt_tan);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "tanh", crt_tanh);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "sqrt", crt_sqrt);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_cabs", crt__cabs);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "ceil", crt_ceil);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "floor", crt_floor);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "frexp", crt_frexp);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_hypot", crt__hypot);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_j0", crt__j0);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_j1", crt__j1);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_jn", crt__jn);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "ldexp", crt_ldexp);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "modf", crt_modf);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_y0", crt__y0);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_y1", crt__y1);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_yn", crt__yn);

    /////////////////////////////////
    // process.h
    /////////////////////////////////
    
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "_beginthread", crt__beginthread);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "_endthread", crt__endthread);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "_beginthreadex", crt__beginthreadex);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "_endthreadex", crt__endthreadex);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_loaddll", crt__loaddll);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "_unloaddll", crt__unloaddll);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_getdllprocaddr", crt__getdllprocaddr);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_getpid", crt__getpid);
    
    /////////////////////////////////
    // setjmp.h
    /////////////////////////////////
    
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_setjmp", crt__setjmp);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "longjmp", crt_longjmp);
    
    /////////////////////////////////
    // signal.h
    /////////////////////////////////
    
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "signal", crt_signal);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "raise", crt_raise);
    
    /////////////////////////////////
    // stdio.h
    /////////////////////////////////

    cpu_define_import(cpu, counter, MSVCRT_DLL, "clearerr", crt_clearerr);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fclose", crt_fclose);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_fdopen", crt__fdopen);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "feof", crt_feof);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "ferror", crt_ferror);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fflush", crt_fflush);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fgetc", crt_fgetc);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fgetpos", crt_fgetpos);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fgets", crt_fgets);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_fileno", crt__fileno);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fopen", crt_fopen);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fprintf", crt_fprintf);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fputc", crt_fputc);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fputs", crt_fputs);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fread", crt_fread);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "freopen", crt_freopen);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fscanf", crt_fscanf);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fseek", crt_fseek);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fsetpos", crt_fsetpos);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "ftell", crt_ftell);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "fwrite", crt_fwrite);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "getc", crt_getc);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "getchar", crt_getchar);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "gets", crt_gets);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_getw", crt__getw);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "_pclose", crt__pclose);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "perror", crt_perror);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "_popen", crt__popen);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "printf", crt_printf);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "putc", crt_putc);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "putchar", crt_putchar);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "puts", crt_puts);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_putw", crt__putw);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "remove", crt_remove);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "rename", crt_rename);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "rewind", crt_rewind);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "scanf", crt_scanf);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "setbuf", crt_setbuf);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "setvbuf", crt_setvbuf);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_snprintf", crt__snprintf);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "sprintf", crt_sprintf);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "sscanf", crt_sscanf);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_tempnam", crt__tempnam);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "tmpfile", crt_tmpfile);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "tmpnam", crt_tmpnam);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "ungetc", crt_ungetc);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "vfprintf", crt_vfprintf);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "vprintf", crt_vprintf);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "vsnprintf", crt_vsnprintf);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_vsnprintf", crt_vsnprintf);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "vsprintf", crt_vsprintf);
    
    /////////////////////////////////
    // stdlib.h
    /////////////////////////////////
    
    cpu_define_import(cpu, counter, MSVCRT_DLL, "abort", crt_abort);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "abs", crt_abs);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "atexit", crt_atexit);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "atof", crt_atof);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "atoi", crt_atoi);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "atol", crt_atol);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_atoi64", crt__atoi64);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "bsearch", crt_bsearch);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "calloc", crt_calloc);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "div", crt_div);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "_ecvt", crt__ecvt);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "exit", crt_exit);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_exit", crt_exit);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "_fcvt", crt__fcvt);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "free", crt_free);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "gcvt", crt__gcvt);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "getenv", crt_getenv);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_itoa", crt__itoa);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "labs", crt_labs);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "ldiv", crt_ldiv);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_abs64", crt__abs64);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "malloc", crt_malloc);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "mblen", crt_mblen);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "mbstowcs", crt_mbstowcs);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "mbtowc", crt_mbtowc);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "_putenv", crt__putenv);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "qsort", crt_qsort);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "rand", crt_rand);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "realloc", crt_realloc);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "srand", crt_srand);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strtod", crt_strtod);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strtol", crt_strtol);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strtoul", crt_strtoul);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_strtoi64", crt__strtoi64);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_strtoui64", crt__strtoui64);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "wcstombs", crt_wcstombs);
    //cpu_define_import(cpu, counter, MSVCRT_DLL, "wctomb", crt_wctomb);

    /////////////////////////////////
    // string.h
    /////////////////////////////////
    
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_memccpy", crt__memccpy);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "memchr", crt_memchr);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "memcmp", crt_memcmp);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "memcpy", crt_memcpy);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "memmove", crt_memmove);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "memset", crt_memset);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strcat", crt_strcat);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strchr", crt_strchr);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strcmp", crt_strcmp);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strcoll", crt_strcoll);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strcpy", crt_strcpy);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strcspn", crt_strcspn);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_strdup", crt__strdup);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strerror", crt_strerror);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_stricmp", crt__stricmp);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strlen", crt_strlen);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_strlwr", crt__strlwr);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strncat", crt_strncat);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strncmp", crt_strncmp);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strncpy", crt_strncpy);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "_strnicmp", crt__strnicmp);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strnlen", crt_strnlen);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strpbrk", crt_strpbrk);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strrchr", crt_strrchr);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strspn", crt_strspn);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strstr", crt_strstr);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strtok", crt_strtok);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "strxfrm", crt_strxfrm);
    
    /////////////////////////////////
    // time.h
    /////////////////////////////////
    
    cpu_define_import(cpu, counter, MSVCRT_DLL, "clock", crt_clock);
    
    /////////////////////////////////
    // wctype.h / wchar.h
    /////////////////////////////////
    
    cpu_define_import(cpu, counter, MSVCRT_DLL, "iswalpha", crt_iswalpha);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "iswupper", crt_iswupper);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "iswlower", crt_iswlower);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "iswdigit", crt_iswdigit);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "iswxdigit", crt_iswxdigit);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "iswspace", crt_iswspace);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "iswpunct", crt_iswpunct);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "iswalnum", crt_iswalnum);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "iswprint", crt_iswprint);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "iswgraph", crt_iswgraph);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "iswcntrl", crt_iswcntrl);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "iswascii", crt_iswascii);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "isleadbyte", crt_isleadbyte);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "towupper", crt_towupper);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "towlower", crt_towlower);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "iswctype", crt_iswctype);
    cpu_define_import(cpu, counter, MSVCRT_DLL, "is_wctype", crt_is_wctype);
    
    /////////////////////////////////
    // kernel32.dll
    /////////////////////////////////
    
    cpu_define_import(cpu, counter, KERNEL32_DLL, "GetModuleFileNameA", crt_GetModuleFileNameA);
    cpu_define_import(cpu, counter, KERNEL32_DLL, "GetSystemDirectoryA", crt_GetSystemDirectoryA);
    
    /////////////////////////////////
    // winsock.h
    /////////////////////////////////
    
    cpu_define_import(cpu, counter, WS2_32_DLL, "bind", crt_bind);
    cpu_define_import(cpu, counter, WS2_32_DLL, "socket", crt_socket);
    cpu_define_import(cpu, counter, WS2_32_DLL, "closesocket", crt_closesocket);
    cpu_define_import(cpu, counter, WS2_32_DLL, "connect", crt_connect);
    cpu_define_import(cpu, counter, WS2_32_DLL, "ioctlsocket", crt_ioctlsocket);
    cpu_define_import(cpu, counter, WS2_32_DLL, "getpeername", crt_getpeername);
    cpu_define_import(cpu, counter, WS2_32_DLL, "getsockname", crt_getsockname);
    cpu_define_import(cpu, counter, WS2_32_DLL, "getsockopt", crt_getsockopt);
    cpu_define_import(cpu, counter, WS2_32_DLL, "htonl", crt_htonl);
    cpu_define_import(cpu, counter, WS2_32_DLL, "htons", crt_htons);
    cpu_define_import(cpu, counter, WS2_32_DLL, "inet_addr", crt_inet_addr);
    cpu_define_import(cpu, counter, WS2_32_DLL, "inet_ntoa", crt_inet_ntoa);
    cpu_define_import(cpu, counter, WS2_32_DLL, "listen", crt_listen);
    cpu_define_import(cpu, counter, WS2_32_DLL, "ntohl", crt_ntohl);
    cpu_define_import(cpu, counter, WS2_32_DLL, "ntohs", crt_ntohs);
    cpu_define_import(cpu, counter, WS2_32_DLL, "recv", crt_recv);
    cpu_define_import(cpu, counter, WS2_32_DLL, "recvfrom", crt_recvfrom);
    cpu_define_import(cpu, counter, WS2_32_DLL, "select", crt_select);
    cpu_define_import(cpu, counter, WS2_32_DLL, "send", crt_send);
    cpu_define_import(cpu, counter, WS2_32_DLL, "sendto", crt_sendto);
    cpu_define_import(cpu, counter, WS2_32_DLL, "setsockopt", crt_setsockopt);
    cpu_define_import(cpu, counter, WS2_32_DLL, "shutdown", crt_shutdown);
    cpu_define_import(cpu, counter, WS2_32_DLL, "socket", crt_socket);
    //cpu_define_import(cpu, counter, WS2_32_DLL, "gethostbyaddr", crt_gethostbyaddr);
    //cpu_define_import(cpu, counter, WS2_32_DLL, "gethostbyname", crt_gethostbyname);
    cpu_define_import(cpu, counter, WS2_32_DLL, "gethostname", crt_gethostname);
    //cpu_define_import(cpu, counter, WS2_32_DLL, "getservbyport", crt_getservbyport);
    //cpu_define_import(cpu, counter, WS2_32_DLL, "getservbyname", crt_getservbyname);
    //cpu_define_import(cpu, counter, WS2_32_DLL, "getprotobynumber", crt_getprotobynumber);
    //cpu_define_import(cpu, counter, WS2_32_DLL, "getprotobyname", crt_getprotobyname);
    cpu_define_import(cpu, counter, WS2_32_DLL, "WSAStartup", crt_WSAStartup);
    cpu_define_import(cpu, counter, WS2_32_DLL, "WSACleanup", crt_WSACleanup);
    cpu_define_import(cpu, counter, WS2_32_DLL, "WSASetLastError", crt_WSASetLastError);
    cpu_define_import(cpu, counter, WS2_32_DLL, "WSAGetLastError", crt_WSAGetLastError);
}

void crt_allocate_data_imports(CPU* cpu)
{
    ImportInfo* importIOB = cpu_find_import(cpu, MSVCRT_DLL"_""_iob");
    if (importIOB && importIOB->DataAddress)
    {
        size_t data = (size_t)cpu_get_real_address(cpu, importIOB->DataAddress);
        *(uint32_t*)(data + (SIZEOF_FILE * 0)) = handles_create(&cpu->FileHandles, (void*)stdin);
        *(uint32_t*)(data + (SIZEOF_FILE * 1)) = handles_create(&cpu->FileHandles, (void*)stdout);
        *(uint32_t*)(data + (SIZEOF_FILE * 2)) = handles_create(&cpu->FileHandles, (void*)stderr);
    }
}

void crt_update_static_str(CPU* cpu, uint32_t* addr, size_t size)
{
    if (*addr == 0)
    {
        *addr = cpu_get_virtual_address(cpu, memmgr_alloc(&cpu->Memory, size));
    }
    else
    {
        char* ptr = (char*)cpu_get_real_address(cpu, *addr);
        if (strlen(ptr) < size)
        {
            *addr = cpu_get_virtual_address(cpu, memmgr_realloc(&cpu->Memory, ptr, size));
        }
    }
}

void crt_update_static_var(CPU* cpu, uint32_t* addr, size_t size)
{
    if (*addr == 0)
    {
        *addr = cpu_get_virtual_address(cpu, memmgr_alloc(&cpu->Memory, size));
    }
}

void crt___getmainargs(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t argcAddr = CPU_STACK_POP_U32(cpu);
    uint32_t argvAddr = CPU_STACK_POP_U32(cpu);
    uint32_t envAddr = CPU_STACK_POP_U32(cpu);
    int32_t doWildCard = CPU_STACK_POP_I32(cpu);
    uint32_t startInfoAddr = CPU_STACK_POP_U32(cpu);
    
    ImportInfo* importArgC = cpu_find_import(cpu, MSVCRT_DLL"_""__argc");
    ImportInfo* importArgV = cpu_find_import(cpu, MSVCRT_DLL"_""__argv");
    if (importArgC != NULL && importArgV != NULL && importArgC->DataAddress > 0 && importArgV->DataAddress > 0)
    {
        if (argcAddr != 0)
        {
            cpu_writeI32(cpu, argcAddr, cpu_readI32(cpu, importArgC->DataAddress));
        }
        if (argvAddr != 0)
        {
            cpu_writeU32(cpu, argvAddr, cpu_readU32(cpu, importArgV->DataAddress));
        }
    }
    else
    {
        if (argcAddr != 0)
        {
            cpu_writeI32(cpu, argcAddr, 0);
        }
        if (argvAddr != 0)
        {
            cpu_writeU32(cpu, argvAddr, 0);
        }
    }
    
    if (envAddr != 0)
    {
        ImportInfo* importEnv = cpu_find_import(cpu, MSVCRT_DLL"_""_environ");
        if (importEnv != NULL && importEnv->DataAddress > 0)
        {
            cpu_writeU32(cpu, envAddr, cpu_readU32(cpu, importEnv->DataAddress));
        }
        else
        {
            cpu_writeU32(cpu, envAddr, 0);
        }
    }
    
    CPU_STACK_RETURN(cpu, 0);
}

/////////////////////////////////
// assert.h
/////////////////////////////////

void crt__assert(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* message = CPU_STACK_POP_PTR_CONST(cpu);
    const char* file = CPU_STACK_POP_PTR_CONST(cpu);
    uint32_t line = CPU_STACK_POP_U32(cpu);
    
#if PLATFORM_WINDOWS
    _assert(message, file, line);
#else
    char* buff = (char*)malloc(strlen(message) + strlen(file) + 64);
    if (buff != NULL)
    {
        sprintf(buff, "assert failed. msg: %s file: %s line: %u", message, file, line);
        cpu_onerror(cpu, buff);
        free(buff);
    }
#endif
}

void crt__wassert(CPU* cpu)
{
    // TODO (wchar_t)
}

/////////////////////////////////
// ctype.h
/////////////////////////////////

#define CTYPE_FUNC(cpu, x) CPU_STACK_BEGIN(cpu);\
    int32_t c = CPU_STACK_POP_I32(cpu);\
    CPU_STACK_RETURN(cpu, x(c));

void crt_isalpha(CPU* cpu)
{
    CTYPE_FUNC(cpu, isalpha);
}

void crt_isupper(CPU* cpu)
{
    CTYPE_FUNC(cpu, isupper);
}

void crt_islower(CPU* cpu)
{
    CTYPE_FUNC(cpu, islower);
}

void crt_isdigit(CPU* cpu)
{
    CTYPE_FUNC(cpu, isdigit);
}

void crt_isxdigit(CPU* cpu)
{
    CTYPE_FUNC(cpu, isxdigit);
}

void crt_isspace(CPU* cpu)
{
    CTYPE_FUNC(cpu, isspace);
}

void crt_ispunct(CPU* cpu)
{
    CTYPE_FUNC(cpu, ispunct);
}

void crt_isalnum(CPU* cpu)
{
    CTYPE_FUNC(cpu, isalnum);
}

void crt_isprint(CPU* cpu)
{
    CTYPE_FUNC(cpu, isprint);
}

void crt_isgraph(CPU* cpu)
{
    CTYPE_FUNC(cpu, isgraph);
}

void crt_iscntrl(CPU* cpu)
{
    CTYPE_FUNC(cpu, iscntrl);
}

void crt_toupper(CPU* cpu)
{
    CTYPE_FUNC(cpu, toupper);
}

void crt_tolower(CPU* cpu)
{
    CTYPE_FUNC(cpu, tolower);
}

/////////////////////////////////
// direct.h
/////////////////////////////////

uint32_t crt__getcwd_impl(CPU* cpu, char* buffer, int maxlen, char* result)
{
    if (buffer != NULL)
    {
        if (result != buffer)
        {
            strncpy(buffer, result, maxlen);
            free(result);
        }
        return cpu_get_virtual_address(cpu, buffer);
    }
    else
    {
        if (result != NULL)
        {
            void* ptr = memmgr_alloc(&cpu->Memory, strlen(result) + 1);
            if (ptr != NULL)
            {
                strcpy(ptr, result);
                free(result);
                return cpu_get_virtual_address(cpu, ptr);
            }
            else
            {
                free(result);
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }
}

void crt__getcwd(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    char* buffer = CPU_STACK_POP_PTR(cpu);
    int maxlen = CPU_STACK_POP_I32(cpu);
    char* result = (char*)getcwd(buffer, maxlen);
    CPU_STACK_RETURN(cpu, crt__getcwd_impl(cpu, buffer, maxlen, result));
}

void crt__getdcwd(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int drive = CPU_STACK_POP_I32(cpu);
    char* buffer = CPU_STACK_POP_PTR(cpu);
    int maxlen = CPU_STACK_POP_I32(cpu);
#if PLATFORM_WINDOWS
    char* result = (char*)_getdcwd(buffer, maxlen);
    CPU_STACK_RETURN(cpu, crt__getcwd_impl(cpu, buffer, maxlen, result));
#else
    CPU_STACK_RETURN(cpu, 0);
#endif
}

void crt__chdir(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* dirname = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, chdir(dirname));
}

void crt__mkdir(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* dirname = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, mkdir(dirname));
}

void crt__rmdir(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* dirname = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, rmdir(dirname));
}

void crt__chdrive(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int drive = CPU_STACK_POP_I32(cpu);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, _chdrive(drive));
#else
    CPU_STACK_RETURN(cpu, 0);
#endif
}

void crt__getdrive(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, _getdrive());
#else
    CPU_STACK_RETURN(cpu, 0);
#endif
}

void crt__getdrives(CPU* cpu)
{
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, _getdrives());
#else
    CPU_STACK_RETURN(cpu, 0);
#endif
}

/////////////////////////////////
// errno.h
/////////////////////////////////

void crt__errno(CPU* cpu)
{
    // This only gives us readonly access to errno... (TODO: Only update errno after calls to real functions?)
    crt_update_static_var(cpu, &cpu->Statics_errno, 4);
    if (cpu->Statics_errno != 0)
    {
        cpu_writeU32(cpu, cpu->Statics_errno, errno);
        CPU_STACK_RETURN(cpu, cpu->Statics_errno);
    }
    else
    {
        CPU_STACK_RETURN(cpu, 0);
    }
}

void crt__set_errno(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int value = CPU_STACK_POP_I32(cpu);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, _set_errno(value));
#else
    CPU_STACK_RETURN(cpu, 0);
#endif
}

void crt__get_errno(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int* value = CPU_STACK_POP_PTR(cpu);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, _get_errno(value));
#else
    CPU_STACK_RETURN(cpu, 0);
#endif
}

/////////////////////////////////
// io.h
/////////////////////////////////

void crt__findfirst(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* filename = CPU_STACK_POP_PTR_CONST(cpu);
    void* findData = CPU_STACK_POP_PTR(cpu);
    
    void* ptr = (void*)_findfirst32(filename, findData);
    if (ptr != NULL)
    {
        CPU_STACK_RETURN(cpu, handles_create(&cpu->DirHandles, (void*)ptr));
    }
    else
    {
        CPU_STACK_RETURN(cpu, 0);
    }
}

void crt__findnext(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    CPU_SIZE_T findHandleId = CPU_STACK_POP_SIZE_T(cpu);
    void* findData = CPU_STACK_POP_PTR(cpu);

    if (findHandleId != 0)
    {
        void* ptr = handles_find(&cpu->DirHandles, findHandleId);
        if (ptr != NULL)
        {
            CPU_STACK_RETURN(cpu, _findnext32(ptr, findData));
            return;
        }
    }
    CPU_STACK_RETURN(cpu, 0);
}

void crt__findclose(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    CPU_SIZE_T findHandleId = CPU_STACK_POP_SIZE_T(cpu);
    
    if (findHandleId != 0)
    {
        void* ptr = handles_find(&cpu->DirHandles, findHandleId);
        if (ptr != NULL)
        {
            _findclose(ptr);
        }
    }
}

void crt__access(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* path = CPU_STACK_POP_PTR_CONST(cpu);
    int mode = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, access(path, mode));
}

void crt__chmod(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* path = CPU_STACK_POP_PTR_CONST(cpu);
    int mode = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, chmod(path, mode));
}

void crt__chsize(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int fd = CPU_STACK_POP_I32(cpu);
    int size = CPU_STACK_POP_I32(cpu);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, chsize(fd, size));
#else
    CPU_STACK_RETURN(cpu, ftruncate(fd, size));
#endif
}

void crt__close(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int fd = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, close(fd));
}

void crt__commit(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int fd = CPU_STACK_POP_I32(cpu);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, _commit(fd));
#else
    CPU_STACK_RETURN(cpu, fsync(fd));
#endif
}

void crt__creat(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* path = CPU_STACK_POP_PTR_CONST(cpu);
    int mode = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, creat(path, mode));
}

void crt__dup(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int fd = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, dup(fd));
}

void crt__dup2(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int fd1 = CPU_STACK_POP_I32(cpu);
    int fd2 = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, dup2(fd1, fd2));
}

void crt__eof(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int fd = CPU_STACK_POP_I32(cpu);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, eof(fd));
#else
    CPU_STACK_RETURN(cpu, 0);
#endif
}

void crt__filelength(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int fd = CPU_STACK_POP_I32(cpu);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, filelength(fd));
#else
    struct stat st;
    if (fstat(fd, &st) == 0)
    {
        CPU_STACK_RETURN(cpu, st.st_size);
    }
    else
    {
        CPU_STACK_RETURN(cpu, 0);
    }
#endif
}

void crt__isatty(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int fd = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, isatty(fd));
}

void crt__locking(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int fd = CPU_STACK_POP_I32(cpu);
    int mode = CPU_STACK_POP_I32(cpu);
    int nbytes = CPU_STACK_POP_I32(cpu);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, locking(fd, mode, nbytes));
#else
    CPU_STACK_RETURN(cpu, lockf(fd, mode, nbytes));// TODO: Check if mode values map 1:1 to win32 mode...
#endif
}

void crt__lseek(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int fd = CPU_STACK_POP_I32(cpu);
    int offset = CPU_STACK_POP_I32(cpu);
    int origin = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, lseek(fd, offset, origin));
}

void crt__mktemp(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    char* template = CPU_STACK_POP_PTR(cpu);
    CPU_STACK_RETURN(cpu, mktemp(template));
}

void crt__open(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* filename = CPU_STACK_POP_PTR_CONST(cpu);
    int oflag = CPU_STACK_POP_I32(cpu);
    int pmode = CPU_STACK_POP_I32(cpu);// TODO: Check oflag first... (this is an optional param)
    CPU_STACK_RETURN(cpu, open(filename, oflag, pmode));
}

void crt__read(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int fd = CPU_STACK_POP_I32(cpu);
    void* buffer = CPU_STACK_POP_PTR(cpu);
    int count = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, read(fd, buffer, count));
}

void crt__setmode(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int fd = CPU_STACK_POP_I32(cpu);
    int mode = CPU_STACK_POP_I32(cpu);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, setmode(fd, mode));
#else
    CPU_STACK_RETURN(cpu, 0);
#endif
}

void crt__sopen(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* filename = CPU_STACK_POP_PTR_CONST(cpu);
    int oflag = CPU_STACK_POP_I32(cpu);
    int shflag = CPU_STACK_POP_I32(cpu);
    int pmode = CPU_STACK_POP_I32(cpu);// TODO: Check oflag first... (this is an optional param)
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, sopen(filename, oflag, shflag, pmode));
#else
    CPU_STACK_RETURN(cpu, 0);
#endif
}

void crt__tell(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int fd = CPU_STACK_POP_I32(cpu);
    int mode = CPU_STACK_POP_I32(cpu);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, tell(fd));
#else
    CPU_STACK_RETURN(cpu, lseek(fd, 0, SEEK_CUR));
#endif
}

void crt__umask(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int mode = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, umask(mode));
}

void crt__write(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int fd = CPU_STACK_POP_I32(cpu);
    void* buffer = CPU_STACK_POP_PTR(cpu);
    int count = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, write(fd, buffer, count));
}

void crt__unlink(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* path = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, unlink(path));
}

/////////////////////////////////
// locale.h
/////////////////////////////////

void crt_setlocale(CPU* cpu)
{
    // To implement this we need to allocate some static memory for the char* response
    CPU_STACK_BEGIN(cpu);
    CPU_STACK_RETURN(cpu, 0);
}

void crt_localeconv(CPU* cpu)
{
    // To implement this we need to allocate some static memory struct size lconv and a bunch of char* entries
    // which need to be copied manually for each entry
    CPU_STACK_BEGIN(cpu);
    CPU_STACK_RETURN(cpu, 0);
}

/////////////////////////////////
// math.h
/////////////////////////////////

void crt_acos(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, acos(x));
}

void crt_asin(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, asin(x));
}

void crt_atan(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, atan(x));
}

void crt_atan2(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    double y = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, atan2(x, y));
}

void crt__copysign(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    double y = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, _copysign(x, y));
}

void crt__chgsign(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, _chgsign(x));
}

void crt_cos(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, cos(x));
}

void crt_cosh(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, cosh(x));
}

void crt_exp(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, exp(x));
}

void crt_fabs(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, fabs(x));
}

void crt_fmod(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    double y = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, fmod(x, y));
}

void crt_log(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, log(x));
}

void crt_log10(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, log10(x));
}

void crt_pow(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    double y = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, pow(x, y));
}

void crt_sin(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, sin(x));
}

void crt_sinh(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, sinh(x));
}

void crt_tan(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, tan(x));
}

void crt_tanh(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, tanh(x));
}

void crt_sqrt(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, sqrt(x));
}

void crt__cabs(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    struct _complex c;
    CPU_STACK_POP_STRUCT(cpu, &c);
    fpu_push(cpu, _cabs(c));
}

void crt_ceil(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, ceil(x));
}

void crt_floor(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, floor(x));
}

void crt_frexp(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    int32_t* exp = CPU_STACK_POP_PTR(cpu);
    fpu_push(cpu, frexp(x, exp));
}

void crt__hypot(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    double y = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, _hypot(x, y));
}

void crt__j0(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, _j0(x));
}

void crt__j1(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, _j1(x));
}

void crt__jn(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    double y = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, _jn(x, y));
}

void crt_ldexp(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    double y = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, ldexp(x, y));
}

void crt_modf(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    double* y = CPU_STACK_POP_PTR(cpu);
    fpu_push(cpu, modf(x, y));
}

void crt__y0(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, _y0(x));
}

void crt__y1(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, _y1(x));
}

void crt__yn(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double x = CPU_STACK_POP_F64(cpu);
    double y = CPU_STACK_POP_F64(cpu);
    fpu_push(cpu, _yn(x, y));
}

/////////////////////////////////
// process.h
/////////////////////////////////

void crt__beginthread(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    CPU_SIZE_T startAddress = CPU_STACK_POP_SIZE_T(cpu);
    uint32_t stackSize = CPU_STACK_POP_U32(cpu);
    CPU_SIZE_T argList = CPU_STACK_POP_SIZE_T(cpu);
    // TODO
    CPU_STACK_RETURN(cpu, 0);
}

void crt__endthread(CPU* cpu)
{
    // TODO
}

void crt__beginthreadex(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    CPU_SIZE_T security = CPU_STACK_POP_SIZE_T(cpu);
    CPU_SIZE_T startAddress = CPU_STACK_POP_SIZE_T(cpu);
    uint32_t stackSize = CPU_STACK_POP_U32(cpu);
    CPU_SIZE_T argList = CPU_STACK_POP_SIZE_T(cpu);
    uint32_t initFlag = CPU_STACK_POP_U32(cpu);
    CPU_SIZE_T threadAddr = CPU_STACK_POP_U32(cpu);
    // TODO
    CPU_STACK_RETURN(cpu, 0);
}

void crt__endthreadex(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t retval = CPU_STACK_POP_U32(cpu);
    // TODO
}

void crt__loaddll(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* fileName = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_SIZE_T result = cpu_loaddll(cpu, fileName);
    CPU_STACK_RETURN(cpu, result);
}

void crt__unloaddll(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    CPU_SIZE_T handle = CPU_STACK_POP_SIZE_T(cpu);
    // TODO (reference count ModuleInfo, etc)
    CPU_STACK_RETURN(cpu, 0);// If the function succeeds, the return value is nonzero.
}

void crt__getdllprocaddr(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    CPU_SIZE_T handle = CPU_STACK_POP_SIZE_T(cpu);
    const char* procName = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_SIZE_T ordinal = CPU_STACK_POP_SIZE_T(cpu);
    CPU_SIZE_T result = 0;
    if (handle != 0 && procName != NULL)
    {
        const char* key;
        map_iter_t iter = map_iter(cpu->Modules);
        while ((key = map_next(&cpu->Modules, &iter)))
        {
            ModuleInfo** modulePtr = map_get(&cpu->Modules, key);
            if (modulePtr != NULL)
            {
                ModuleInfo* moduleInfo = *modulePtr;
                if (moduleInfo->VirtualAddress == handle)
                {
                    // Copy from win32 main.c PE loader
                    char mappedImportName[MAX_FUNC_NAME_LEN];
                    mappedImportName[0] = 0;
                    if (strlen(procName) > 8 && strncmp(procName, "wc86dll_", 8) == 0)
                    {
                        strcpy(mappedImportName, procName + 8);
                    }
                    else
                    {
                        // Append the module name (without the dll extension)
                        for (int i = 0; ; i++)
                        {
                            char c = moduleInfo->Name[i];
                            if (!c || c == '.')
                            {
                                mappedImportName[i] = '_';
                                break;
                            }
                            mappedImportName[i] = tolower(c);
                        }
                        strcat(mappedImportName, procName);
                    }
                    uint32_t* mappedImportAddr = map_get(&cpu->ModuleExportsMap, mappedImportName);
                    if (mappedImportAddr != NULL)
                    {
                        result = (CPU_SIZE_T)*mappedImportAddr;
                    }
                    else
                    {
                        ImportInfo* import = cpu_find_import(cpu, mappedImportName);
                        if (import != NULL)
                        {
                            result = (CPU_SIZE_T)import->ThunkAddress;
                        }
                    }
                    break;
                }
            }
        }
    }
    CPU_STACK_RETURN(cpu, result);
}

void crt__getpid(CPU* cpu)
{
    CPU_STACK_RETURN(cpu, 1);
}

/////////////////////////////////
// setjmp.h
/////////////////////////////////

typedef struct
{
    uint32_t Ebp;
    uint32_t Ebx;
    uint32_t Esi;
    uint32_t Edi;
    uint32_t Esp;
    uint32_t Eip;
    uint32_t Registration;
    uint32_t TryLevel;
    uint32_t Cookie;
    uint32_t UnwindFunc;
    uint32_t UnwindData[6];
} EMU86_JUMP_BUFFER;

void crt__setjmp(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    EMU86_JUMP_BUFFER* buffer = CPU_STACK_POP_PTR(cpu);
    memset(buffer, 0, sizeof(*buffer));
    buffer->Ebp = (uint32_t)cpu->Reg[REG_EBP];
    buffer->Ebx = (uint32_t)cpu->Reg[REG_EBX];
    buffer->Esi = (uint32_t)cpu->Reg[REG_ESI];
    buffer->Edi = (uint32_t)cpu->Reg[REG_EDI];
    buffer->Esp = (uint32_t)cpu->Reg[REG_ESP];
    buffer->Eip = (uint32_t)cpu->EIP;
    CPU_STACK_RETURN(cpu, 0);
}

void crt_longjmp(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    EMU86_JUMP_BUFFER* buffer = CPU_STACK_POP_PTR(cpu);
    int value = CPU_STACK_POP_I32(cpu);
    cpu->Reg[REG_EBP] = (int32_t)buffer->Ebp;
    cpu->Reg[REG_EBX] = (int32_t)buffer->Ebx;
    cpu->Reg[REG_ESI] = (int32_t)buffer->Esi;
    cpu->Reg[REG_EDI] = (int32_t)buffer->Edi;
    cpu->Reg[REG_ESP] = (int32_t)buffer->Esp;
    cpu->EIP = (uint32_t)buffer->Eip;
    CPU_STACK_RETURN(cpu, value);
}

/////////////////////////////////
// signal.h
/////////////////////////////////

void crt_signal(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t sig = CPU_STACK_POP_I32(cpu);
    void* handler = CPU_STACK_POP_PTR(cpu);
    // TODO
    CPU_STACK_RETURN(cpu, 0);
}

void crt_raise(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t sig = CPU_STACK_POP_I32(cpu);
    // TODO
    CPU_STACK_RETURN(cpu, 0);
}

/////////////////////////////////
// stdio.h
/////////////////////////////////

FILE* get_file(CPU* cpu, uint32_t handleId)
{
    FILE* fileHandle = NULL;
    if (handleId != 0)
    {
        fileHandle = handles_find(&cpu->FileHandles, handleId);
        if (fileHandle == NULL)
        {
            // iob FILE addresses are accessed directly, their handleId can be found at the start of the struct
            ImportInfo* importIOB = cpu_find_import(cpu, MSVCRT_DLL"_""_iob");
            if (importIOB != NULL &&
                importIOB->DataAddress &&
                handleId >= importIOB->DataAddress &&
                handleId < importIOB->DataAddress + (SIZEOF_FILE * IOB_NUM))
            {
                return get_file(cpu, cpu_readU32(cpu, handleId));
            }
        }
    }
    return fileHandle;
}

// hmm... simplify this crap (too much code expansion)
#define PRINTF_TEMP_FORMAT_LEN 64
#define PRINTF_TEMP_BUFFER_LEN 128
#define DO_FORMAT(x)\
    if (fmtLen + dataSize >= PRINTF_TEMP_BUFFER_LEN){\
        if (tempBufferExLen < fmtLen + dataSize){\
            tempBufferExLen = fmtLen + dataSize + 1;\
            if (tempBufferEx != NULL){\
                char* p = realloc(tempBufferEx, tempBufferExLen);\
                if (p == NULL){\
                    free(tempBufferEx);\
                    tempBufferEx = NULL;\
                }else{\
                    tempBufferEx = p;\
                }\
            }else{\
                tempBufferEx = malloc(tempBufferExLen);\
            }\
            if (tempBufferEx == NULL)\
                tempBufferExLen = 0;\
        }\
        targetTempBuffer = tempBufferEx;\
    }\
    if (targetTempBuffer != NULL){\
        targetTempBuffer[0] = '\0';\
        if (width1 != INT64_MAX)\
            if (width2 != INT64_MAX)\
                sprintf(targetTempBuffer, tempFmt, (int32_t)width1, (int32_t)width2, x);\
            else\
                sprintf(targetTempBuffer, tempFmt, (int32_t)width1, x);\
        else\
            sprintf(targetTempBuffer, tempFmt, x);\
    }

const char* printf_skip_digits(const char* p, char* c)
{
    while (*p && isdigit(*p))
    {
        p++;
    }
    if (c != NULL)
    {
        *c = *p;
    }
    return p;
}

typedef enum
{
    FT_Std,
    FT_File,
    FT_Buffer,
    FT_BufferLen
} FormatType;

typedef enum
{
    FVA_No,
    FVA_Yes
} FormatVa;

void printf_impl(CPU* cpu, FormatType type, FormatVa va)
{
    CPU_STACK_BEGIN(cpu);
    
    FILE* stream = NULL;
    char* buffer = NULL;
    int64_t bufferSize = -1; 
    const char* fmt = NULL;
    switch (type)
    {
        case FT_File:
            {
                uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
                stream = get_file(cpu, handleId);
                if (stream == NULL)
                {
                    return;
                }
            }
            break;
        case FT_Buffer:
        case FT_BufferLen:
            buffer = CPU_STACK_POP_PTR(cpu);
            if (buffer == NULL)
            {
                return;
            }
            if (type == FT_BufferLen)
            {
                bufferSize = CPU_STACK_POP_U32(cpu);
                if (bufferSize == 0)
                {
                    return;
                }
                bufferSize--;// Give room for the null terminator
            }
            break;
    }
    fmt = CPU_STACK_POP_PTR_CONST(cpu);
    size_t writtenBytes = 0;
    
    int32_t espStart = cpu->Reg[REG_ESP];
    if (va == FVA_Yes)
    {
        cpu->Reg[REG_ESP] = CPU_STACK_POP_U32(cpu);
        CPU_STACK_RESET(cpu);
    }

    char tempFmt[PRINTF_TEMP_FORMAT_LEN];
    char tempBuffer[PRINTF_TEMP_BUFFER_LEN];
    char* tempBufferEx = NULL;
    size_t tempBufferExLen = 0;
    
    const char* lazyPrintf = NULL;
    const char* ptr = fmt;
    while (*ptr && (bufferSize == -1 || writtenBytes < bufferSize))
    {
        int64_t width1 = INT64_MAX;// * width
        int64_t width2 = INT64_MAX;// .* width
        
        const char* startPtr = ptr;
        tempFmt[0] = '\0';
        
        char c = *ptr++;
        if (c != '%')
        {
            switch (type)
            {
                case FT_Std:
                    // using putchar(c) would be ideal, but it's quite slow when called many times
                    if (lazyPrintf == NULL)
                    {
                        lazyPrintf = ptr - 1;
                    }
                    break;
                case FT_File:
                    fputc(c, stream);
                    writtenBytes++;
                    break;
                case FT_Buffer:
                case FT_BufferLen:
                    *buffer++ = c;
                    writtenBytes++;
                    break;
            }
            continue;
        }
        const char* lazyPrintfFmtStart = ptr - 1;
        
        int32_t complete = 0;
        while (!complete)
        {
            complete = 1;
            c = *ptr++;
            switch (c)
            {
                case '-':
                case '+':
                case ' ':
                case '#':
                case '0':
                    complete = 1;
                    break;
            }
        }
        
        if (isdigit(c))
        {
            ptr = printf_skip_digits(ptr, &c);
        }
        else if (c == '*')
        {
            c = *ptr++;
            width1 = CPU_STACK_POP_I32(cpu);
        }
        
        if (c == '.')
        {
            c = *ptr++;
            if (isdigit(c))
            {
                ptr = printf_skip_digits(ptr, &c);
            }
            else if (c == '*')
            {
                width2 = CPU_STACK_POP_I32(cpu);
            }
        }
        
        // Length specifiers
        int32_t dataSize = 0;
        int32_t numLengthSpecifiers = 1;
        switch (c)
        {
            case 'l':
                dataSize = 4;
                if (*ptr == 'l')
                {
                    dataSize = 8;
                    numLengthSpecifiers++;
                }
                break;
            case 'h':
                //dataSize = 2;
                if (*ptr == 'h')
                {
                    //dataSize = 1;
                    numLengthSpecifiers++;
                }
                break;
            case 'j':
            case 'z':
            case 't':
                dataSize = 4;
                break;
            case 'L':
                dataSize = 8;// long double
                break;
            default:
                numLengthSpecifiers = 0;
                break;
        }
        if (numLengthSpecifiers > 0)
        {
            c = ptr[numLengthSpecifiers - 1];// Already 1 ahead
            ptr += numLengthSpecifiers;
        }
        
        size_t fmtLen = ptr - startPtr;
        strncat(tempFmt, startPtr, fmtLen);
        char* targetTempBuffer = tempBuffer;
        
        int32_t handled = 0;
        switch (c)
        {
            case 'c':
            case 'C':
                {
                    int32_t value = CPU_STACK_POP_I32(cpu);
                    DO_FORMAT(value);
                }
                break;
            case 's':
            case 'S':
                {
                    const char* value = CPU_STACK_POP_PTR_CONST(cpu);
                    DO_FORMAT(value);
                }
                break;
            case 'n':
                {
                    const void* value = CPU_STACK_POP_PTR_CONST(cpu);
                    DO_FORMAT(value);
                }
                break;
            case 'A':
            case 'a':
            case 'G':
            case 'g':
            case 'E':
            case 'e':
            case 'F':
            case 'f':
                {
                    double value = CPU_STACK_POP_F64(cpu);
                    DO_FORMAT(value);
                }
                break;
            case 'd':
            case 'i':
            case 'U':
            case 'u':
            case 'X':
            case 'x':
            case 'B':
            case 'b':
            case 'O':
            case 'o':
            case 'p':
                {
                    if (dataSize == 8)
                    {
                        int64_t value = CPU_STACK_POP_I64(cpu);
                        DO_FORMAT(value);
                    }
                    else
                    {
                        int32_t value = CPU_STACK_POP_I32(cpu);
                        DO_FORMAT(value);
                    }
                }
                break;
        }
        
        size_t len = strlen(targetTempBuffer);
        switch (type)
        {
            case FT_Std:
                if (lazyPrintf != NULL)
                {
                    size_t lazyPrintfLen = lazyPrintfFmtStart - lazyPrintf;
                    writtenBytes += lazyPrintfLen;
                    printf("%.*s", lazyPrintfLen, lazyPrintf);
                    lazyPrintf = NULL;
                }
                printf("%s", targetTempBuffer);
                writtenBytes += len;
                break;
            case FT_File:
                fprintf(stream, "%s", targetTempBuffer);
                writtenBytes += len;
                break;
            case FT_Buffer:
            case FT_BufferLen:
                {
                    if (len > 0)
                    {
                        if (bufferSize > 0 && writtenBytes + len > bufferSize)
                        {
                            len = bufferSize - writtenBytes;
                        }
                        memcpy(buffer, targetTempBuffer, len);
                        buffer += len;
                        writtenBytes += len;
                    }
                }
                break;
        }
    }
    if (lazyPrintf != NULL)
    {
        size_t lazyPrintfLen = ptr - lazyPrintf;
        writtenBytes += lazyPrintfLen;
        printf("%.*s", lazyPrintfLen, lazyPrintf);
    }
    if (buffer != NULL)
    {
        *buffer = '\0';
    }
    cpu->Reg[REG_ESP] = espStart;
    CPU_STACK_RETURN(cpu, writtenBytes);
}

// This is a super lazy implementation (grabs pointers until it finds an invalid one, then passes the pointers to real functions)
#define MAX_SCANF_ARGS 32
void scanf_impl(CPU* cpu, FormatType type, FormatVa va)
{
    CPU_STACK_BEGIN(cpu);
    
    FILE* stream = NULL;
    char* buffer = NULL;
    const char* fmt = NULL;
    switch (type)
    {
        case FT_File:
            {
                uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
                stream = get_file(cpu, handleId);
                if (stream == NULL)
                {
                    return;
                }
            }
            break;
        case FT_Buffer:
            buffer = CPU_STACK_POP_PTR(cpu);
            if (buffer == NULL)
            {
                return;
            }
            break;
    }
    fmt = CPU_STACK_POP_PTR_CONST(cpu);
    size_t writtenBytes = 0;// Only used for FT_BufferLen (don't rely on it for anything else)
    
    if (va == FVA_Yes)
    {
        cpu->Reg[REG_ESP] = cpu_pop32(cpu); 
        CPU_STACK_RESET(cpu);
    }
    
    void* args[MAX_SCANF_ARGS] = {0};
    int32_t numArgs = 0;
    while (numArgs < MAX_SCANF_ARGS)
    {
        CPU_SIZE_T addr = CPU_STACK_POP_SIZE_T(cpu);
        if (addr == 0)
        {
            break;
        }
        if (cpu_is_valid_address(cpu, addr, sizeof(CPU_SIZE_T)))
        {
            args[numArgs] = cpu_get_real_address(cpu, addr);
        }
        else
        {
            break;
        }
        numArgs++;
    }
    
    if (numArgs == 0)
    {
        return;
    }
    
    switch (type)
    {
        case FT_Std:
            if (numArgs <= 4) scanf(fmt, args[0], args[1], args[2], args[3]);
            else if (numArgs <= 8) scanf(fmt, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
            else if (numArgs <= 16) scanf(fmt, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15]);
            else scanf(fmt, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19], args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27], args[28], args[29], args[30], args[31]);
            break;
        case FT_File:
            if (numArgs <= 4) fscanf(stream, fmt, args[0], args[1], args[2], args[3]);
            else if (numArgs <= 8) fscanf(stream, fmt, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
            else if (numArgs <= 16) fscanf(stream, fmt, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15]);
            else fscanf(stream, fmt, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19], args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27], args[28], args[29], args[30], args[31]);
            break;
        case FT_Buffer:
            if (numArgs <= 4) sscanf(buffer, fmt, args[0], args[1], args[2], args[3]);
            else if (numArgs <= 8) sscanf(buffer, fmt, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
            else if (numArgs <= 16) sscanf(buffer, fmt, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15]);
            else sscanf(buffer, fmt, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19], args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27], args[28], args[29], args[30], args[31]);
            break;
    }
}

void crt_clearerr(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    clearerr(stream);
}

void crt_fclose(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    if (stream != NULL)
    {
        handles_remove(&cpu->FileHandles, handleId);
        CPU_STACK_RETURN(cpu, fclose(stream));
    }
    else
    {
        CPU_STACK_RETURN(cpu, 0);
    }
}

void crt__fdopen(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t fd = CPU_STACK_POP_I32(cpu);
    const char* mode = CPU_STACK_POP_PTR_CONST(cpu);
    
    FILE* stream = fdopen(fd, mode);
    if (stream != NULL)
    {
        CPU_STACK_RETURN(cpu, handles_create(&cpu->FileHandles, stream));
    }
    else
    {
        CPU_STACK_RETURN(cpu, 0);
    }
}

void crt_feof(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, feof(stream));
}

void crt_ferror(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, ferror(stream));
}

void crt_fflush(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, fflush(stream));
}

void crt_fgetc(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, fgetc(stream));
}

void crt_fgetpos(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    fpos_t* pos = CPU_STACK_POP_PTR(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, fgetpos(stream, pos));
}

void crt_fgets(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    char* str = CPU_STACK_POP_PTR(cpu);
    int32_t num = CPU_STACK_POP_I32(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, fgets(str, num, stream)));
}

void crt__fileno(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, fileno(stream));
}

void crt_fopen(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* filename = CPU_STACK_POP_PTR_CONST(cpu);
    const char* mode = CPU_STACK_POP_PTR_CONST(cpu);
    
    FILE* stream = fopen(filename, mode);
    if (stream != NULL)
    {
        CPU_STACK_RETURN(cpu, handles_create(&cpu->FileHandles, stream));
    }
    else
    {
        CPU_STACK_RETURN(cpu, 0);
    }
}

void crt_fprintf(CPU* cpu)
{
    printf_impl(cpu, FT_File, FVA_No);
}

void crt_fputc(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t character = CPU_STACK_POP_I32(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, fputc(character, stream));
}

void crt_fputs(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, fputs(str, stream));
}

void crt_fread(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    void* ptr = CPU_STACK_POP_PTR(cpu);
    CPU_SIZE_T size = CPU_STACK_POP_SIZE_T(cpu);
    CPU_SIZE_T count = CPU_STACK_POP_SIZE_T(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, fread(ptr, size, count, stream));
}

void crt_freopen(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* filename = CPU_STACK_POP_PTR_CONST(cpu);
    const char* mode = CPU_STACK_POP_PTR_CONST(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    if (stream != NULL)
    {
        handles_remove(&cpu->FileHandles, handleId);
    }
    FILE* result = freopen(filename, mode, stream);
    if (result != NULL)
    {
        CPU_STACK_RETURN(cpu, handles_create(&cpu->FileHandles, result));
    }
    else
    {
        CPU_STACK_RETURN(cpu, 0);
    }
}

void crt_fscanf(CPU* cpu)
{
    scanf_impl(cpu, FT_File, FVA_No);
}

void crt_fseek(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    int32_t offset = CPU_STACK_POP_I32(cpu);
    int32_t origin = CPU_STACK_POP_I32(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, fseek(stream, offset, origin));
}

void crt_fsetpos(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    fpos_t* pos = CPU_STACK_POP_PTR(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, fsetpos(stream, pos));
}

void crt_ftell(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, ftell(stream));
}

void crt_fwrite(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const void* ptr = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_SIZE_T size = CPU_STACK_POP_SIZE_T(cpu);
    CPU_SIZE_T count = CPU_STACK_POP_SIZE_T(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    cpu_validate_real_address(cpu, ptr);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, fwrite(ptr, size, count, stream));
}

void crt_getc(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, getc(stream));
}

void crt_getchar(CPU* cpu)
{
    CPU_STACK_RETURN(cpu, getchar());
}

void crt_gets(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    char* str = CPU_STACK_POP_PTR(cpu);
    
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, gets(str)));
}

void crt__getw(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, getw(stream));
}

void crt__pclose(CPU* cpu)
{
    // TODO (pipes)
}

void crt_perror(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);
    
    perror(str);
}

void crt__popen(CPU* cpu)
{
    // TODO (pipes)
}

void crt_printf(CPU* cpu)
{
    printf_impl(cpu, FT_Std, FVA_No);
}

void crt_putc(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t character = CPU_STACK_POP_I32(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, putc(character, stream));
}

void crt_putchar(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t character = CPU_STACK_POP_I32(cpu);
    
    CPU_STACK_RETURN(cpu, putchar(character));
}

void crt_puts(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);
    
    CPU_STACK_RETURN(cpu, puts(str));
}

void crt__putw(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t character = CPU_STACK_POP_I32(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, putw(character, stream));
}

void crt_remove(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* filename = CPU_STACK_POP_PTR_CONST(cpu);
    
    CPU_STACK_RETURN(cpu, remove(filename));
}

void crt_rename(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* oldname = CPU_STACK_POP_PTR_CONST(cpu);
    const char* newname = CPU_STACK_POP_PTR_CONST(cpu);
    
    CPU_STACK_RETURN(cpu, rename(oldname, newname));
}

void crt_rewind(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    rewind(stream);
}

void crt_scanf(CPU* cpu)
{
    scanf_impl(cpu, FT_Std, FVA_No);
}

void crt_setbuf(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    char* buffer = CPU_STACK_POP_PTR(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    setbuf(stream, buffer);
}

void crt_setvbuf(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    char* buffer = CPU_STACK_POP_PTR(cpu);
    int32_t mode = CPU_STACK_POP_I32(cpu);
    CPU_SIZE_T size = CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, setvbuf(stream, buffer, mode, size));
}

void crt__snprintf(CPU* cpu)
{
    printf_impl(cpu, FT_BufferLen, FVA_No);
}

void crt_sprintf(CPU* cpu)
{
    printf_impl(cpu, FT_Buffer, FVA_No);
}

void crt_sscanf(CPU* cpu)
{
    scanf_impl(cpu, FT_Buffer, FVA_No);
}

void crt__tempnam(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* dif = CPU_STACK_POP_PTR_CONST(cpu);
    const char* pfx = CPU_STACK_POP_PTR_CONST(cpu);
    
    CPU_STACK_RETURN(cpu, 0);
    char* result = tempnam(dif, pfx);
    if (result != NULL)
    {
        size_t len = strlen(result);
        char* str = memmgr_alloc(&cpu->Memory, len + 1);
        if (str != NULL)
        {
            memcpy(str, result, len);
            str[len] = 0;
            CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, str));
        }
        free(result);
    }
    
}

void crt_tmpfile(CPU* cpu)
{
    FILE* result = tmpfile();
    if (result != NULL)
    {
        CPU_STACK_RETURN(cpu, handles_create(&cpu->FileHandles, result));
    }
    else
    {
        CPU_STACK_RETURN(cpu, 0);
    }
}

void crt_tmpnam(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    char* str = CPU_STACK_POP_PTR(cpu);
    
    if (str == NULL)
    {
        crt_update_static_var(cpu, &cpu->Statics_tmpname, L_tmpnam);
        if (cpu->Statics_tmpname != 0)
        {
            tmpnam(cpu_get_real_address(cpu, cpu->Statics_tmpname));
            CPU_STACK_RETURN(cpu, cpu->Statics_tmpname);
        }
        else
        {
            CPU_STACK_RETURN(cpu, 0);
        }
    }
    else
    {
        CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, tmpnam(str)));
    }
}

void crt_ungetc(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t character = CPU_STACK_POP_I32(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    
    FILE* stream = get_file(cpu, handleId);
    CPU_STACK_RETURN(cpu, ungetc(character, stream));
}

void crt_vfprintf(CPU* cpu)
{
    printf_impl(cpu, FT_File, FVA_Yes);
}

void crt_vprintf(CPU* cpu)
{
    printf_impl(cpu, FT_Std, FVA_Yes);
}

void crt_vsnprintf(CPU* cpu)
{
    printf_impl(cpu, FT_BufferLen, FVA_Yes);
}

void crt_vsprintf(CPU* cpu)
{
    printf_impl(cpu, FT_Buffer, FVA_Yes);
}

/////////////////////////////////
// stdlib.h
/////////////////////////////////

void crt_abort(CPU* cpu)
{
    cpu->Complete = 1;
}

void crt_abs(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t x = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, abs(x));
}

void crt_atexit(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    CPU_SIZE_T funcPtr = CPU_STACK_POP_SIZE_T(cpu);
    cpu->AtExitFuncPtr = funcPtr;
    CPU_STACK_RETURN(cpu, 0);
}

void crt_atof(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);
    fpu_push(cpu, atof(str));
}

void crt_atoi(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, atoi(str));
}

void crt_atol(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, atol(str));
}

void crt__atoi64(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN_I64(cpu, _atoi64(str));
#else
    CPU_STACK_RETURN_I64(cpu, atoll(str));
#endif
}

void crt_bsearch(CPU* cpu)
{
    // TODO
}

void crt_calloc(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    CPU_SIZE_T num = CPU_STACK_POP_SIZE_T(cpu);
    CPU_SIZE_T size = CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, memmgr_calloc(&cpu->Memory, num, size)));
}

void crt_div(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t numer = CPU_STACK_POP_I32(cpu);
    int32_t denom = CPU_STACK_POP_I32(cpu);
    div_t result = div(numer, denom);// int(quot) / int(rem)
    CPU_STACK_RETURN_I64(cpu, *(int64_t*)&result);
}

void crt__ecvt(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double value = CPU_STACK_POP_F64(cpu);
    int32_t count = CPU_STACK_POP_I32(cpu);
    int32_t* dec = CPU_STACK_POP_PTR(cpu);
    int32_t* sign = CPU_STACK_POP_PTR(cpu);
    char* result = NULL;
#if PLATFORM_WINDOWS
    //result = _ecvt(value, count, dec, sign);
#else
    //result = ecvt(value, count, dec, sign);
#endif
    // TODO
}

void crt_exit(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t status = CPU_STACK_POP_I32(cpu);
    cpu->Complete = 1;
}

void crt__fcvt(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double value = CPU_STACK_POP_F64(cpu);
    int32_t count = CPU_STACK_POP_I32(cpu);
    int32_t* dec = CPU_STACK_POP_PTR(cpu);
    int32_t* sign = CPU_STACK_POP_PTR(cpu);
    char* result = NULL;
#if PLATFORM_WINDOWS
    //result = _fcvt(value, count, dec, sign);
#else
    //result = fcvt(value, count, dec, sign);
#endif
    // TODO
}

void crt_free(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    CPU_SIZE_T ptr = CPU_STACK_POP_SIZE_T(cpu);
    memmgr_free(&cpu->Memory, cpu_get_real_address(cpu, ptr));
}

void crt__gcvt(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    double value = CPU_STACK_POP_F64(cpu);
    int32_t digits = CPU_STACK_POP_I32(cpu);
    char* buffer = CPU_STACK_POP_PTR(cpu);
#if PLATFORM_WINDOWS
    _gcvt(value, digits, buffer);
#else
    gcvt(value, digits, buffer);
#endif
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, buffer));
}

void crt_getenv(CPU* cpu)
{
    // TODO
}

void crt__itoa(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t value = CPU_STACK_POP_I32(cpu);
    char* sp = CPU_STACK_POP_PTR(cpu);
    int32_t radix = CPU_STACK_POP_I32(cpu);

    // Taken from https://stackoverflow.com/questions/3440726/what-is-the-proper-way-of-implementing-a-good-itoa-function/12386915#12386915
    {
        char tmp[16];// be careful with the length of the buffer
        char *tp = tmp;
        int i;
        unsigned v;

        int sign = (radix == 10 && value < 0);    
        if (sign)
            v = -value;
        else
            v = (unsigned)value;

        while (v || tp == tmp)
        {
            i = v % radix;
            v /= radix;
            if (i < 10)
              *tp++ = i+'0';
            else
              *tp++ = i + 'a' - 10;
        }

        int len = tp - tmp;

        if (sign) 
        {
            *sp++ = '-';
            len++;
        }

        while (tp > tmp)
            *sp++ = *--tp;
        
        CPU_STACK_RETURN(cpu, len);
    }
}

void crt_labs(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t x = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, labs(x));
}

void crt_ldiv(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t numer = CPU_STACK_POP_I32(cpu);
    int32_t denom = CPU_STACK_POP_I32(cpu);
    div_t result = div(numer, denom);// int(quot) / int(rem)
    CPU_STACK_RETURN_I64(cpu, *(int64_t*)&result);
}

void crt__abs64(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int64_t x = CPU_STACK_POP_I64(cpu);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN_I64(cpu, _abs64(x));
#else
    CPU_STACK_RETURN_I64(cpu, llabs(x));
#endif
}

void crt_malloc(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    CPU_SIZE_T size = CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, memmgr_alloc(&cpu->Memory, size)));
}

void crt_mblen(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_SIZE_T n = CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_RETURN(cpu, mblen(str, n));
}

void crt_mbstowcs(CPU* cpu)
{
    // TODO (wchar_t)
}

void crt_mbtowc(CPU* cpu)
{
    // TODO (wchar_t)
}

void crt__putenv(CPU* cpu)
{
    // TODO
}

void crt_qsort(CPU* cpu)
{
    // TODO
}

void crt_rand(CPU* cpu)
{
    CPU_STACK_RETURN(cpu, rand());
}

void crt_realloc(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    CPU_SIZE_T ptr = CPU_STACK_POP_SIZE_T(cpu);
    CPU_SIZE_T size = CPU_STACK_POP_SIZE_T(cpu);
    void* result = memmgr_realloc(&cpu->Memory, cpu_get_real_address(cpu, ptr), size);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, result));
}

void crt_srand(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t seed = CPU_STACK_POP_U32(cpu);
    srand(seed);
}

void crt_strtod(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_SIZE_T endptr = CPU_STACK_POP_SIZE_T(cpu);
    char* endptr2;
    double result = strtod(str, &endptr2);
    if (endptr != 0)
    {
        cpu_writeU32(cpu, endptr, (uint32_t)cpu_get_virtual_address(cpu, str) + (uint32_t)(endptr2 - str));
    }
    fpu_push(cpu, result);
}

#define strtox(cpu, t, f, r)\
    CPU_STACK_BEGIN(cpu);\
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);\
    CPU_SIZE_T endptr = CPU_STACK_POP_SIZE_T(cpu);\
    int32_t base = CPU_STACK_POP_I32(cpu);\
    char* endptr2;\
    t result = f(str, &endptr2, base);\
    if (endptr != 0)\
    {\
        cpu_writeU32(cpu, endptr, (uint32_t)cpu_get_virtual_address(cpu, str) + (uint32_t)(endptr2 - str));\
    }\
    r(cpu, result);

void crt_strtol(CPU* cpu)
{
    strtox(cpu, long int, strtol, CPU_STACK_RETURN);
}

void crt_strtoul(CPU* cpu)
{
    strtox(cpu, unsigned long int, strtoul, CPU_STACK_RETURN);
}

void crt__strtoi64(CPU* cpu)
{
#if PLATFORM_WINDOWS
    strtox(cpu, int64_t, _strtoi64, CPU_STACK_RETURN_I64);
#else
    strtox(cpu, int64_t, strtoll, CPU_STACK_RETURN_I64);
#endif
}

void crt__strtoui64(CPU* cpu)
{
#if PLATFORM_WINDOWS
    strtox(cpu, uint64_t, _strtoui64, CPU_STACK_RETURN_I64);
#else
    strtox(cpu, uint64_t, strtoull, CPU_STACK_RETURN_I64);
#endif
}

void crt_wcstombs(CPU* cpu)
{
    // TODO (wchar_t)
}

void crt_wctomb(CPU* cpu)
{
    // TODO (wchar_t)
}

/////////////////////////////////
// string.h
/////////////////////////////////

void crt__memccpy(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    void* dest = CPU_STACK_POP_PTR(cpu);
    const void* src = CPU_STACK_POP_PTR_CONST(cpu);
    int32_t c = CPU_STACK_POP_I32(cpu);
    CPU_SIZE_T n = CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, memccpy(dest, src, c, n)));
}

void crt_memchr(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    void* ptr = CPU_STACK_POP_PTR(cpu);
    int32_t value = CPU_STACK_POP_I32(cpu);
    CPU_SIZE_T num = CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, memchr(ptr, value, num)));
}

void crt_memcmp(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    void* ptr1 = CPU_STACK_POP_PTR(cpu);
    void* ptr2 = CPU_STACK_POP_PTR(cpu);
    CPU_SIZE_T num = CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_RETURN(cpu, memcmp(ptr1, ptr2, num));
}

void crt_memcpy(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    void* dest = CPU_STACK_POP_PTR(cpu);
    const void* src = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_SIZE_T num = CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, memcpy(dest, src, num)));
}

void crt_memmove(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    void* dest = CPU_STACK_POP_PTR(cpu);
    const void* src = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_SIZE_T num = CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, memmove(dest, src, num)));
}

void crt_memset(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    void* ptr = CPU_STACK_POP_PTR(cpu);
    int32_t value = CPU_STACK_POP_I32(cpu);
    CPU_SIZE_T num = CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_RETURN(cpu, memset(ptr, value, num));
}

void crt_strcat(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    char* dest = CPU_STACK_POP_PTR(cpu);
    const char* src = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, strcat(dest, src)));
}

void crt_strchr(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR(cpu);
    int32_t c = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, strchr(str, c)));
}

void crt_strcmp(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str1 = CPU_STACK_POP_PTR_CONST(cpu);
    const char* str2 = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, strcmp(str1, str2));
}

void crt_strcoll(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str1 = CPU_STACK_POP_PTR_CONST(cpu);
    const char* str2 = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, strcoll(str1, str2));
}

void crt_strcpy(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    char* dest = CPU_STACK_POP_PTR(cpu);
    const char* src = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, strcpy(dest, src)));
}

void crt_strcspn(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str1 = CPU_STACK_POP_PTR_CONST(cpu);
    const char* str2 = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, strcspn(str1, str2));
}

void crt__strdup(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, strdup(str)));
}

void crt_strerror(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t errnum = CPU_STACK_POP_I32(cpu);
    
    // There are going to be differences on different platforms, but there isn't a great way of implementing this
    // without tampering with errno (which is what is mostly passed into strerror)
    char* str = strerror(errnum);
    if (str != NULL)
    {
        crt_update_static_str(cpu, &cpu->Statics_streerror, strlen(str) + 1);
        if (cpu->Statics_streerror != 0)
        {
            strcpy(cpu_get_real_address(cpu, cpu->Statics_streerror), str);
            CPU_STACK_RETURN(cpu, cpu->Statics_streerror);
        }
        else
        {
            CPU_STACK_RETURN(cpu, 0);
        }
    }
    else
    {
        CPU_STACK_RETURN(cpu, 0);
    }
}

void crt__stricmp(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const unsigned char* str1 = CPU_STACK_POP_PTR_CONST(cpu);
    const unsigned char* str2 = CPU_STACK_POP_PTR_CONST(cpu);
    while (tolower(*str1) == tolower(*str2))
    {
        if (*str1 == '\0')
        {
            CPU_STACK_RETURN(cpu, 0);
            return;
        }
        str1++;
        str2++;
    }
    CPU_STACK_RETURN(cpu, tolower(*str1) - tolower(*str2));
}

void crt_strlen(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, strlen(str));
}

void crt__strlwr(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);
    if (str != NULL)
    {
        unsigned char *p = (unsigned char *)str;
        while (*p)
        {
            *p = tolower((unsigned char)*p);
            p++;
        }
    }
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, str));
}

void crt_strncat(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    char* dest = CPU_STACK_POP_PTR(cpu);
    const char* src = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_SIZE_T num = CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, strncat(dest, src, num)));
}

void crt_strncmp(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str1 = CPU_STACK_POP_PTR_CONST(cpu);
    const char* str2 = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_SIZE_T num = CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_RETURN(cpu, strncmp(str1, str2, num));
}

void crt_strncpy(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    char* dest = CPU_STACK_POP_PTR(cpu);
    const char* src = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_SIZE_T num = CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, strncpy(dest, src, num)));
}

void crt__strnicmp(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const unsigned char* str1 = CPU_STACK_POP_PTR_CONST(cpu);
    const unsigned char* str2 = CPU_STACK_POP_PTR_CONST(cpu);
    uint32_t count = CPU_STACK_POP_U32(cpu);
    int32_t i = 0;
    while (i < count && tolower(str1[i]) == tolower(str2[i]))
    {
        if (str1[i] == '\0')
        {
            CPU_STACK_RETURN(cpu, 0);
            return;
        }
        i++;
    }
    if (i >= count)
    {
        CPU_STACK_RETURN(cpu, 0);
    }
    else
    {
        CPU_STACK_RETURN(cpu, tolower(*str1) - tolower(*str2));
    }
}

void crt_strnlen(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_SIZE_T n = CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_RETURN(cpu, strnlen(str, n));
}

void crt_strpbrk(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str1 = CPU_STACK_POP_PTR_CONST(cpu);
    const char* str2 = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, strpbrk(str1, str2)));
}

void crt_strrchr(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str = CPU_STACK_POP_PTR_CONST(cpu);
    int32_t c = CPU_STACK_POP_I32(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, strrchr(str, c)));
}

void crt_strspn(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str1 = CPU_STACK_POP_PTR_CONST(cpu);
    const char* str2 = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, strspn(str1, str2));
}

void crt_strstr(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* str1 = CPU_STACK_POP_PTR_CONST(cpu);
    const char* str2 = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, strstr(str1, str2)));
}

void crt_strtok(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    char* str = CPU_STACK_POP_PTR(cpu);
    const char* delim = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_RETURN(cpu, cpu_get_virtual_address(cpu, strtok(str, delim)));
}

void crt_strxfrm(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    char* str1 = CPU_STACK_POP_PTR(cpu);
    const char* str2 = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_SIZE_T num = CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_RETURN(cpu, strxfrm(str1, str2, num));
}

/////////////////////////////////
// time.h
/////////////////////////////////

void crt_clock(CPU* cpu)
{
    CPU_STACK_RETURN(cpu, clock());
}

/////////////////////////////////
// wctype.h / wchar.h
/////////////////////////////////

#define WCTYPE_FUNC(cpu, x) CPU_STACK_BEGIN(cpu);\
    wint_t c = CPU_STACK_POP_I32(cpu);\
    CPU_STACK_RETURN(cpu, x(c));

void crt_iswalpha(CPU* cpu)
{
    WCTYPE_FUNC(cpu, iswalpha);
}

void crt_iswupper(CPU* cpu)
{
    WCTYPE_FUNC(cpu, iswupper);
}

void crt_iswlower(CPU* cpu)
{
    WCTYPE_FUNC(cpu, iswlower);
}

void crt_iswdigit(CPU* cpu)
{
    WCTYPE_FUNC(cpu, iswdigit);
}

void crt_iswxdigit(CPU* cpu)
{
    WCTYPE_FUNC(cpu, iswxdigit);
}

void crt_iswspace(CPU* cpu)
{
    WCTYPE_FUNC(cpu, iswspace);
}

void crt_iswpunct(CPU* cpu)
{
    WCTYPE_FUNC(cpu, iswpunct);
}

void crt_iswalnum(CPU* cpu)
{
    WCTYPE_FUNC(cpu, iswalnum);
}

void crt_iswprint(CPU* cpu)
{
    WCTYPE_FUNC(cpu, iswprint);
}

void crt_iswgraph(CPU* cpu)
{
    WCTYPE_FUNC(cpu, iswgraph);
}

void crt_iswcntrl(CPU* cpu)
{
    WCTYPE_FUNC(cpu, iswcntrl);
}

void crt_iswascii(CPU* cpu)
{
    WCTYPE_FUNC(cpu, iswascii);
}

void crt_isleadbyte(CPU* cpu)
{
    WCTYPE_FUNC(cpu, isleadbyte);
}

void crt_towupper(CPU* cpu)
{
    WCTYPE_FUNC(cpu, towupper);
}

void crt_towlower(CPU* cpu)
{
    WCTYPE_FUNC(cpu, towlower);
}

void crt_iswctype(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    wint_t c = CPU_STACK_POP_I32(cpu);
    wctype_t desc = CPU_STACK_POP_U16(cpu);
    CPU_STACK_RETURN(cpu, iswctype(c, desc));
}

void crt_is_wctype(CPU* cpu)
{
    crt_iswctype(cpu);
}

/////////////////////////////////
// kernel32.dll (__stdcall)
/////////////////////////////////

void crt_GetModuleFileNameA(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    CPU_SIZE_T hModule = CPU_STACK_POP_SIZE_T(cpu);
    char* lpFilename = CPU_STACK_POP_PTR(cpu);
    uint32_t nSize = CPU_STACK_POP_U32(cpu);
    CPU_STACK_END(cpu);
    
    if (hModule == 0)
    {
        hModule = cpu->MainModule->VirtualAddress;
    }
    
    uint32_t result = 0;
    if (hModule != 0 && lpFilename != NULL)
    {
        const char* key;
        map_iter_t iter = map_iter(cpu->Modules);
        while ((key = map_next(&cpu->Modules, &iter)))
        {
            ModuleInfo** modulePtr = map_get(&cpu->Modules, key);
            if (modulePtr != NULL)
            {
                ModuleInfo* moduleInfo = *modulePtr;
                if (moduleInfo->VirtualAddress == hModule)
                {
                    for (uint32_t i = 0; i < nSize; i++)
                    {
                        lpFilename[i] = moduleInfo->Path[i];
                        if (moduleInfo->Path[i] == '\0')
                        {
                            break;
                        }
                        result = i;
                    }
                    break;
                }
            }
        }
    }
    CPU_STACK_RETURN(cpu, result);
}

void crt_GetSystemDirectoryA(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    char* lpBuffer = CPU_STACK_POP_PTR(cpu);
    uint32_t uSize = CPU_STACK_POP_U32(cpu);
    CPU_STACK_END(cpu);
    
    // "C:/Windows/System32"
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, GetSystemDirectoryA(lpBuffer, uSize));
#else
    char* systemPath = "/";
    if (lpBuffer != NULL)
    {
        size_t len = strlen(systemPath);
        if (uSize < len)
        {
            len = uSize + 1;
        }
        strncpy(lpBuffer, systemPath, len);
        lpBuffer[uSize] = '\0';
        CPU_STACK_RETURN(cpu, len);
    }
    else
    {
        CPU_STACK_RETURN(cpu, 0);
    }
#endif
}

/////////////////////////////////
// winsock.h (__stdcall)
/////////////////////////////////

SOCKET get_socket(CPU* cpu, uint32_t handleId)
{
    SOCKET s = INVALID_SOCKET;
    if (handleId != 0)
    {
        s = (SOCKET)handles_find(&cpu->SocketHandles, handleId);
    }
    return s;
}

void crt_accept(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    struct sockaddr* addr = CPU_STACK_POP_PTR(cpu);
    int32_t* addrlen = CPU_STACK_POP_PTR(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
    SOCKET result = accept(s, addr, addrlen);
    if (result != SOCKET_ERROR && result != INVALID_SOCKET)
    {
        CPU_STACK_RETURN(cpu, handles_create(&cpu->SocketHandles, (void*)result));
    }
    else
    {
        CPU_STACK_RETURN(cpu, result);
    }
}

void crt_bind(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    struct sockaddr* name = CPU_STACK_POP_PTR(cpu);
    int32_t namelen = CPU_STACK_POP_I32(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
    CPU_STACK_RETURN(cpu, bind(s, name, namelen));
}

void crt_closesocket(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, closesocket(s));
#else
    CPU_STACK_RETURN(cpu, close(s));
#endif
    handles_remove(&cpu->SocketHandles, handleId);
}

void crt_connect(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    struct sockaddr* name = CPU_STACK_POP_PTR(cpu);
    int32_t namelen = CPU_STACK_POP_I32(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
    CPU_STACK_RETURN(cpu, connect(s, name, namelen));
}

void crt_ioctlsocket(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    int32_t cmd = CPU_STACK_POP_I32(cpu);
    uint32_t* argp = CPU_STACK_POP_PTR(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, ioctlsocket(s, cmd, argp));
#else
    CPU_STACK_RETURN(cpu, ioctl(s, cmd, argp));
#endif
}

void crt_getpeername(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    struct sockaddr* name = CPU_STACK_POP_PTR(cpu);
    int32_t* namelen = CPU_STACK_POP_PTR(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
    CPU_STACK_RETURN(cpu, getpeername(s, name, namelen));
}

void crt_getsockname(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    struct sockaddr* name = CPU_STACK_POP_PTR(cpu);
    int32_t* namelen = CPU_STACK_POP_PTR(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
    CPU_STACK_RETURN(cpu, getsockname(s, name, namelen));
}

void crt_getsockopt(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    int32_t level = CPU_STACK_POP_I32(cpu);
    int32_t optname = CPU_STACK_POP_I32(cpu);
    char* optval = CPU_STACK_POP_PTR(cpu);
    int* optlen = CPU_STACK_POP_PTR(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
    CPU_STACK_RETURN(cpu, getsockopt(s, level, optname, optval, optlen));
}

void crt_htonl(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t hostlong = CPU_STACK_POP_U32(cpu);
    CPU_STACK_END(cpu);
    
    CPU_STACK_RETURN(cpu, htonl(hostlong));
}

void crt_htons(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint16_t hostshort = CPU_STACK_POP_U16(cpu);
    CPU_STACK_END(cpu);
    
    CPU_STACK_RETURN(cpu, htons(hostshort));
}

void crt_inet_addr(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    const char* cp = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_END(cpu);
    
    CPU_STACK_RETURN(cpu, inet_addr(cp));
}

void crt_inet_ntoa(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t in = CPU_STACK_POP_U32(cpu);
    CPU_STACK_END(cpu);
    
    crt_update_static_var(cpu, &cpu->Statics_inet_ntoa, 16);
    if (cpu->Statics_inet_ntoa != 0)
    {
        char* result = inet_ntoa(*(struct in_addr*)&in);
        if (result != NULL)
        {
            strcpy(cpu_get_real_address(cpu, cpu->Statics_inet_ntoa), result);
            CPU_STACK_RETURN(cpu, cpu->Statics_inet_ntoa);
        }
        else
        {
            CPU_STACK_RETURN(cpu, 0);
        }
    }
    else
    {
        CPU_STACK_RETURN(cpu, 0);
    }
}

void crt_listen(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    int32_t backlog = CPU_STACK_POP_I32(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
    CPU_STACK_RETURN(cpu, listen(s, backlog));
}

void crt_ntohl(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t netlong = CPU_STACK_POP_U32(cpu);
    CPU_STACK_END(cpu);
    
    CPU_STACK_RETURN(cpu, ntohl(netlong));
}

void crt_ntohs(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint16_t netshort = CPU_STACK_POP_U16(cpu);
    CPU_STACK_END(cpu);
    
    CPU_STACK_RETURN(cpu, ntohs(netshort));
}

void crt_recv(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    char* buf = CPU_STACK_POP_PTR(cpu);
    int32_t len = CPU_STACK_POP_I32(cpu);
    int32_t flags = CPU_STACK_POP_I32(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
    CPU_STACK_RETURN(cpu, recv(s, buf, len, flags));
}

void crt_recvfrom(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    char* buf = CPU_STACK_POP_PTR(cpu);
    int32_t len = CPU_STACK_POP_I32(cpu);
    int32_t flags = CPU_STACK_POP_I32(cpu);
    struct sockaddr* from = CPU_STACK_POP_PTR(cpu);
    int32_t* fromlen = CPU_STACK_POP_PTR(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
    CPU_STACK_RETURN(cpu, recvfrom(s, buf, len, flags, from, fromlen));
}

void crt_select(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t nfds = CPU_STACK_POP_I32(cpu);
    fd_set* readfds = CPU_STACK_POP_PTR(cpu);
    fd_set* writefds = CPU_STACK_POP_PTR(cpu);
    fd_set* exceptfds = CPU_STACK_POP_PTR(cpu);
    const struct timeval* timeout = CPU_STACK_POP_PTR_CONST(cpu);
    CPU_STACK_END(cpu);
    
    CPU_STACK_RETURN(cpu, select(nfds, readfds, writefds, exceptfds, timeout));
}

void crt_send(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    char* buf = CPU_STACK_POP_PTR(cpu);
    int32_t len = CPU_STACK_POP_I32(cpu);
    int32_t flags = CPU_STACK_POP_I32(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
    CPU_STACK_RETURN(cpu, send(s, buf, len, flags));
}

void crt_sendto(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    char* buf = CPU_STACK_POP_PTR(cpu);
    int32_t len = CPU_STACK_POP_I32(cpu);
    int32_t flags = CPU_STACK_POP_I32(cpu);
    struct sockaddr* to = CPU_STACK_POP_PTR(cpu);
    int32_t tolen = CPU_STACK_POP_I32(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
    CPU_STACK_RETURN(cpu, sendto(s, buf, len, flags, to, tolen));
}

void crt_setsockopt(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    int32_t level = CPU_STACK_POP_I32(cpu);
    int32_t optname = CPU_STACK_POP_I32(cpu);
    const char* optval = CPU_STACK_POP_PTR_CONST(cpu);
    int32_t optlen = CPU_STACK_POP_I32(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
    CPU_STACK_RETURN(cpu, setsockopt(s, level, optname, optval, optlen));
}

void crt_shutdown(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint32_t handleId = (uint32_t)CPU_STACK_POP_SIZE_T(cpu);
    int32_t how = CPU_STACK_POP_I32(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = get_socket(cpu, handleId);
    CPU_STACK_RETURN(cpu, shutdown(s, how));
}

void crt_socket(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t af = CPU_STACK_POP_I32(cpu);
    int32_t type = CPU_STACK_POP_I32(cpu);
    int32_t protocol = CPU_STACK_POP_I32(cpu);
    CPU_STACK_END(cpu);
    
    SOCKET s = socket(af, type, protocol);
    if (s != INVALID_SOCKET)
    {
        CPU_STACK_RETURN(cpu, handles_create(&cpu->SocketHandles, (void*)s));
    }
    else
    {
        CPU_STACK_RETURN(cpu, 0);
    }
}

void crt_gethostbyaddr(CPU* cpu)
{
    // TODO
}

void crt_gethostbyname(CPU* cpu)
{
    // TODO
}

void crt_gethostname(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    char* name = CPU_STACK_POP_PTR(cpu);
    int namelen = CPU_STACK_POP_I32(cpu);
    CPU_STACK_END(cpu);
    
    CPU_STACK_RETURN(cpu, gethostname(name, namelen));
}

void crt_getservbyport(CPU* cpu)
{
    // TODO
}

void crt_getservbyname(CPU* cpu)
{
    // TODO
}

void crt_getprotobynumber(CPU* cpu)
{
    // TODO
}

void crt_getprotobyname(CPU* cpu)
{
    // TODO
}

void crt_WSAStartup(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    uint16_t wVersionRequested = CPU_STACK_POP_U16(cpu);
    void* lpWSAData = CPU_STACK_POP_PTR(cpu);
    CPU_STACK_END(cpu);
    
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, WSAStartup(wVersionRequested, lpWSAData));
#else
    CPU_STACK_RETURN(cpu, 0);
#endif
}

void crt_WSACleanup(CPU* cpu)
{
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, WSACleanup());
#else
    CPU_STACK_RETURN(cpu, 0);
#endif
}

void crt_WSASetLastError(CPU* cpu)
{
    CPU_STACK_BEGIN(cpu);
    int32_t iError = CPU_STACK_POP_I32(cpu);
    CPU_STACK_END(cpu);
    
#if PLATFORM_WINDOWS
    WSASetLastError(iError);
#else
    errno = iError;
#endif
}

void crt_WSAGetLastError(CPU* cpu)
{
#if PLATFORM_WINDOWS
    CPU_STACK_RETURN(cpu, WSAGetLastError());
#else
    CPU_STACK_RETURN(cpu, errno);
#endif
}