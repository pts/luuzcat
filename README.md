# luuzcat: lightweight, universal, Unix decompression filter

luuzcat is a decompression filter (i.e. a decompressor which reads
compressed data from stdin and writes to stdout) of many formats and methods
commonly used on Unix systems between 1977 and 1995. It is provided as C89
(ANSI C) and C++98 source code, and as executable program files a released
for many modern operating systems (in 2025) and some ancient historical
systems as well. luuzcat can be used as a drop-in replacement for the *zcat*
tool of gzip with the advantages is that luuzcat supports more compressed
file formats, and luuzcat provides binary releases for more platforms
(operating systems).

## Features

Decompression support for compressed file formats:

|compressed file format              |luuzcat          |gzip, zcat |extension     |signature at start       |
|------------------------------------|-----------------|-----------|--------------|-------------------------|
|gzip                                |yes              |yes        |.gz           |0x1f 0x8b                |
|zlib                                |yes              |–          |(.zlib)       |(8 start byte options)   |
|raw Deflate                         |with `-r`        |–          |(.deflate)    |(192 start byte options) |
|first ZIP member                    |yes              |yes        |.zip          |0x50 0x4b 0x03 0x04      |
|multiple ZIP members                |with `-m`        |–          |.zip          |0x50 0x4b 0x03 0x04      |
|ZIP with junk in front of records   |–                |–          |.zip          |(anything)               |
|4.3BSD-Quasijarus Strong compression|yes              |–          |(.Z)          |0x1f 0xa1                |
|Unix (n)compress (LZW)              |yes              |yes        |.Z            |0x1f 0x9d                |
|old Unix pack                       |yes              |–          |.z            |0x1f 0x1f                |
|new Unix pack                       |yes              |yes (*)    |.z            |0x1f 0x1e                |
|Freeze 1                            |yes              |–          |.F, .cxf, .lzc|0x1f 0x9e                |
|Freeze 2                            |yes              |–          |.F, .cxf, .lzc|0x1f 0x9f                |
|BSD compact                         |yes              |–          |.C            |0x1f 0xff; 0xff 0x1f (**)|
|SCO compress LZH                    |yes              |yes        |(.Z)          |0x1f 0xa0                |

(*) gzip-1.6 has some bugs in the decompressor, for example it fails with *code out of range* for some valid inputs. This has been fixed in gzip-1.6.

(**) 0xff 0x1f is the more commonly used signature in the wild.

The SCO compress LZH format can be created using *compress -H* on SCO Unix.

The 4.3BSD-Quasijarus Strong compression can be created using *compress -n*
on 4.3BSD-Quasijarus, or by prepending the 2-byte signature to a raw Deflate
stream.

See more information about most of these file formats in
[the 0x1f compression family](http://fileformats.archiveteam.org/wiki/Compress_(Unix)#The_0x1f_compression_family)
table. See also the [ZIP page](http://fileformats.archiveteam.org/wiki/ZIP).

Other features:

|feature                                    |luuzcat  |gzip, gunzip, zcat|
|-------------------------------------------|---------|------------------|
|compression                                |–        |yes               |
|decompression to file                      |–        |yes               |
|decompression to stdout                    |yes      |yes: `gzip -cd`; `gunzip -d`; `zcat`|
|restoring the original filename            |–        |yes               |
|restoring the original file mtime          |–        |yes               |
|checksum (CRC-32, Adler-32) check          |yes      |yes               |
|input data error checkin                   |full (**)|full              |
|safe to use on untrusted input             |yes (**) |yes               |
|static buffers only, no malloc(3)          |yes (*)  |yes               |
|decompression of concatenated .gz files    |yes      |yes               |
|decompression of concatenated non-.gz files|yes      |–                 |

(*): Except that for the i86 targets (8086 CPU or x86 real mode or x86 16-bit protected mode) some tricks are done to use more than 64 KiB of memory for (n)compress decompression with maxbits >= 15. On DOS, the available memory region after the first 64 KIB is used for the (n)compress buffers. On Minix i86 (and ELKS), the process is fork()ed (to 4 processes for maxbits =\= 15 and 5 processes for maxbits =\= 16), and pipe()s are used for communication between the processes; together they have enough memory for the (n)compress decompression.

(**): Only true for gzip >=1.14. Earlier versions of gzip don't implement these checks properly.

luuzcat also supports decompression of a compressed input file which is a
concatenation of any of files of the supported compressed file formats
above, except for Unix (n)compress (LZW). That's because it's impossible for
(n)compress: it spans all the way to the end of the compressed file (EOF),
it doesn't contain an end-of-stream marker.

The following compressed file formats have been omitted from luuzcat because
they need much more than 256 KiB of memory to decompress: bzip2, LZMA, lzip,
LZMA2, XZ, Zstandard (zstd).

Some compressed file formats have been omitted from luuzcat because of size
constraints: in the DOS 8086 target (*luuzcat.com* and parts of
*luuzcat.exe*), code + data + stack has to fit in less than 64 KiB, so there
is no room for more formats. These formats have been omitted for this
reason: LZO, LZ4, ELKS executable compression.

## Compiling luuzcat

Most users don't need to compile luuzcat from source. If unsure, just skip
this section.

To compile luuzcat on a Unix system with a C compiler, clone the repository
(`git clone https://github.com/pts/luuzcat.git && cd luuzcat`), and run
`make` in the luuzcat directory. The output is the executable program file
*luuzcat*. Run it as `./luuzcat <input.gz >output`.

Please note that if you do a release build, you may want to add some
optimization and flags to your C compiler. See comments in the
[Makefile](Makefile) about that.

To recompile the official executable program files (such as *luuzcat.elf*
and *luuzcat.com*, see in the next section) on a Linux i386 (or Linux amd64)
system, run `./release.sh all` in the luuzcat directory. To recompile and
run the tests as well, run `./release.sh all test` instead. For compiling
the macOS targets (*luuzcat.da6* and *luuzcat.di3*), a Linux amd64 system is
needed; if you only have Linux i386, then compile without the macOS targets,
like this: `./release.sh all nomacos`. These are deterministic and fully
reproducible builds. Most tools, including the shell, the C compilers, the C
libraries (libc), the assembler and the emulators for running the tests are
included in the [tools](tools) directory. The non-bundled tools (currently
only [pts-osxcross](https://github.com/pts/pts-osxcross) for compiling the
macOS targets) have to be downloaded, please follow the instructions
displayed by *release.sh*.

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
* *luuzcat.com*: A DOS .com executable program. It works on IBM PC DOS 2.0 (1983-03-08) or later and MS-DOS 2.0 or later and DOS emulators.
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

## Compression methods

General introduction of compression algorithms used in the methods supported
by luuzcat:

* [variable-length
  encoding](https://en.wikipedia.org/wiki/Variable-length_encoding) of
  integers (varint): The input of the encoder is a sequence of unsigned
  integer values, the output is a sequene of bit strings. The mapping is
  reversible. The encoder and the decoder have agreed on the mapping before
  the encoder has sent the first bit. Typically less frequent values (or
  larger values) have longer corresponding bit strings.
* [run-length encoding](https://en.wikipedia.org/wiki/Run-length_encoding) (RLE):
  The encoder encodes consecutive runs of the same input
  value as a (repeat_count, value) pair. It makes no changes (thus it
  provides no size reduction) if a string longer than 1 value is repeating.
  Some byte-level framing (such as sending lengths) ensures that the
  decompressor can distinguish pairs from literal bytes.
* [Huffman coding](https://en.wikipedia.org/wiki/Huffman_coding):
  Variable-length encoding of bytes or other values, typically unsigned
  integers: more frequent values are assigned shorter code bit strings. The
  code bit strings form a prefix code, and they can be visualized in a
  binary tree, called the Huffman tree. Traversing the Huffman tree (i.e.
  moving down left or right depending on the next input bit) is a simple
  (but not the fastest) way of decoding.
  * dynamic Huffman coding: First the Huffman table (specifying the code bit
    string for each value) is sent, then the code bit strings. The actual
    encoding of the Huffman table can make a big difference in size. A
    typical smart encoding is sending the code bit lengths for each byes,
    and letting the decompressor build the bit strings from their lengths.
    There are even smarter ones (see Deflate below). After the Huffman table
    has been sent, dynamic Huffman is a special case of variable-length
    encoding.
  * partial dynamic Huffman coding: The first few bits of each value is
    Huffman-coded, and the remaining bits are sent literally. Example: LZSS
    match distances in the Freeze 2 method are 13 bits (the actual range
    is 0..7935). The first 6 bits (values 0..61) are Huffman-coded, and the
    actual value is the Huffman-coded value multiplied by 128, and to that a
    fixed 7-bit unsigned integer read from the input is added, thus the
    maximum is 61 * 128 + 127 == 7935. It's also possible to have a
    different number of extra bits, based on the Huffman-coded value.
  * fixed Huffman coding: Like dynamic Huffman coding, but the same Huffman
    table is hardcoded to the compressor and the decompressor, it is not
    sent. Example: LZSS match distances in Freeze 1 emthod.
  * adaptive Huffman: No Huffman table is sent, the decompressor builds and
    updates its Huffman table based on the frequencies of values received so
    far. Example: tokens (including literals and LZSS match length) in
    Freeze 1 and 2.
* [LZSS](https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Storer%E2%80%93Szymanski)
  (similar to LZ77):
  A byte string which is a copy of a string is already sent is called
  a match, and is sent as a (distance, length) pair, meaning: start at the
  *distance* bytes before the decompressed output so far, and copy *length*
  bytes from there, appending them to the output. Everything else is sent
  as a literal byte. Distances and lengths are typically sent as
  variable-length integers or are Huffman-coded. Some bit-level framing
  (such as sending a prefix bit) ensures that the decompressor can
  distinguish pairs from literal bytes. It's OK to have a match with its
  length larger than its distance, for example literal *a*, literal *b*,
  literal *c*, match (distance=1, length=5) will be decompressed as
  *abcbcbcb*. (distance=1 means: from the end skip the last byte, plus 1
  byte, thus start with *b*).
  The maximum allowed distance value is also called window size,
  ring buffer size or dictionary size, because this is the number of bytes
  that the decompressor must remember. This can be done in a ring buffer,
  which can be used as the write output buffer (flushing it when end
  reached).
* [LZW](https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Welch)
  (similar to LZ78): Both the compressor and the decompressor build a
  dictionary of (some) byte strings encountered so far. Each byte string has
  an index in the dictionary: an index in 0..255 signifies a single
  (literal) byte, a larger index signifies a byte string of at least 2
  bytes. Only the indexes are sent. The dictionary has the maximum size of
  65536 entries, it won't grow beyond that. The compressor may send a reset
  code (index 256) to reset the dictionary (i.e. keep only the literal
  bytes), which is useful if the input continues with very different kind of
  data.

Some of the more modern methods (most of them invented after 1995):

* prediction by partial matching (PPM, PPMd)
* Burrows–Wheeler transform (BWT)
* arithmetic coding, range coding and other techniques instead of Huffman
  coding
* context modeling (e.g. as used in LZMA)
* Lempel–Ziv–Markov chain (LZMA)
* filter: Before running the compressor, do a reversible transformation on
  the input (which doesn't change its size), to make it better compressible.
  Some typical filters:
  * branch-call-jump (BCJ) filter: Replace relative offsets in call
    instructions in machine code with absolute addresses. It's OK (but
    degrades the compression ratio) to apply it to false positives (i.e. an
    opcode which looks like a call instruction, but it isn't).
  * difference filter: Used for images (such as predictors in PNG), video and
    audio.
  * moved-to-front (MTF) filter; Used before the Burrows-Wheeler transform.

Specific details of compression methods supported by luuzcat:

* Deflate (used by gzip, zlib, raw Deflate, ZIP, 4.3BSD-Quasijarus Strong
  compression): LZSS + Huffman; LZSS with 32 KiB window; supports Huffman
  reset; first Huffman table for fixed or partial dynamic Huffman coding of
  (literal bytes, LZSS match lengths and Huffman reset); second Huffman
  table for fixed or partial dynamic Huffman coding of LZSS match distances;
  efficient encoding of the Huffman tables using bit lengths, RLE and
  (another) Huffman coding; supports uncompressed blocks; supports both
  fixed and dynamic Huffman coding; both Huffman tables are optimized by the
  compressor to fit the input; supports concatenation. Mostly because of all
  these features and a relatively large window size, Deflate typically
  compresses better than anything else below, see details in the section
  about the success story of Deflate below.
* Unix (n)compress (LZW); LZW; indexes are sent in 9 bits, which can grow up
  to maxbits, which can be between 9 and 16; reset code is supported;
  doesn't support concatenation, the compressed LZW stream continues until the
  end of the compressed input stream (end of file, EOF).
* old Unix pack and new Unix pack: dynamic Huffman coding of the input
  bytes; the encoding of the Huffman table is different between old and
  new, both do it less efficiently than Deflate; supports concatenation.
* BSD compact: adaptive Huffman coding of the input bytes using the
  [Faller–Gallager–Knuth (FGK)
  algorithm](https://en.wikipedia.org/wiki/Adaptive_Huffman_coding#FGK_Algorithm);
  slow to decompress, because per-code updates to the Huffman tree (Huffman
  table in binary tree structure) are slow; adapts less precisely than the
  Vitter algorithm in Freeze 2; supports concatenation.
* SCO compress LZH: LZSS + Huffman; LZSS with 8 KiB window; same as the
  *\-lh5\-* method in ar002 and LHA; dynamic Huffman coding of (literal
  bytes and LZSS match lengths); partial dynamic Huffman coding of LZSS match
  distances: the Huffman-coded value just specifies the number of extra
  literally sent bits; efficient encoding of the literal-and-length Huffman
  table using bit lengths, RLE and (another) Huffman coding; supports
  Huffman reset by counting the number of remaining tokens in the current
  block; doesn't support uncompressed blocks; both Huffman tables are
  optimized by the compressor to fit the input; supports concatenation.
* Freeze 1 and 2: LZSS + adaptive Huffman; LZSS with 4 KiB window for Freeze
  1, and ~8 KiB (7936 byte) window for Freeze 2; doesn't support Huffman
  reset; adaptive Huffman coding for (literal bytes and lengths) using the
  [Vitter
  algorithm](https://en.wikipedia.org/wiki/Adaptive_Huffman_coding#Vitter_algorithm),
  same as in LZHUF; fixed (Freeze 1.x) or partial dynamic Huffman coding for
  distances; efficient encoding of the dynamic Huffman table using RLE of
  increasing bit lengths; doesn't support uncompressed blocks; the Huffman
  table for distances is usually not optimized to fit the input, but manual
  and inconvenient user action is needed for that (this is suboptimal); the
  adaptive Huffman format is derived from LZHUF, and is faster and less
  complex than in BSD compact; supports concatenation.

## The 16-bit era of compression

For the years between 1977 and 1993, we can coin the term 16-bit era of
compression. That's because the popular decompressors used in
general-purpose, lossless compression methods could be implemented with
16-bit integer arithmetic (with a very few exceptions such as file size
stored in 32 bits), and the memory needed for decompression was less than 64
KiB, so it was addressable with 16-bit pointers. In contrast, the range
coding decoder in LZMA (2001) uses unsigned 32-bit integers and does a
32-bit integer multiplication for each input bit. This would have been
prohibitively slow in the 1997--1993 era with CPUs with only 16-bit
multiplication support, or even no support for multiplication.

As an illustration, let's break down the memory requirements o Deflate
(1991) decompression:

* For LZSS decompression, a 32 KiB ring buffer window is needed. The
  decompressor can use the same buffer as a write output buffer, flushing it
  after each 32 KiB appended.
* The two Huffman trees used by Huffman decoding can be stored in an array
  of 1490 elements (16 bits for each element) and the bit lengths used for
  creating the Huffman trees can be stored in an array of 318 elements (8
  bits for each element). That's less than 3.4 KiB in total for Huffman
  decoding.
* All other variables (such as number of unflushed bytes in the write output
  buffer) are small and few, less than 0.1 KiB in total.
* Total code size of the decompressor is less than 1.7 KiB of written and C
  and using a decent C compiler (e.g. OpenWatcom C compiler targeting DOS
  8086, not using the OpenWatcom C library). If fully implemented in
  hand-optimized 8086 assembly (see [source
  code](https://github.com/pts/pts-zcat/blob/master/zcatc.nasm)), it can be
  less than 0.9 KiB.
* The stack usage is less than 0.8 KiB, of which 0.5 KiB is dedicated to the
  operating system.
* Total memory usage: 38 KiB if written in C, of which 32 KiB is the ring
  buffer window size. Other compression methods tend to use a smaller window
  (16 KiB, 8 KiB or 4 KiB), which makes their memory usage much smaller, but
  it also significantly degrades the compression ratio, because only smaller
  distances can be used in LZSS matches, thus repetitions of earlier data
  can't be encoded. Reducing the Huffman table sizes could save only ~15%,
  which is not much saving.

A notable exception which needs more 64 KiB for decompression is Unix
(n)compress (LZW). Its tables need 24 bits (16 bits in the prefix table and
8 bits in the suffix table) for each code. The number of codes is `(1 <<
maxbits) - 256`. So for maxbits == 16, 191.25 KiB is needed (only for the
LZW tables), for maxbits == 15, 95.25 KiB is needed. For maxbits == 14,
47.25 KiB is needed. On these 16-bit systems (DOS, Minix i86 and ELKS),
luuzcat supports maxbits == 14 with less than 64 KiB of data memory usage
(total, not only the LZW tables), and it uses system-specific methods (far
pointers on DOS and fork()ed subprocesses communicating over pipe()s on
Minix i86 and ELKS) for maxbits == 15 and 16. Other implementations of
(n)compress decompressors are much less ambitious: they typically support
only maxbits <= 13 (or even just maxbits <= 12) only.

## The success story of Deflate

Why was the Deflate compression method the clear winner of general-purpose,
lossless compression between 1991 and 1995, outperforming its competitors
released between 1977 and 1995, including all other methods supported by
luuzcat? The quick answer: because not only its file format design was
better, but it had a few excellent compressor implementations, which were in
general faster, and produced smaller compressed output than the competitors.
Here is the full story.

Deflate compression was invented and implemented by Phil Katz in 1990--1991,
building upon his own previous compression methods (most specifically the
implode method in PKZIP) and best practices of other compressors in the last
~8 years. Deflate debuted in [PKZIP
1.93a](http://cd.textfiles.com/bbox4/archiver/pkz193a.exe) beta (1991-10-15)
with DOS executable program *pkzip.exe* containing the compressor, and
*pkunzip.exe* containing the decompressor, and *appnote.txt* containing a
description of the file format of the Deflate-compressed stream. The name
Deflate wasn't mentioned in the documentation, but *pkzip.exe* displayed the
message *deflating*, and *pkunzip.exe* displayed the message *inflating*.
PKZIP is closed-source, proprietary software.

Deflate was was quickly adapted in the following open source software:

* The Deflate file format has been thoroughly documented in [RFC
  1951](https://www.rfc-editor.org/rfc/rfc1951.txt) (1996-05).
* gzip 0.1 alpha (1992-10-31) by Jean-loup Gailly: A tool for compressing
  and decompressing single files using Deflate. It has introduced its own
  container format (gzip), documented in [RFC
  1952](https://www.rfc-editor.org/rfc/rfc1952.txt). Both the compressor and
  the decompressor has written by Jean-loup Gailly and has been released as
  free software under the GNU GPL; it doesn't contain any code from PKZIP.
* gzip 0.2 (1992-12-21) by Jean-loup Gailly: This is the earliest version of
  gzip whose [source
  code](https://discmaster.textfiles.com/file/40219/minerva4.zip/minerva4/BBS_UUCP/GZIP05.TAZ)
  has survived as of 2025-11-30.
* Zip 1.5 (1992-02-17) by Mark Adler and UnZip 5.0 (1992-08-22) by Mark
  Adler, both the predecessors of Info-Zip (both 1992-08-22). ZIP archive
  creator (updater) and extractor tool. Deflate (method 8) support has been
  added to Zip 1.4 (compression) and UnZip 5.0 (decompression).
* zlib 0.8 (1995-04-29) by Jean-loup Gailly (focusing on the compression
  routines) and Mark Adler (focusing on the decompression routines): a C
  library to do streaming Deflate compression and decompression. It
  introduced its own container format (zlib), documented in
  [RFC 1950](https://www.rfc-editor.org/rfc/rfc1950.txt) (1996-05).
* libpng 0.5 (1995-05) by Guy Eric Schalnat: C library for creating and
  decoding PNG images. It uses zlib for both compression and decompression
  of the image data. It has added 6 predictors (filters). The PNG file
  format is documented in
  [RFC 2087](https://www.rfc-editor.org/rfc/rfc2083.txt) (1997-03).

Why did the Deflate file format win?

* It supports both LZSS and dynamic Huffman coding. Some competitiors only
  do one of them.
* LZSS (also used by Deflate) typically provides the size savings much more
  quickly than LZW. For example, if the uncompressed input is
  *abcdef-bcdef*, then the compressed LZW stream can indicate that *bc* and
  *de* are repeating (and nothing more found), but the compresses LZSS
  stream can indicate that the entire *bcdef* is repeating.
* LZSS (also used by Deflate) wastes fewer bits on literals than LZW.
* It supports separate Huffman tables for (literal bytes and LZSS match
  lengths) ad for LZSS match distances. These typically have different
  distributions, and it's also beneficial that the LZSS match distances code
  is only used for a match (not for literals).
* Its window size of 32 KiB is larger than of most of the competitors (so it
  leads to better compression ratio), but still not too large so that a
  decompressor using less than 64 KiB of data can be written. This has been
  important between 1985 and 1993, where some Unix systems had a 64 KiB
  limit on (data) memory per process.
* It supports resetting of the Huffman codes. This is important for
  compressing a long input file whose content changes over time: compressing
  its parts with Huffman codes optimized for each part makes the output
  smaller. It is debatable whether adaptive Huffman coding (which Deflate
  doesn't use) can provide these benefits. With a smart Deflate compressor,
  Deflate can adapt to changes in the input faster, but it also needs extra
  bytes to send the new Huffman codes.
* It supports partial dynamic Huffman coding for both Huffman codes, so the
  very precise distribution of the lower bits of LZSS match lengths and
  distances doesn't pollute the Huffman codes.
* It encodes both Huffman tables very efficiently by sending only the
  lengths of each code bit string, and also doing RLE compression and also
  Huffman coding on the list of bit lengths.
* It supports uncompressed blocks, so it can send uncompressible data with
  <0.0077% of overhead. It still adds the end of the data to the ring buffer
  window, so that if parts of the uncompressible data gets repeated, this
  can be encoded by a (short) LZSS match.
* It supports both fixed and dynamic Huffman coding. So for some common
  distributions of input, the Huffman table doesn't have to be sent at all,
  and the fixed Huffman code can be used instead.

What features does the gzip tool have to achieve good compression ratio and
high speed?

* It uses hashing to find matches. It won't find all matches though, the
  accuracy can be configured with the *level* parameter between 1 and 9:
  level 1 is the fastest, it finds fewer and/or shorter matches;
  level 9 is the slowest, it finds the best matches.
* It optimizes both Huffman tables, fitting them to the uncompressed input.
* It finds cleverly where the Huffman tables should be reset.
