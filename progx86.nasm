;
; progx86.nasm: glue code for building i386 programs for various systems in NASM
; by pts@fazekas.hu at Mon Oct 13 03:35:21 CEST 2025
;
; Executable binary compatibility with i386 (32-bit protected mode) targets:
;
; * prog.elf (-DELF, default):
;   Linux >=1.0.4 (1994-05-22) i386 and possibly earlier
;   (tested on Linux 1.0.4 (1994-03-22), 5.4.0 (2019-11-24)),
;   FreeBSD >=2.2.1 (1997-04-05) i386
;   (tested on FreeBSD 2.2.1 (1997-04-05), 2.2.2 (1997-05-29),
;   2.2.5 (1997-10-21), 2.2.8 (1998-11-29), 2.2.9 (2006-03-31),
;   3.2 (1999-05-18), 3.5.1 (2000-07-26), 9.3 (2014-07-11)),
;   NetBSD >=1.5.2 i386 and possibly earlier
;   (tested on NetBSD 1.5.2 (2001-09-10) and 10.1 (2024-11-17)),
;   OpenBSD >=3.4 i386 and possibly a bit earlier
;   (tested on OpenBSD 3.4 (2003-09-27) and 7.8 (2025-10-12)).
;   DragonFly BSD i386
;   (tested on DragonFly BSD 1.4.0 (2006-01-05) and 3.8.2 (2014-08-14),
;   later versions can't run i386 exeutables),
;   Minix >=3.2 i386
;   (tested on 3.2.0 (2012-02-28) and 3.3.0 (2014-09-14)),
;   AT&T Unix System V/386 Release 4 (SVR4) i386
;   (tested on version 2.1) and drivatives (such as
;   INTERACTIVE UNIX SYSTEM V RELEASE 4.0, Dell Unix, Olivetti Unix, Intel
;   Unix),
;   [ibcs-us](https://ibcs-us.sourceforge.io/) running on Linux i386
;   (tested with ibcs-us 4.1.6),
;   qemu-i386 running on Linux (any architecture) (tested with
;   qemu-i386 2.11.1).
; * prog.coff (-DCOFF):
;   AT&T Unix System V/386 Release 3.x (SVR3, 1987--) i386,
;   386/ix >=1.0.6, INTERACTIVE UNIX >=2.2 (all versions), Microport Unix
;   (all versions) i386, Coherent >=4.x,
;   [ibcs-us](https://ibcs-us.sourceforge.io/) running on Linux i386.
; * prog.3b (-DS386BSD):
;   386BSD >=1.0 (tested on 386BSD 1.0 (1994-10-27)).
; * prog.m23 (-DMINIX2I386):
;   Minix 1.5--3.2.x i386 (tested on Minix 1.5 i386 (~1990),
;   Minix 1.7.0 i386 (1995-05-30),
;   [Minix 1.7.0 i386 vmd](https://web.archive.org/web/20240914234222/https://www.minix-vmd.org/pub/Minix-vmd/1.7.0/)
;   (1996-11-06),
;   Minix 2.0.0 i386 (1996-10-01), Minix 2.0.4 i386 (2003-11-09),
;   Minix 3.1.0 i386 (2005-10-18), Minix 3.2.0 i386 (2012-02-28).
;   For [Minix-386vm](https://ftp.funet.fi/pub/minix/Minix-386vm/1.6.25.1/)
;   1.6.25.1, use -DMINIX386VM instead. Minix 3.3.0 has dropped a.out
;   support, it supports now ELF-32 executables only, use -DELF instead.
; * prog.m3vm (-DMINIX386VM):
;   [Minix-386vm](https://ftp.funet.fi/pub/minix/Minix-386vm/1.6.25.1/)
;   1.6.25.1 (1994-03-10, based on Minix 1.6.25). Also tested with Minix
;   1.7.0 i386 (1996-11-06). Neither Minix-386vm is able to run
;   regular Minix i386 programs, nor the other way round. [Minix 1.7.0 i386
;   vmd](https://web.archive.org/web/20240914234222/https://www.minix-vmd.org/pub/Minix-vmd/1.7.0/)
;   is able to run both.
; * prog.v7x (-DV7X86):
;   [v7x86](https://www.nordier.com/) (tested on v7x86 0.8a (2007-10-04)).
; * prog.x63 (-DXV6I386):
;   [xv6-i386](https://github.com/mit-pdos/xv6-public) (tested on xv6-i386
;   2020-01-21).
; * prog.exe (-DWIN32WL): Win32 i386 console application. It works on
;   Windows NT >=3.1, Windows 95--98--ME, Windows
;   2000--XP--Vista--7--8--10--11, ReactOS >=0.4.14. It also works on various
;   emulators such as Wine >=1.6, WDOSX DOS extender and HX DOS extender. It
;   doesn't work on Win32s, because Win32s can't run Win32 console
;   applications.
;
; Executable binary compatibility with x86 16-bit real mode (8086 and later)
; x86 16-bit protected mode (286 and later) targets:
;
; * prog.com (-DDOSCOM): DOS .com program. MS-DOS >=2.0 (1983-03-08), IBM PC
;   DOS >=2.0 (1983-03-08) and DOS emulators. Can use more than 64 KiB of
;   memory using far pointers.
; * progd.exe (-DDOSEXE): DOS .exe program. MS-DOS >=2.0 (1983-03-08), IBM PC
;   DOS >=2.0 (1983-03-08) and DOS emulators. Memory limits are the same as
;   DOS .com (even though it could easily be 64 KiB of code + 64 KiB of
;   data, but with that we'd lose easy PSP access), so it also uses the tiny
;   model (code + data + stack <= 0xff00 bytes).
;   !! Specify -DDOSEXE_STUB to get
;   a DOS .exe best fitted for the `wlink op stub=...` Win32 stub.
;
; High priority TODOs:
;
; * Minix 1.x and 2.x 8086 (16-bit).
; * ELKS.
; * Combined DOS 8086 + Win32 i386 in the same .exe file.
;
; Some low priority TODOs:
;
; * x.out 32-bit for older Xenix/386 (e.g. 2.2.2c). (Xenix 2.3 already supportes
;   COFF.)
; * x.out 16-bit for older Xenix. Does it support segment arithmetic?
; * OS/2 1.x NE 16-bit. Does it support segment arithmetic?
;
; These programs don't run on Xenix 2.2 (but the COFF variant runs on Xenix
; 2.3), SunOS 4.0.x, macOS (except in Docker), DOS or Win32 (except in
; Docker or WSL).
;
; !! Rename MINIX2I386 TO MINIXI386.
; !! Add 16-bit targets MINIXI86 and ELKS.
; !! Add Xenix/86 (large model) and Xenix/386 targets.
; !! Add FreeBSD 2.0.5 (1995-07-10) with the a.out executable format.
; !! Add Linux target with various a.out executable formats. (Linux 1.0 has already supported ELF.)
; !! Test it on FreeBSD 3.5 (1995-05-28).
; !! Test it on 386BSD 0.0 (1992-03-17) or 386BSD 0.1 (1992-07-14).
;    https://gunkies.org/wiki/386BSD says: Once patchkit 023 is installed, 386BSD 0.1 will then run under Qemu 0.11.x
; !! For FreeBSD, try alternative of ELF_OSABI.FreeBSD (i.e. at EI_ABIVERSION == EI_BRAND == 8). Which versions of FreeBSD support it? NetBSD 10.1 also allows it.
; !! For 16-bit targets with ES == DS (such as MINIX1I86, but not DOSEXE or DOSCOM), remove the `mov ax, ds' ++ `mov es, ax' and `push ds ++ pop es' instructions from *_16.nasm, and also from the libc, because they are useless.
;
; Please note that this file implements a minialistic and simplified libc
; for console applications (i.e. no GUI). Especially argv and envp
; generation is very limited on non-Unix systems. Also I/O is very limited:
; it's not possibly to open files, only stdin, stdout and stderr can be
; used for read(...)ing and write(...)ing.
;
; Compatibility note: The OpenWatcom C compiler puts floating point
; constants to CONST (in addition to string literals) as well, unaligned.
; Does it also put integers for `x / 10' in 16-bit mode? Test if this works.
;

%ifdef COFF
  %define COFF 1
%else
  %define COFF 0
%endif
%ifdef ELF
  %define ELF 1
%else
  %define ELF 0
%endif
%ifdef S386BSD
  %define S386BSD 1
%else
  %define S386BSD 0
%endif
%ifdef MINIX2I386
  %define MINIX2I386 1
%else
  %define MINIX2I386 0
%endif
%ifdef MINIX386VM
  %define MINIX386VM 1
%else
  %define MINIX386VM 0
%endif
%ifdef V7X86
  %define V7X86 1
%else
  %define V7X86 0
%endif
%ifdef XV6I386
  %define XV6I386 1
%else
  %define XV6I386 0
%endif
%ifdef WIN32WL  ; Win32 target with `nasm -f obj' + OpenWatcom wlink(1) `wlink form win nt'.
  %define WIN32WL 1  ; !! Write a method which generates much shorter Win32 PE headers, saving up to 2 KiB of the final .exe.
%else
  %define WIN32WL 0
%endif
%ifdef DOSCOM
  %define DOSCOM 1
%else
  %define DOSCOM 0
%endif
%ifdef DOSEXE
  %define DOSEXE 1
%else
  %define DOSEXE 0
%endif
%if COFF+ELF+S386BSD+MINIX2I386+MINIX386VM+V7X86+XV6I386+WIN32WL+DOSCOM+DOSEXE==0
  %define ELF 1  ; Default.
%endif
%if COFF+ELF+S386BSD+MINIX2I386+MINIX386VM+V7X86+XV6I386+WIN32WL+DOSCOM+DOSEXE>1
  %error ERROR_MULTIPLE_SYSTEMS_SPECIFIED
  times -1 nop
%endif

%macro __prog_default_cpu_and_bits 0
  %if DOSCOM+DOSEXE
    bits 16
    cpu 8086
  %else
    cpu 386
    bits 32
  %endif
%endm
__prog_default_cpu_and_bits

; --- Executable program file header generation.

%ifidn __OUTPUT_FORMAT__, obj
  %define __PROG_IS_OBJ_OR_ELF 1
%elifidn __OUTPUT_FORMAT__, elf
  %define __PROG_IS_OBJ_OR_ELF 1
%else
  %define __PROG_IS_OBJ_OR_ELF 0
%endif
%if DOSCOM+DOSEXE
  __prog_general_alignment equ 2
%else
  __prog_general_alignment equ 4
%endif

; This header generation aims for maximum compatibility with existing
; operating systems, and it sacrifices file sizes, i.e. the generated
; executable may be a few dozen bytes longer than absolutely necessary, so
; that it loads correctly in many operating systems.
%ifidn __OUTPUT_FORMAT__, bin
  %define __FILESECTIONNAME__TEXT .text
  %define __FILESECTIONNAME_CONST .rodata.str
  %define __FILESECTIONNAME_CONST2 .rodata
  %define __FILESECTIONNAME__DATA .data
  %define __FILESECTIONNAME__DATAUNA .data  ; Like _DATA, but unaligned. Will be put in front of _DATA. It may also get mixed with _DATA, so if the caller uses this, it should start each `section _DATA' with `times ($$-$)&3 db 0'.
  %define __FILESECTIONNAME__BSS .bss

  %if COFF  ; SysV SVR3 i386 COFF executable program.
    %define __SECTIONNAME_.comment COMMENT
    %define __FILESECTIONNAME_COMMENT .comment  ; Only COFF has this.
    _coff_data_base_org equ 0x400000  ; Standard SysV SVR3 value.
    section .text align=1 valign=1 start=0 vstart=0
    _text_start:
    section .rodata.str align=1 valign=1 follows=.text vfollows=.text  ; CONST. Adds virtual address alignment of 4 without valign=1.
    _rodatastr_start:
    section .rodata align=4 valign=4 follows=.rodata.str vfollows=.rodata.str
    _rodata_start:
    section .data align=4 valign=4 follows=.rodata vstart=_data_vstart
    _data_start:
    section .bss align=4 follows=.data nobits
    _bss_start:
    section .comment align=4 valign=4 follows=.data vfollows=.data
    _comment_start:
    section .text

    coff_timestamp equ 0x239cea6b  ; Reproducible value: 1988-12-07 08:23:07 GMT.

    I386MAGIC: equ 0514q  ; 0x14c. coff_filehdr.f_magic constant.

    ZMAGIC: equ 0413q  ; 0x10b. Demand-load format. coff_aouthdr.ZMAGIC constant.

    F:  ; coff_filehdr.f_flags constants .
    .RELFLG:  equ 00001q  ; relocation info stripped from file.
    .EXEC:    equ 00002q  ; file is executable  (i.e. no unresolved externel references).
    .LNNO:    equ 00004q  ; line nunbers stripped from file.
    .LSYMS:   equ 00010q  ; local symbols stripped from file.
    .MINMAL:  equ 00020q  ; this is a minimal object file (".m") output of fextract.
    .UPDATE:  equ 00040q  ; this is a fully bound update file, output of ogen.
    .SWABD:   equ 00100q  ; this file has had its bytes swabbed (in names).
    .AR16WR:  equ 00200q  ; this file has the byte ordering of an AR16WR (e.g. 11/70) machine (it was created there, or was produced by conv).
    .AR32WR:  equ 00400q  ; this file has the byte ordering of an AR32WR machine(e.g. vax).
    .AR32W:   equ 01000q  ; this file has the byte ordering of an AR32W machine (e.g. 3b,maxi).
    .PATCH:   equ 02000q  ; file contains "patch" list in optional header.
    .NODFL:   equ 02000q  ; (minimal file only) no decision functions for replaced functions.

    STYP:  ; coff_scnhdr.s_flags constants.
    .REG:     equ 0x00  ; "regular" section: allocated, relocated, loaded.
    .DSECT:   equ 0x01  ; "dummy" section: not allocated, relocated, not loaded.
    .NOLOAD:  equ 0x02  ; "noload" section: allocated, relocated, not loaded.
    .GROUP:   equ 0x04  ; "grouped" section: formed of input sections.
    .PAD:     equ 0x08  ; "padding" section: not allocated, not relocated, loaded.
    .COPY:    equ 0x10  ; "copy" section: for decision function used by field update; not allocated, not relocated, loaded; reloc & lineno entries processed normally.
    .TEXT:    equ 0x20  ; section contains text only.
    .DATA:    equ 0x40  ; section contains data only.
    .BSS:     equ 0x80  ; section contains bss only.
    .INFO:    equ 0x200  ; comment section : not allocated not relocated, not loaded.
    .OVER:    equ 0x400  ; overlay section : relocated not allocated or loaded.
    .LIB:     equ 0x800  ; for .lib section : same as INFO.

    ; COFF headers compatible with Unix System V Release 3.2.
    coff_filehdr:  ; COFF file header.
    .f_magic:	dw I386MAGIC  ; magic number.
    .f_nscns:	dw (coff_scnhdr_end-coff_scnhdr_text)/40  ; number of sections == 4.
    .f_timdat:	dd coff_timestamp  ; time & date stamp.
    .f_symptr:	dd 0  ; file pointer to symtab.
    .f_nsyms:	dd ERROR_MISSING_F_END  ; 0. number of symtab entries.
    .f_opthdr:	dw coff_aouthdr.end-coff_aouthdr  ; sizeof(optional hdr) == 0x1c.
    .f_flags:	dw F.RELFLG|F.EXEC|F.LNNO|F.LSYMS|F.AR32WR  ; flags.

    coff_aouthdr:  ; COFF optional header.
    .magic:		dw ZMAGIC  ; magic number.
    .vstamp:		dw 0  ; version stamp.
    .tsize:		dd _text_size+_rodatastr_size+_rodata_size-(coff_filehdr_end-coff_filehdr)  ; text size in bytes, aligned to 4.
    .dsize:		dd _data_size  ; initialized data in bytes, aligned to 4.
    .bsize:		dd _bss_size  ; uninitialized data in bytes, aligned to 4.
    .entry:		dd _start  ; entry point.
    .text_start:	dd _text_start+(coff_filehdr_end-coff_filehdr)  ; base of text used for this file == 0xd0.
    .data_start:	dd _data_start  ; base of data used for this file.
    .end:

    coff_scnhdr_text:  ; Section header.
    .s_name:	db '.text', 0, 0, 0  ; section name.
    .s_paddr:	dd _text_start+(coff_filehdr_end-coff_filehdr)  ; physical address, aliased s_nlib.
    .s_vaddr:	dd _text_start+(coff_filehdr_end-coff_filehdr)  ; virtual address.
    .s_size:	dd _text_size+_rodatastr_size+_rodata_size-(coff_filehdr_end-coff_filehdr)  ; section size.
    .s_scnptr:	dd _coff_text_fofs+(coff_filehdr_end-coff_filehdr)  ; file ptr to raw data for section.
    .s_relptr:	dd 0  ; file ptr to relocation.
    .s_lnnoptr:	dd 0  ; file ptr to line numbers.
    .s_nreloc:	dw 0  ; number of relocation entries.
    .s_nlnno:	dw 0  ; number of line number entries.
    .s_flags:	dd STYP.TEXT  ; flags.

    coff_scnhdr_data:  ; Section header.
    .s_name:	db '.data', 0, 0, 0  ; section name.
    .s_paddr:	dd _data_start  ; physical address, aliased s_nlib.
    .s_vaddr:	dd _data_start  ; virtual address.
    .s_size:	dd _data_size  ; section size.
    .s_scnptr:	dd _coff_data_fofs  ; file ptr to raw data for section.
    .s_relptr:	dd 0  ; file ptr to relocation.
    .s_lnnoptr:	dd 0  ; file ptr to line numbers.
    .s_nreloc:	dw 0  ; number of relocation entries.
    .s_nlnno:	dw 0  ; number of line number entries.
    .s_flags:	dd STYP.DATA  ; flags.

    coff_scnhdr_bss:  ; Section header.
    .s_name:	db '.bss', 0, 0, 0, 0  ; section name.
    .s_paddr:	dd _bss_start  ; physical address, aliased s_nlib.
    .s_vaddr:	dd _bss_start  ; virtual address.
    .s_size:	dd _bss_size  ; section size.
    .s_scnptr:	dd 0  ; file ptr to raw data for section.
    .s_relptr:	dd 0  ; file ptr to relocation.
    .s_lnnoptr:	dd 0  ; file ptr to line numbers.
    .s_nreloc:	dw 0  ; number of relocation entries.
    .s_nlnno:	dw 0  ; number of line number entries.
    .s_flags:	dd STYP.BSS  ; flags.

    coff_scnhdr_comment:  ; Section header.
    .s_name:	db '.comment'  ; section name.
    .s_paddr:	dd 0  ; physical address, aliased s_nlib.
    .s_vaddr:	dd 0  ; virtual address.
    .s_size:	dd _comment_size  ; section size.
    .s_scnptr:	dd _coff_comment_fofs  ; file ptr to raw data for section.
    .s_relptr:	dd 0  ; file ptr to relocation.
    .s_lnnoptr:	dd 0  ; file ptr to line numbers.
    .s_nreloc:	dw 0  ; number of relocation entries.
    .s_nlnno:	dw 0  ; number of line number entries.
    .s_flags:	dd STYP.INFO  ; flags.

    coff_scnhdr_end:
    coff_filehdr_end:
  %elif ELF
    ; !! OpenBSD support is incomplete here, because write(2) in OpenBSD >=7.3 treats .rodata as execute-only, and returns EFAULT. Currently it needs application-specific workaround, but we could just have 2 PT_LOADs. See https://stackoverflow.com/q/79806755 for more.
    %define __FILESECTIONNAME__DATAUNA .data.una  ; Like _DATA, but unaligned. ELF implements it, other executable file formats don't.
    _base_org equ 0x8048000  ; Standard Linux i386, FreeBSD i386 and SysV SVR4 i386 value.
    section .header align=1 valign=1 start=0 vstart=_base_org
    _header_start:
    section .text align=1 valign=1 follows=.header vfollows=.header
    ;_text_start:  ; Will be defined in f_end.
    section .rodata.str align=1 valign=1 follows=.text vfollows=.text  ; CONST. Same PT_LOAD as .text.
    _rodatastr_start:
    section .rodata align=4 valign=4 follows=.rodata.str vfollows=.rodata.str  ; CONST2. Same PT_LOAD as .text.
    _rodata_start:
    section .data.una align=1 valign=1 follows=.rodata vstart=_data_vstart
    _datauna_start:
    section .data align=4 valign=4 follows=.data.una vfollows=.data.una
    _data_start:
    section .bss align=4 follows=.data nobits  ; align=4 is also important for the clearing with `rep stosd'.
    _bss_start:
    section .text

    ELF_OSABI:  ; ELF EI_OSABI constants.
    .SysV equ 0
    .Linux equ 3
    .FreeBSD equ 9  ; DragonFly BSD <=3.8.2 can run FreeBSD executables out of the box.

    ELF_PT:  ; ELF PHDR type constants.
    .LOAD equ 1
    .NOTE equ 4
    .OPENBSD_SYSCALLS equ 0x65a3dbe9
    section .header
    ; We use ELF_OSABI.FreeBSD, because newer FreeBSD checks it. Linux, NetBSD, OpenBSD and SysV SVR4 don't check it.
    ; FreeBSD 3.5.1 requires the NUL-terminated string "FreeBSD" at file offset 8 (== EI_BRAND), otherwise it fails with:
    ;   ELF binary type not known.  Use "brandelf" to brand it.
    ; FreeBSD 9.3 doesn't require such a brand.
    Elf32_Ehdr:
		db 0x7f, 'ELF', 1, 1, 1, ELF_OSABI.FreeBSD, 'FreeBSD', 0, 2, 0, 3, ERROR_MISSING_F_END
		dd 1, _start, Elf32_Phdr0-Elf32_Ehdr, 0, 0
		dw Elf32_Phdr0-Elf32_Ehdr, Elf32_Phdr1-Elf32_Phdr0, (Elf32_Phdr.end-Elf32_Phdr0)>>5, 0x28, 0, 0
    Elf32_Phdr0:
		dd ELF_PT.LOAD, Elf32_header.end-Elf32_Ehdr, Elf32_header.end, 0, _text_size+_rodatastr_size+_rodata_size-(Elf32_header.end-Elf32_Ehdr), _text_size+_rodatastr_size+_rodata_size-(Elf32_header.end-Elf32_Ehdr), 5, 1<<12
    Elf32_Phdr1:
		dd ELF_PT.LOAD, _text_size+_rodatastr_size+_rodata_size, _datauna_start, 0, _datauna_size+_data_size, _datauna_size+_data_size+_data_endalign_extra+_bss_size, 6, 1<<12
    Elf32_Phdr2:
		dd ELF_PT.NOTE, Elf32_note-Elf32_Ehdr, Elf32_note, 0, Elf32_note.end-Elf32_note, Elf32_note.end-Elf32_note, 4, 1<<2
    Elf32_Phdr3:
		dd ELF_PT.OPENBSD_SYSCALLS, Elf32_openbsd_syscalls-Elf32_Ehdr, 0, 0, Elf32_openbsd_syscalls.end-Elf32_openbsd_syscalls, Elf32_openbsd_syscalls.end-Elf32_openbsd_syscalls, 4, 1<<2
    Elf32_Phdr.end:
    Elf32_note:  ; NetBSD checks it. Actual value is same as in /bin/echo in NetBSD 1.5.2 (2001-08-18). It also works with NetBSD 10.1 (2024-11-17). Minix 3.2.0 and 3.3.0 don't check it.
		dd 7, 4, 1  ; Size of the name, size of the value, node type.
		db 'NetBSD', 0  ; 7-byte name.
		db 0  ; Alignment padding to 4.
		dd 199905  ; Some version number in decimal.
		dd 7, 7, 2  ; Size of the name, size of the value, node type.
		db 'NetBSD', 0  ; 7-byte name.
		db 0  ; Alignment padding to 4.
		db 'netbsd', 0
		db 0  ; Alignment padding to 4.
		dd 8, 4, 1  ; Size of the name, size of the value, node type.
		db 'OpenBSD', 0  ; 8-byte name.
		dd 0  ; Version number.
    .end:
    ; Elf32_openbsd_syscalls will be added in f_end.
    section .text
  %elif S386BSD
    ; The minimum size for a 386BSD a.out executable (tried, it works) is
    ; 0x2001 bytes: 0x1000 bytes for the header, 0x1000 bytes for total text
    ; and 1 byte for total data. (Actual 386BSD 1.0 executables have their
    ; total data size rounded up to the page size 0x1000, padded with NULs.)
    section .header align=1 valign=1 start=0 vstart=0
    section .text align=0x1000 valign=0x1000 start=0x1000 vstart=0
    _text_start:
    section .rodata.str align=1 valign=1 follows=.text vfollows=.text  ; CONST. Grouped together with .text.
    _rodatastr_start:
    section .rodata align=1 valign=1 follows=.rodata.str vfollows=.rodata.str  ; CONST2. Grouped together .text.
    _rodata_start:
    section .data align=0x1000 valign=0x1000 follows=.rodata vfollows=.rodata
    _data_start:
    section .bss align=4 follows=.data nobits
    _bss_start:
    section .header
    S386BSD_exec:  ; `struct exec' in usr/src/kernel/include/sys/exec.h
    .a_magic:	dd 0x10b  ; ZMAGIC signature (magic number).
    .a_text:	dd (_text_size+_rodatastr_size+_rodata_size+0xfff)&~0xfff  ; Size of text (code) segment. Always a multiple of 0x1000.
    .a_data:	dd _data_size  ; Size of initialized data segment.
    .a_bss:	dd _data_endalign_extra+_bss_size  ; Size of uninitialized data segment. Can have any value, even 0.
    .a_syms:	dd 0  ; Symbol table size.
    .a_entry:	dd _start  ; Virtual address (vaddr) of entry point.
    .a_trsize:	dd 0  ; Text relocation size.
    .a_drsize:	dd 0  ; Data relocation size.
  %elif MINIX2I386+MINIX386VM
    %ifndef MINIX_I386_STACK
      %define MINIX_I386_STACK 0x4000  ; 16 KiB. Minix i386 programs (such as cat(1)) use this value. This is just a hint, the actual stack usage will be smaller.
    %endif
    %if MINIX_I386_STACK<0x1000  ; Must include argv and environ strings.
      %define MINIX_I386_STACK 0x1000
    %endif
    %if MINIX2I386
      section .header align=1 valign=1 start=0 vstart=-0x20  ; -0x20 to make the relative jmp correct in __prog_j_start blow.
      section .text align=1 valign=1 follows=.header vstart=(__prog_j_start.end-__prog_j_start)
      ; It would be more traditional to group .rodata.str and .rodata together
      ; with .text (rather than .data), just like Linux does it, but that
      ; wouldn't work, because for that we'd have to generate the `cs;' prefix
      ; in assembly instructions reading data from .rodata. That's because in
      ; Minix, CS can be used to access a_text only (no a_data), and DS and ES
      ; can be used to access a_data only (no a_text), and offsets reset to 0
      ; at the beginning of a_text. This is not the flat memory model!
      section .rodata.str align=1 valign=0x1000 follows=.text vstart=0  ; CONST.
    %elif MINIX386VM
      section .header align=1 valign=1 start=0 vstart=0x1000  ; -0x20 to make the relative jmp correct in __prog_j_start blow.
      section .text align=1 valign=1 follows=.header vstart=(0x1020+__prog_j_start.end-__prog_j_start)
      section .rodata.str align=1 valign=0x1000 follows=.text vstart=0x1000  ; CONST.
    %else
      %error ERROR_UNKNOWN_MINIX_TARGET
      times -1 nop
    %endif
    _rodatastr_start:
    section .rodata align=1 valign=1 follows=.rodata.str vfollows=.rodata.str  ; CONST2.
    _rodata_start:
    section .data align=1 valign=1 follows=.rodata vfollows=.rodata
    _data_start:
    section .bss align=4 follows=.data nobits
    _bss_start:
    section .header
    minix_i386_exec:  ; `struct exec' in usr/include/a.out.h
    .a_magic: db 1, 3  ; Signature (magic number).
    %if MINIX2I386
      .a_flags: db 0x20  ; Flags. 0x20 == A_SEP (text and data share the same virtual address space).
    %elif MINIX386VM
      .a_flags: db 0x23  ; Flags. 0x23 == A_SEP (text and data share the same virtual address space) | A_UZP (== 1, unmapped zero page) | A_PAL (== 2, page-aligned executable).
    %else
      %error ERROR_UNKNOWN_MINIX_TARGET
      times -1 nop
    %endif
    .a_cpu: db 0x10  ; CPU ID.
    .a_hdrlen: db .size  ; Length of header.
    .a_unused: db 0  ; Reserved for future use.
    .a_version: dw 0  ; Version stamp (unused by Minix).
    .a_text: dd _text_size  ; Size of text segement in bytes.
    .a_data: dd _rodatastr_size+_rodata_size+_data_size  ; Size of data segment in bytes.
    .a_bss: dd _data_endalign_extra+_bss_size  ; Size of bss segment in bytes.
    %if MINIX2I386
      .a_entry: dd 0  ; Virtual address (vaddr) of entry point. It will start at __prog_j_start below. Minix 1.5--1.7.0 i386 ignores this value, and always uses 0.
    %elif MINIX386VM
      .a_entry: dd 0x1020  ; Repurposed to be checksum field, its value must be (UZP ? page_size : 0) + (PAL ? a_hdrlen : 0). The entry point virtual address is always 0.
    %else
      %error ERROR_UNKNOWN_MINIX_TARGET
      times -1 nop
    %endif
    .a_total: dd _data_size+_bss_size+MINIX_I386_STACK  ; Total memory allocated for a_data, a_bss and stack, including argv and environ strings. text is not included because of A_SEP.
    .a_syms: dd 0  ; Size of symbol table.
    .size: equ $-minix_i386_exec
    __prog_j_start:
    jmp strict near _start  ; Minix 1.7.0 i386 always uses vaddr 0 as the entry point, ignoring .a_start below. To work it around, add a jump here.
    .end:
    section .text
    _text_start: equ $-(__prog_j_start.end-__prog_j_start)
  %elif V7X86
    ; V7X86 supports two a.out executable formats, which only differ in the
    ; memory layout of a_data: NMAGIC uses up to 0xfff bytes more of virtual
    ; memory, but can share a_text among multiple processes of the same
    ; executable; FMAGIC can't do any sharing. The majority of the a.out
    ; executables in v7x86 is NMAGIC, so we also do that here.
    section .header align=1 valign=1 start=0 vstart=0
    section .text align=1 valign=0x1000 follows=.header vstart=0
    _text_start:
    section .rodata.str align=1 valign=1 follows=.text vfollows=.text  ; CONST. Grouped together with .text.
    _rodatastr_start:
    section .rodata align=1 valign=1 follows=.rodata.str vfollows=.rodata.str  ; CONST2. Grouped together .text.
    _rodata_start:
    section .data align=1 valign=0x1000 follows=.rodata vfollows=.rodata  ; FMAGIC would have valign=1 for .text.
    _data_start:
    section .bss align=4 follows=.data nobits
    _bss_start:
    section .header
    V7X86_exec:  ; `struct exec' in usr/include/a.out.h
    .a_magic:	dd 0x108  ; NMAGIC signature (magic number). FMAGIC would be 0x107.
    .a_text:	dd _text_size+_rodatastr_size+_rodata_size  ; Size of text (code) segment.
    .a_data:	dd _data_size  ; Size of initialized data segment.
    .a_bss:	dd _data_endalign_extra+_bss_size  ; Size of uninitialized data segment. Can have any value, even 0.
    .a_syms:	dd 0  ; Symbol table size.
    .a_entry:	dd _start  ; Virtual address (vaddr) of entry point.
    .a_trsize:	dd 0  ; Text relocation size.
    .a_drsize:	dd 0  ; Data relocation size.
  %elif XV6I386
    ; xv6-i386 uses ELF which is mostly incompatible with Linux, FreeBSD etc. (because e.g. .text vstart is not page-aligned for xv6-i386), so we keep it separate from ELF above.
    ;_base_org equ 0  ; xv6-i386 uses ELF with base 0. It would work with higher bases, but then it would waste memory. 0x8048000 would be way too much to waste.
    section .header align=1 valign=1 start=0 vstart=0
    section .text align=0x10 valign=0x1000 follows=.header vstart=0
    _text_start:
    section .rodata.str align=1 valign=1 follows=.text vfollows=.text  ; CONST. Same PT_LOAD as .text.
    _rodatastr_start:
    section .rodata align=4 valign=4 follows=.rodata.str vfollows=.rodata.str  ; CONST2. Same PT_LOAD as .text.
    _rodata_start:
    section .data align=4 valign=4 follows=.rodata vfollows=.rodata  ; There is no alignment at EOF if .rodata and .data are empty.
    _data_start:
    section .bss align=4 follows=.data nobits  ; align=4 is also important for the clearing with `rep stosd'.
    _bss_start:

    ELF_OSABI:  ; ELF EI_OSABI constants.
    .SysV equ 0

    ELF_PT:  ; ELF PHDR type constants.
    .LOAD equ 1

    section .header
    ; We use ELF_OSABI.FreeBSD, because newer FreeBSD checks it. Linux, NetBSD and SysV SVR4 don't check it.
    Elf32_Ehdr:
		db 0x7F, 'ELF', 1, 1, 1, ELF_OSABI.SysV, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, ERROR_MISSING_F_END
		dd 1, _start, Elf32_Phdr0-Elf32_Ehdr, 0, 0
		dw Elf32_Phdr0-Elf32_Ehdr, Elf32_Phdr1missing-Elf32_Phdr0, (Elf32_Phdr.end-Elf32_Phdr0)>>5, 0x28, 0, 0
    Elf32_Phdr0:
		dd ELF_PT.LOAD, (Elf32_header.end-Elf32_Ehdr+0xf)&~0xf, 0, 0, _text_size+_rodatastr_size+_rodata_size+_data_size, _text_size+_rodatastr_size+_rodata_size+_data_size+_data_endalign_extra+_bss_size, 7, 1<<4
    Elf32_Phdr1missing:
    Elf32_Phdr.end:
    Elf32_header.end:
    ;times ($$-$)&0xf db 0  ; Not needed, `section .text align=0x10' will take care of it.
  %elif DOSCOM
    %define __SECTIONNAME_.startsec STARTSEC
    %define __FILESECTIONNAME_STARTSEC .startsec  ; Only DOSCOM has it, to prevent the need for `jmp _start' at the beginning of the DOS .com program file.
    __prog_para_count_from_psp equ 0x1000  ; 64 KiB == 0x1000 16-byte paragraphs.
    ; No header for DOS .com programs, everything is implicit.
    section .startsec align=0x10000 valign=0x100 start=0 vstart=0x100
    _startsec_start:
    section .text align=1 valign=1 follows=.startsec vfollows=.startsec
    _text_start:
    section .rodata.str align=1 valign=1 follows=.text vfollows=.text  ; CONST. Same PT_LOAD as .text.
    _rodatastr_start:
    section .rodata align=2 valign=2 follows=.rodata.str vfollows=.rodata.str  ; CONST2. Same PT_LOAD as .text.
    _rodata_start:
    section .data align=2 valign=2 follows=.rodata vfollows=.rodata  ; There is no alignment at EOF if .rodata and .data are empty.
    _data_start:
    section .bss align=2 follows=.data nobits  ; align=4 is also important for the clearing with `rep stosd'.
    _bss_start:
    section .text
  %elif DOSEXE
    ;__prog_para_count_from_psp equ ... ; Populated in f_end.
    %ifdef DOSEXE_STACK_SIZE
      %assign DOSEXE_STACK_SIZE DOSEXE_STACK_SIZE
    %else
      %assign DOSEXE_STACK_SIZE 0xc0+0x100  ; 0xc0 is the minimum (see below), 0x100 is actual stack usage of the program.
    %endif
    %if DOSEXE_STACK_SIZE<0xc0  ; Minimum safe value to be reserved for the stack, based on https://retrocomputing.stackexchange.com/a/31813
      %assign DOSEXE_STACK_SIZE 0xc0
    %endif
    __prog_dosexe_hdrsize equ 1
    section .header
    %if __prog_dosexe_hdrsize==0
      section .text align=0x10 valign=0x100 follows=.header vstart=0x100
    %endif
    dos_exe_header:  ; DOS .exe header of the compressor: http://justsolve.archiveteam.org/wiki/MS-DOS_EXE
    .signature:	db 'MZ'
    .lastsize:	dw (__prog_dosexe_file_image_size+(__prog_dosexe_hdrsize<<4))&0x1ff  ; Length of load module mod 200H.
    .nblocks:	dw (__prog_dosexe_file_image_size+(__prog_dosexe_hdrsize<<4)+0x1ff)>>9  ; Number of 200H pages in load module.
    .nreloc:	dw 0  ; Number of relocation items; no relocations.
    .hdrsize:	dw __prog_dosexe_hdrsize  ; Size of header in paragraphs.
    .minalloc:	dw __prog_dosexe_minalloc  ; Minimum number of paragraphs required above load mod.
    .maxalloc:	dw 0xffff  ; Maximum number of paragraphs required above load mod.
    .ss:	dw 0xfff0  ;  Offset of stack segment in load module. Initial CS == DS == ES == PSP segment.
    %if __prog_dosexe_hdrsize==1
      section .text align=0x10 valign=0x100 follows=.header vstart=0x100
      dos_exe_header_continued:
    %endif
    .sp:	dw __prog_dosexe_offset_limit&0xffff  ; Initial value of SP.
    .checksum:	dw 0  ; Checksum. We don't compute it.
    .ip:	dw _start  ; Initial value of IP.
    .cs:	dw 0xfff0  ; Offset of code segment within load module (segment). Initial SS == DS == ES == PSP segment.
    %if 0  ; Unused header fields.
      .relocpos: dw 0  ; File offset of first relocation item. Unused because of nreloc == 0.
      .noverlay: dw 0  ; Overlay number. Unused.
    %endif
    %if __prog_dosexe_hdrsize>1
      section .text align=0x10 valign=0x100 start=(__prog_dosexe_hdrsize<<4) vstart=0x100
    %endif
    _text_start:
    section .rodata.str align=1 valign=1 follows=.text vfollows=.text  ; CONST. Same PT_LOAD as .text.
    _rodatastr_start:
    section .rodata align=2 valign=2 follows=.rodata.str vfollows=.rodata.str  ; CONST2. Same PT_LOAD as .text.
    _rodata_start:
    section .data align=2 valign=2 follows=.rodata vfollows=.rodata  ; There is no alignment at EOF if .rodata and .data are empty.
    _data_start:
    section .bss align=2 follows=.data nobits  ; align=4 is also important for the clearing with `rep stosd'.
    _bss_start:
    section .text
  %else
    %error ERROR_ASSERT_UNKNOWN_EXECUTABLE_FORMAT
    times -1 nop
  %endif
  section .text

  %macro __prog_pin_syscall_if_needed 2
    %ifdef __NEED_@%2
      dd %1, %2
    %endif
  %endm

  %macro f_end 0  ; !! Rename this to prog_end or something longer.
    ERROR_MISSING_F_END equ 0  ; If you are getting ERROR_MISSING_F_END errors, then add `f_end' to the end of your .nasm source file.

    %if DOSCOM
      section .startsec
      _startsec_endu:
    %endif
    section .text
    _text_endu:
    section .rodata.str
    _rodatastr_endu:
    section .rodata
    _rodata_endu:
    %if ELF
      %undef section  ; So that the `section ...' directive below does the NASM default.
      section .header
      %idefine section __prog_section
      Elf32_openbsd_syscalls:  ; ELF section .openbsd.syscalls for syscall pinning introduced in OpenBSD 7.5 (2024-04-05).
      ; Read more (search for pinsyscalls): https://www.openbsd.org/75.html
      ; Read more (assembly example): https://nullprogram.com/blog/2025/03/06/
      ; Read more: https://man.openbsd.org/pinsyscalls.2
      __prog_pin_syscall_if_needed @syscall_location_write, SYS.write
      __prog_pin_syscall_if_needed @syscall_location_any, SYS.read
      __prog_pin_syscall_if_needed @syscall_location_any, SYS.write
      __prog_pin_syscall_if_needed @syscall_location_any, SYS.ioctl
      __prog_pin_syscall_if_needed @syscall_location_any, SYS.exit
      .end:
      Elf32_header.end:
      section .text
      _text_start: equ $$-(Elf32_header.end-Elf32_Ehdr)
      section .data
      _data_size_before_oscompat equ $-$$
      section .data.una
      %if ($-$$)+_data_size_before_oscompat==0
        ; The reason why we add @oscompat to .data.una here rather
        ; than to anywhere in .bss is to make sure that .data.una+.data is not
        ; empty, which would cause ibcs-us 4.1.6 to segfault.
        %define IS_OSCOMPAT_ADDED 1
        global @oscompat
        @oscompat: db 0  ; Initial value doesn't matter.
      %endif
      _datauna_endu:
    %endif
    section .data
    %if COFF
      %if $-$$==0
        dd 0  ; Workaround to avoid an empty .data, which would cause ibcs-us 4.1.6 to segfault.
      %endif
    %endif
    %if S386BSD
      %if $-$$==0
        db 0  ; Workaround to avoid an empty .data, which would prevent 386BSD 1.0 from loading the program.
      %endif
    %endif
    _data_endu:
    section .text

    %if MINIX2I386+MINIX386VM
      %if _data_endu==_data_start && _rodata_endu==_rodata_start  ; Both .rodata and .data are empty.
        _rodatastr_endalign equ 0  ; For S386BSD this wouldn't make a difference because of the page-alignment of nonempty data. For compatibility, we make everything a multiple of 4 in COFF.
      %else
        _rodatastr_endalign equ (-(_rodatastr_endu-_rodatastr_start))&3
      %endif
      %if _data_endu==_data_start  ; For S386BSD this wouldn't make a difference because of the page-alignment of nonempty data. For compatibility, we make everything a multiple of 4 in COFF.
        _rodata_endalign equ 0
      %else
        _rodata_endalign equ (-(_rodata_endu-_rodata_start))&3
      %endif
      _data_endalign equ (-(_rodatastr_endu+_rodatastr_endalign-_rodatastr_start)-(_rodata_endu+_rodata_endalign-_rodata_start)-(_data_endu-_data_start))&3
    %elif COFF+S386BSD
      _rodatastr_endalign equ (-(_text_endu-_text_start)-(_rodatastr_endu-_rodatastr_start))&3
      _rodata_endalign equ (-(_rodata_endu-_rodata_start))&3
      _data_endalign equ (-(_data_endu-_data_start))&3
    %elif V7X86
      %if _data_endu==_data_start && _rodata_endu==_rodata_start
        _rodatastr_endalign equ 0
      %else
        _rodatastr_endalign equ (-(_text_endu-_text_start)-(_rodatastr_endu-_rodatastr_start))&3
      %endif
      _rodata_endalign equ 0  ; .data has align=1 valign=0x1000, no need to add any alignment before .data to the file.
      _data_endalign equ (-(_data_endu-_data_start))&3
    %elif XV6I386
      %if _data_endu==_data_start && _rodata_endu==_rodata_start
        _rodatastr_endalign equ 0
      %else
        _rodatastr_endalign equ (-(_text_endu-_text_start)-(_rodatastr_endu-_rodatastr_start))&3
      %endif
      %if _data_endu==_data_start
        _rodata_endalign equ 0
      %else
        _rodata_endalign equ (-(_text_endu-_text_start)-(_rodatastr_endu+_rodatastr_endalign-_rodatastr_start)-(_rodata_endu+_rodata_endalign-_rodata_start))&3
      %endif
      _data_endalign equ (-(_text_endu-_text_start)-(_rodatastr_endu+_rodatastr_endalign-_rodatastr_start)-(_rodata_endu+_rodata_endalign-_rodata_start)-(_data_endu-_data_start))&3
    %elif DOSCOM+DOSEXE
      %if DOSEXE
        _startsec_start equ 0  ; Fake.
        _startsec_endu  equ 0  ; Fake.
      %endif
      %if _data_endu==_data_start && _rodata_endu==_rodata_start
        _rodatastr_endalign equ 0
      %else
        _rodatastr_endalign equ (-(_startsec_endu-_startsec_start+_text_endu-_text_start)-(_rodatastr_endu-_rodatastr_start))&1
      %endif
      %if _data_endu==_data_start
        _rodata_endalign equ 0
      %else
        _rodata_endalign equ (-(_startsec_endu-_startsec_start+_text_endu-_text_start)-(_rodatastr_endu+_rodatastr_endalign-_rodatastr_start)-(_rodata_endu+_rodata_endalign-_rodata_start))&1
      %endif
      _data_endalign equ (-(_startsec_endu-_startsec_start+_text_endu-_text_start)-(_rodatastr_endu+_rodatastr_endalign-_rodatastr_start)-(_rodata_endu+_rodata_endalign-_rodata_start)-(_data_endu-_data_start))&1
    %elif ELF
      %if _data_endu==_data_start && _rodata_endu==_rodata_start
        _rodatastr_endalign equ 0
      %else
        _rodatastr_endalign equ (-(_text_endu-_text_start)-(_rodatastr_endu-_rodatastr_start))&3
      %endif
      _rodata_endalign equ 0  ; _datauna_endalign below will take care of aligning .data.
      %if _data_endu==_data_start
        _datauna_endalign equ 0
      %else
        _datauna_endalign equ (-(_text_endu-_text_start)-(_rodatastr_endu+_rodatastr_endalign-_rodatastr_start)-(_rodata_endu+_rodata_endalign-_rodata_start)-(_datauna_endu-_datauna_start))&3
      %endif
      _data_endalign equ (-(_text_endu-_text_start)-(_rodatastr_endu+_rodatastr_endalign-_rodatastr_start)-(_rodata_endu+_rodata_endalign-_rodata_start)-(_datauna_endu+_datauna_endalign-_datauna_start)-(_data_endu-_data_start))&3
    %endif
    %if DOSCOM
      section .startsec
      _startsec_end:
      _startsec_size equ $-$$
    %else
      _startsec_size equ 0
    %endif
    section .text
    _text_end:
    _text_size equ $-_text_start  ; $$ is different from _text_start for MINIX2I386+MINIX386VM. _text_start has the correct value.
    section .rodata.str
    times _rodatastr_endalign db 0
    _rodatastr_end:
    _rodatastr_size equ $-$$
    section .rodata
    times _rodata_endalign db 0
    _rodata_end:
    _rodata_size equ $-$$  ; _rodata_size is a multiple of 4 or 2.
    %if ELF
      section .data.una
      times _datauna_endalign db 0
      _datauna_end:
      _datauna_size equ $-$$
    %else
      _datauna_size equ 0
    %endif
    section .data
    %if COFF
      _data_endalign_extra equ 0
      times _data_endalign db 0
    %else  ; For non-COFF, it's OK that the end of .data is not padded with NUL bytes for alignment.
      _data_endalign_extra equ _data_endalign
    %endif
    _data_end:
    _data_size equ $-$$  ; _data_size is a multiple of 4 or 2.
    section .bss
    %if ELF && IS_OSCOMPAT_ADDED==0
      %define IS_OSCOMPAT_ADDED 1
      global @oscompat
      @oscompat: resb 1  ; Initial value doesn't matter.  Put it to the end of the .bss, because other parts may need larger alignment.
    %endif
    %if COFF
      %if _text_size+_rodatastr_size+_rodata_size+_data_size+($-$$)<0x1000
        resb 0x1000-(_text_size+_rodatastr_size+_rodata_size+_data_size+($-$$))  ; Workaround to avoid the `<3>mm->brk does not lie within mmap' warning in ibcs-us 4.1.6.
      %endif
    %endif
    resb ($$-$)&(__prog_general_alignment-1)  ; Align end of section to a multiple of 2 or 4.
    _bss_end:
    _bss_size equ $-$$  ; _bss_size is a multiple of 4 or 2.
    section .text
    %if DOSEXE
      __prog_dosexe_file_image_size equ _text_size+_rodatastr_size+_rodata_size+_data_size
      __prog_para_count_from_psp equ (0x100+_text_size+_rodatastr_size+_rodata_size+_data_size+_data_endalign_extra+_bss_size+DOSEXE_STACK_SIZE+0xf)>>4
      __prog_dosexe_offset_limit equ 0x100+_text_size+_rodatastr_size+_rodata_size+_data_size+_data_endalign_extra+_bss_size+DOSEXE_STACK_SIZE
      ; This based on the formula used by MS-DOS 6.22, FreeDOS 1.2, DOSBox 0.74-4 and kvikdos. The .lastsize (modulo 0x1ff) value doesn't matter.
      __prog_dosexe_minalloc0 equ ((_text_size+_rodatastr_size+_rodata_size+_data_size+_data_endalign_extra+_bss_size+DOSEXE_STACK_SIZE+0xf)>>4)-((__prog_dosexe_file_image_size+(__prog_dosexe_hdrsize<<4)+0x1ff)>>9<<5)+__prog_dosexe_hdrsize
      %if __prog_dosexe_minalloc0<1  ; The values 0 and -1 == 0xffff are special. Just use 1 instead of them.
        __prog_dosexe_minalloc equ 1
      %else
        __prog_dosexe_minalloc equ __prog_dosexe_minalloc0
      %endif
      %if __prog_dosexe_offset_limit>0x10000
        %error ERROR_DOSEXE_MEMORY_USAGE_TOO_LARGE
        times -1 nop
      %endif
    %elif DOSCOM
      %if 0x100+_text_size+_rodatastr_size+_rodata_size+_data_size+_data_endalign_extra+_bss_size+0xc0>0x10000  ; 0x100 is the size of the PSP, 0xc0 is reserved for the stack, based on https://retrocomputing.stackexchange.com/a/31813
        %error ERROR_DOSCOM_MEMORY_USAGE_TOO_LARGE
        times -1 nop
      %endif
    %endif
    %if MINIX2I386+MINIX386VM
      _data_start_pagemod equ _rodatastr_size+_rodata_size
    %elif S386BSD+V7X86
      _data_start_pagemod equ 0
    %else
      _data_start_pagemod equ _startsec_size+_text_size+_rodatastr_size+_rodata_size+_datauna_size
    %endif
    %if (_data_start_pagemod&(__prog_general_alignment-1)) && _data_size
      %assign ERROR_VALUE1 _startsec_size
      %assign ERROR_VALUE2 _text_size
      %assign ERROR_VALUE3 _rodatastr_size
      %assign ERROR_VALUE4 _rodata_size
      %assign ERROR_VALUE5 _datauna_size
      %if ELF
        %assign ERROR_VALUE6 _datauna_endalign
      %else
        %assign ERROR_VALUE6 0
      %endif
      %error ERROR_DATA_NOT_ALIGNED ERROR_VALUE1 ERROR_VALUE2 ERROR_VALUE3 ERROR_VALUE4 ERROR_VALUE5 ERROR_VALUE6
      times -1 nop
    %endif
    _bss_start_pagemod equ _data_start_pagemod+_data_size+_data_endalign_extra
    %if _bss_start_pagemod&(__prog_general_alignment-1)
      %error ERROR_BSS_NOT_ALIGNED
      times -1 nop
    %endif
    %if COFF
      _coff_text_fofs equ 0
      _coff_data_fofs equ _text_size+_rodatastr_size+_rodata_size
      _coff_comment_fofs equ _coff_data_fofs+_data_size
      section .comment
      %ifndef COFF_PROGRAM_NAME
        %define COFF_PROGRAM_NAME 'prog'  ; Just a default.
      %endif
      db '@.@(#) ', COFF_PROGRAM_NAME, 0  ; Fake version comment.
      times ($$-$)&3 db 0  ; Align end of section to a multiple of 4.
      _comment_end:
      _comment_size equ $-$$  ; _comment_size is a multiple of 4.
      _data_vstart equ _coff_data_base_org+_coff_data_fofs
    %endif
    %if ELF
      _data_vstart equ _base_org+(_text_size+_rodatastr_size+_rodata_size)+(-(_text_size+_rodatastr_size+_rodata_size)&0xfff)+((_text_size+_rodatastr_size+_rodata_size)&0xfff)  ; Can be ...+...+0+0, but typically 0x1000.
    %endif
    %if ELF+S386BSD
      %if _bss_size+_bss_start_pagemod>(_bss_start_pagemod+0xfff)&~0xfff
        _bss_end_of_first_page equ _bss_start+(-_bss_start_pagemod&0xfff)  ; This can be _bss.
      %else
        _bss_end_of_first_page equ _bss_start+_bss_size  ; This is not page-aligned, and the Linux mmap(...) will fail. But that's OK, because it has nothing to do in that case.
      %endif
    %endif
  %endm
%elif __PROG_IS_OBJ_OR_ELF  ; Output of NASM will be sent to a linker: OpenWatcom wlink(1) (tested with `nasm -f obj' only) or GNU ld(1) (untested).
  %if ELF+WIN32WL==0
    %error ERROR_OUTPUT_FORMAT_OBJ_NEEDS_ELF_OR_WIN32_EXECUTABLE
    times -1 nop
  %endif
  ; We must have all `extern' directives above the first `section'
  ; directive, otherwise the OpenWatcom wlink(1) would misunderstand the
  ; output of `nasm -f obj' for this file, and wlink(1) would generate an
  ; incorrect each time these extern symbols are referenced.
  extern _edata  ; Both OpenWatcom wlink(1) and GNU ld(1) generate the symbol _edata. OpenWatcom wlink(1) also generates __edata, and GNU ld(1) also generates edata, with the same value.
  %define _bss_start _edata
  extern _end
  %define _bss_end _end  ; Both OpenWatcom wlink(1) and GNU ld(1) generate the symbol _end. OpenWatcom wlink(1) also generates __end, and GNU ld(1) also generates end, with the same value.

  %ifidn __OUTPUT_FORMAT__, obj
    ; !! Group CONST with _TEXT. OpenWatcom wlink(1) groups CONST and CONST2
    ;    with _DATA in the output PT_LOAD, rather than _TEXT. Grouping with
    ;    _TEXT is mor common on Linux, and for some programs, _DATA may
    ;    remain empty.
    %define __FILESECTIONNAME__TEXT _TEXT
    %define __FILESECTIONNAME_CONST CONST
    %define __FILESECTIONNAME_CONST2 CONST2
    %define __FILESECTIONNAME__DATA _DATA
    %define __FILESECTIONNAME__DATAUNA _DATA
    %define __FILESECTIONNAME__BSS _BSS
    section _TEXT  USE32 class=CODE align=1
    section CONST  USE32 class=DATA align=1
    section CONST2 USE32 class=DATA align=4
    section _DATA  USE32 class=DATA align=4
    section _BSS   USE32 class=BSS  align=4 NOBITS
    group DGROUP CONST CONST2 _DATA _BSS
    section _TEXT
  %else
    %define __FILESECTIONNAME__TEXT .text
    %define __FILESECTIONNAME_CONST .rodata.str
    %define __FILESECTIONNAME_CONST2 .rodata
    %define __FILESECTIONNAME__DATA .data
    %define __FILESECTIONNAME__DATAUNA .data.una
    %define __FILESECTIONNAME__BSS .bss
    section .text align=1
    section .rodata.str align=1
    section .rodata align=4
    section .data align=4
    section .data.una align=1
    section .bss align=4 nobits
    section .text
  %endif

  %macro f_end 0
    %if ELF && IS_OSCOMPAT_ADDED==0
      __prog_section _BSS
      %define IS_OSCOMPAT_ADDED 1
      global @oscompat
      @oscompat: resb 1  ; Initial value doesn't matter.  Put it to the end of the .bss, because other parts may need larger alignment.
    %endif
    __prog_section _TEXT
  %endm
%else
  %error ERROR_UNSUPPORTED_OUTPUT_FORMAT __OUTPUT_FORMAT__
  times -1 nop
%endif
%define IS_OSCOMPAT_ADDED 0
; The definitions below include aliases.
%define __SECTIONNAME__TEXT _TEXT
%define __SECTIONNAME_TEXT _TEXT
%define __SECTIONNAME_CODE _TEXT
%define __SECTIONNAME_CONST CONST
%define __SECTIONNAME_RODATASTR CONST
%define __SECTIONNAME_CONST2 CONST2
%define __SECTIONNAME_RODATA CONST2
%define __SECTIONNAME__DATAUNA _DATAUNA
%define __SECTIONNAME__DATA _DATA
%define __SECTIONNAME__BSS _BSS
%define __SECTIONNAME_DATAUNA _DATAUNA
%define __SECTIONNAME_DATA _DATA
%define __SECTIONNAME_BSS _BSS
%define __SECTIONNAME_.text _TEXT
%define __SECTIONNAME_.code _TEXT
%define __SECTIONNAME_.rodata.str CONST
%define __SECTIONNAME_.rodatastr CONST
%define __SECTIONNAME_.rodata CONST2
%define __SECTIONNAME_.data.una _DATAUNA
%define __SECTIONNAME_.datauna _DATAUNA
%define __SECTIONNAME_.data _DATA
%define __SECTIONNAME_.bss _BSS
%define __SECTIONNAME_text _TEXT
%define __SECTIONNAME_code _TEXT
%define __SECTIONNAME_rodatastr CONST
%define __SECTIONNAME_rodata CONST2
%define __SECTIONNAME_datauna _DATAUNA
%define __SECTIONNAME_data _DATA
%define __SECTIONNAME_bss _BSS
; `__prog_section _TEXT' is safer against typos in the NASM code than
; `__prog_section__TEXT', because NASM silently ignores the latter (or just
; displays a warning with `nasm -w+orphan-labels') if there is a typo in
; `_TEXT'.
%macro __prog_section 1
  %undef align
  %idefine align ,  ; Convert `__prog_section CONST align=1' to `__prog_section CONST ,1' so that `__prog_section_low 2+' below can catch the error.
  __prog_section_low %1
  %undef align
%endm
%macro __prog_section 0
  %error ERROR_MISSING_SECTION_NAME  ; Solution: Add an argument to `section', e.g. `section .rodata.str'.
  times -1 nop
%endm
%macro __prog_section 2+
  %error ERROR_TOO_MANY_ARGUMENTS_TO_SECTION
  times -1 nop
%endm
%macro __prog_section_low 1
  %ifdef __SECTIONNAME_%1
    __prog_section_low2 __SECTIONNAME_%1  ; Expands __SECTIONNAME_%1 first. This does canonicalization: it converts section name aliases to canonical section names.
  %else
    %error ERROR_UNKNOWN_SECTION_%1  ; This indicates a bug in the user program: trying to use an unknown section, maybe a typo.
    times -1 nop
  %endif
%endm
%macro __prog_section_low 2
  %error ERROR_SPECIFY_SECTION_WITHOUT_ALIGNMENT %1  ; progx86.nasm has set up alignment properly for all sections. Changing it would make things fall apart.
  times -1 nop
%endm
%macro __prog_section_low2 1
  %ifdef __FILESECTIONNAME_%1
    %undef section  ; So that the `section ...' directive below does the NASM default.
    section __FILESECTIONNAME_%1  ; The expansion of __FILESECTIONNAME_%1 depends on the executable file format.
    %idefine section __prog_section
  %else
    %error ERROR_UNKNOWN_FILE_SECTION_%1  ; This indicates a bug in progx86.nasm.
    times -1 nop
  %endif
%endm
%define PROG_SECTIONS_DEFINED PROG_SECTIONS_DEFINED  ; The program source files will notice as `%ifdef PROG_SECTIONS_DEFINED'.
__prog_section _TEXT

; --- Including the program source files.
;
; Example invocation: nasm -DINCLUDES="'mysrc1_32.nasm','mysrc2_32.nasm'"

%ifndef INCLUDES
  %error ERROR_MISSING_INCLUDES  ; Specify it like this: nasm -DINCLUDES="'f1.nasm','f2.nasm'"
  times -1 nop
%endif
%macro __prog_force_extern_in_text 1
  %ifnidn __OUTPUT_FORMAT__, bin
    %undef extern  ; So that the `extern ...' directive below does the NASM default.
    ; This breaks the ELF output of OpenWatcom wlink(1) if used for extern
    ; symbols outside .text. To make it work for symbols everywhere, we'd
    ; have to move this NASM `extern' directive above the first `section'
    ; definition.
    extern %1
    %idefine extern __prog_extern
  %endif
%endm
%macro __prog_extern_in_text 1
  %if __GLOBAL_%1+0==2
    %undef extern  ; So that the `extern ...' directive below does the NASM default.
    ; This breaks the ELF output of OpenWatcom wlink(1) if used for extern
    ; symbols outside .text. To make it work for symbols everywhere, we'd
    ; have to move this NASM `extern' directive above the first `section'
    ; definition.
    extern %1
    %idefine extern __prog_extern
  %endif
%endm
%macro __prog_global 1
  %ifdef __GLOBAL_%1
    %error ERROR_MULTIPLE_DEFINITIONS_FOR_%1
    times -1 nop
  %endif
  %define __GLOBAL_%1
  %undef global  ; So that the `global ...' directive below does the NASM default.
  global %1  ; Adds the symbol to the object file (for __OUTPUT_FORMAT__ elf and obj).
  %idefine global __prog_global
%endm
%idefine global __prog_global  ; Like the NASM default `global', but fails on multiple definitions of the same symbol.
%ifndef MULTIPLEOBJS  ; Usually true.
  %macro __prog_extern 1
    %define __NEED_%1
  %endm
  %idefine extern __prog_extern  ; Like the NASM default `extern', but with different functionality.
%endif
%macro __prog_do_includes 0-*
  %rep %0
    %ifnidn (%1), ()  ; This also does some of the `%define __NEED_...'.
      __prog_default_cpu_and_bits
      __prog_section CONST2
      times ($$-$)&(__prog_general_alignment-1) db 0  ; Align to a multiple of 2 or 4. We do this to provide proper alingment for the %included() .nasm files.
      __prog_section _DATA
      times ($$-$)&(__prog_general_alignment-1) db 0  ; Align to a multiple of 2 or 4. We do this to provide proper alingment for the %included() .nasm files.
      __prog_section _BSS
      resb ($$-$)&(__prog_general_alignment-1)  ; Align to a multiple of 4. We do this to provide proper alingment for the %included() .nasm files.
      __prog_section _TEXT
      %include %1
    %endif
    %rotate 1
  %endrep
%endmacro
__prog_default_cpu_and_bits
__prog_do_includes INCLUDES  ; This also does all the `%define __NEED_...'.
__prog_default_cpu_and_bits
__prog_section _TEXT

; --- Function and global variable dependency analysis.

%macro __prog_libc_declare 1
  %ifdef __NEED_G@%1
    %define __NEED_%1
  %endif
%endm
%macro __prog_libc_export 0+  ; Usage: myfunc: __prog_libc_export 'int myfunc(void);'
  global G@%00
  G@%00:  ; Do this first, so that the .sub labels inherit from those without G@.
  global %00  ; Use the label in front of the macro.
  %00:
%endm
%macro __prog_libc_export_alias 1+  ; Usage: myfunc: __prog_libc_export myfunc_orig, 'int myfunc(void);'
  global G@%00
  G@%00: equ %1  ; Do this first, so that the .sub labels inherit from those without G@.
  global %00  ; Use the label in front of the macro.
  %00: equ %1
%endm
%macro __prog_depend 2
  %ifdef __NEED_%1
    %define __NEED_%2
  %endif
%endm

; For each `%ifdef __NEED_...' below, we need `__prog_libc_declare ...' here.
__prog_libc_declare _cstart_
__prog_libc_declare __argc
__prog_libc_declare exit_
__prog_libc_declare _exit
__prog_libc_declare _exit_
__prog_libc_declare __exit
__prog_libc_declare _read
__prog_libc_declare read_
__prog_libc_declare read_DOSREG
__prog_libc_declare _write
__prog_libc_declare _write_binary
__prog_libc_declare _write_nonzero
__prog_libc_declare _write_nonzero_binary
__prog_libc_declare write_
__prog_libc_declare write_binary_
__prog_libc_declare write_nonzero_
__prog_libc_declare write_nonzero_binary_
__prog_libc_declare write_nonzero_binary_DOSREG
__prog_libc_declare _isatty
__prog_libc_declare isatty_
__prog_libc_declare isatty_DOSREG
__prog_libc_declare _setmode
__prog_libc_declare setmode_
__prog_libc_declare _strlen
__prog_libc_declare strlen_
__prog_libc_declare _memset
__prog_libc_declare memset_
__prog_libc_declare _memcpy
__prog_libc_declare memcpy_
__prog_libc_declare progx86_para_alloc_
__prog_libc_declare progx86_para_reuse_
__prog_libc_declare progx86_main_returns_void

__prog_depend _exit, __exit  ; Will be repeated below to get more dependencies.
__prog_depend exit_, _exit_  ; Will be repeated below to get more dependencies.
%if DOSCOM+DOSEXE
  __prog_depend _exit, exit_
  __prog_depend __exit, _exit_
%else
  %define __NEED_exit_   ; After main(...) returns.
%endif
__prog_depend _exit, __exit  ; Repeat it.
__prog_depend exit_, _exit_  ; Repeat it.
__prog_depend _isatty, isatty_
%if XV6I386+WIN32WL+DOSCOM+DOSEXE==0
  __prog_depend isatty_, @ioctl_wrapper
%endif
__prog_depend _write, _write_binary
__prog_depend _write_nonzero, _write_nonzero_binary
__prog_depend write_, write_binary_
__prog_depend write_nonzero_, write_nonzero_binary_
%if DOSCOM+DOSEXE==0
  __prog_depend _write_nonzero_binary, _write_binary
  __prog_depend write_nonzero_, write_
%endif
%if DOSCOM+DOSEXE+WIN32WL==0
  __prog_depend _write_nonzero, _write
  __prog_depend write_nonzero_binary_, write_binary_
%endif
%if WIN32WL
  %macro __prog_kimport 3
    %ifdef __NEED_%1
      __prog_kimport_low %2, _%2@%3
    %endif
  %endm
  %macro __prog_kimport_low 2
    %ifndef __IMPORTED%2
      %define __IMPORTED%2  ; TODO(pts): Detect duplicates of %1 (without the argument byte count).
      __prog_force_extern_in_text %2
      import %2 kernel32.dll %1
    %endif
  %endm
  __prog_depend _read, @read_write_helper
  __prog_depend _write, @read_write_helper
  __prog_depend _write_binary, @read_write_helper
  __prog_depend _read, @fd_handles
  __prog_depend _write_binary, @fd_handles
  __prog_depend _isatty, @fd_handles
  __prog_depend isatty_, @fd_handles
  __prog_depend _setmode, @fd_handles
  __prog_depend setmode_, @fd_handles
  __prog_depend @fd_handles, @fd_modes
  __prog_kimport @fd_handles, GetStdHandle, 4
  __prog_kimport _exit_, ExitProcess, 4
  __prog_kimport __exit, ExitProcess, 4
  __prog_kimport __argc, GetCommandLineA, 0
  __prog_kimport _read, ReadFile, 20
  __prog_kimport _write, WriteFile, 20
  __prog_kimport _isatty, GetFileType, 4
  __prog_kimport isatty_, GetFileType, 4
%endif
%ifdef __NEED_progx86_para_alloc_
  %ifdef __NEED_progx86_para_alloc_
    %error ERROR_CONFLICTING_PARA_REUSE_AND_ALLOC
    times -1 nop
  %endif
%endif
%if ELF  ; For OpenBSD syscall pinning.
  __prog_depend _exit, @SYS.exit
  __prog_depend _read, @SYS.read
  __prog_depend _write, @SYS.write
  __prog_depend _write_binary, @SYS.write
  __prog_depend _isatty, @SYS.ioctl
  __prog_depend exit_, @SYS.exit
  __prog_depend read_, @SYS.read
  __prog_depend write_, @SYS.write
  __prog_depend write_binary_, @SYS.write
  __prog_depend isatty_, @SYS.ioctl
%endif

; --- Defines specific to the operating system.

SYS:
%if XV6I386  ; xv6-i386 syscall numbers, from syscall.h
.exit equ 2
.read equ 5
.write equ 16
%else  ; Linux i386, FreeBSD i386, NetBSD i386, OpenBSD i386, v7x86, SysV SVR3 i386, SysV SVR4 i386, iBCS2, 386BSD, Minix 1.5--3.2.x (but not Minix 3.2.x) syscall numbers.
.exit equ 1  ; MM in Minix.
.read equ 3  ; FS in Minix.
.write equ 4  ; FS in Minix.
.ioctl equ 54  ; FS in Minix.
%endif

%if MINIX2I386+MINIX386VM
MINIX_WHO:  ; include/lib.h . Minix syscall ``who'' subsystems.
.MM equ 0
.FS equ 1
%endif

IOCTL_Linux:  ; Linux i386.
.TCGETS         equ 0x5401  ; sizeof(struct termios) == 36 == 0x24. isatty(3) in minilibc686, uClibc and dietlibc use TCGETS.
.TCGETA         equ 0x5405  ; sizeof(struct termio) == 26 == 0x1a.
.TCGETS2        equ 0x542a  ; sizeof(struct termios2) == 44 == 0x2c.
.TIOCGETD       equ 0x5424  ; Get the line discipline. sizeof(int) == 4.
.TIOCGPGRP      equ 0x540f
.TIOCGWINSZ     equ 0x5413
.TIOCGSOFTCAR   equ 0x5419
.TIOCGSERIAL    equ 0x541e
.TIOCGSID       equ 0x5429  ; Return the session ID of FD
.TIOCGRS485     equ 0x542e
;.TIOCGPTN      _IOR('T', 0x30, unsigned int)  ; Get Pty Number (of pty-mux device)
;.TIOCGDEV      _IOR('T', 0x32, unsigned int)  ; Get primary device node of /dev/console
;.TIOCGPKT      _IOR('T', 0x38, int) ;  Get packet mode state
;.TIOCGPTLCK    _IOR('T', 0x39, int) ;  Get Pty lock state
;.TIOCGEXCL     _IOR('T', 0x40, int) ;  Get exclusive mode state
;.TIOCGPTPEER   _IO('T', 0x41)  ; Safely open the slave
.TIOCGLCKTRMIOS equ 0x5456
.TIOCGICOUNT    equ 0x545d  ; read serial port __inline__ interrupt counts
;.TIOCGETA  ; Missing.
;.TIOCFETP  ; Missing.

IOCTL_FreeBSD:  ; FreeBSD 3.5 i386.
.TIOCGETA       equ 0x402c7413  ; _IOR('t', 19, struct termios). Get termios struct. sizeof(struct termios) == 44 == 0x2c. isatty(3) uses TIOCGETA.
.TIOCGDRAINWAIT equ 0x40047456  ; _IOR('t', 86, int). Get ttywait timeout. sizeof(int) == 4.
.TIOCGETC       equ 0x40067412  ; _IOR('t', 18, struct tchars). Get special characters. sizeof(struct tchars) == 6.
.TIOCGETD       equ 0x4004741a  ; _IOR('t', 26, int). Get line discipline. sizeof(int) == 4.
.TIOCGETP       equ 0x40067408  ; _IOR('t', 8, struct sgttyb). Get parameters -- gtty. sizeof(struct sgttyb) == 6.
.TIOCGLTC       equ 0x40067474  ; _IOR('t', 116, struct ltchars). Get local special chars. sizeof(struct ltchars) == 6.
.TIOCGPGRP      equ 0x40047477  ; _IOR('t', 119, int). Get pgrp of TTY. sizeof(int) == 4.
.TIOCGWINSZ     equ 0x40087468  ; _IOR('t', 104, struct winsize). Get window size. sizeof(struct winsize) == 8.
.OTIOCGETD      equ 0x40047400  ; _IOR('t', 0, int). Old get line discipline. sizeof(int) == 4.

IOCTL_NetBSD:  ; NetBSD 1.5.2 i386.
.TIOCGETA       equ 0x402c7413  ; _IOR('t', 19, struct termios). Get termios struct. sizeof(struct termios) == 44 == 0x2c. isatty(3) uses TIOCGETA.

IOCTL_386BSD:  ; 386BSD 1.0 in usr/src/kernel/include/sys/ioctl.h
.TIOCGETA  equ 0x402c7413  ; _IOR('t', 19, struct termios). Get termios struct. sizeof(struct termios) == 44 == 0x2c. isatty(3) uses TIOCGETA.
.TIOCFLUSH equ 0x802c7415  ; _IOW('t', 16, int). flush buffers
.TIOCSETAW equ 0x802c7415  ; _IOW('t', 21, struct termios). drain output, set
.TIOCGPGRP equ 0x40047477  ; _IOR('t', 119, int). get pgrp of tty
.TIOCSPGRP equ 0x80047476  ; _IOW('t', 118, int). set pgrp of tty
.TIOCSBRK  equ 0x2000747b  ; _IO('t', 123). set break bit
.TIOCCBRK  equ 0x2000747a  ; _IO('t', 122). clear break bit
.TIOCDRAIN equ 0x2000745e  ; _IO('t', 94). wait till output drained
.TIOCSTART equ 0x2000746e  ; _IO('t', 110). start output, like ^Q
.TIOCGETD  equ 0x4004741a  ; _IOR('t', 26, int). get line discipline
.TIOCGETP  equ 0x40067408  ; _IOR('t', 8, struct sgttyb). Get parameters -- gtty. sizeof(struct sgttyb) == 6.
.TIOCGETC  equ 0x40067412  ; _IOR('t', 18, struct tchars). Get special characters. sizeof(struct tchars) == 6.
.OTIOCGETD equ 0x40047400  ; _IOR('t', 0, int)  ; old get line discipline
;.TIOCGETP  ; Missing.

IOCTL_SysV:  ; AT&T Unix SysV SVR3 i386 and SVR4 i386.
.TCGETA     equ 0x5401  ; SVR3 and SVR4. sizeof(struct termio) == 18. isatty(3) uses TCGETA.
.TIOCGETP   equ 0x7408  ; sizeof(struct sgttyb) == 8.
.TIOCGWINSZ equ 0x5468  ; sizeof(struct winsize) == 8.
.TCGETS     equ 0x540d  ; SVR4 only. sizeof(struct termios) == 36 == 0x2c.
.TCGETX     equ 0x5801  ; SVR4 only. sizeof(struct termiox) == 16.
.TIOCGPGRP  equ 0x7414  ; SVR4 only. Get process group of TTY.
.TIOCGSID   equ 0x7416  ; SVR4 only. Get session ID of CTTY.
.TIOCGETD   equ 0x7400  ; SVR4 only. Get line discipline. sizeof(int) == 4.
.TIOCGETC   equ 0x7412  ; SVR4 only. sizeof(struct ltchars) == 6.
.TIOCGLTC   equ 0x7474	; SVR4 only. Get local special chars. sizeof(struct ltchars) == 6.
.TIOCLGET   equ 0x747c  ; SVR4 only. Get local modes. sizeof(int) == 4.

IOCTL_Coherent4:  ; Coherent 4.x i386. Similar to SysV SVR3.
.TCGETA     equ 0x5401  ; Get terminal parameters. sizeof(struct termio) == 18.
.TIOCGETP   equ 0x7408  ; Terminal get modes (old gtty). sizeof(struct sgttyb) == 8. isatty(3) uses TIOCGETP.
.TIOCGWINSZ equ 0x5468  ; Get window size. sizeof(struct winsize) == 8.
.TIOCGETC   equ 0x7412  ; Get characters. sizeof(struct tchars) == 6. SVR3 doesn't have it.

IOCTL_Minix2:  ; Minix 1.7.4--1.7.5--2.0.4--3.2.0 i386. Minix for i86 has the low 16 bits of these only.
.TCGETS     equ 0x80245408  ; _IOR('T',  8, struct termios). sizeof(struct termios) == 36 == 0x24. isatty(3) uses TCGETS.
.TIOCGWINSZ equ 0x80085410  ; _IOR('T', 16, struct winsize). sizeof(struct sinsize) == 8.
.TIOCGPGRP  equ 0x40045412  ; _IOW('T', 18, int). There is a bug, it should be _IOR. sizeof(int) == 4.
.TIOCGETP   equ 0x80087401  ; _IOR('t',  1, struct sgttyb). sizeof(struct sgttyb) == 8.
.TIOCGETC   equ 0x80067403  ; _IOR('t',  3, struct tchars). sizeof(struct tchars) == 6.

IOCTL_Minix33:  ; Minix 3.3.0 i386.
.TIOCGETA equ 0x402c7413  ; sizeof(struct termios) == 44 == 0x2c. iastty(3) uses this. Same value for TIOCGETA as on FreeBSD, NetBSD and 386BSD.

IOCTL_Minix1.5:  ; Minix 1.5 i86 and i386.
.TCGETS     equ 0x7408  ; There is no struct pointer passed in Minix 1.5. isatty(3) uses TCGETS.

IOCTL_Minix1.7:  ; Minix 1.7.0--1.7.2 i86 and i386.
.TIOCGETP   equ 0x7408  ; There is no struct pointer passed in Minix 1.7.0. isatty(3) uses TIOCGETP.

IOCTL_v7x86:  ; v7x86 0.8a in usr/include/sgtty.h
.TIOCGETP equ 0x7408  ; (('t'<<8)|8).  sizeof(struct sgttyb) == 8. There is no `struct termio' or `struct termios' in Unix V7.
.TIOCSETP equ 0x7409  ; (('t'<<8)|9).  sizeof(struct sgttyb) == 8.
.TIOCSETC equ 0x7411  ; (('t'<<8)|17).  sizeof(struct tchars) == 6.
.TIOCGETC equ 0x7412  ; (('t'<<8)|18).  sizeof(struct tchars) == 6.
.TIOCGETD equ 0x7400  ; (('t'<<8)|0).  sizeof(int) == 1.
.TIOCSETD equ 0x7401  ; (('t'<<8)|`).  sizeof(int) == 1.

IOCTL_iBCS2:  ; FreeBSD 3.5 has these in src/sys/i386/ibcs2/ibcs2_termios.h
.TCGETA     equ 0x5401  ; sizeof(struct termio) == 18.
.TIOCGWINSZ equ 0x5468  ; sizeof(struct winsize) == 8.
.TIOCGPGRP  equ 0x5477  ; sizeof(int) == 4.
.TCGETSC    equ 0x5422  ; Get scan codes.
.XCGETA     equ 0x695801  ; sizoef(struct termios) == 24. Not implemented in SVR3 or SRV4.
.OXCGETA    equ 0x7801  ; sizoef(struct termios) == 24. Not implemented in SVR3 or SRV4.

%if ELF
  SYS_Linux:  ; Linux i386 syscalls. Syscall input: EAX: syscall number; EBX: arg1; ECX: arg2; EDX: arg3.
  .mmap equ 90

  SYS_Minix330:  ; Minix 3.3.0 i386 syscalls.
  ; Defined in minix/minix/include/minix/callnr.h
  .PM_EXIT   equ 0x1
  .VFS_READ  equ 0x100  ; 0xfd+SYS.read .
  .VFS_WRITE equ 0x101  ; 0xfd+SYS.write .
  .VFS_IOCTL equ 0x118

  MAP_Linux:  ; Symbolic constants for Linux i386 mmap(2).
  .PRIVATE equ 2
  .FIXED equ 0x10
  .ANONYMOUS equ 0x20

  PROT:  ; Symbolic constants for Linux and FreeBSD mmap(2).
  .READ equ 1
  .WRITE equ 2

  OSCOMPAT:  ; Our platform flags. @oscompat is a bitmask of these.
  .LINUX equ 0  ; (All ELF.) Linux i386 native; Linux i386 running on Linux amd64; Linux (any architecture) qemu-i386; Linux i386 ibcs-us ELF.
  .SYSV equ 1  ; AT&T Unix System V/386 (SysV) Release 3 (SVR3) COFF; AT&T Unix System V/386 (SysV) Release 4 (SVR4) ELF; Coherent 4.x COFF; iBCS2 COFF; Linux i386 ibcs-us COFF.
  .MANYBSD equ 2  ; (All ELF.) FreeBSD, NetBSD or OpenBSD i386.
  .MINIX32 equ 4  ; Minix 3.2.0.
  .MINIX33 equ 6  ; Minix 3.3.0.
  .MINIX3_MASK equ 4  ; Bit set for both .MINIX32 and .MINIX33.
  .MINIX33_MANYBSD_MASK equ 2  ; Bit set for both .MANYBSD and .MINIX33.

  ; Initial Unix process state:
  ; * The GPRs (general-purpose registers) are EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI.
  ; * From top of stack (ESP): argc, argv[0], ..., argv[argc-1], NULL, envp[0], ... envp[envc-1], NULL, afterenv.
  ; * .LINUX: afterenv is auxt[0] (non-NULL) since Linux 1.0; EDX == 0; ESP is set as above; other GPRs are either arbitrary (for Linux <2.2) or 0 (for Linux >=2.2) (https://asm.sourceforge.net/articles/startup.html and https://stackoverflow.com/q/9147455)
  ; * .MANYBSD: afterenv is auxt[0] (non-NULL); ESP is set as above; treat other GPRs as arbitrary; distinguish it from Linux by: if write(-1, "", 0) returns a negative errno value, then it's Linux, otherwise it's FreeBSD or NetBSD; don't chck CF, earlier versions of Linux may also set it to 1
  ; * .MINIX32: afterenv is the argv[0] string, i.e. argv[0] == &afterenv; ESP is set as above (a bit below 0x7ffffffc); ECX == 3 (the code of SENDREC from the previous program); EAX == 0 (is it always 0?); other GPRs are arbitrary (mostly inherited from the previous program)
  ; * .MINIX33: afterenv is auxt[0] == NULL; ESP is set as above (a bit below EBX); EBX is 0xeffffff0; other GPRs are 0
  ; * .SVR4: afterenv is the argv[0] string, i.e. argv[0] == &afterenv; ESP is set as above (a bit below the first PT_LOAD page); EAX == 0; other GPRs are arbitrary (mostly inherited from the previous program).
%endif

%if WIN32WL
  STD_INPUT_HANDLE equ -11
  STD_OUTPUT_HANDLE equ -11
  STD_ERROR_HANDLE equ -12

  ;INVALID_HANDLE_VALUE equ -1
  ;NULL equ 0

  FILE_TYPE_CHAR equ 2  ; For _GetFileType@4.
%endif

%define __PROG_JMP_MAIN 0
%if DOSCOM+DOSEXE
  @cmdline: equ 0x81  ; PSP-relative offset of the CR-terminated command-line string.
  @prog_mem_end_seg: equ 2  ; Offset within the DOS Program Segment Prefix (PSP): https://fd.lod.bz/rbil/interrup/dos_kernel/2126.html
  %ifdef __NEED_progx86_para_alloc_
    @first_empty_seg: equ 0x7e  ; An otherwise unused part of the DOS Program Segment Prefix (PSP): https://fd.lod.bz/rbil/interrup/dos_kernel/2126.html
  %endif
  %ifdef __NEED_progx86_main_returns_void
    %if DOSCOM
      %define __PROG_JMP_MAIN 1  ; When main(...) returns, its `ret' will pop the `0' from the stack, and jumping there does an exit(0).
    %endif
  %endif
%endif

__prog_section _TEXT

; --- Program entry point (_start), system autodetection and exit.

%ifdef __NEED__cstart_  ; If there is a main function, the OpenWatcom C compiler generates this symbol.
  _cstart_: __prog_libc_export '__noreturn __no_return_address void __cdecl cstart_(int argc, ...);'
%endif
%if DOSCOM
  section .startsec  ; We use .startsec to make sure that the code below starts at the beginning of the DOS .com program file, exactly where DOS starts running the program.
%endif
global _start  ; Program entry point. This is a libc implementation detail, we only but still making it global to aid debugging.
_start:  ; __noreturn __no_return_address void __cdecl start(int argc, ...);
		; Now the stack looks like (from top to bottom) for XV6I386+WIN32WL+DOSCOM+DOSEXE==0:
		;   dword [esp]: argc
		;   dword [esp+4]: argv[0] pointer
		;   esp+8...: argv[1..] pointers
		;   NULL that ends argv[]
		;   environment pointers
		;   NULL that ends envp[]
		;   ELF Auxiliary Table (auxv): key-value pairs ending with (AT_NULL (== 0), NULL).
		;   argv strings, environment strings, program name etc.
		; Now the stack looks like (frop top to bottom) for XV6I386:
		;   dword [esp]: exit(...) return address from main
		;   dword [esp+4]: argc
		;   dword [esp+8}: argv
		; Now the stack looks like (from top to bottom) for WIN32WL:
		;   (Nothing useful pushed.)
		; Now the stack looks like (from top to bottom) for DOSCOM:
		;   word [sp]: 0
		; Now the stack looks like (from top to bottom) for DOSEXE (nothing particular).
%if DOSCOM+DOSEXE==0
		cld  ; Not all systems set DF := 0, so we play it safe.
%endif
%if ELF  ; Auto-detect the operating system (OSCOMPAT) for ELF.
		cmp ebx, 0xeffffff0
		jne short .not_minix33
		test eax, eax
		jnz .not_minix33
		test ecx, ecx
		jnz .not_minix33
		test edx, edx
		jnz .not_minix33
		test esi, esi
		jnz .not_minix33
		test edi, edi
		jnz .not_minix33
		test ebp, ebp
		jnz .not_minix33
		jmp short .cont1  ; EBP == 0 indicates possible OSCOMPAT.MINIX33.
  .not_minix33:
		xor ebp, ebp
		inc ebp  ; EBP == 1 indicates that OSCOMPAT.MINIX33 is impossible.
  .cont1:
		; Linux >=1.0 and FreeBSD >=3.5 both set up a non-empty
		; auxv, even for statically linked executable. SVR4 2.1
		; doesn't even add an auxv: the argv[0] string directly
		; follows the NULL after envp.
  %ifdef __NEED___argc  ; This branch keeps argc on the stack. Automatically defined by __WATCOMC__ iif main(...) takes arguments. For __GNUC__, use `nasm -D__NEED___argc'.
		mov eax, [esp]  ; argc.
		lea edi, [esp+eax*4+8]  ; EDI := envp.
		mov ecx, [esp+4]  ; argv[0].
  %else
		pop eax  ; Discard argc. Also ESP := argv.
		lea edi, [esp+eax*4+4]  ; EDI := envp.
		mov ecx, [esp]  ; argv[0].
  %endif
		xor eax, eax
  .next_envvar:
		scasd
		jne short .next_envvar
		; Now: EDI is auxv.
		cmp edi, ecx
		; Jump iff there is no auxv (but argv[0] starts right after
		; the NULL of environ). This happens on SysV SVR4 for
		; statically linked ELF-32 i386 programs, but not on Linux
		; >=1.0.4, ibcs-us (which inherits to auxv from the host
		; Linux or FreeBSD >=3.5), FreeBSD 3.5, NetBSD 1.5.2,
		; OpenBSD 3.4.
		je short .sysv_minix32  ; OSCOMPAT.SYSV or OSCOMPAT.MINIX32.
		test ebp, ebp
		jnz short .not_minix33_minix32_sysv
		mov edx, [edi-2*4]  ; EDX := envp[envc - 1].
		test edx, edx
		jnz short .edx_is_last
		mov edx, [edi-3*4]  ; EDX := argv[argc - 1].
		test edi, edi
		jz short .minix33   ; Both argv and envp are empty, EBX is 0xeffffff0.
  .edx_is_last:
		; Now EDX points to the beginning of envp[envc - 1] (or, if envp is empty, and argv is nonempty, argv[argc  1]).
		mov edi, edx
  .next_char:
		scasb
		jne short .next_char
		times 3 inc edi
		and edi, byte ~3
		cmp edi, ebx  ; EDI == 0xeffffff0 ?
		jne .not_minix33_minix32_sysv
  .minix33:
		push byte OSCOMPAT.MINIX33
  %if 0  ; Just for debugging.
		pop eax
		mov [@oscompat], al  ; We must do this after the clearing of the first page of .bss above.
		mov al, 33
		jmp strict near exit_
  %else
		jmp short .detected
  %endif
  .not_minix33_minix32_sysv:
  %if 0  ; This is a useless check.
		scasd
		; Jump iff the first element of auxv is AT_NULL (== 0). This
		; happens on NetBSD 1.5.2, but not on Linux
		; >=1.0.4, ibcs-us (which inherits to auxv from the host
		; Linux or FreeBSD >=3.5), FreeBSD 3.5. We don't reach this
		; on SysV SVR4, because above we've checked that the auxv is
		; empty.
		je short .auxv_starts_with_at_null
  %endif
  .linux_or_bsd:
		; Now we are running on Linux (OSCOMPAT.LINUX) or some kind
		; of BSD (supported: FreeBSD, NetBSD and OpenBSD)
		; (OSCOMPAT.MANYBSD). Let's distinguish these two cases
		; by attempting a write(-1, NULL, 0), and checking how they
		; return the errno.
		;
		; Please note that some early versions of Linux set CF
		; (carry flag) to 0 after a successful `int 0x80', so that
		; method (with getpid()) wouldn't be a reliable way to
		; detect Linux, because BSDs also set CF to 0.
		push byte SYS.write  ; SYS_write for both Linux i386 and FreeBSD.
		pop eax
		xor edx, edx  ; Argument count of Linux i386 SYS_write.
		push edx  ; Argument count of FreeBSD SYS_write.
		xor ecx, ecx  ; Argument buf of Linux i386 SYS_write.
		push ecx  ; Argument buf of FreeBSD SYS_write.
		or ebx, byte -1  ; Argument fd of Linux i386 SYS_write.
		push ebx  ; Argument fd of FreeBSD SYS_write.
		push eax  ; Fake return address of FreeBSD syscall.
  @syscall_location_write: equ $
		int 0x80  ; Linux i386 or OSCOMAT.MANYBSD syscall. It fails because of the negative fd.
		add esp, byte 4*4  ; Clean up syscall arguments above.
		test eax, eax
		jns short .freenetbsd  ; Jump iff FreeBSD (EAX == EBADF, positive). Linux has EAX == -EBADF, negative here.
		; Now we are running on Linux or Linux ibcs-us.
		; Map .bss again using mmap(2). This is a workaround for ibcs-us 4.1.6 running on Linux i386, which maps pages of .data but not .bss.
		push edx  ;  Argument offset == 0. We've set EDX to 0 above (for argument count of Linux i386 SYS_write).
		push strict byte -1  ; Argument fd.
		push strict byte MAP_Linux.PRIVATE|MAP_Linux.ANONYMOUS|MAP_Linux.FIXED  ; Argument flags.
		push strict byte PROT.READ|PROT.WRITE  ; Argument prot.
  %ifidn __OUTPUT_FORMAT__, bin  ; The subtractions below work only in a single section.
		push strict dword (ERROR_MISSING_F_END+_bss_end-_bss_end_of_first_page+0xfff)&~0xfff  ; Argument length. mmap(2) doesn't need rounding to page boundary, but we do it for extra compatibility.
		push strict dword _bss_end_of_first_page  ; Argument addr. mmap(2) needs rounding to page boundary, and we use a rounded value to avoid overwriting the last page of .data.
  %else
    global _start.startref1
    .startref1:  ; fix_elf_edata.pl will find this symbol by name, and then it will fix the value in the instruction below.
		mov eax, _bss_start  ; Symbol provided by the linker.
		add eax, strict dword 0xfff
		and eax, strict dword ~0xfff  ; EDX := _bss_start rounded up to page boundary.
    global _start.endref1
    .endref1:  ; fix_elf_edata.pl will find this symbol by name, and then it will fix the value in the instruction below.
		mov ecx, _bss_end  ; Symbol provided by the linker.
		sub ecx, eax
		jnc short .mmap_length_ok
		xor ecx, ecx  ; .bss fits within the last page of .data. Argument length := 0.
    .mmap_length_ok:
		push ecx  ; Argument length. mmap(2) doesn't need rounding to page boundary.
		push eax  ; Argument addr. mmap(2) needs rounding to page boundary, and we use a rounded value to avoid overwriting the last page of .data.
  %endif
		mov ebx, esp  ; buffer, to be passed to SYS.mmap as arg1.
		push byte SYS_Linux.mmap
		pop eax
		int 0x80  ; Linux i386 syscall. This fails if length == 0, but that's fine to ignore here.
		add esp, byte 6*4  ; Clean up arguments of SYS.mmap above from the stack.
		push byte OSCOMPAT.LINUX  ; We're running on Linux: either natively or in ibcs-us.
		jmp short .detected
  .freenetbsd:
		push byte OSCOMPAT.MANYBSD
		jmp short .detected
  .sysv_minix32:
  %if 0  ; Not needed, the ESP test below is enough.
		cmp ecx, byte 3
		jne short .sysv
		test eax, eax
		jne short .sysv
  %endif
  %ifidn __OUTPUT_FORMAT__, bin  ; The subtractions below work only in a single section.
		cmp esp, _base_org
  %else
		cmp esp, _start  ; _base_org not defined. _start is a bit larger, but good enough to check here.
  %endif
		jb short .sysv  ; More specifically: For OSCOMPAT.SYSV, envp[envc - 1] rounded up to the multiple of 0x10 is _base_org; for OSCOMPAT.MINIX32, envp[envc - 1] rounded up to the multiple of 0x10 is 0x80000000.
		push byte OSCOMPAT.MINIX32
		jmp short .detected
  .sysv:
		push byte OSCOMPAT.SYSV
  .detected:
%endif  ; Of %if ELF
%if DOSCOM+DOSEXE  ; Clear BSS. DOS doesn't do it for us.
		xor ax, ax
		mov di, _bss_start  ; Already aligned to 2.
		mov cx, (_bss_end-_bss_start+1)>>1
		rep stosw
  %ifdef __NEED_progx86_para_alloc_
		mov ax, cs
		add ax, __prog_para_count_from_psp
		mov [@first_empty_seg], ax
  %endif
%endif
%if ELF+S386BSD  ; Not needed for COFF+MINIX2I386+MINIX386VM+V7X86+XV6I386, because they don't do page-aligned mapping of the executable program file.
		; Now we clear the .bss part of the last page of .data.
		; 386BSD 1.0 and some early Linux kernels put junk there if
		; there is junk at the end of the ELF-32 executable program
		; file after the last PT_LOAD file byte. (Example for such
		; junk: ELF section header (e_shoff) and sections.)
		xor eax, eax
  global _start.startref2
  .startref2:  ; fix_elf_edata.pl will find this symbol by name, and then it will fix the value in the instruction below.
		mov edi, _bss_start  ; Must be a multiple of 4.
  %ifidn __OUTPUT_FORMAT__, bin
		mov ecx, (ERROR_MISSING_F_END+_bss_end_of_first_page-_bss_start)>>2  ; Both of them are aligned to a multiple of 4.
  %else
		mov ecx, edi
		neg ecx
		and ecx, 0xfff
		shr ecx, 2
  %endif
		rep stosd
%endif
%if ELF
		pop eax
		mov [@oscompat], al  ; We must do this after the clearing of the first page of .bss above.
%endif
%if WIN32WL
  %ifdef __NEED_@fd_handles
		mov edi, @fd_handles
		push byte STD_INPUT_HANDLE
		call _GetStdHandle@4  ; Ruins EDX and ECX.
		;call check_handle
		stosd  ; STDIN_FILENO.
		push byte STD_OUTPUT_HANDLE
		call _GetStdHandle@4  ; Ruins EDX and ECX.
		;call check_handle
		stosd  ; STDOUT_FILENO.
		push byte STD_ERROR_HANDLE
		call _GetStdHandle@4  ; Ruins EDX and ECX.
		;call check_handle
		stosd  ; STDERR_FILENO.
  %endif
%endif
%ifndef __GLOBAL__main
  %ifdef __GLOBAL_G@_main
    __prog_extern_in_text G@_main  ; Make it work even if we don't %include the other .nasm source files.
    %define __GLOBAL__main
    _main: equ G@_main
  %endif
%endif
%ifndef __GLOBAL_main_
  %ifdef __GLOBAL_G@main_
    __prog_extern_in_text G@main_  ; Make it work even if we don't %include the other .nasm source files.
    %define __GLOBAL_main_
    main_: equ G@main_
  %endif
%endif
%ifdef __NEED___argc
  %if XV6I386
                pop eax  ; Return address of main, pushed by the xv6-i386 kernel. Will call exit(...). We discard it here, because we don't need it.
                pop eax  ; argc.
                pop edx  ; argv.
                lea ebx, [edx+eax*4-4]  ; Address of NULL pointer argv[argc-1]. We'll use it as a fake envp for main(...).
  %elif WIN32WL
		; This is argc--argv--envp initilization code is very
		; limited: it fakes an empty environment, it fakes an empty
		; argv[0], and it puts all command-line arguments to argv[1]
		; (no splitting on whitespace), it keeps double quotes (")
		; and backslashes (\) in argv[1], it keeps double quotes in
		; argv[0].
		call _GetCommandLineA@0
		; Parse the _GetCommandLineA@0 string to argv[0] and
		; possibly argv[1]. It is simple and limited compared to
		; _CommandLineToArgvW@8 in shell32.dll:
		;
		; * Even if there is whitespace for separating arguments,
		;   all aguments end up in argv[1].
		; * Double quotes (") and backslashes and whitespace in argv[1] are kept.
		; * Double quotes (") in argv[0] are kept.
		;
		; See also https://nullprogram.com/blog/2022/02/18/
		mov edx, eax  ; EDX := address of start of argv[0]. We'll NUL-terminate it below.
		xchg esi, eax  ; EAX := start of the command line.
    .next_progname_byte:
		lodsb
		cmp al, 0
		je short .after_progname
		cmp al, ' '
		je short .after_progname
		cmp al, 9  ; '\t'.
		je short .after_progname
		cmp al, '"'
		jne short .next_progname_byte
    .next_quoted_progname_byte:
		lodsb
		cmp al, '"'
		je short .next_progname_byte
		cmp al, 0
		jne short .next_quoted_progname_byte
    .after_progname:
		mov byte [esi-1], 0  ; NUL-terminate argv[0]. Modify the statically allocated buffer returned by _GetCommandLineA@0 in place.
		db 0xa8  ; `test al, ...'. Skip over the next `lodsb' byte.
    .next_separator_byte:
		lodsb
    %if $-.next_separator_byte!=1
      %error ERROR_ASSERT_LODSB_MUST_BE_1_BYTE  ; For the `test al, ...' above to work.
    %endif
		cmp al, ' '
		je short .next_separator_byte
		cmp al, 9  ; '\t'.
		je short .next_separator_byte
		dec esi
		push byte 0  ; NULL: terminator of argv and fake envp.
		mov ebx, esp  ; envp: fake empty environment.
		xor ecx, ecx
		inc ecx  ; ECX (argc) := 1.
		cmp al, 0  ; '\0' == NUL.
		je short .argv1_done
		push esi  ; argv[1].
		inc ecx  ; ECX (argc) := 2.
    .argv1_done:
		push edx  ; argv[0]. For simplicity, it contains the double quotes ("), if any.
		mov edx, esp  ; argv.
		xchg eax, ecx  ; EAX (argc) := ECX (argc); ECX := junk.
  %elif DOSCOM+DOSEXE
		; This is argc--argv--envp initilization code is very
		; limited: it fakes an empty environment, it fakes an empty
		; argv[0], and it puts all command-line arguments to argv[1]
		; (no splitting on whitespace).
		;
		; There is a shorter, but system-specific implementation
		; with the same limitations: compile your C program with
		; -D_PROGX86_DOSPSPARGV.
		xor ax, ax
		push ax  ; NULL: terminator of argv and fake envp.
		mov bx, sp  ; envp: fake empty environment.
		mov si, @cmdline
    .next_separator_byte:
		lodsb
		cmp al, ' '
		je .next_separator_byte
		cmp al, 9  ; '\t'.
		je short .next_separator_byte
		dec si
		xor cx, cx
		inc cx  ; CX (argc) := 1.
		cmp al, 0  ; '\0' == NUL.
		je short .argv1_done
		push si  ; argv[1].
		inc cx  ; CX (argc) := 2.
    .argv1_done:
		mov dx, sp  ; argv.
    .next_argv1_byte:
		cmp byte [si], 13  ; '\r' == CR.
		je short .end_of_argv1
		inc si
		jmp short .next_argv1_byte
    .end_of_argv1:
		mov [si], ch  ; Change the terminating CR to a terminating NUL. CH is 0 now.
		push si  ; Fake argv[0]: empty string. We could get the real one after the environment as a far copy.
		xchg ax, cx  ; AX (argc) := CX (argc); CX := junk.
  %else
		pop eax  ; EAX := argc.
		mov edx, esp  ; EDX := argv.
		lea ebx, [esp+eax*4+4]  ; EBX := envp.
  %endif
  %ifdef __GLOBAL__main  ; int __cdecl main(int argc, char **argv, char **envp).
    %if DOSCOM+DOSEXE
		push bx  ; envp for _main.
		push dx  ; argv for _main.
		push ax  ; argc for _main.
    %else
		push ebx  ; envp for _main.
		push edx  ; argv for _main.
		push eax  ; argc for _main.
    %endif
  %elifdef __GLOBAL_main_  ; int __watcall main(int argc, char **argv, char **envp).
  %endif
%endif
%ifdef __NEED___argc
  __argc: __prog_libc_export ''  ; If there is a main function with argc+argv, the OpenWatcom C compiler generates this symbol.
%endif
%ifdef __GLOBAL__main  ; int __cdecl main(...).
  %ifdef __GLOBAL_main_  ; int __watcall main(...).
    %error ERROR_MULTIPLE_MAIN_FUNCTIONS_CDECL_WATCALL
    times -1 nop
  %endif
		__prog_extern_in_text _main  ; Make it work even if we don't %include the other .nasm source files.
  %if __PROG_JMP_MAIN
		jmp  _main
  %else
		call _main
  %endif
%elifdef __GLOBAL_main_  ; int __watcall main(...).
		__prog_extern_in_text main_  ; Make it work even if we don't %include the other .nasm source files.
  %if __PROG_JMP_MAIN
		jmp  main_  ; According the __watcall, argc is in EAX, argv is in EDX and envp is in EBX at call time.
  %else
		call main_   ; According the __watcall, argc is in EAX, argv is in EDX and envp is in EBX at call time.
  %endif
%else
  %error ERROR_MISSING_MAIN_FUNCTION
  times -1 nop
%endif
%ifdef __NEED_progx86_main_returns_void
  %if __PROG_JMP_MAIN
  %elif DOSEXE
		int 0x20  ; exit(0) if CS == PSP segment. That's true for DOSEXE.
  %else
    %error ERROR_UNEXPECTED_MAIN_RETURNS_VOID
    times -1 nop
  %endif
%endif
		; Fall through to __watcall exit(...). We already have the return value of main(...) in EAX.

%ifdef __NEED_exit_
  exit_:  __prog_libc_export '__noreturn void __watcall  exit(int exit_code);'
		; Fall through to __watcall _exit(...).
%endif
%ifdef __NEED_exit_
  _exit_: __prog_libc_export '__noreturn void __watcall _exit(int exit_code);'
  %if DOSCOM+DOSEXE
		mov ah, 0x4c  ; DOS syscall for _exit(2).
		int 0x21  ; DOS syscall. Doesn't return.
  %elifdef __NEED___exit
		push eax  ; exit_code argument of __cdecl _exit(...) below.
		push eax  ; Fake return address.
		; Fall through to __cdecl _exit(...).
  %elif MINIX2I386+MINIX386VM
		;mov [@minix_syscall_msg+8], eax  ; .m1_i1, at offset 8 of the struct. Set it to exit_code. This is automatic in @minix_syscall_cont.
		push byte SYS.exit  ; .m_type, at offset 4 of the struct.
		push byte MINIX_WHO.MM
		; Fall through to @minix_syscall_cont.
  %elif WIN32WL
		push eax  ; exit_code.
		call _ExitProcess@4  ; Doesn't return.
  %elif ELF
		push eax  ; exit_code for OSCOMPAT.MINIX3, fake return address for anything else.
		test byte [@oscompat], OSCOMPAT.MINIX3_MASK
		jz short .not_minix3
		; This works for OSCOMPAT.MINIX32 and OSCOMPAT.MINIX33.
		push byte SYS_Minix330.PM_EXIT
		xor eax, eax  ; MINIX_WHO.MM == 0.
		push eax  ; Arbitrary value.
		mov ebx, esp
		push byte 3  ; SENDREC.
		pop ecx
		int 0x21  ; Minix i386 syscall. It ruins EAX, EBX, ECX and EDX.
  .not_minix3:
		push eax  ; exit_code.
		push byte SYS.exit
  %else
		push eax  ; Fake return address.
		push eax  ; exit_code.
		push byte SYS.exit
		; Fall through to @simple_syscall3_pop.
  %endif
%endif

%ifdef __NEED__exit
  _exit:  __prog_libc_export '__noreturn void __cdecl  exit(int exit_code);'
		; Fall through to __cdecl _exit(...).
%endif
%ifdef __NEED___exit
  __exit: __prog_libc_export '__noreturn void __cdecl _exit(int exit_code);'
  %if MINIX2I386+MINIX386VM
		pop eax  ; Discard return address.
		pop eax  ; EAX := exit_code.
		;mov [@minix_syscall_msg+8], eax  ; .m1_i1, at offset 8 of the struct. Set it to exit_code. This is automatic in @minix_syscall_cont.
		push byte SYS.exit  ; .m_type, at offset 4 of the struct.
		push byte MINIX_WHO.MM
		; Fall through to @minix_syscall_cont.
  %elif WIN32WL
		pop eax  ; Discard return address.
		call _ExitProcess@4  ; Doesn't return.
  %elif DOSCOM+DOSEXE
		pop ax  ; Discard return address.
		pop ax  ; AX := exit_code.
		jmp short _exit_  ; Doesn't return.
  %elif ELF
		test byte [@oscompat], OSCOMPAT.MINIX3_MASK
		jz short .not_minix3
		; This works for OSCOMPAT.MINIX32 and OSCOMPAT.MINIX33.
		;push eax  ; exit_code.  Alrready pushed in _exit_ above.
		push byte SYS_Minix330.PM_EXIT
		xor eax, eax  ; MINIX_WHO.MM == 0.
		push eax  ; Arbitrary value.
		mov ebx, esp
		push byte 3  ; SENDREC.
		pop ecx
		int 0x21  ; Minix i386 syscall. It ruins EAX, EBX, ECX and EDX.
  .not_minix3:
		push byte SYS.exit
  %else
		push byte SYS.exit
		; Fall through to @simple_syscall3_pop
  %endif
%endif
%if DOSCOM+DOSEXE
  section .text  ; Back from .startsec to .text.
%endif

%if MINIX2I386+MINIX386VM
  section .bss
  resb ($$-$)&3
  @minix_syscall_msg: resb 36  ; In Minix 1.5 i386, passing syscall arguments doesn't work on the stack. So we use this global variable instead.
  section .text
  global @minix_syscall_cont
  @minix_syscall_cont:
		; Minix 1.5 i386 receives incorrect syscall arguments if
		; they are passed on the stack (e.g. `mov ebx, esp' here).
		; So we use a global variable, just like the Minix 1.5 libc
		; exit(...), callm1(...) and callx(...) functions do. This
		; works in Minix i386 1.5--2.0.4. It is not thread-safe, but
		; Minix doesn't support threads anyway.
		mov ebx, @minix_syscall_msg  ; Needed by `int 0x21' below.
		mov [ebx+8], eax  ; Populate the dword at offset 8 of the struct. This is to make the overall program code shorter.
		pop eax  ; MINIX_WHO.FS or MINIX_WHO.MM.
		pop dword [ebx+4]  ; Save the syscall number to .m_type (offset 4).
		push byte 3  ; SENDREC.
		pop ecx
		int 0x21  ; Minix 2.x i386 syscall. Inputs: EAX, EBX and ECX; returns EAX. Ruins EBX (on both Minix 1.5 i386 and Minix 1.7.0 i386).
		test eax, eax
		jnz short .return_based_on_sign
		or eax, [@minix_syscall_msg+4]  ; We can't do `or eax, [ebx+4]' here, because int 0x21' has already ruined EBX (in Minix 1.5 i386 and Minix 1.7.0 i386).
  .return_based_on_sign:
		pop ebx  ; Restore.
		jns short .ret
		; Now we could get errno from the negative of EAX.
		; Fall through to .syscall_error.
%elif WIN32WL
%elif DOSCOM+DOSEXE
%else
  %if ELF
		jmp short @simple_syscall3_pop
    global @simple_syscall3_minix3_helper
    @simple_syscall3_minix3_helper:  ; This works for SYS.read and SYS.write. It also works for SYS.ioctl of the ioctl(2) request number has the correct system-specific value.
		lea esp, [esp-(16-6)*4]  ; We must reserve 16*4 bytes for the Minix message buffer on OSCOMPAT.MINIX33. 9*4 bytes are enough on OSCOMPAT.MINIX32.
		push ecx  ; Syscall argument 3: buf for read(2) and write(2). Only used by OSCOMPAT.MINIX32. At message offset +5*4.
		push edx  ; Syscall argument 2: count for read(2) and write(2). Only used by OSCOMPAT.MINIX33. At message offset +4*4.
		je short .minix32
    .minix33:
		add eax, strict dword 0xfd  ; SYS.read --> SYS_Minix330.VFS_READ; SYS.write --> SYS_Minix330.VFS_WRITE.
		cmp al, (SYS.ioctl+0xfd)&0xff
		jne short .minix33_syscall_number_ok
		sub eax, byte (SYS.ioctl+0xfd)-SYS_Minix330.VFS_IOCTL  ; Change syscall number for SYS.ioctl to SYS_Minix330.VFS_IOCTL.
    .minix33_syscall_number_ok:
		push ecx  ; Syscall argument 2: buf for read(2) and write(2). At message offset +3*4.
		db 0xa8  ; Opcode byte for `test al, ...'. Skips over the `push edx' below.
    .minix32:
		push edx  ; Syscall argument 2: count for read(2) and write(2). At message offset +3*4.
    %if $-.minix32!=1
      %error ERROR_ASSERT_PUSH_EDX_MUST_BE_1_BYTE  ; For the `test al, ...' above to work.
    %endif
    .minix3_common:
		push ebx  ; Syscall argument 1: fd. At message offset +2*4.
		push eax  ; Syscall number (SYS.read, SYS.write, SYS_Minix330.VFS_READ, SYS_Minix330.VFS_WRITE or SYS_Minix330.VFS_IOCTL). At message offset +1*4.
		push eax  ; Arbitrary value. At message offset +0*4.
		mov ebx, esp
		cmp al, SYS.ioctl  ; This matches SYS.ioctly only for OSCOMPAT.MINIX32.
		jne short .minix3_common2
    .minix32_ioctl:
		mov [ebx+4*4], ecx  ; ioctl(2) argument 2: request. At message offset +4*4 == +16.
		mov [ebx+7*4], edx  ; ioctl(2) argument 3: arg. At message offset +7*4 == +28.
    .minix3_common2:
		xor eax, eax
		inc eax  ; MINIX_WHO.FS == 1.
		push byte 3  ; SENDREC.
		pop ecx
		int 0x21  ; Minix i386 syscall. Inputs: EAX, EBX and ECX; returns EAX. Ruins EBX (on both Minix 1.5 i386 and Minix 1.7.0 i386).
		pop ecx  ; Discard first dword (placeholder for who).
		pop ecx  ; ECX := result.
		add esp, byte (16-2)*4  ; Clean up Minix message buffer from stack.
		pop ebx  ; Restore.
		test eax, eax
		jnz short @simple_syscall3_eax.return_based_on_sign
		or eax, ecx
		jmp short @simple_syscall3_eax.return_based_on_sign
  %endif
  ; Calls a single systcall of 0, 1, 2 or 3 arguments.
  ;
  ; Input stack: dword [esp]: syscall number (SYS....); dword [esp+4]: return
  ; address; dword [esp+2*4] syscall argument 1; dword [esp+3*4] syscall
  ; argument 2; dword [esp+4*4] syscall argument 3.
  ;
  ; Output: EAX is -1 on error, otherwise EAX contains the result. The syscall
  ; number and the return address have been popped from the stack, the caller
  ; is responsible for cleaning up the rest.
  global @simple_syscall3_pop
  @simple_syscall3_pop:
		pop eax
		; Fall through to @simple_syscall3_eax.

  @simple_syscall3_eax:
  ; TOSO(pts): Size optimization: Don't generate the return value porcessing if the program uses only exit(...).
  %if COFF+S386BSD
		call 7:dword 0  ; SysV syscall.
		jnc short .ret
  %elif V7X86
		int 0x30  ; v7x86 syscall.
		jnc short .ret
  %elif XV6I386
		int 0x40  ; xv6-i386 syscall.
		; It directly returns -1 in EAX on error. There is no errno value returned.
  %elif ELF
		mov cl, [@oscompat]
		cmp cl, OSCOMPAT.SYSV
		je short .sysv
		cmp cl, OSCOMPAT.MANYBSD
		je short .freenetbsd
		; OSCOMPAT.LINUX or OSCOMPAT.MINIX32 or OSCOMPAT.MINIX33.
		cmp cl, OSCOMPAT.MINIX32
    %if OSCOMPAT.MINIX33<OSCOMPAT.MINIX32
      %error ERROR_BAD_ORDERING_OF_OSCOMPAT_MINIX3  ; Needed by the `jae short .minix23' above.
      times -1 nop
    %endif
		push ebx  ; Save.
		mov ebx, [esp+2*4]  ; Syscall argument 1.
		mov ecx, [esp+3*4]  ; Syscall argument 2.
		mov edx, [esp+4*4]  ; Syscall argument 3.
		jae short @simple_syscall3_minix3_helper
    .linux:
		int 0x80  ; Linux i386 syscall.
		pop ebx  ; Restore.
		test eax, eax
    .return_based_on_sign:
		js short .syscall_error  ; We could get errno from the negative of EAX.
		ret
    .sysv:
		call 7:dword 0  ; SysV i386 syscall.
		jmp short .after_syscall
    .freenetbsd:
    @syscall_location_any: equ $
		int 0x80  ; OS.MANYBSD i386 syscall.
    .after_syscall:
		jnc short .ret
  %else
    %error ERROR_MISSING_SYSCALL_WRAPPER
    times -1 nop
  %endif
%endif
%if DOSCOM+DOSEXE==0
  %if XV6I386==0
  .syscall_error:
		;neg eax  ; Negation needed only for Linux and Minix 2.x.
		;mov [errno], eax  ; errno is unused in this program.
		or eax, byte -1  ; Return -1 to indicate error.
  .ret:
  %endif
  %if WIN32WL==0
    %ifdef __NEED_setmode_
      setmode_: __prog_libc_export 'void __watcall setmode(int fd, unsigned char mode);'  ; Nonstandard argument types.
    %endif
    %ifdef __NEED__setmode
      _setmode: __prog_libc_export 'void __cdecl setmode(int fd, unsigned char mode);'  ; Nonstandard argument types.
    %endif
  %endif
		ret
%endif

; --- Syscall wrappers (except for SYS.exit, which has been done above).

%ifdef __NEED_read_
  %if DOSCOM+DOSEXE
    read_: __prog_libc_export 'int __watcall read(int fd, void *buf, unsigned count);'
		push cx  ; Save.
		xchg ax, bx  ; AX := count; BX := fd.
		xchg ax, cx  ; CX := count; AX := junk.
		mov ah, 0x3f  ; DOS syscall for read(2).
    .common:
		int 0x21  ; DOS syscall.
		jnc short .ok
		sbb ax, ax  ; AX := -1.
    .ok:
		pop cx  ; Restor.
		ret
  %endif
%endif

%ifdef __NEED__read
  %if DOSCOM+DOSEXE==0
    _read: __prog_libc_export 'int __cdecl read(int fd, void *buf, unsigned count);'
    %if MINIX2I386+MINIX386VM  ; This seems to be correct for Minix 1.5 i386 as well.
		push ebx  ; Save.
		push byte SYS.read  ; .m_type, at offset 4 of the struct.
      %ifdef __NEED__write
		jmp short _write.common
      %else
		push byte MINIX_WHO.FS
		mov ebx, @minix_syscall_msg+12
		mov ecx, esp  ; Make the mov()s below shorter.
		mov eax, [ecx+5*4]  ; buf.
		mov [ebx-12+20], eax  ; .m1_p1.
		mov eax, [ecx+6*4]  ; count.
		mov [ebx-12+12], eax  ; ; .m1_i2.
		mov eax, [ecx+4*4]  ; fd.
		;mov [ebx-12+8], eax  ; .m1_i1. This is automatic in @minix_syscall_cont.
		jmp short @minix_syscall_cont
      %endif
    %elif WIN32WL
		mov edx, _ReadFile@20
		jmp short @read_write_helper
    %else
		push byte SYS.read
		jmp short @simple_syscall3_pop
    %endif
  %endif
%endif

%ifdef __NEED_@read_write_helper  ; WIN32WL.
  global @read_write_helper  ; Making it global to aid debugging.
  @read_write_helper:  ; Inputs: dword [esp] is function return address; dword [esp+4] is fd; dword [esp+8] is buf; dword [esp+12] is count; EDX is function pointer to _ReadFile@20 or _WriteFile@20. Outputs: result in EAX.
		push eax  ; Make room for numberOfBytesWritten. The pushed value doesn't matter.
		mov eax, esp
		push byte 0  ; lpOverlapped. NULL.
		push eax  ; lpNumberOfBytesWritten.
		push dword [esp+6*4]  ; nNumberOfBytesToWrite.
		push dword [esp+6*4]  ; lpBuffer.
		mov eax, [esp+6*4]  ; hFile.
		; handle_from_fd. EAX --> EAX.
		cmp eax, byte 3  ; CONFIG_FILE_HANDLE_COUNT
		jnc short .bad_fd
		mov eax, [@fd_handles+eax*4]
		jmp short .have_handle
  .bad_fd:
		or eax, byte -1
  .have_handle:
		push eax  ; hFile.
		call edx  ; _WriteFile@20 or _ReadFile@20. EAX := success indication. Ruins EDX and ECX.
		test eax, eax
		jnz short .ok
		pop eax  ; Discard numberOfBytesWritten.
		or eax, byte -1
		jmp short .done
  .ok:
		pop eax  ; numberOfBytesWritten.
  .done:
		ret
%endif

%ifdef __NEED_write_
  %if DOSCOM+DOSEXE
    write_: __prog_libc_export 'int __watcall write(int fd, const void *buf, unsigned count);'
  %endif
  ; Fall through to write_binary_.
%endif
%ifdef __NEED_write_binary_
  %if DOSCOM+DOSEXE
    write_binary_: __prog_libc_export 'int __watcall write_binary(int fd, const void *buf, unsigned count);'
		push cx  ; Save.
		xchg ax, bx  ; AX := count; BX := fd.
		xchg ax, cx  ; CX := count; AX := junk.
		xor ax, ax  ; AX := 0.
    %ifdef __NEED_read_
		jcxz read_.ok  ; We skip the call if count == 0, because that would truncate the file at the current position.
		mov ah, 0x40  ; DOS syscall for write(2).
		jmp short read_.common
    %else
		jcxz .ok  ; We skip the call if count == 0, because that would truncate the file at the current position.
		mov ah, 0x40  ; DOS syscall for write(2).
		int 0x21  ; DOS syscall.
		jnc short .ok
		sbb ax, ax  ; AX := -1.
    .ok:
		pop cx  ; Restore.
		ret
    %endif
  %endif
%endif

%ifdef __NEED_write_nonzero_
  %if DOSCOM+DOSEXE
    %ifdef __NEED_write_
      write_nonzero_: __prog_libc_export_alias write_
    %else
      ; This function will misbehave (by truncating the file at the current poisition) when called with count == 0, so don't do it: call write(...) instead.
      write_nonzero_: __prog_libc_export 'int __watcall write_nonzero(int fd, const void *buf, unsigned count);'
    %endif
  %endif
  ; Fall through to write_binary_.
%endif
%ifdef __NEED_write_nonzero_binary_
  %if DOSCOM+DOSEXE
    %ifdef __NEED_write_binary_
      write_nonzero_binary_: __prog_libc_export_alias write_binary_
    %else:
      ; This function will misbehave (by truncating the file at the current poisition) when called with count == 0, so don't do it: call write(...) instead.
      write_nonzero_binary_: __prog_libc_export 'int __watcall write_nonzero_binary(int fd, const void *buf, unsigned count);'
		push cx  ; Save.
		xchg ax, bx  ; AX := count; BX := fd.
		xchg ax, cx  ; CX := count; AX := junk.
		mov ah, 0x40  ; DOS syscall for write(2).
      %ifdef __NEED_read_
		jmp short read_.common
      %else
		int 0x21  ; DOS syscall.
		jnc short .ok
		sbb ax, ax  ; AX := -1.
      .ok:
		pop cx  ; Restore.
		ret
      %endif
    %endif
  %endif
%endif

%ifdef __NEED_write_nonzero_binary_DOSREG
  %if DOSCOM+DOSEXE
    write_nonzero_binary_DOSREG: __prog_libc_export 'int __usercall write_nonzero_binary_DOSREG [AX](int fd [BX], const void *buf [DX], unsigned count [CX]);'
		mov ah, 0x40  ; DOS syscall for write(2).
    %ifdef __NEED_read_DOSREG
		db 0xa9  ; Opode byte for `test ax, ...'. Fall through to read_DOSREG, and skip its leading `mov ah, 0x3f'.
		.end:
    %else
		int 0x21  ; DOS syscall.
		jnc short .ok
		sbb ax, ax  ; AX := -1.
      .ok:
		ret
    %endif
  %endif
%endif

%ifdef __NEED_read_DOSREG
  %if DOSCOM+DOSEXE
    %ifdef __NEED_write_nonzero_binary_DOSREG
      %if $!=write_nonzero_binary_DOSREG.end
        %error ERROR_READ_DOSREG_MUST_START_AFTER_WRITE
        times -1 nop
      %endif
    %endif
    read_DOSREG: __prog_libc_export 'int __usercall read_DOSREG [AX](int fd [BX], void *buf [DX], unsigned count [CX]);'
		mov ah, 0x3f  ; DOS syscall for read(2).
    %ifdef __NEED_write_nonzero_binary_DOSREG
      %if $!=write_nonzero_binary_DOSREG.end+2
        %error ERROR_READ_DOSREG_MUST_END_AFTER_WRITE
        times -1 nop
      %endif
    %endif
		int 0x21  ; DOS syscall.
		jnc short .ok
		sbb ax, ax  ; AX := -1.
      .ok:
		ret
  %endif
%endif

%ifdef __NEED__write
  %if WIN32WL+DOSCOM+DOSEXE==0
    _write: __prog_libc_export 'int __cdecl write(int fd, const void *buf, unsigned count);'
    _write_nonzero: __prog_libc_export 'int __cdecl write_nonzero(int fd, const void *buf, unsigned count);'
  %endif
  ; Fall through to _write_binary.
%endif
%ifdef __NEED__write_binary
  %if DOSCOM+DOSEXE==0
    _write_binary: __prog_libc_export 'int __cdecl write_binary(int fd, const void *buf, unsigned count);'
    _write_nonzero_binary: __prog_libc_export 'int __cdecl write_nonzero_binary(int fd, const void *buf, unsigned count);'
    %if MINIX2I386+MINIX386VM  ; This seems to be correct for Minix 1.5 i386 as well.
		push ebx  ; Save.
		push byte SYS.write
      _write.common: equ $0
		push byte MINIX_WHO.FS
		mov ebx, @minix_syscall_msg+12
		mov ecx, esp  ; Make the mov()s below shorter.
		mov eax, [ecx+5*4]  ; buf.
		mov [ebx-12+20], eax  ; .m1_p1.
		mov eax, [ecx+6*4]  ; count.
		mov [ebx-12+12], eax  ; ; .m1_i2.
		mov eax, [ecx+4*4]  ; fd.
		;mov [ebx-12+8], eax  ; .m1_i1. This is automatic in @minix_syscall_cont.
		jmp short @minix_syscall_cont
    %elif WIN32WL
		mov edx, _WriteFile@20
		jmp short @read_write_helper
    %else
		push byte SYS.write
		jmp short @simple_syscall3_pop
    %endif
  %endif
%endif

%ifdef __NEED__write
  %if WIN32WL
    _write_nonzero: __prog_libc_export 'int __cdecl write_nonzero(int fd, const void *buf, unsigned count);'
    _write: __prog_libc_export 'int __cdecl write(int fd, const void *buf, unsigned count);'
		mov eax, [esp+1*4]  ; fd.
		cmp eax, byte 3
		jnc short _write_binary
		cmp byte [@fd_modes+eax], ah  ; Check for O_BINARY (nonzero); AH is now 0.
		jne short _write_binary  ; Jump iff O_BINARY found in flags.
		; Now we write the input, stopping before each LF ('\n'),
		; writing a CRLF ("\r\n") instead, then continuing. This is
		; a bit slow, because we for each line we call _WriteFile@20
		; twice.
		;
		; State: address limit (ESI), current address (EDI).
		push edi  ; Save.
		push esi  ; Save.
		mov edi, [esp+4*4]  ; buf.
		mov esi, edi
		add esi, [esp+5*4]  ; count.
		jmp short .try_more
    .more:  ; Now ECX is the number of remaining bytes, and ECX >= 1; AL == 10.
		mov edx, edi  ; EDX := buf for _write_binary below.
		repne scasb
		jne short .scan_done  ; This is correct only if ECX wasn't 0 (true).
		dec edi  ; Go back to the LF == '\n' just found.
    .scan_done:
		push edx  ; Save buf for _write_binary below.
		sub edi, edx
		push edi  ; Argument count for _write_binary below.
		add edi, edx
		push edx  ; Argument buf for _write_binary below.
		push dword [esp+6*4]  ; Argument fd for _write_binary.
		call _write_binary  ; Sets EAX to the result, ruins ECX and EDX.
		add esp, byte 3*4  ; Clean up arguments of _write_binary above from the stack.
		pop edx  ; Restore EDX := buf for _write_binary.
		cmp eax, byte -1
		je short .done
		add eax, edx
		cmp eax, edi
		jne short .done  ; Stop at short write.
		cmp edi, esi
		je short .done
		push dword (13|10<<8)  ; CRLF == "\r\n".
		mov eax, esp
		push byte 2  ; count for the binary write.
		push eax  ; buf for _write_binary.
		push dword [esp+6*4]  ; fd for _write_binary.
		call _write_binary  ; Sets EAX to the result, ruins ECX and EDX.
		add esp, byte 4*4  ; Clean up arguments of _write_binary above and also the CRLF from the stack.
		cmp eax, byte 2
		jne short .done
		inc edi  ; Skip over the LF in the input.
    .try_more:
		mov al, 10  ; LF == '\n'.
		mov ecx, esi
		sub ecx, edi
		jnz short .more
		; It's important that EAX != 0 here, so that .swap below will be done if we're done writing.
    .done:
		sub edi, [esp+4*4]  ; EDI := total number of input bytes written (starting at the original buf).
		jnz short .swap
		cmp eax, byte -1
		je short .done_swap
    .swap:
		xchg eax, edi  ; EAX := result (number of input bytes written); EDI := junk.
    .done_swap:
		pop esi  ; Restore.
		pop edi  ; Restore.
		ret
  %endif
%endif

%if WIN32WL
  %ifdef __NEED_setmode_
    setmode_: __prog_libc_export 'void __watcall setmode(int fd, unsigned char mode);'  ; Nonstandard argument types.
		cmp eax, byte 3  ; fd.
		jnc .done
		mov byte [@fd_modes+eax], dl  ; mode.
    .done:
		ret
  %endif
  %ifdef __NEED__setmode
    _setmode: __prog_libc_export 'void __cdecl setmode(int fd, unsigned char mode);'  ; Nonstandard argument types.
		cmp byte [esp+4], byte 3  ; fd.
		jnc .done
		mov dl, [esp+2*4]  ; mode.
		mov byte [@fd_modes+eax], dl
    .done:
		ret
  %endif
%endif

%ifdef __NEED_@fd_handles  ; WIN32WL.
  section _BSS
  global @fd_handles  ; Making it global to aid debugging.
  @fd_handles: resd 3  ; int [3]. stdin, stdout, stderr.
  @fd_modes: resb 4  ; unsigned char [3]. stdin, stdout, stderr, dummy. A nonzero value means O_BINARY. Zero by default.
  section _TEXT
%endif

%ifdef __NEED_@ioctl_wrapper
  global @ioctl_wrapper  ; The `request' and `arg' is not system-independent, but still making it global to aid debugging.
  @ioctl_wrapper:  ; int __cdecl ioctl_wrapper(int fd, unsigned long request, void *arg);
  %if MINIX2I386+MINIX386VM  ; This is correct for Minix 1.5--2.0.4 i386. Please note that the TCGETS constant is different for Minix 1.5 from the rest, also in Minix 1.5 don't have to set the pointer to the message struct.
		push ebx  ; Save.
		push byte SYS.ioctl
		push byte MINIX_WHO.FS
		mov ebx, @minix_syscall_msg+16
		mov ecx, esp  ; Make the mov()s below shorter.
		mov eax, [ecx+5*4]  ; request.
		mov [ebx-16+16], eax  ; .m2_i3, at offset 16 of the struct (TTY_REQUEST == COUNT == m2_i3 == m_u.m_m2.m2i3 == 16 == 0x10). Set it to request.
		mov eax, [ecx+6*4]  ; arg.
		mov [ebx-16+28], eax  ; .m2_p1, at offset 28 of the struct (ADDRESS == m2_p1 == m_u.m_m2.m2p1 == 28).
		mov eax, [ecx+4*4]  ; fd.
		;mov [ebx-16+8], eax  ;  ; .m2_i1, at offset 8 of the struct (TTY_LINE == DEVICE == m2_i1 == m_u.m_m2.m2i1 == 8). Set it to fd. This is automatic in @minix_syscall_cont.
		jmp short @minix_syscall_cont
  %elif XV6I386+WIN32WL+DOSCOM+DOSEXE
    %error ERROR_ASSERT_NO_IOCTL_WRAPPER  ; This is fine, isatty(2) below is a dummy for xv6-i386.
    times -1 nop
  %else
		push byte SYS.ioctl
		jmp short @simple_syscall3_pop
  %endif
%endif

; By adding and using the __watcall counterpart for @simple_syscall3_pop
; (__NEED_simple_syscall3_WAT), the program file doesn't become shorter,
; because FreeBSD needs the syscall arguments on the stack.

; --- System-specific libc functions.

%ifdef __NEED__isatty
  _isatty: __prog_libc_export 'int __cdecl isatty(int fd);'
		mov eax, [esp+1*4]  ; Argument fd.
		; Fall through to isatty_.
%endif
%ifdef __NEED_isatty_
  isatty_: __prog_libc_export 'int __watcall isatty(int fd);'
  %if XV6I386+DOSCOM+DOSEXE==0
		push ecx  ; Save.
		push edx  ; Save.
  %endif
  %if COFF
    ISATTY_TERMIOS_SIZE equ 18+2  ; sizeof(struct termio) == 18 for SYSV and iBCS2 (we round it up to 20, to dword bounadry).
		sub esp, byte ISATTY_TERMIOS_SIZE
		push esp  ; Argument 3 arg of @ioctl_wrapper.
		push strict dword IOCTL_Linux.TCGETS  ; We need the one which works on SysV. Same value as IOCTL_SysV.TCGETA, IOCTL_Coherent4.TCGETA, IOCTL_iBCS2.TCGETA.
		push eax  ; Argument fd of this isatty(...).
  %elif S386BSD
    ISATTY_TERMIOS_SIZE equ 44  ; sizeof(struct termios) == 44 on 386BSD.
		sub esp, byte ISATTY_TERMIOS_SIZE
		push esp  ; Argument 3 arg of @ioctl_wrapper.
		push strict dword IOCTL_386BSD.TIOCGETA
		push eax  ; Argument fd of this isatty(...).
  %elif MINIX2I386+MINIX386VM
    ISATTY_TERMIOS_SIZE equ 36  ; sizeof(struct termio) == 36 on MINIX2I386+MINIX386VM.
		sub esp, byte ISATTY_TERMIOS_SIZE
		; Minix 2.0 has a different ioctl from Minix 1.5--1.7, so we try both, and if any of them returns nonnegative, then it's a TTY.
		push esp  ; Argument 3 arg of @ioctl_wrapper.
		push strict dword IOCTL_Minix1.5.TCGETS  ; Same as IOCTL_Minix1.7.TIOCGETP.
		push eax  ; Argument fd of this isatty(...).
		call @ioctl_wrapper
		cmp eax, byte -1
		jne short .return  ; Found a TTY.
		pop eax  ; Restore EAX := argument fd. That works, because @ioctl_wrapper doesn't modify its arguments on the stack.
		lea esp, [esp+4]  ; Clean up the argument request of @ioctl_wrapper above from the stack, without modifying the flags.
		push strict dword IOCTL_Minix2.TCGETS  ; Alternatively, we could use IOCTL_Minix2.TIOCGETP (Minix 2.0 only) with the smaller sizeof(struct sgttyb) == 8. Or use TIOCGETP (sizeof(int) == 4) everywhere available.
		push eax  ; Argument fd of this isatty(...).
  %elif V7X86
    ISATTY_TERMIOS_SIZE equ 8  ; sizeof(struct agttyb) == 8 on V7X86.
		sub esp, byte ISATTY_TERMIOS_SIZE
		push esp  ; Argument 3 arg of @ioctl_wrapper.
		push strict dword IOCTL_v7x86.TIOCGETP  ; Alternatively, we could use IOCTL_v7x86.TIOCGETD with the smaller sizeof(int) == 3.
		push eax  ; Argument fd of this isatty(...).
  %elif XV6I386
		xor eax, eax
		inc eax  ; EAX := 1. This is a fallback of isatty(...) always returning true (1), because there is no isatty(3) or ioctl(2) on xv6-i386.
  %elif WIN32WL
		; handle_from_fd. EAX --> EAX. !! Move to a helper function.
		cmp eax, byte 3  ; CONFIG_FILE_HANDLE_COUNT
		jnc short .bad_fd
		mov eax, [@fd_handles+eax*4]
		jmp short .have_handle
    .bad_fd:
		or eax, byte -1
    .have_handle:
		push eax  ; hFile.
		call _GetFileType@4  ;  Ruins EDX and ECX.
		xchg edx, eax  ; EDX := file type; EAX := junk.
		xor eax, eax  ; EAX := 0.
		cmp edx, byte FILE_TYPE_CHAR
		sete al
  %elif DOSCOM+DOSEXE
    ; Just returns whether fd is a character device. (On Unix, there are
    ; non-TTY character devices, but this implementation pretends that they
    ; don't exist.) It works correctly on
    ; [kvikdos](https://github.com/pts/kvikdos), emu2 and (windowed) DOSBox.
    ; OpenWatcom lib does the same (
    ; https://github.com/open-watcom/open-watcom-v2/blob/2d1ea451c2dbde4f1efd26e14c6dea3b15a1b42b/bld/clib/environ/c/isatt.c#L58C10-L59 )
		push bx  ; Save.
		push dx  ; Save.
		xchg bx, ax  ; BX := fd; AX := junk.
		mov ax, 0x4400  ; DOS syscall ioctl(2), get device information.
		int 0x21  ; DOS syscall.
		mov ax, 0
		jc .done  ; Jumpp if I/O error. In this case, return 0 (not a TTY).
		test dl, dl
		jns .done
		inc ax
    .done:
		pop dx  ; Restore.
		pop bx  ; Restore.
  %else  ; ELF.
    ISATTY_TERMIOS_SIZE equ 44  ; Maximum sizeof(struct termios), sizeof(struct termio), sizeof(struct sgttyb) for OSCOMPAT.LINUX, OSCOMPAT.SYSV and OSCOMPAT.MANYBSD.
		sub esp, byte ISATTY_TERMIOS_SIZE
		push esp  ; Argument 3 arg of @ioctl_wrapper.
		push strict dword IOCTL_FreeBSD.TIOCGETA  ; Argument 2 request of @ioctl_wrapper. Same value as IOCTL_NetBSD.TIOCGETA, IOCTL_386BSD.TIOCETA and IOCTL_Minix330.TIOCGETA. Correct value for OSCOMPAT.MANYBSD and OSCOMPAT.MINIX33.
		push eax  ; Argument 1 fd of @ioctl_wrapper == argument fd of this isatty(...)
		mov al, [@oscompat]
		test al, OSCOMPAT.MINIX33_MANYBSD_MASK
		jnz short .ioctl_args_pushed
		mov dword [esp+4], IOCTL_Linux.TCGETS  ; Same value as IOCTL_SysV.TCGETA, IOCTL_Coherent4.TCGETA, IOCTL_iBCS2.TCGETA. Correct value for OSCOMPAT.LINUX and OSCOMPAT.SYSV.
		cmp al, OSCOMPAT.MINIX32
		jne short .ioctl_args_pushed
		mov dword [esp+4], IOCTL_Minix2.TCGETS  ; Correct value for OSCOMPAT.MINIX32.
    .ioctl_args_pushed:
  %endif
  %if XV6I386+WIN32WL+DOSCOM+DOSEXE==0
		call @ioctl_wrapper
    .return:
		add esp, byte 3*4+ISATTY_TERMIOS_SIZE  ; Clean up the arguments of @ioctl_wrapper above from the stack, and also clean up the arg struct.
		; Now convert result EAX: -1 to 0, everything else to 1.
		inc eax
    %if 0  ; Faster (because it avoids the jump) but 1 byte longer.
		setnz al
		movzx eax, al
    %else
		jz short .have_retval
		xor eax, eax
		inc eax  ; EAX := 1.
      .have_retval:
    %endif
  %endif
  %if XV6I386+DOSCOM+DOSEXE==0
		pop edx  ; Restore.
		pop ecx  ; Restore.
  %endif
		ret
%endif

%ifdef __NEED_isatty_DOSREG
  %if DOSCOM+DOSEXE
    isatty_DOSREG: __prog_libc_export 'int __usercall isatty_DOSREG [AX](int fd [BX]);'
    ; Just returns whether fd is a character device. (On Unix, there are
    ; non-TTY character devices, but this implementation pretends that they
    ; don't exist.) It works correctly on
    ; [kvikdos](https://github.com/pts/kvikdos), emu2 and (windowed) DOSBox.
    ; OpenWatcom lib does the same (
    ; https://github.com/open-watcom/open-watcom-v2/blob/2d1ea451c2dbde4f1efd26e14c6dea3b15a1b42b/bld/clib/environ/c/isatt.c#L58C10-L59 )
		push dx  ; Save.
		mov ax, 0x4400  ; DOS syscall ioctl(2), get device information.
		int 0x21  ; DOS syscall.
		mov ax, 0
		jc .done  ; Jumpp if I/O error. In this case, return 0 (not a TTY).
		test dl, dl
		jns .done
		inc ax
    .done:
		pop dx  ; Restore.
		ret
  %endif
%endif

%ifdef __NEED_progx86_para_alloc_
  %if DOSCOM+DOSEXE
    ; Allocates para_count << 4 bytes of memory, aligned to 16 (paragraph)
    ; boundary, and returns the segment register value pointing to the
    ; beginning of it. Returns 0 on out-of-memory.
    ;
    ; For shorter code, consider using progx86_para_reuse(...) instead. For
    ; even shorter code, try
    ; progx86_is_para_less_than(...)+progx86_para_reuse_start().
    progx86_para_alloc_: __prog_libc_export 'unsigned __watcall progx86_para_alloc(unsigned para_count);'
		add ax, [@first_empty_seg]
		jc short .out_of_memory  ; Jump iff verflow.
		cmp ax, [@prog_mem_end_seg]
		ja short .out_of_memory
		xchg [@first_empty_seg], ax
		; db 0xa9  ; Opcode of `test ax, ...'. Skips over the `xor ax, ax' below.
		ret
    .out_of_memory:
		xor ax, ax
		ret
  %endif
%endif

%ifdef __NEED_progx86_para_reuse_
  %if DOSCOM+DOSEXE
    ; Finds para_count << 4 bytes of memory, aligned to 16 (paragraph)
    ; boundary, and returns the segment register value pointing to the
    ; beginning of it. Returns 0 on out-of-memory. Subsequent calls will
    ; return the same address (so the memory is reused), also the same
    ; address as progx86_para_alloc(...) returns first. So don't use this
    ; function if you need memory which no unrelated program code is allowed
    ; to overwrite.
    ;
    ; For shorter code, try
    ; progx86_is_para_less_than(...)+progx86_para_reuse_start().
    progx86_para_reuse_: __prog_libc_export 'unsigned __watcall progx86_para_reuse(unsigned para_count);'
		push dx
		xchg dx, ax  ; DX := para_count; AX := junk.
		mov ax, cs
		add ax, strict word __prog_para_count_from_psp
		add dx, ax
		jc short .out_of_memory
		cmp dx, [@prog_mem_end_seg]
		ja short .out_of_memory
		db 0xa9  ; Opcode of `test ax, ...'. Skips over the `xor ax, ax' below.
    .out_of_memory:
		xor ax, ax
    %if $-.out_of_memory!=1
      %error ERROR_ASSERT_XOR_AX_AX_MUST_BE_1_BYTE  ; For the `test ax, ...' above to work.
    %endif
		pop dx
		ret
  %endif
%endif

; --- System-independent libc functions.

%ifdef __NEED__strlen  ; Longer code than strlen_.
  %if DOSCOM+DOSEXE==0
    _strlen: __prog_libc_export 'size_t __cdecl strlen(const char *s);'
		push edi
		mov edi, [esp+8]  ; Argument s.
		xor eax, eax
		or ecx, byte -1  ; ECX := -1.
		repne scasb
		sub eax, ecx
		dec eax
		dec eax
		pop edi
		ret
  %endif
%endif

%ifdef __NEED_strlen_
  strlen_: __prog_libc_export 'size_t __watcall strlen(const char *s);'
  %if DOSCOM+DOSEXE
    %if 0  ; This is 1 byte shorter (or 1+2 byte shorter with `pop es') than the alternative, but slower.
		push si
		xchg si, ax  ; SI := AX, AX := junk.
		mov ax, -1
      .next:
		cmp byte ptr [si], 1
		inc si
		inc ax
		jnc short .next
		pop si
    %else
		push ds
		pop es  ; This code is needed, unless ES == DS is guaranteed.
		push di  ; Save.
		xchg di, ax  ; EDI := argument s; EAX : = junk.
		xor ax, ax
		mov cx, -1
		repne scasb
		sub ax, cx
		dec ax
		dec ax
		pop di  ; Restore.
    %endif
  %else
		push edi  ; Save.
		xchg edi, eax  ; EDI := argument s; EAX : = junk.
		xor eax, eax
		or ecx, byte -1  ; ECX := -1.
		repne scasb
		sub eax, ecx
		dec eax
		dec eax
		pop edi  ; Restore.
  %endif
		ret
%endif

%ifdef __NEED__memset  ; Longer code than memset_.
  %if DOSCOM+DOSEXE==0
    _memset: __prog_libc_export 'void * __cdecl memset(void *s, int c, size_t n);'
		push edi
		mov edi, [esp+8]  ; Argument s.
		mov al, [esp+0xc]  ; Argument c.
		mov ecx, [esp+0x10]  ; Argument n.
		push edi
		rep stosb
		pop eax  ; Result is argument s.
		pop edi
		ret
  %endif
%endif

%ifdef __NEED_memset_
  memset_: __prog_libc_export 'void * __watcall memset(void *s, int c, size_t n);'
  %if DOSCOM+DOSEXE
		push ds
		pop es  ; This code is needed, unless ES == DS is guaranteed.
		push di  ; Save.
		xchg di, ax  ; DI := AX (argument s); EAX := junk.
		xchg ax, dx  ; AX := DX (argument c); EDX := junk.
		xchg cx, bx  ; CX := BX (argument n); EBX := saved CX.
		push di
		rep stosb
		pop ax  ; Result is argument s.
		xchg cx, bx  ; CX := saved CX; BX := 0 (unused).
		pop di  ; Restore.
  %else
		push edi  ; Save.
		xchg edi, eax  ; EDI := EAX (argument s); EAX := junk.
		xchg eax, edx  ; EAX := EDX (argument c); EDX := junk.
		xchg ecx, ebx  ; ECX := EBX (argument n); EBX := saved ECX.
		push edi
		rep stosb
		pop eax  ; Result is argument s.
		xchg ecx, ebx  ; ECX := saved ECX; EBX := 0 (unused).
		pop edi  ; Restore.
  %endif
		ret
%endif

%ifdef __NEED__memcpy  ; Longer code than memcpy_.
  %if DOSCOM+DOSEXE==0
    _memcpy: __prog_libc_export 'void * __cdecl memcpy(void *dest, const void *src, size_t n);'
		push edi
		push esi
		mov ecx, [esp+0x14]
		mov esi, [esp+0x10]
		mov edi, [esp+0xc]
		push edi
		rep movsb
		pop eax  ; Result: pointer to dest.
		pop esi
		pop edi
		ret
  %endif
%endif

%ifdef __NEED_memcpy_
  memcpy_: __prog_libc_export 'void * __watcall memcpy(void *dest, const void *src, size_t n);'
  %if DOSCOM+DOSEXE
		push ds
		pop es  ; This code is needed, unless ES == DS is guaranteed.
		push di
		xchg si, dx
		xchg di, ax  ; EDI := dest; EAX := junk.
		xchg cx, bx
		push di
		rep movsb
		pop ax  ; Will return dest.
		xchg cx, bx  ; Restore CX from BX; BX := junk. And BX is scratch, we don't care what we put there.
		xchg si, dx  ; Restore SI.
		pop di
  %else
		push edi
		xchg esi, edx
		xchg edi, eax  ; EDI := dest; EAX := junk.
		xchg ecx, ebx
		push edi
		rep movsb
		pop eax  ; Will return dest.
		xchg ecx, ebx  ; Restore ECX from BX; BX := junk. Ans BX is scratch, we don't care what we put there.
		xchg esi, edx  ; Restore ESI.
		pop edi
  %endif
		ret
%endif

f_end

; __END__
