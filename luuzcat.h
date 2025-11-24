/* by pts@fazekas.hu at Thu Oct  2 22:32:09 CEST 2025 */

#ifndef _LUUZCAT_H
#define _LUUZCAT_H

#ifdef USE_DEBUG
#  define USE_CHECK 1
#endif

#if defined(__i386) || defined(__i386__) || defined(__amd64__) || defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64) || defined(__386) || \
    defined(__X86_64__) || defined(_M_I386) || defined(_M_I86) || defined(_M_X64) || defined(_M_AMD64) || defined(_M_IX86) || defined(__386__) || \
    defined(__X86__) || defined(__I86__) || defined(_M_I86) || defined(_M_I8086) || defined(_M_I286) || \
    defined(MSDOS) || defined(_MSDOS) || defined(_WIN32) || defined(_WIN64) || defined(__DOS__) || defined(__COM__) || defined(__NT__) || \
    defined(__MSDOS__) || defined(__OS2__)
#  define IS_X86 1
#  define IS_UNALIGNED_LE 1  /* There is unaligned little-endian access of 32-bit integers. */
#else
#  if (defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__)) && defined(__ARM_FEATURE_UNALIGNED) || \
       ((defined(__LITTLE_ENDIAN__) || (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || \
         defined(__LITTLE_ENDIAN) || defined(_LITTLE_ENDIAN)) && \
         (defined(__arm__) || defined(__aarch64__) || defined(__powerpc__) || defined(_M_PPC) || defined(_ARCH_PPC) || defined(__PPC__) || \
          defined(__PPC) || defined(PPC) || defined(__powerpc) || defined(powerpc)))
#    define IS_UNALIGNED_LE 1
#  endif
#endif

#if (!defined(__i386) && !defined(__i386__) && !defined(__amd64__) && !defined(__x86_64__) && !defined(_M_X64) && !defined(_M_AMD64) && !defined(__386) && \
     !defined(__X86_64__) && !defined(_M_I386) && !defined(_M_X64) && !defined(_M_AMD64) && !defined(__386__)) && \
    ((!defined(__WATCOMC__) && (defined(_M_I86) || defined(_M_IX86) || defined(__X86__) || defined(__I86__) || defined(_M_I8086) || defined(_M_I286) || defined(__TURBOC__)) && \
      (defined(MSDOS) || defined(_MSDOS) || defined(__DOS__) || defined(__COM__) || defined(__MSDOS__))) || \
     (defined(__WATCOMC__) && defined(_M_I86)))
#  define IS_X86_16 1
#endif

#if IS_X86_16 && (defined(MSDOS) || defined(_MSDOS) || defined(__DOS__) || defined(__COM__) || defined(__MSDOS__) || defined(__TURBOC__))
#  define IS_DOS_16 1
#endif

#if defined(_PROGX86) && !defined(LIBC_PREINCLUDED)
  /* Expecting an external tiny libc for OpenWatcom C compiler creating
   * system-independent i386 code.
   */
#  ifndef __WATCOMC__
#    error OpenWatcom C compiler needed by _PROGX86.
#  endif
#  if !defined(__386__) && !IS_X86_16
#    error x86 CPU (16-bit or 32-bit) is needed by _PROGX86.
#  endif
#  define LIBC_PREINCLUDED 1
#  if IS_X86_16
#    define __PROGX86_IO1_CALLCONV __watcall
#  else
#    define __PROGX86_IO1_CALLCONV __cdecl
#  endif
#  define LIBC_HAVE_WRITE_NONZERO 1
  int __PROGX86_IO1_CALLCONV write_binary(int fd, const void *buf, unsigned count);
  int __PROGX86_IO1_CALLCONV write_nonzero_binary(int fd, const void *buf, unsigned count);
#  if defined(_PROGX86_WRITEDOSREG) && IS_X86_16  /* _PROGX86_READDOSREG and _PROGX86_WRITEDOSREG together would make luuzcat.com 3 bytes longer. */
#    pragma aux write_nonzero_binary "write_nonzero_binary_DOSREG" __parm [__bx] [__dx] [__cx] __value [__ax] __modify __exact []
#  endif
#  ifdef _PROGX86_ONLY_BINARY
#    define write(fd, buf, count) write_binary(fd, buf, count)
#    define write_nonzero(fd, buf, count) write_nonzero_binary(fd, buf, count)
#  else
#    define O_BINARY 4  /* Any power of 2 at least 4 is fine. */
    int setmode(int fd, unsigned char mode);
    int __PROGX86_IO1_CALLCONV write(int fd, const void *buf, unsigned count);
    int __PROGX86_IO1_CALLCONV write_nonzero(int fd, const void *buf, unsigned count);
#  endif
  int __PROGX86_IO1_CALLCONV read(int fd, void *buf, unsigned int count);
#  if defined(_PROGX86_READDOSREG) && IS_X86_16  /* _PROGX86_READDOSREG and _PROGX86_WRITEDOSREG together would make luuzcat.com 3 bytes longer. */
#    pragma aux read "read_DOSREG" __parm [__bx] [__dx] [__cx] __value [__ax] __modify __exact []
#  endif
  int isatty(int fd);
#  if defined(_PROGX86_ISATTYDOSREG) && IS_X86_16  /* _ISATTY_READDOSREG makes luuzcat.com 4 bytes shorter, so it's beneficial to enable it. */
#    pragma aux isatty "isatty_DOSREG" __parm [__bx] __value [__ax] __modify __exact []
#  endif
#  if IS_X86_16 && defined(_PROGX86_DOSEXIT)  /* Shorter for luuzcat. */
    __declspec(aborts) void __libc_exit_low(unsigned int exit_code_plus_0x4c00);
#    pragma aux __libc_exit_low = "int 21h" __parm [__ax] __modify __exact []
#    define exit(exit_code) __libc_exit_low(0x4c00U + (unsigned char)(exit_code))
#  else
    __declspec(aborts) void exit(unsigned char exit_code);
#  endif
#  if IS_X86_16 && defined(__MINIX__)
    int __PROGX86_IO1_CALLCONV fork(void);
    int __PROGX86_IO1_CALLCONV close(int fd);
    int __PROGX86_IO1_CALLCONV dup(int fd);
    int __PROGX86_IO1_CALLCONV pipe(int pipefd[2]);
#  endif
  unsigned int strlen(const char *s);
  /* To prevent wlink: Error! E2028: strlen_ is an undefined reference */
  void *memset(void *s, int c, unsigned int n);
  void *memcpy(void *dest, const void *src, unsigned n);
  void *memcpy_backward(void *dest, const void *src, unsigned n);  /* Not a standard C function. */
  void *memmove(void *dest, const void *src, unsigned n);
#  if IS_X86_16
#    if !(defined(__SMALL__) || defined(__MEDIUM__))  /* This works in any memory model. */
#      pragma intrinsic(strlen)  /* This generates shorter code than the libc implementation. */
#    else
#      pragma aux strlen = "push ds"  "pop es"  "mov cx, -1"  "xor ax, ax"  "repne scasb"  "not cx"  "dec cx"  __parm [__di] __value [__cx] __modify __exact [__es __di __ax]  /* 2 bytes shorter than the `#pragma intrinsic', because `push ds ++ pop es' is shorter than 2 movs. */
#    endif
#    pragma intrinsic(memset)  /* This generates shorter code than the libc implementation. */
#    pragma intrinsic(memcpy)  /* This generates shorter code than the libc implementation. */
#    pragma intrinsic(memmove)  /* No effect, __WATCOMC__ can't generate memmove(3) code as intrinsic. */
    /* It must not modify `bp', otherwise the OpenWatcom C compiler omits the
     * `pop bp' between the `mov sp, bp' and the `ret'.
     */
#    ifdef _PROGX86_DOSPSPARGV
      char *main0_argv1(void);
#      pragma aux main __modify [__ax __bx __cx __dx __si __di __es]
#      pragma aux main0_argv1 = "mov si, 81h"  "next:"  "lodsb"  "cmp al, 32" \
          "je next"  "cmp al, 9"  "je next"  "dec si" \
          __value [__si] __modify __exact [__si __al]  /* This is correct for a DOS .com program, and a PSP-based, short-model DOS .exe program (rare). */
#      define main0() void __watcall main(void)
#      define main0_exit0() do {} while (0)
#      define main0_exit(exit_code) _exit(exit_code)
#      define main0_is_progarg_null(arg) 0  /* Size optimization, never NULL. */
#      pragma extref "progx86_main_returns_void";
#    else
#      pragma aux main __value [__ax] __modify [__bx __cx __dx __si __di __es]
#    endif
#  endif
#  if IS_DOS_16
    /* Allocates para_count << 4 bytes of memory, aligned to 16 (paragraph)
     * boundary, and returns the segment register value pointing to the
     * beginning of it. Returns 0 on out-of-memory.
     */
    unsigned __watcall progx86_para_alloc(unsigned para_count);
#    ifdef _PROGX86_REUSE
      /* Finds para_count << 4 bytes of memory, aligned to 16 (paragraph)
       * boundary, and returns the segment register value pointing to the
       * beginning of it. Returns 0 on out-of-memory. Subsequent calls will
       * return the same address (so the memory is reused), also the same
       * address as progx86_para_alloc(...) returns first. So don't use this
       * function if you need memory which no unrelated program code is allowed
       * to overwrite.
       *
       * For most uses, progx86_para_alloc(...) is safer, but
       * progx86_para_reuse(...) is 6 bytes shorter.
       */
      unsigned __watcall progx86_para_reuse(unsigned para_count);
#      ifdef _PROGX86_DOSMEM  /* Shorter implementations of progx86_para_reuse(...), and even shorter implementation: progx86_is_para_less_than(...)+progx86_para_reuse_start(). */
#        pragma aux progx86_para_reuse = "mov ax, cs"  "add ax, 1000h"  "add dx, ax"  "jc short oom"  "cmp dx, word ptr ds:[2]"  "ja short oom" \
            "db 0xa9"  /* Skips over the `xor ax, ax' below. */  \
            "oom: xor ax, ax"  __parm [__dx] __value [__ax] __modify __exact [__dx]  /* 8 bytes shorter than the libc function. */
#        define _PROGX86_HAVE_PARA_REUSE_START 1
#        define __LIBC_PROG_PARA_COUNT 0x1000U  /* 64 KiB == 0x1000 16-byte paragraphs. */
#        define __libc_get_prog_mem_end_seg() (*(const unsigned*)2)  /* Fetch it from the DOS Program Segment Prefix (PSP): https://fd.lod.bz/rbil/interrup/dos_kernel/2126.html */
        unsigned __libc_get_psp_seg(void);
#        pragma aux __libc_get_psp_seg = "mov ax, cs" __value [__ax] __modify []  /* This is correct for a DOS .com program, but not for an .exe. */
#        define progx86_is_para_less_than(para_count) ((para_count) + __LIBC_PROG_PARA_COUNT > __libc_get_prog_mem_end_seg() - __libc_get_psp_seg())  /* Thus works only for para_count < 0xf000. */
#        define progx86_para_reuse_start() (__libc_get_psp_seg() + __LIBC_PROG_PARA_COUNT)
#      endif
#      define progx86_para_reuse_or_alloc(para_count) progx86_para_reuse(para_count)
#    else
#      define progx86_para_reuse_or_alloc(para_count) progx86_para_alloc(para_count)
#    endif
#  endif
#  ifdef __386__
    /* It must not modify `bp', otherwise the OpenWatcom C compiler omits the
     * `pop bp' between the `mov sp, bp' and the `ret'.
     */
#    pragma aux main __value [__eax] __modify [__ebx __ecx __edx __esi __edi]
#  endif
#  ifdef _PROGX86_CRLF
#    define LUUZCAT_NL "\r\n"
#  else
#    define LUUZCAT_NL "\n"  /* Line ending for error messages on stderr. */
#  endif
#endif

#if defined(_DOSCOMSTART) && !defined(LIBC_PREINCLUDED)
  /* Tiny libc for OpenWatcom C compiler creating a DOS 8086 .com program.
   * For compilation instructions, see _DOSCOMSTART above.
   */
#  ifndef __WATCOMC__
#    error OpenWatcom C compiler needed by _DOSCOMSTART.
#  endif
#  ifndef IS_X86_16
#    error 8086 CPU is needed by _DOSCOMSTART.  /* And the IS_X86_16 macro must work correctly. */
#  endif
#  ifndef __SMALL__
#    error The small memory model is needed by _DOSCOMSTART.
#  endif
#  define LIBC_PREINCLUDED 1
  /* This must be put to a separate function than _comstart, otherwise wlink
   * wouldn't remove these 0x100 bytes from the beginning of the DOS .com
   * program.
   */
#  ifdef LUUZCAT_MAIN
    __declspec(naked) void __watcall _com_nuls(void) {
        __asm { db 100h dup (?) } }

    extern char _edata[], _end[];
    extern void __watcall main0(void);
    /* It must not modify `bp', otherwise the OpenWatcom C compiler omits the
     * `pop bp' between the `mov sp, bp' and the `ret'.
     */
#    pragma aux main0 __modify [__ax __bx __cx __dx __si __di __es]

    /* This entry point function must be the first function defined in the .c
     * file, because the entry point of DOS .com programs is at the beginning.
     *
     * This function zero-initializes _BSS, then jumps to main0. When main0
     * returns, it will do a successful exit(0). If _BSS is missing (no matter
     * how long _DATA is), then both _end and _edata are NULL here, but that's
     * fine here.
     */
    __declspec(naked) void __watcall _comstart(void) {
        __asm { xor ax, ax }
        __asm { mov di, offset _edata }
#    ifdef _DOSCOMSTART_DISTART
          __asm { mov ax, offset _end }  /* This code is deliberately incorrect (it should be `mov cx, ...'). It's size optimization, because `mov cx, _end-_edata' doesn't work here. shorten_to_bss.pl will fix it. */
#    else
          __asm { mov cx, offset _end } __asm { sub cx, di }
#    endif
        __asm { rep stosb } __asm { jmp main0 } }

    /* At the end, the `ret' instruction will exit(0) the DOS .com program. */
#    define main0() void __watcall main0(void)
#    define main0_exit0() do {} while (0)
#    define main0_exit(exit_code) _exit(exit_code)
#    define main0_is_progarg_null(arg) 0  /* Size optimization, never NULL. */
#  endif


#  define LIBC_HAVE_WRITE_NONZERO_VOID 1
  /* Like write(...), but count must be nonzero, and it returns void. */
  static void write_nonzero_void(int fd, const void *buf, unsigned int count);
#  pragma aux write_nonzero_void = "mov ah, 40h"  "int 21h" \
      __parm [__bx] [__dx] [__cx] __modify [__ax]

#  define LIBC_HAVE_WRITE_NONZERO 1
  /* Like write(...), but count must be nonzero. */
  int write_nonzero(int fd, const void *buf, unsigned int count);
#  pragma aux write_nonzero = "mov ah, 40h"  "int 21h"  "jnc ok"  \
      "sbb ax, ax"  "ok:"  \
      __value [__ax] __parm [__bx] [__dx] [__cx] __modify __exact [__ax]

  int write(int fd, const void *buf, unsigned count);
#  pragma aux write = "xor ax, ax"  "jcxz ok"  "mov ah, 40h"  "int 21h" \
      "jnc ok"  "sbb ax, ax"  "ok:"  \
      __value [__ax] __parm [__bx] [__dx] [__cx] __modify __exact [__ax]

  int read(int fd, void *buf, unsigned int count);
#  pragma aux read = "mov ah, 3fh"  "int 21h"  "jnc ok"  "sbb ax, ax"  "ok:" \
      __value [__ax] __parm [__bx] [__dx] [__cx] __modify __exact [__ax]

  /* Just returns whether fd is a character device. (On Unix, there are
   * non-TTY character devices, but this implementation pretends that they
   * don't exist.) It works correctly on
   * [kvikdos](https://github.com/pts/kvikdos), emu2 and (windowed) DOSBox.
   * OpenWatcom lib does the same (
   * https://github.com/open-watcom/open-watcom-v2/blob/2d1ea451c2dbde4f1efd26e14c6dea3b15a1b42b/bld/clib/environ/c/isatt.c#L58C10-L59
   * ).
   */
  int isatty(int fd);
#  pragma aux isatty = "mov ax, 4400h"  "int 21h"  "mov ax, 0"  "jc done"  \
      "test dl, dl"  "jns done"  "inc ax"  "done:" \
      __value [__ax] __parm [__bx] __modify __exact [__ax __bx __dx]

#  if 1  /* Shorter for luuzcat. */
    __declspec(aborts) void __libc_exit_low(unsigned int exit_code_plus_0x4c00);
#    pragma aux __libc_exit_low = "int 21h" __parm [__ax] __modify __exact []
#    define exit(exit_code) __libc_exit_low(0x4c00U + (unsigned char)(exit_code))
#  else
    __declspec(aborts) void exit(unsigned char exit_code);
#    pragma aux exit = "mov ah, 4ch"  "int 21h" __parm [__al] __modify __exact []
#  endif

  char *main0_argv1(void);
#  pragma aux main0_argv1 = "mov si, 81h"  "next:"  "lodsb"  "cmp al, 32" \
      "je next"  "cmp al, 9"  "je next"  "dec si" \
      __value [__si] __modify __exact [__si __al]  /* This is correct for a DOS .com program, but not for an .exe. */

  unsigned int strlen(const char *s);
  void *memset(void *s, int c, unsigned int n);  /* Not used in luuzcat with IS_X86_16, memset_void(...) is used instead. */
  void *memcpy(void *dest, const void *src, unsigned n);  /* Not used in luuzcat with IS_X86_16. */
  /* To prevent wlink: Error! E2028: strlen_ is an undefined reference */
#  if !(defined(__SMALL__) || defined(__MEDIUM__))  /* This works in any memory model. */
#    pragma intrinsic(strlen)
#  else
#    pragma aux strlen = "push ds"  "pop es"  "mov cx, -1"  "xor ax, ax"  "repne scasb"  "not cx"  "dec cx"  __parm [__di] __value [__cx] __modify __exact [__es __di __ax]  /* 2 bytes shorter than the `#pragma intrinsic', because `push ds ++ pop es' is shorter than 2 movs. */
#  endif
#  if !(defined(__SMALL__) || defined(__MEDIUM__))  /* This works in any memory model. */
#    pragma intrinsic(memset)  /* Not used in luuzcat with IS_X86_16, memset_void(...) is used instead. */
#  else
#    pragma aux memset = "push ds"  "pop es"  "push di"  "rep stosb"  "pop di" __parm [__di] [__al] [__cx] __value [__di] __modify __exact [__es __cx]  /* Shorter than the `#pragma intrinsic', because `push ds ++ pop es' is shorter than 2 movs. */
#  endif
#  pragma intrinsic(memcpy)  /* Not used in luuzcat with IS_X86_16. */

  /* Finds para_count << 4 bytes of memory, aligned to 16 (paragraph)
   * boundary, and returns the segment register value pointing to the
   * beginning of it. Returns 0 on out-of-memory. Subsequent calls will
   * return the same address (so the memory is reused), also the same
   * address as progx86_para_alloc(...) returns first. So don't use this
   * function if you need memory which no unrelated program code is allowed
   * to overwrite.
   *
   * For most uses, progx86_para_alloc(...) is safer, but
   * progx86_para_reuse(...) is 6 bytes shorter.
   */
  unsigned para_reuse(unsigned para_count);
#  pragma aux para_reuse = "mov ax, cs"  "add ax, 1000h"  "add dx, ax"  "jc short oom"  "cmp dx, word ptr ds:[2]"  "ja short oom" \
      "db 0xa9"  /* Skips over the `xor ax, ax' below. */  \
      "oom: xor ax, ax"  __parm [__dx] __value [__ax] __modify __exact [__dx]  /* 8 bytes shorter than the libc function. */
#  define __LIBC_PROG_PARA_COUNT 0x1000U  /* 64 KiB == 0x1000 16-byte paragraphs. */
#  define __libc_get_prog_mem_end_seg() (*(const unsigned*)2)  /* Fetch it from the DOS Program Segment Prefix (PSP): https://fd.lod.bz/rbil/interrup/dos_kernel/2126.html */
  unsigned __libc_get_psp_seg(void);
#  pragma aux __libc_get_psp_seg = "mov ax, cs" __value [__ax] __modify []  /* This is correct for a DOS .com program, but not for an .exe. */
#  define is_para_less_than(para_count) ((para_count) + __LIBC_PROG_PARA_COUNT > __libc_get_prog_mem_end_seg() - __libc_get_psp_seg())  /* Thus works only for para_count < 0xf000. */
#  define para_reuse_start() (__libc_get_psp_seg() + __LIBC_PROG_PARA_COUNT)

#  define LUUZCAT_NL "\r\n"  /* Line ending for error messages on stderr. */
#endif

#ifndef LIBC_PREINCLUDED
#  ifdef __MMLIBC386__
#    include <mmlibc386.h>
#    define LUUZCAT_NL "\r\n"  /* We want \r\n, and write(STDERR_FILENO, ..., ...) doesn't convert "\n" to "\r\n". */
#  else
#    include <stdlib.h>  /* exit(...), abort(...). */
#    include <string.h>
#    include <limits.h>   /* INT_MAX. */
    /* Turbo C++ 1.01 defines __MSDOS__. OpenWatcom C compiler defines __DOS__, __COM__, __NT__ or __OS2__. */
#    if defined(MSDOS) || defined(_MSDOS) || defined(_WIN32) || defined(_WIN64) || defined(__DOS__) || defined(__COM__) || defined(__NT__) || defined(__MSDOS__) || defined(__OS2__)
#      include <fcntl.h>  /* O_BINARY. */
#      include <io.h>  /* read(...), write(...), setmode(...), isatty(0). */
#    else
#      include <unistd.h>  /* read(...), write(...), isatty(...). */
#    endif
#    ifdef USE_DEBUG
#      include <stdio.h>  /* fprintf(stderr, ...); */
#    endif
#    define LUUZCAT_NL "\n"  /* On DOS etc. write(STDERR_FILENO, ..., ...) will append an "\r" because the lack of O_BINARY. */
#  endif
#endif

#ifndef memset_void
#  if defined(__WATCOMC__) && defined(__386__)
    void *memset_void(void *s, char c, unsigned int n);
#    pragma aux memset_void = "rep stosb" __parm [__edi] [__al] [__ecx] __modify __exact [__edi __ecx]  /* Shorter than the `#pragma intrinsic', because `push di ++ ... ++ pop di' is omitted. */
#    define memset_void(s, c, n) memset_void(s, c, n)
#  else
#    if defined(__WATCOMC__) && IS_X86_16 && (defined(__SMALL__) || defined(__MEDIUM__))
      void *memset_void(void *s, char c, unsigned int n);
#      pragma aux memset_void = "push ds"  "pop es"  "rep stosb" __parm [__di] [__al] [__cx] __modify __exact [__es __di __cx]  /* Shorter than the `#pragma intrinsic', because `push ds ++ pop es' is shorter than 2 movs, and `push di ++ ... ++ pop di' is omitted. */
#      define memset_void(s, c, n) memset_void(s, c, n)
#    else
#      define memset_void(s, c, n) memset(s, c, n)
#    endif
#  endif
#endif

#if (INT_MAX >> 15 >> 15 || __INT_MAX__ >> 15 >> 15 || __SIZEOF_INT__ >= 4 || __INTSIZE >= 4 || defined(__LP64__) || defined(_LP64) \
     || defined(vax) || defined(sun) \
     || defined(_WIN32) || defined(_WIN64) || defined(__NT__) || defined(WIN32) || defined(__WIN32__) || defined(__DOS32__) \
     || defined(__i386) || defined(__i386__) || defined(__386) || defined(__386__) || defined(_M_I386) \
     || defined(__amd64__) || defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64) || defined(__X86_64__) || defined(__x86_64) || defined(_M_X64) \
     || defined(_M_ARM64) || defined(__AARCH64EB__) || defined(__AARCH64EL__) || defined(__aarch64__) \
     || defined(__ia64__) || defined(_M_IA64) || defined(__s390x__) || defined(__ppc64__) || defined(__PPC64__) \
     || defined(__riscv__) || defined(__mips__) \
    ) && !(defined(__WATCOMC__) && _M_I86)
typedef unsigned int um32;  /* Avoid using 8 bytes for um32. */
#else
typedef unsigned long um32;  /* At least 32 bits. */
#endif

#ifndef NULL
#  define NULL ((void*)0)
#endif

#ifndef   STDIN_FILENO
#  define STDIN_FILENO 0
#endif

#ifndef   STDOUT_FILENO
#  define STDOUT_FILENO 1
#endif

#ifndef   STDERR_FILENO
#  define STDERR_FILENO 2
#endif

#ifndef   EXIT_SUCCESS
#  define EXIT_SUCCESS 0
#endif

#ifndef   EXIT_FAILURE
#  define EXIT_FAILURE 1
#endif

#ifdef __TURBOC__  /* Turbo C++ 1.01 emits a subspicious pointer conversion warning without the (char*) cast. */
#  define WRITE_PTR_ARG_CAST(arg) ((char*)(arg))
#else
#  define WRITE_PTR_ARG_CAST(arg) ((const char*)(arg))
#endif

#ifdef __noreturn
#else
#  ifdef __GNUC__
#    define __noreturn  __attribute__((__noreturn__))
#  else
#    ifdef __WATCOMC__
      /* __declspec(noreturn) is similar but it generates longer code: it generates `push ...' instructions at the beginning of the function code. */
#      define __noreturn __declspec(aborts)
#    else
#      define __noreturn
#    endif
#  endif
#endif

#ifndef LIBC_HAVE_WRITE_NONZERO
#  define write_nonzero(fd, buf, count) write(fd, buf, count)
#endif

#ifndef LIBC_HAVE_WRITE_NONZERO_VOID
#  define write_nonzero_void(fd, buf, count) (void)!write_nonzero(fd, buf, count)
#endif

#if IS_DOS_16 && (defined(__WATCOMC__) || defined(__TURBOC__))
#  define LUUZCAT_MALLOC_OK 1  /* For uncompress.c. */
#endif

/* --- Typedefs etc. */

typedef unsigned char uc8;  /* Always a single byte. For interfacing with read(2) and write(2). */
typedef unsigned char um8;  /* Unsigned integer type which is at least 8 bits, preferably 8 bits. */
typedef unsigned char ub8;  /* Unsigned integer type which is at least 1 bit (boolean), preferably 8 bits. */
typedef unsigned short um16;  /* Unsigned integer type which is at least 16 bits, preferably 16 bits. Changing this to `unsigned long' would also work, but that would waste memory. */
typedef unsigned short us16;  /* Always 2 bytes. Code which needs it adds a static assert. */
typedef char assert_sizeof_uc8[sizeof(uc8) == 1 ? 1 : -1];  /* We also rely on the implicit `& 0xff' behavior of global_bitbuf16 for USE_TREE. */
typedef char assert_sizeof_um16[sizeof(um16) >= 2 ? 1 : -1];

/* --- Error reporting. */

#if defined(__WATCOMC__) && IS_X86_16
#  pragma aux fatal_msg __parm [__dx]  /* This is better argument register allocation than the default (__ax). */
#endif
__noreturn void fatal_msg(const char *msg);
__noreturn void fatal_read_error(void);
__noreturn void fatal_write_error(void);
__noreturn void fatal_unexpected_eof(void);
__noreturn void fatal_corrupted_input(void);
#ifdef LUUZCAT_MALLOC_OK
  __noreturn void fatal_out_of_memory(void);
#endif
__noreturn void fatal_unsupported_feature(void);

/* READ_BUFFER_SIZE is the number of bytes that should be read to
 * global_read_buffer at a time. It must be a power of 2 divisible by 0x1000
 * for disk block alignment.
 *
 * READ_BUFFER_SIZE and global_read_buffer are used by all decompressors
 * including decompress_compress_nohdr(...) with LUUZCAT_DUCML.
 *
 * Memory layout for LUUZCAT_DUCML:
 *
 * * -0x10...0: DOS Memory Control Block (MCB).
 * * 0...0x100: DOS Program Segment Prefix (PSP).
 * * 0x100..0x10d: _start, with BSS initialization (`rep stosw') and a `jmp strict near main_' at the end.
 * * 0x10d...~0x718: uncompress.c code (_TEXT): library code (fatal_msg(...), fatal_*(void), read_byte(...)), tab_init_noarg(...), decompress_compress_nohdr(...).
 * * For decompress_compress_nohdr(...) in uncompress.c with bits <= 14 only:
 *   * 0xc00..0x2c00:  uc8 uncompress_ducml_write_buffer_14[1U << 14]; 8 KiB.
 *   * 0x2c00..0x6c00: uc8  tab_suffix_14_ary[1U << 14]; 16 KiB.
 *   * 0x6c00..0xec00: um16 tab_prefix_14_ary[1U << 14]; 32 KiB. Separated to odd and even halves.
 * * For decompress_compress_nohdr(...) in uncompress.c with bits == 15 only:
 *   * 0xc00..0x6c00:  uc8 uncompress_ducml_write_buffer_15[0x6000]; 24 KiB.
 *   * 0x6c00..0xec00: uc8 tab_suffix_15_ary[1U << 15]; 32 KiB.
 *   * On the heap: um16 tab_prefix_15_ary[1U << 15]; 64 KiB. Separated to odd and even halves.
 * * For decompress_compress_nohdr(...) in uncompress.c with bits == 16 only:
 *   * 0xc00..0xec00: uc8 uncompress_ducml_write_buffer_16[0xe000]; 56 KiB.
 *   * On the heap: uc8  tab_suffix_16_ary[1U << 16]; 64 KiB.
 *   * On the heap: um16 tab_prefix_16_ary[1U << 16]; 128 KiB. Separated to odd and even halves.
 * * For all decompressors other than decompress_compress_nohdr(...):
 *   * ~0x718...0x2700: rest of the code (_TEXT): luuzcatc.c, unscolzh.c, uncompact.c, unopack.c, unpack.c, undeflate.c, unfreeze.c
 *   * ~0x2700...~0x27d0: string constants (CONST): error messages and the usage message; some of the error messages are inlined to _TEXT
 *   * ~0x27d0...~0x27d0 (empty): other global constants (CONST2): empty; only unfreeze.c had table1, which has been inlined to to _TEXT as table1_func.
 *   * ~0x27d0...~0x27d0 (empty): non-constant, initialized global variables (_DATA): empty
 *   * ~0x27d0...<0xec00: non-constant, non-constant, zero-initialized global variables (_BSS) except for global_write_buffer: global_read_buffer, big and others.
 *   * The code and data here overlaps with data of uncompress.c. This is fine, because decompress_compress_nohdr(...) doesn't return.
 * 0xec00...0xfc03: uc8 global_read_buffer[READ_BUFFER_SIZE + READ_BUFFER_EXTRA + READ_BUFFER_OVERSHOOT]; 0x1003 == 4099 bytes.
 * 0xfc03...0xfc04: ub8 global_read_had_eof;
 * 0xfc04...0xfc06: unsigned int global_insize;
 * 0xfc06...0xfc08: unsigned int global_inptr;
 * 0xfc08...0xfc0c: unsigned int global_total_read_size;
 * 0xfc0c...0xfc0e: __segment tab_suffix_seg;
 * 0xfc0e...0xfc10: __segment tab_prefix0_seg;
 * 0xfc10...0xfc12: __segment tab_prefix1_seg;
 * 0xfc12...0xfc14: unsigned int uncompress_ducml_write_buffer_size;
 * 0xfc14...0x10000: stack: 0xc0 bytes reserved for DOS etc. (https://retrocomputing.stackexchange.com/a/31813), 0x3ec bytes (plenty) can be used by the program.
 */
#ifdef LUUZCAT_DUCML
#  ifndef __WATCOMC__
#    error LUUZCAT_DUCML requires __WATCOMC__.
#  endif
#  if !IS_DOS_16
#    error LUUZCAT_DUCML requires DOS 8086 (16-bit).
#  endif
#  ifndef __SMALL__
#    error LUUZCAT_DUCML requires the __SMALL__ memory model.
#  endif
#  ifndef _PROGX86
#    error LUUZCAT_DUCML requires _PROGX86.
#  endif
#  ifndef _PROGX86_DOSMEM
#    error LUUZCAT_DUCML requires _PROGX86_DOSMEM.
#  endif
#  ifndef _PROGX86_HAVE_PARA_REUSE_START  /* Defined above. */
#    error LUUZCAT_DUCML requires _PROGX86_HAVE_PARA_REUSE_START.
#  endif
#  ifdef _DOSCOMSTART
#    error LUUZCAT_DUCML conflicts with _DOSCOMSTART.
#  endif
#  ifdef _DOSCOMSTART_UNCOMPRC
#    error LUUZCAT_DUCML conflicts with _DOSCOMSTART_UNCOMPRC.
#  endif
#  ifdef LUUZCAT_SMALLBUF
#    error LUUZCAT_DUCML conflicts with LUUZCAT_SMALLBUF.
#  endif
#  ifdef __TURBOC__
#  endif
#  ifdef READ_BUFFER_SIZE
#    error LUUZCAT_DUCML conflicts with custom READ_BUFFER_SIZE.
#  endif
#  define READ_BUFFER_SIZE 0x1000
#endif

#ifdef LUUZCAT_SMALLBUF  /* Optimize for 64 KiB data segment in Minix i86. */
#  ifdef LUUZCAT_DUCML
#    error LUUZCAT_SMALLBUF conflicts with LUUZCAT_SMALLBUF.
#  endif
#  define READ_BUFFER_SIZE  0x2000  /* Decreasing this to <=0x1c00 would only make Minix i86 .a_total 576 bytes smaller. The difference may be important for LZW decompression (decompress_compress_with_fork(...) with maxbits == 16 on ELKS 0.4.0. */
#  define WRITE_BUFFER_SIZE 0x8000  /* >=0x8000 is required by decompress_deflate(). */
#endif

/* --- Generic helpers. */

/* i may be a char, short, int or long, or the signed or unsigned variants of these.
 * __GCC__ (GCC >=4.8, Clang), __WATCOMC__ (OpenWatcom 2), __TURBOC__ (Turbo C++ >=1.00) are all smart enough to optimize this to a `< 0' check.
 * OpenWatcom 2 C compiler generates suboptimal code for `(i & 0x80000000U) != 0' on i386. We avoid that here by doing `i < 0' instead.
 */
#define is_high_bit_set(i) ((int)~0U != -1 ? ((i) & (1UL << (sizeof(i) * 8 - 1))) != 0 :  /* Fallback for not 2s complement signed integers. */ \
    sizeof(i) == sizeof(char) ? (signed char)(i) < 0 : sizeof(i) == sizeof(short) ? (short)(i) < 0 : sizeof(i) == sizeof(int) ? (int)(i) < 0 : (long)((i) + 0L) < 0)

/* --- Reading. */

#ifndef READ_BUFFER_SIZE
#  define READ_BUFFER_SIZE 0x2000
#endif

/* We allocate a few more bytes (READ_BUFFER_EXTRA bytes) of read buffer so
 * that compress (LZW) decompression can always read READ_BUFFER_SIZE bytes
 * at a time (even if it doesn't read to the start of the buffer), so it
 * remains on disk block boundary.
 */
#define READ_BUFFER_EXTRA 2
/* We allocate even more bytes (READ_BUFFER_OVERSHOOT bytes) of read buffer
 * so that reading a few more bytes using `((int*)p)[0]' wouldn't be an
 * out-of-bounds array access.
 */
#define READ_BUFFER_OVERSHOOT (sizeof(unsigned int) > 2 ? sizeof(unsigned int) - 2 : 1)
/* uc8 global_read_buffer[READ_BUFFER_SIZE + READ_BUFFER_EXTRA + READ_BUFFER_OVERSHOOT]; */
#ifdef LUUZCAT_DUCML
#  if 0  /* #if and #else are equivalent, but one of them produces a smaller program file because of different code generation in the OpenWatcom C compiler. */
#    define global_read_buffer ((uc8*)0xec00U)
#  else
    extern uc8 global_read_buffer[];
#    pragma aux global_read_buffer "global_read_buffer__FIXOFS_0xec00"  /* __FIXOFS_... is processed by wasm2nasm.pl. */
#  endif
#  if 0
#    define global_read_had_eof (*(ub8*)0xfc03U)
#  else
    extern ub8 global_read_had_eof;
#    pragma aux global_read_had_eof "global_read_had_eof__FIXOFS_0xfc03"
#  endif
#  if 0
#    define global_insize (*(unsigned int*)0xfc04U)
#  else
    extern unsigned int global_insize;
#    pragma aux global_insize "global_insize__FIXOFS_0xfc04"
#  endif
#  if 0
#    define global_inptr (*(unsigned int*)0xfc06U)
#  else
    extern unsigned int global_inptr;
#    pragma aux global_inptr "global_inptr__FIXOFS_0xfc06"
#  endif
#  if 0
#    define global_total_read_size (*(um32*)0xfc08U)
#  else
    extern um32 global_total_read_size;
#    pragma aux global_total_read_size "global_total_read_size__FIXOFS_0xfc08"
#  endif
#else
#  ifndef LUUZCAT_SMALLBUF
    extern uc8 global_read_buffer[];
#  endif
  extern ub8 global_read_had_eof;  /* Was there an EOF already when reading? */
  extern unsigned int global_insize; /* Number of valid bytes in global_read_buffer. */
  extern unsigned int global_inptr;  /* Index of next byte to be processed in global_read_buffer. */
  extern um32 global_total_read_size;  /* read_byte(...) increses it after each read from the filehandle to global_read_buffer. */
#endif

#define BEOF (-1U)  /* Returned by read_byte(1) and try_byte(). */

unsigned int read_byte(ub8 is_eof_ok);
void read_force_eof(void);
unsigned int get_le16(void);

/* These are fast wrappers around read_byte(...) for speed. */
#define get_byte() (global_inptr < global_insize ? global_read_buffer[global_inptr++] : (uc8)read_byte(0))  /* Returns uc8. Fails with a fatal error on EOF. */
#define try_byte() (global_inptr < global_insize ? (unsigned int)global_read_buffer[global_inptr++] : read_byte(1))  /* Returns unsigned int: either 0..255 for a byte or BEOF == (-1U) to indicate EOF. */

/* --- Writing. */

/* WRITE_BUFFER_SIZE and global_write_buffer are used by all decompressors
 * except for decompress_compress_nohdr(...) with LUUZCAT_DUCML.
 */
#ifndef WRITE_BUFFER_SIZE
#  define WRITE_BUFFER_SIZE 0x8000U
#endif

#ifdef LUUZCAT_SMALLBUF
#  define global_read_buffer  big.deflate.read_buffer
#  define global_write_buffer big.deflate.write_buffer
#  define RW_BUFFER_DEF uc8 write_buffer[WRITE_BUFFER_SIZE]; uc8 read_buffer[READ_BUFFER_SIZE + READ_BUFFER_EXTRA + READ_BUFFER_OVERSHOOT + (-(READ_BUFFER_SIZE + READ_BUFFER_EXTRA + READ_BUFFER_OVERSHOOT) & 3)];
  extern uc8 *global_write_buffer_to_flush;
#else
  extern uc8 global_read_buffer[];
  extern uc8 global_write_buffer[];
#  define RW_BUFFER_DEF
#endif

unsigned int flush_write_buffer(unsigned int size);

#define flush_full_write_buffer_in_the_beginning() do {} while (0)  /* Usually this is a no-op, because the write buffer is not full in the beginning. */

/* --- Decompression. */

/* Value is this because build_crc32_table_if_needed(...) checks crc32_table[1] != 0. */
#define CRC32_TABLE_DUMMY_SIZE (sizeof(um32) << 1)

#if IS_X86_16
#  define CRC32_TABLE_DEF um32 crc32_table[256];  /* Replaces CRC32_TABLE_DUMMY. */
#  define CRC32_TABLE_DUMMY uc8 crc32_table_dummy[CRC32_TABLE_DUMMY_SIZE];
#  define invalidate_deflate_crc32_table() do { ((um8*)big.deflate.crc32_table)[sizeof(um32)] = 0; } while (0)  /* This makes `(um8)crc32_table[1] != 0' false in build_crc32_table_if_needed(...). */
#else
#  define CRC32_TABLE_DEF
#  define CRC32_TABLE_DUMMY
#  define invalidate_deflate_crc32_table() do {} while (0)
#endif

#define SCOLZH_NC (255 + 1 + 256 + 2 - 3)
#define SCOLZH_NPT 19

/* We specify this struct in the .h file so what we overlap it in memory with other big struct in `big' below. */
struct scolzh_big {
  RW_BUFFER_DEF CRC32_TABLE_DUMMY
  um16 left[ 2 * SCOLZH_NC - 1];
  um16 right[2 * SCOLZH_NC - 1];

  um8  c_len[SCOLZH_NC];
  um16 c_table[1U << 12];

  um8  pt_len[SCOLZH_NPT];  /* gzip-1.14/unlzh.c has pt_len[1 << TBIT] here, which is larger. It doesn't seem to be needed, there are checks below for indexing. */
  um16 pt_table[1U << 8];
};

#define COMPACT_NF 258

typedef um16 compact_dicti_t;
typedef um16 compact_diri_t;
typedef um16 compact_dictini_t;
typedef um8 compact_flags_t;

struct compact_fpoint {
  compact_dicti_t fpdni;  /* NULL is indicated by the value NFNULL == NF. */
  compact_flags_t flags;
};

struct compact_index {
  compact_dicti_t ipt;
  compact_diri_t nextri;
};

struct compact_son {
  compact_dictini_t spdii;  /* NFNULL value not allowed for the dicti_t. */
  compact_diri_t topri;
  um32 count;
};

struct compact_node {
  struct compact_fpoint fath;
  struct compact_son sons[2];
};

struct compact_big {
  RW_BUFFER_DEF CRC32_TABLE_DUMMY
  struct compact_node dict[COMPACT_NF];
  struct compact_fpoint in[COMPACT_NF];
  struct compact_index dir[COMPACT_NF << 1];
  um16 bitbuf;
};

#define OPACK_TREESIZE 1024

struct opack_big {
  RW_BUFFER_DEF CRC32_TABLE_DUMMY
  um16 tree[OPACK_TREESIZE];
};

#define PACK_EOF_IDX 256

struct pack_big {
  RW_BUFFER_DEF CRC32_TABLE_DUMMY
  um16 leaf_count[24];  /* leaf_count[i] is the number of leaves on level i. Can be 0..257. */
  um8 intnode_count[24];  /* intnode_count[i] is the number of internal nodes on level i. Can be 0..254. */
  um16 byte_indexes[24];  /* This is an index in .bytes (0..255), or 1 more to indicate EOF. Even a valid index can be EOF if there are less bytes. */
  um8 bytes[256];
};

typedef um8 deflate_huffman_bit_count_t;

#define DEFLATE_MAX_TREE_SIZE 1490
#define DEFLATE_BIT_COUNT_ARY_SIZE 318

struct deflate_big {
  RW_BUFFER_DEF CRC32_TABLE_DEF
  um16 huffman_trees_ary[DEFLATE_MAX_TREE_SIZE];
  deflate_huffman_bit_count_t huffman_bit_count_ary[DEFLATE_BIT_COUNT_ARY_SIZE];
};


#ifdef LUUZCAT_COMPRESS_FORK
  /* ELKS PIPE_BUFSIZ values: 0.1.4--0.3.0: PIPE_BUF == PAGE_SIZE == 512; 0.4.0: 512; 0.5.0--0.8.1: 80. */
#  define COMPRESS_FORK_BUFSIZE 0x400  /* As large as possible (for faster char reversing) without increasing .a_total for Minix i86 and ELKS. !!! Increase it to 0x800, 0xc00, 0x1000 etc. if it fits for ELKS 0.4.0 and 0.8.1. */
#  define COMPRESS_FORK_DICTSIZE 13056U  /* # of local dictionary entries: ((1UL << 16) - 256U) == 13056U * 5U. */

  struct compress_big_sf {
    uc8 sf_read_buffer [COMPRESS_FORK_BUFSIZE];
    uc8 sf_write_buffer[COMPRESS_FORK_BUFSIZE];
    us16 dindex[COMPRESS_FORK_DICTSIZE];  /* dictionary: index to substring;  no need to initialize; 25.5 KiB. */
    us16 dchar [COMPRESS_FORK_DICTSIZE];  /* dictionary: last char of string; no need to initialize; 25.5 KiB. */
    us16 wstops[(COMPRESS_FORK_DICTSIZE / (unsigned int)((COMPRESS_FORK_BUFSIZE * 15UL + 31U) >> 5)) + 2U];  /* Output block stop indexes in dindex when writing in reverse order. */
  };
#endif

#ifdef LUUZCAT_SMALLBUF
#  define COMPRESS_SMALLBUF_READ_BUFFER_SIZE 0x400
#  define COMPRESS_SMALLBUF_WRITE_BUFFER_SIZE 0x1400  /* As large as possible (for faster char reversing) without increasing .a_total for Minix i86 and ELKS. */
#  define COMPRESS_SMALLBUF_BITS 14
#  if defined(COMPRESS_FORK_BUFSIZE) && COMPRESS_FORK_BUFSIZE != COMPRESS_SMALLBUF_READ_BUFFER_SIZE
#    error COMPRESS_FORK_BUFSIZE must be the same as COMPRESS_SMALLBUF_READ_BUFFER_SIZE.
#  endif
  struct compress_big_sn {
    uc8 sn_read_buffer [COMPRESS_SMALLBUF_READ_BUFFER_SIZE + READ_BUFFER_EXTRA + READ_BUFFER_OVERSHOOT + (-(COMPRESS_SMALLBUF_READ_BUFFER_SIZE + READ_BUFFER_EXTRA + READ_BUFFER_OVERSHOOT) & 3)];
    uc8 sn_write_buffer[COMPRESS_SMALLBUF_WRITE_BUFFER_SIZE];
    us16 tab_prefix_ary[(1U << COMPRESS_SMALLBUF_BITS) - 256U];
    uc8  tab_suffix_ary[(1U << COMPRESS_SMALLBUF_BITS) - 256U];
  };
#endif

#define FREEZE_N_CHAR2 511
#define FREEZE_T2 2043

struct freeze_big {
  RW_BUFFER_DEF CRC32_TABLE_DUMMY
  um16 freq[FREEZE_T2 + 1];  /* Frequency table. */
  um16 child[FREEZE_T2];  /* Points to child node (child[i], child[i+1]). */
  um16 parent[FREEZE_T2 + FREEZE_N_CHAR2];  /* Points to parent node. */
  uc8 p_len[64];
  uc8 d_len[256];
  um8 code[256];
  um8 table2[8];
};

/* This contains the large arrays other than global_read_buffer and global_write_buffer. */
extern union big_big {
  /* CRC32_TABLE_DUMMY. This is not the first field anymore. */
  struct scolzh_big scolzh;
  struct compact_big compact;
  struct opack_big opack;
  struct pack_big pack;
  struct deflate_big deflate;
  struct freeze_big freeze;
  uc8 unused_dummy[1];  /* Used as lzw_stack in the never-taken `if (use_lzw_stack(maxbits))' branch in decompres_compress_nohdr(....). */
#ifdef LUUZCAT_COMPRESS_FORK  /* fork(...) multiple processes, connect them with pipe(...)s to do high-bits decompress_compress_nohdr(...). */
  struct compress_big_sf compress_sf;
#endif
#ifdef LUUZCAT_SMALLBUF
  struct compress_big_sn compress_sn;
#endif
} big;

/* This is always true, otherwise there is no way to communicate the write_idx. !! Add global_write_idx. */
#define LUUZCAT_WRITE_BUFFER_IS_EMPTY_AT_START_OF_DECOMPRESS 1
/* These functions decompress from stdin (fd STDIN_FILENO == 0) to stdout. */
void decompress_scolzh_nohdr(void);
void decompress_compact_nohdr(void);
void decompress_opack_nohdr(void);
void decompress_pack_nohdr(void);
void decompress_deflate(void);
#if 0
  void decompress_quasijarus_nohdr(void)
#else
#  define decompress_quasijarus_nohdr() decompress_deflate()
#endif
void decompress_zlib_nohdr(void);
void decompress_gzip_nohdr(void);
void decompress_zip_struct_nohdr(uc8 b);  /* b is the byte after the "PK" signature. */
void decompress_compress_nohdr(void);
void decompress_freeze1_nohdr(void);
void decompress_freeze2_nohdr(void);

#endif  /* Of #ifndef _LUUZCAT_H */
