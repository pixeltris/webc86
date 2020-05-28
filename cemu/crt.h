#ifndef WEBC_CRT_H
#define WEBC_CRT_H

#include "cpu.h"

// TODO: Add support for some kind of virtual file system?
// TODO: wchar_t sizes are different on windows. There needs to be additional conversion on these types (or just implement them elsewhere).
// TODO: Make sure fpos_t sizes are the same on all platforms (expected to be int64), otherwise use conversions
// TODO: printf currently gets chopped up. This will cause issues on threaded programs. Fix this.
// TODO: scanf %p (and possibly other specifiers) will write 8 bytes on x64 platforms. This needs to be 4 bytes.
//       To fix this we need to pre-process the fmt arg and find which args are 8 bytes on x64 and 4 bytes on x86. Then remap them.
// TODO: socket structs probably need to be converted (likely size differences on x64) - especially select(). also constants differences (socket options, etc) - see wine ws2_32/socket.c
// TODO: threading
// TODO: signals
// TODO: pipes?
// TODO: scanf?
// TODO: qsort / bsearch

void crt_init_imports(CPU* cpu, int32_t* counter);
void crt_allocate_data_imports(CPU* cpu);

/////////////////////////////////
// misc
/////////////////////////////////

void crt_ignore(CPU* cpu);
void crt___getmainargs(CPU* cpu);

/////////////////////////////////
// assert.h
/////////////////////////////////

void crt__wassert(CPU* cpu);
void crt__assert(CPU* cpu);

/////////////////////////////////
// ctype.h
/////////////////////////////////

void crt_isalpha(CPU* cpu);
void crt_isupper(CPU* cpu);
void crt_islower(CPU* cpu);
void crt_isdigit(CPU* cpu);
void crt_isxdigit(CPU* cpu);
void crt_isspace(CPU* cpu);
void crt_ispunct(CPU* cpu);
void crt_isalnum(CPU* cpu);
void crt_isprint(CPU* cpu);
void crt_isgraph(CPU* cpu);
void crt_iscntrl(CPU* cpu);
void crt_toupper(CPU* cpu);
void crt_tolower(CPU* cpu);

/////////////////////////////////
// direct.h
/////////////////////////////////

void crt__getcwd(CPU* cpu);
void crt__getdcwd(CPU* cpu);
void crt__chdir(CPU* cpu);
void crt__mkdir(CPU* cpu);
void crt__rmdir(CPU* cpu);
void crt__chdrive(CPU* cpu);
void crt__getdrive(CPU* cpu);
void crt__getdrives(CPU* cpu);

/////////////////////////////////
// errno.h
/////////////////////////////////

void crt__errno(CPU* cpu);
void crt__set_errno(CPU* cpu);
void crt__get_errno(CPU* cpu);

/////////////////////////////////
// io.h
/////////////////////////////////

// TODO: We need to implement something like the following (either in the emulator itself, or in helper code)
// https://github.com/libgd/libgd/blob/master/tests/gdtest/readdir.c

void crt__findfirst(CPU* cpu);
void crt__findnext(CPU* cpu);
void crt__findclose(CPU* cpu);
void crt__access(CPU* cpu);
void crt__chmod(CPU* cpu);
void crt__chsize(CPU* cpu);
void crt__close(CPU* cpu);
void crt__commit(CPU* cpu);
void crt__creat(CPU* cpu);
void crt__dup(CPU* cpu);
void crt__dup2(CPU* cpu);
void crt__eof(CPU* cpu);
void crt__filelength(CPU* cpu);
void crt__isatty(CPU* cpu);
void crt__locking(CPU* cpu);
void crt__lseek(CPU* cpu);
void crt__mktemp(CPU* cpu);
void crt__open(CPU* cpu);
void crt__read(CPU* cpu);
void crt__setmode(CPU* cpu);
void crt__sopen(CPU* cpu);
void crt__tell(CPU* cpu);
void crt__umask(CPU* cpu);
void crt__write(CPU* cpu);
void crt__unlink(CPU* cpu);

/////////////////////////////////
// locale.h
/////////////////////////////////

void crt_setlocale(CPU* cpu);
void crt_localeconv(CPU* cpu);

/////////////////////////////////
// math.h
/////////////////////////////////

void crt_acos(CPU* cpu);
void crt_asin(CPU* cpu);
void crt_atan(CPU* cpu);
void crt_atan2(CPU* cpu);
void crt__copysign(CPU* cpu);
void crt__chgsign(CPU* cpu);
void crt_cos(CPU* cpu);
void crt_cosh(CPU* cpu);
void crt_exp(CPU* cpu);
void crt_fabs(CPU* cpu);
void crt_fmod(CPU* cpu);
void crt_log(CPU* cpu);
void crt_log10(CPU* cpu);
void crt_pow(CPU* cpu);
void crt_sin(CPU* cpu);
void crt_sinh(CPU* cpu);
void crt_tan(CPU* cpu);
void crt_tanh(CPU* cpu);
void crt_sqrt(CPU* cpu);
void crt__cabs(CPU* cpu);
void crt_ceil(CPU* cpu);
void crt_floor(CPU* cpu);
void crt_frexp(CPU* cpu);
void crt__hypot(CPU* cpu);
void crt__j0(CPU* cpu);
void crt__j1(CPU* cpu);
void crt__jn(CPU* cpu);
void crt_ldexp(CPU* cpu);
void crt_modf(CPU* cpu);
void crt__y0(CPU* cpu);
void crt__y1(CPU* cpu);
void crt__yn(CPU* cpu);

/////////////////////////////////
// process.h
/////////////////////////////////

void crt__beginthread(CPU* cpu);
void crt__endthread(CPU* cpu);
void crt__beginthreadex(CPU* cpu);
void crt__endthreadex(CPU* cpu);
void crt__loaddll(CPU* cpu);
void crt__unloaddll(CPU* cpu);
void crt__getdllprocaddr(CPU* cpu);
void crt__getpid(CPU* cpu);

/////////////////////////////////
// setjmp.h
/////////////////////////////////

void crt__setjmp(CPU* cpu);
void crt_longjmp(CPU* cpu);

/////////////////////////////////
// signal.h
/////////////////////////////////

void crt_signal(CPU* cpu);
void crt_raise(CPU* cpu);

/////////////////////////////////
// stdio.h
/////////////////////////////////

void crt_clearerr(CPU* cpu);
void crt_fclose(CPU* cpu);
void crt__fdopen(CPU* cpu);
void crt_feof(CPU* cpu);
void crt_ferror(CPU* cpu);
void crt_fflush(CPU* cpu);
void crt_fgetc(CPU* cpu);
void crt_fgetpos(CPU* cpu);
void crt_fgets(CPU* cpu);
void crt__fileno(CPU* cpu);
void crt_fopen(CPU* cpu);
void crt_fprintf(CPU* cpu);
void crt_fputc(CPU* cpu);
void crt_fputs(CPU* cpu);
void crt_fread(CPU* cpu);
void crt_freopen(CPU* cpu);
void crt_fscanf(CPU* cpu);
void crt_fseek(CPU* cpu);
void crt_fsetpos(CPU* cpu);
void crt_ftell(CPU* cpu);
void crt_fwrite(CPU* cpu);
void crt_getc(CPU* cpu);
void crt_getchar(CPU* cpu);
void crt_gets(CPU* cpu);
void crt__getw(CPU* cpu);
void crt__pclose(CPU* cpu);
void crt_perror(CPU* cpu);
void crt__popen(CPU* cpu);
void crt_printf(CPU* cpu);
void crt_putc(CPU* cpu);
void crt_putchar(CPU* cpu);
void crt_puts(CPU* cpu);
void crt__putw(CPU* cpu);
void crt_remove(CPU* cpu);
void crt_rename(CPU* cpu);
void crt_rewind(CPU* cpu);
void crt_scanf(CPU* cpu);
void crt_setbuf(CPU* cpu);
void crt_setvbuf(CPU* cpu);
void crt__snprintf(CPU* cpu);
void crt_sprintf(CPU* cpu);
void crt_sscanf(CPU* cpu);
void crt__tempnam(CPU* cpu);
void crt_tmpfile(CPU* cpu);
void crt_tmpnam(CPU* cpu);
void crt_ungetc(CPU* cpu);
void crt_vfprintf(CPU* cpu);
void crt_vprintf(CPU* cpu);
void crt_vsnprintf(CPU* cpu);
void crt_vsprintf(CPU* cpu);

/////////////////////////////////
// stdlib.h
/////////////////////////////////

void crt_abort(CPU* cpu);
void crt_abs(CPU* cpu);
void crt_atexit(CPU* cpu);
void crt_atof(CPU* cpu);
void crt_atoi(CPU* cpu);
void crt_atol(CPU* cpu);
void crt__atoi64(CPU* cpu);// atoll
void crt_bsearch(CPU* cpu);
void crt_calloc(CPU* cpu);
void crt_div(CPU* cpu);
void crt__ecvt(CPU* cpu);
void crt_exit(CPU* cpu);
void crt__fcvt(CPU* cpu);
void crt_free(CPU* cpu);
void crt__gcvt(CPU* cpu);
void crt_getenv(CPU* cpu);
void crt__itoa(CPU* cpu);
void crt_labs(CPU* cpu);
void crt_ldiv(CPU* cpu);
void crt__abs64(CPU* cpu);// llabs
void crt_malloc(CPU* cpu);
void crt_mblen(CPU* cpu);
void crt_mbstowcs(CPU* cpu);
void crt_mbtowc(CPU* cpu);
void crt__putenv(CPU* cpu);
void crt_qsort(CPU* cpu);
void crt_rand(CPU* cpu);
void crt_realloc(CPU* cpu);
void crt_srand(CPU* cpu);
void crt_strtod(CPU* cpu);
void crt_strtol(CPU* cpu);
void crt_strtoul(CPU* cpu);
void crt__strtoi64(CPU* cpu);// strtoll
void crt__strtoui64(CPU* cpu);// strtoull
void crt_wcstombs(CPU* cpu);
void crt_wctomb(CPU* cpu);

/////////////////////////////////
// string.h
/////////////////////////////////

void crt__memccpy(CPU* cpu);// memccpy
void crt_memchr(CPU* cpu);
void crt_memcmp(CPU* cpu);
void crt_memcpy(CPU* cpu);
void crt_memmove(CPU* cpu);
void crt_memset(CPU* cpu);
void crt_strcat(CPU* cpu);
void crt_strchr(CPU* cpu);
void crt_strcmp(CPU* cpu);
void crt_strcoll(CPU* cpu);
void crt_strcpy(CPU* cpu);
void crt_strcspn(CPU* cpu);
void crt__strdup(CPU* cpu);// strdup
void crt_strerror(CPU* cpu);
void crt__stricmp(CPU* cpu);
void crt_strlen(CPU* cpu);
void crt__strlwr(CPU* cpu);
void crt_strncat(CPU* cpu);
void crt_strncmp(CPU* cpu);
void crt_strncpy(CPU* cpu);
void crt__strnicmp(CPU* cpu);
void crt_strnlen(CPU* cpu);
void crt_strpbrk(CPU* cpu);
void crt_strrchr(CPU* cpu);
void crt_strspn(CPU* cpu);
void crt_strstr(CPU* cpu);
void crt_strtok(CPU* cpu);
void crt_strxfrm(CPU* cpu);

/////////////////////////////////
// time.h - TODO: More functions
/////////////////////////////////

void crt_clock(CPU* cpu);

/////////////////////////////////
// wctype.h / wchar.h
/////////////////////////////////

void crt_iswalpha(CPU* cpu);
void crt_iswupper(CPU* cpu);
void crt_iswlower(CPU* cpu);
void crt_iswdigit(CPU* cpu);
void crt_iswxdigit(CPU* cpu);
void crt_iswspace(CPU* cpu);
void crt_iswpunct(CPU* cpu);
void crt_iswalnum(CPU* cpu);
void crt_iswprint(CPU* cpu);
void crt_iswgraph(CPU* cpu);
void crt_iswcntrl(CPU* cpu);
void crt_iswascii(CPU* cpu);
void crt_isleadbyte(CPU* cpu);
void crt_towupper(CPU* cpu);
void crt_towlower(CPU* cpu);
void crt_iswctype(CPU* cpu);
void crt_is_wctype(CPU* cpu);

/////////////////////////////////
// kernel32.dll
/////////////////////////////////

void crt_GetModuleFileNameA(CPU* cpu);
void crt_GetSystemDirectoryA(CPU* cpu);

/////////////////////////////////
// winsock.h
/////////////////////////////////

void crt_accept(CPU* cpu);
void crt_bind(CPU* cpu);
void crt_closesocket(CPU* cpu);
void crt_connect(CPU* cpu);
void crt_ioctlsocket(CPU* cpu);
void crt_getpeername(CPU* cpu);
void crt_getsockname(CPU* cpu);
void crt_getsockopt(CPU* cpu);
void crt_htonl(CPU* cpu);
void crt_htons(CPU* cpu);
void crt_inet_addr(CPU* cpu);
void crt_inet_ntoa(CPU* cpu);
void crt_listen(CPU* cpu);
void crt_ntohl(CPU* cpu);
void crt_ntohs(CPU* cpu);
void crt_recv(CPU* cpu);
void crt_recvfrom(CPU* cpu);
void crt_select(CPU* cpu);
void crt_send(CPU* cpu);
void crt_sendto(CPU* cpu);
void crt_setsockopt(CPU* cpu);
void crt_shutdown(CPU* cpu);
void crt_socket(CPU* cpu);
void crt_gethostbyaddr(CPU* cpu);
void crt_gethostbyname(CPU* cpu);
void crt_gethostname(CPU* cpu);
void crt_getservbyport(CPU* cpu);
void crt_getservbyname(CPU* cpu);
void crt_getprotobynumber(CPU* cpu);
void crt_getprotobyname(CPU* cpu);
void crt_WSAStartup(CPU* cpu);
void crt_WSACleanup(CPU* cpu);
void crt_WSASetLastError(CPU* cpu);
void crt_WSAGetLastError(CPU* cpu);

#endif