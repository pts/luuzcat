# luuzcat: lightweight, universal, Unix decompression filter

luuzcat is a decompression filter (i.e. a decompressor which reads compressed data from stdin and writes to stdout) of many formats and methods commonly used in Unix system between 1977 and 1993. It is provided as C89 (ANSI C) and C++98 source code, and as executable program files a released for many modern operating systems (in 2025) and some ancient historical systems as well. luuzcat can be used as a drop-in replacement for the *zcat* tool of gzip with the advantages is that luuzcat supports more compressed file formats, and luuzcat provides binary releases for more platforms (operating systems).

## Features

Decompression support for compressed file formats:

|compressed file format              |luuzcat          |gzip, zcat |extension |signature at start      |
|------------------------------------|-----------------|-----------|----------|------------------------|
|gzip                                |yes              |yes        |.gz       |0x1f 0x8b               |
|zlib                                |yes              |--         |(.zlib)   |(8 start byte options)  |
|raw Deflate                         |with `-r`        |--         |(.deflate)|(192 start byte options)|
|first ZIP member                    |yes              |yes        |.zip      |0x50 0x4b 0x03 0x04     |
|multiple ZIP members                |with `-m`        |--         |.zip      |0x50 0x4b 0x03 0x04     |
|ZIP with junk in front of records   |--               |--         |.zip      |(anything)              |
|4.3BSD-Quasijarus Strong compression|yes              |--         |(.Z)      |0x1f 0xa1               |
|Unix (n)compress (LZW)              |yes              |yes        |.Z        |0x1f 0x9d               |
|old Unix pack                       |yes              |--         |.z        |0x1f 0x1f               |
|new Unix pack                       |yes              |yes        |.z        |0x1f 0x1e               |
|Freeze 1                            |yes              |--         |.F, .lzc  |0x1f 0x9e               |
|Freeze 2                            |yes              |--         |.F, .lzc  |0x1f 0x9f               |
|BSD compact                         |yes              |--         |.C        |0x1f 0xff; 0xff 0x1f    |
|SCO compress LZH                    |yes              |yes        |(.Z)      |0x1f 0xa0               |

Other features:

|feature                          |luuzcat|gzip, gunzip, zcat|
|---------------------------------|-------|------------------|
|compression                      |--     |yes               |
|decompression to file            |--     |yes               |
|decompression to stdout          |yes    |yes: `gzip -cd`; `gunzip -d`; `zcat`|
|restoring the original filename  |--     |yes               |
|restoring the original file mtime|--     |yes               |
|checksum (CRC-32, Adler-32) check|yes    |yes               |
|input data error checkin         |full   |full              |
|safe to use on untrusted input   |yes    |yes               |
|static buffers only, no malloc(3)|yes (*)|yes               |

(*): Except that for the i86 targets (8086 CPU or x86 real mode or x86 16-bit protected mode) some tricks are done to use more than 64 KiB of memory for (n)compress decompression with maxbits >= 15. On DOS, the available memory region after the first 64 KIB is used for the (n)compress buffers. On Minix i86 (and ELKS), the process is fork()ed (to 4 processes for maxbits =\= 15 and 5 processes for maxbits =\= 16), and pipe()s are used for communication between the processes; together they have enough memory for the (n)compress decompression.

## Compiling luuzcat

Most users don't need to compile luuzcat from source. If unsure, just skip this section.

To compile luuzcat on a Unix system with a C compiler, clone the repository (`git clone https://github.com/pts/luuzcat.git && cd luuzcat`), and run `make` in the luuzcat directory. The output is the executable program file *luuzcat*. Run it as `./luuzcat <input.gz >output`.

Please note that if you do a release build, you may want to add some optimization and flags to your C compiler. See comments in the [Makefile](Makefile) about that.

## Using luuzcat

First, get luuzcat:

* Windows users should download the executable program file *luuzcat.exe* from the [releases page](https://github.com/pts/luuzcat/releases), and copy it next to the compressed input file (easy), or put it to the *%PATH%* (hard).
* Unix (e.g. macOS, Linux, BSD) users should download the appropriate *luuzcat.\** executable program files from the [releases page](https://github.com/pts/luuzcat/releases), rename it to *luuzcat*, make it executable (`chmod +x luuzcat`), and put it to the `$PATH` (or, temporarily: `alias luuzcat="$PWD"/luuzcat`).

After that, run `luuzcat <input.gz >output`in a terminal window (command prompt). In this command, replace `input.gz` with the compressed input file name, and `output` with the desired output file name (will be uncompressed). If luuzcat completes sucessfully, it doesn't print anything, otherwise it prints an error message. Succes is also indicated in the process exit code (errorlevel on Windows, `$?` on Unix).

You can pass command-line flags to modify the behavior of luuzcat:

* `luuzcat -r <input.deflate >output` enables raw Deflate input support (and disables zlib and ZIP input support). Unfortunately these formats have a conflicting signature, so all of them can't be enabled at the same time.
* `luuzcat -m <input.zip >output` enables concatenating all archive members in a ZIP archive input file.  By default, for compatibility with gzip *zcat*, only the first member is written to stdout, and luuzcat fails with an error if there are multiple members.
* The `-c` and `-d` flags are ignored for compatibility with gzip `gzip -cd` and gzip `gunzip -c`.
* By default, luuzcat exits with a usage message if there are no flags specified, and stdin is a terminal (console). To force reading compressed input from a terminal, specify any flag, e.g. `luuzcat - >output`. (Press *Enter* to exit.)
* Only the first command-line argument is used for flags, so specify them together, e.g. `luuzcat -cdm <input.zip >output`.

## Supported platforms (operating systems)

If you have a C89 (ANSI C) or a C++98 compiler, you can compile luuzcat for yourself for your system of choice. For convenience, the [releases page](https://github.com/pts/luuzcat/releases) provides luuzcat as precompiled executable programs for a great many systems. In short, executable programs are provided for both the modern desktop systems in 2025 (Linux i386, Windows i386 (Win32), macOS amd64 (x86_64), FreeBSD i386, NetBSD i386, OpenBSD i386) and some many historical systems (such as DOS and Minix).

macOS users should use the *luuzcat.da6* executable program file, which runs on macOS 10.5 Leopard or later. It's a Mach-O amd64 (x86_64) executable, which also works on Apple Silicon, using the Rosetta 2 emulator built in to macOS.

Linux, FreeBSD, NetBSD and OpenBSD users on x86 should use the *luuzcat.elf* executable program file. It's an ELF-32 i386 executable, which works on both amd64 (64-bit) and i386 (32-bit) systems. It detects the operating system ABI at startup (e.g. syscall arguments are passed differently on Linux and FreeBSD). Linux users on non-x86 can also run it using QEMU: `qemu-i386 ./luuzcat <input.gz >output`. It works on old and new Linux systems, starting with Linux 1.0 (1994-03-14).

Windows and DOS users should use the *luuzcat.exe* executable program files It's both a Win32 (i386) PE executable and a DOS MZ .exe executable. It works on both amd64 (64-bit) and i386 (32-bit) Windows systems, starting with Windows NT 3.1 (1993-07-27). It also works on IBM PC DOS 2.0 (1983-03-08) or later and MS-DOS 2.0 or later.

Here is the full list of executable program files for Intel amd64 (x86_64) systems:

* *luuzat.da6*:  It's a Mach-O amd64 (x86_64) executable, which also works on Apple Silicon, using the Rosetta 2 emulator built in to macOS. It runs on macOS 10.5 (Leopard) or later. 

Here is the full list of executable program files for Intel i386 systems:

* *luuzcat.exe*: It's both a Win32 (i386) PE executable and a DOS MZ .exe executable. It works on both amd64 (64-bit) and i386 (32-bit) Windows systems, starting with Windows NT 3.1 (1993-07-27). It also works on IBM PC DOS 2.0 (1983-03-08) or later and MS-DOS 2.0 or later and DOS emulators.
   It works on
   Windows NT >=3.1, Windows 95–98–ME, Windows
   2000–XP–Vista–7–8–10–11, ReactOS >=0.4.14. It also works on various
   emulators such as Wine >=1.6, WDOSX DOS extender and HX DOS extender. It
   doesn't work on Win32s, because Win32s can't run Win32 console
   applications.
* *luuzat.di3*:  It's a Mach-O i386 executable It runs on macOS 10.5 Leopard or later, up to (including) macOS 10.14 Mojave (2018-09-24). Most Mac users should use *luuzcat.da6* instead 
* *luuzcat.elf*:
  * Linux >=1.0 (1994-03-34) i386 and possibly earlier
   (tested on Linux 1.0.4 (1994-03-22), 5.4.0 (2019-11-24)).
  * FreeBSD >=2.2.1 (1997-04-05) i386
   (tested on FreeBSD 2.2.1 (1997-04-05), 2.2.2 (1997-05-29),
   2.2.5 (1997-10-21), 2.2.8 (1998-11-29), 2.2.9 (2006-03-31),
   3.2 (1999-05-18), 3.5.1 (2000-07-26), 9.3 (2014-07-11)).
  * NetBSD >=1.5.2 i386 and possibly earlier
   (tested on NetBSD 1.5.2 (2001-09-10) and 10.1 (2024-11-17)).
  * OpenBSD >=3.4 i386 and possibly a bit earlier
   (tested on OpenBSD 3.4 (2003-09-27) and 7.8 (2025-10-12)).
  * DragonFly BSD i386
   (tested on DragonFly BSD 1.4.0 (2006-01-05) and 3.8.2 (2014-08-14),
   later versions can't run i386 exeutables).
  * Minix >=3.2 i386
   (tested on 3.2.0 (2012-02-28) and 3.3.0 (2014-09-14)). The Minix 3.3.0 syscall ABI is very different from Minix 3.2.0. The version is detected at process startup by looking at EBX and at the address of the end of the stack.
  * AT&T Unix System V/386 Release 4 (SVR4) i386
   (tested on version 2.1) and drivatives (such as
   INTERACTIVE UNIX SYSTEM V RELEASE 4.0, Dell Unix, Olivetti Unix, Intel
   Unix).
  * [ibcs-us](https://ibcs-us.sourceforge.io/) running on Linux i386
   (tested with ibcs-us 4.1.6).
  * qemu-i386 running on Linux (any architecture) (tested with
   qemu-i386 2.11.1).
   * Operating system is autodetected at process startup. This is very tricky, because most of the autodetection must happen before the first system call, so it can be based on the initial contents of the registers and the stack (argc, argv, environ and auxv). The BSDs are distinguished from Linux by calling write(2) (syscall number 4) with an invalid fd, and checking whether the value returned in EAX is positive (BSDs) or negative (Linux). 
* *luuzcat.coff*:
  * AT&T Unix System V/386 Release 3.x (SVR3, 1987–) i386.
  * AT&T Unix System V/386 Release 4 (SVR4) i386
   (tested on version 2.1) and drivatives (such as
   INTERACTIVE UNIX SYSTEM V RELEASE 4.0, Dell Unix, Olivetti Unix, Intel
   Unix).
  * 386/ix >=1.0.6, INTERACTIVE UNIX >=2.2 (all versions),.
  * Microport Unix (all versions) i386.
  * Coherent >=4.x (untested).
  * Xenix >=2.3 (untested), 
   [ibcs-us](https://ibcs-us.sourceforge.io/) running on Linux i386.
   * All off these systems use the same ABI. There is no need for autodetection at runtime.
* *luuzcat.3b*: 386BSD >=1.0 (tested on 386BSD 1.0 (1994-10-27)).
* *luuzcat.mi3*:
   Minix 1.5–3.2.x i386 (tested on Minix 1.5 i386 (1990-06-01),
   Minix 1.7.0 i386 (1995-05-30),
   [Minix 1.7.0 i386 vmd](https://web.archive.org/web/20240914234222/https://www.minix-vmd.org/pub/Minix-vmd/1.7.0/)
   (1996-11-06),
   Minix 2.0.0 i386 (1996-10-01), Minix 2.0.4 i386 (2003-11-09),
   Minix 3.1.0 i386 (2005-10-18), Minix 3.2.0 i386 (2012-02-28).
   There are minor ABI differences between Minix versions, these are detected at runtime. 
   For [Minix-386vm](https://ftp.funet.fi/pub/minix/Minix-386vm/1.6.25.1/)
   1.6.25.1, run *luuzcat.m3vm* instead. Minix 3.3.0 has dropped a.out
   support, it supports now ELF-32 executables only, run *luuzcat.elf* instead.
* *luuzcat.m3vm*:
   [Minix-386vm](https://ftp.funet.fi/pub/minix/Minix-386vm/1.6.25.1/)
   1.6.25.1 (1994-03-10, based on Minix 1.6.25). Also tested with Minix
   1.7.0 i386 (1996-11-06). Neither Minix-386vm is able to run
   regular Minix i386 programs, nor the other way round. [Minix 1.7.0 i386
   vmd](https://web.archive.org/web/20240914234222/https://www.minix-vmd.org/pub/Minix-vmd/1.7.0/)
   is able to run both.
* *prog.v7x*: [v7x86](https://www.nordier.com/) (tested on v7x86 0.8a (2007-10-04)).
* *luuzcat.x63*: [xv6-i386](https://github.com/mit-pdos/xv6-public) (tested on xv6-i386 2020-01-21).

Here is the full list of executable program files for Intel i86 systems:

* *luuzcat.exe*: It's both a Win32 (i386) PE executable and a DOS MZ .exe executable. It works on both amd64 (64-bit) and i386 (32-bit) Windows systems, starting with Windows NT 3.1 (1993-07-27). It also works on IBM PC DOS 2.0 (1983-03-08) or later and MS-DOS 2.0 or later and DOS emulators.
* *luuzcat.mi8*: 
   Minix 1.5–2.0.4 i86 (tested with Minix 1.5 i86 (1990-06-01),
   Minix 1.7.0 i86 (1995-05-30), Minix 1.7.5 i86 (1996-09-03),
   Minix 2.0.0 i86 (1996-10-01), Minix 2.0.4 i86 (2003-11-09)),
   ELKS 0.1.4–0.8.1– (tested with
   ELKS 0.1.4 (2012-02-19), ELKS 0.2.0 (2015-03-01),
   ELKS 0.4.0 (2021-12-07), EKS 0.8.1 (2024-10-16)).
   Operating system is autodetected at process startup.
   There are minor ABI differences between Minix versions, these are detected at runtime. 
   It also works in elksemu 0.8.1 on Linux i386, except for (n)compress with maxbits >= 15, because elksemu 0.8.1 doesn't implement fork(2) correctly.

This broad range of operating system support (except for macOS) is provided by the compatibility layer implemented in [progx86.nasm](progx86.nasm) and [luuzcat.h](luuzcat.h), both created just for luuzcat. The macOS targets have been built by [pts-osxcross](https://github.com/pts/pts-osxcross).

Support is missing (but it would be fun to have) for the following notable systems:

* x.out 32-bit for older Xenix/386 (e.g. 2.2.2c). (Xenix 2.3 already supports COFF.)
* x.out 16-bit for older Xenix
* OS/2 1.x NE 16-bit
* Research Unix V7 on PDP-11
* 4.3BSD on VAX
* NetBSD on VAX
* OpenVMS on VAX
* CP/M-86 on IBM PC with 8086 CPU
* WebAssembly WASI
* WebAssembly in the browser: drag-and drop a file to the the web page
* asm.js in the browser: drag-and drop a file to the the web page
* Java JRE 1.2 or later: command line application
* Linux on MIPS I, also emulated in Perl

If you want to get support for your favorite Intel x86 operating system, please open an issue, provide a HDD disk image and a  *qemu-system-i386* command line to run your system.

## Copyright and license

luuzcat is released under the GNU GPL license, version 2.0. Most of luuzcat is written and copyright by Péter Szabó (pts), except for code based on code written by others, see below.

Authors:

* The (n)compress decompressor is based on unlzw.c in [gzip 1.2.4](https://web.archive.org/web/20251122220330/https://mirror.netcologne.de/gnu/gzip/gzip-1.2.4.tar.gz) by Jean-loup Gailly (1993-08-18), which is based on public-domain code compress.c and ncompress.c by Spencer Thomas, Joe Orost, James Woods, Jim McKie, Steve Davies, Ken Turkowski, Dave Mack and Peter Jannesen. See also the [history of (n)compress](https://vapier.github.io/ncompress/). The fork()ing implementation is based on  [decomp16.c](https://web.archive.org/web/20251122213226/https://raw.githubusercontent.com/ghaerr/elks/e083ab36cd28bcfa62cb275bb86e1d2c8d7f69d0/elkscmd/minix1/decomp16.c) by John N. White and Will Rose (1992-03-25).
* The old Unix pack decompressor is based on pcat.c by Steve Zucker (before 1977-07-13). Get it from the file 2/rand/s2/pcat.c in [ug091377.tar.gz](https://tuhs.v6sh.org/UnixArchiveMirror/Applications/Usenix_77/ug091377.tar.gz).
* The new Unix pack decompressor is based on unpack.c in [gzip 1.2.4](https://web.archive.org/web/20251122220330/https://mirror.netcologne.de/gnu/gzip/gzip-1.2.4.tar.gz) by Jean-loup Gailly (1993-08-18), with extra error checks from unpack.c in gzip 1.14. The original implementation is a program by Thomas G. Szymanski  (1978-03), from which [pcat.c ](https://minnie.tuhs.org/cgi-bin/utree.pl?file=SysIII/usr/src/cmd/pcat.c) in Unix System III (1978-04) is adapted, from which [unpack.c](https://web.archive.org/web/20251123001300/https://raw.githubusercontent.com/illumos/illumos-gate/7c478bd95313f5f23a4c958a745db2134aa03244/usr/src/cmd/unpack/unpack.c) in OpenSolaris and illumos is derived. 
* The Freeze 1 and 2 decompressors are based on melt.c etc. in [Freeze 2.5](https://ibiblio.org/pub/linux/utils/compress/freeze-2.5.0.tar.gz) by Leonid A. Broukhis (1999-05-20). Get Freeze 2.1 (1991-03-25) from this Usenet [post on comp.sources.misc](http://cd.textfiles.com/sourcecode/usenet/compsrcs/misc/volume17/freeze/).
* The BSD compact decompressor is based on [uncompact.c](https://minnie.tuhs.org/cgi-bin/utree.pl?file=4.4BSD/usr/src/old/compact/uncompact/uncompact.c) (1987-12-21), [tree.c](https://minnie.tuhs.org/cgi-bin/utree.pl?file=4.4BSD/usr/src/old/compact/common_source/tree.c) and [compact.h](https://minnie.tuhs.org/cgi-bin/utree.pl?file=4.4BSD/usr/src/old/compact/common_source/compact.h) in [4.4BSD-Alpha](https://www.tuhs.org/Archive/Distributions/UCB/4.4BSD-Alpha/src.tar.gz), originally written by Colin L. McMaster (1979-02-14).
* The SCO compress LZH decompressor is based on unlzh.c in [gzip 1.2.4](https://web.archive.org/web/20251122220330/https://mirror.netcologne.de/gnu/gzip/gzip-1.2.4.tar.gz) by Jean-loup Gailly (1993-08-18), which is directly derived from the public domain [ar002](https://web.archive.org/web/20230601015857/http://cd.textfiles.com/sourcecode/msdos/arc_lbr/ar002.zip) written by Haruhiko Okumura (1990-08-15).
* Some the code above had substantial improvements (such as bugfixes, improved error handling, memory usage improvements, speedups, update to ANSI C (C89), getting rid of C compiler warnings) by Péter Szabó (pts).
* All the other code in luuzcat (including the raw Deflate decompressor, the gzip decompressor, the zlib decompressor, the 4.3BSD-Quasijarus Strong compression decompressor and the ZIP decompressor) is written by Péter Szabó (pts).

## Memory usage

Memory usage of luuzcat (including, code, data and stack, excluding kernel buffers):

|target                                                  |  memory usage|
|--------------------------------------------------------|--------------|
|all non-macOS i386 targets                              | <340 KiB|
|DOS 8086 for (n)compress with maxbits =\= 16            | <256 KiB|
|DOS 8086 for (n)compress with maxbits =\= 15            | <128 KiB|
|DOS 8086 for everything else                            |  <64 KiB|
|Minix i86 (and ELKS) for (n)compress with maxbits =\= 16| <334 KiB|
|Minix i86 (and ELKS) for (n)compress with maxbits =\= 15| <267 KiB|
|Minix i86 (and ELKS) for everything else                |  <67 KiB|

Tricks done to reduce memory usage for (n)compress on i86 (16-bit x86) targets:

* Overlapping of buffers used by the different decompressors (see `union big` in [luuzcat.h](luuzcat.h)).
* On DOS 8086:
  * Overlapping the cached CRC-32 table with buffers used by the other decompressors, and regenarating the CRC-32 table when overwritten by others. 
  * Overlapping of (n)compress buffers with data and code (!) used by the other decompressors.
  * Reducing the read and write buffer sizes to a few KiB.
* On Minix i86 (and ELKS):
  * Overlapping of (n)compress buffers with data used by the other decompressors.
  * Reducing the read and write buffer sizes to a few KiB.
  * fork()ing multiple processes for maxbits >= 15 (in total there are, 1 parent + 3 child processes for maxbits =\= 15 and 1 parent + 4 child processes for maxbits =\= 16), and using pipe()s for communication between the processes. Using multiple processes is needed to exeed the per-process limit of  a_data + a_bss + stack < 64 KiB in Minix i86. The implementation is based on [decomp16.c](https://web.archive.org/web/20251122213226/https://raw.githubusercontent.com/ghaerr/elks/e083ab36cd28bcfa62cb275bb86e1d2c8d7f69d0/elkscmd/minix1/decomp16.c), with substantial speed improvements and memory usage improvements.

