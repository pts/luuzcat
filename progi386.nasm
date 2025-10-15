;
; progi386.nasm: glue code for building i386 programs for various systems in NASM
; by pts@fazekas.hu at Mon Oct 13 03:35:21 CEST 2025
;
; Executable binary compatibility:
;
; * prog.elf (-DELF, default):
;   Linux >=1.0.4 (1994-05-22) i386 and possibly earlier
;   (tested on Linux 1.0.4 (1994-03-22), 5.4.0 (2019-11-24)),
;   FreeBSD >=3.5 (1995-05-28) i386 and possibly earlier
;   (tested on FreeBSD 9.3 (2014-07-11)),
;   NetBSD >=1.5.2 i386 and possibly earlier
;   (tested on NetBSD 1.5.2 (2001-09-10), 10.1 (2024-11-17)),
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
;   Minix 2.x i386 (tested on Minix 2.0.4 (2003-11-09)).
; * prog.v7x (-DV7X86):
;   [v7x86](https://www.nordier.com/) (tested on v7x86 0.8a (2007-10-04)).
;
; These programs don't run on Xenix, SunOS 4.0.x, macOS (except in Docker),
; DOS or Win32 (except in Docker or WSL) or 386BSD.
;
; !! Which version of Xenix/386 can run these iBCS2 COFF programs?
; !! Add binary release for older Xenix/386.
; !! Test on FreeBSD 3.5 (1995-05-28).
; !! For FreeBSD, try alternative of ELF_OSABI.FreeBSD (i.e. at EI_ABIVERSION == EI_BRAND == 8). NetBSD 10.1 also allows it.
; !! Test on 386BSD 0.0 (1992-03-17), 386BSD 0.1 (1992-07-14).
;    https://gunkies.org/wiki/386BSD says: Once patchkit 023 is installed, 386BSD 0.1 will then run under Qemu 0.11.x
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
%ifdef V7X86
  %define V7X86 1
%else
  %define V7X86 0
%endif
%if COFF+ELF+S386BSD+MINIX2I386+V7X86==0
  %define ELF 1  ; Default.
%endif
%if COFF+ELF+S386BSD+MINIX2I386+V7X86>1
  %error ERROR_MULTIPLE_SYSTEMS_SPECIFIED
  times -1 nop
%endif

bits 32
cpu 386

; Executable program file header generation.

; This header generation aims for maximum compatibility with existing
; operating systems, and it sacrifices file sizes, i.e. the generated
; executable may be a few dozen bytes longer than absolutely necessary, so
; that it loads correctly in many operating systems.
%ifidn __OUTPUT_FORMAT__, bin
  %if COFF  ; SysV SVR3 i386 COFF executable program.
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
  %endif
  %if ELF
    _base_org equ 0x8048000  ; Standard Linux i386, FreeBSD i386 and SysV SVR4 i386 value.
    section .text align=1 valign=1 start=0 vstart=_base_org
    _text_start:
    section .rodata.str align=1 valign=1 follows=.text vfollows=.text  ; CONST. Same PT_LOAD as .text.
    _rodatastr_start:
    section .rodata align=1 valign=1 follows=.rodata.str vfollows=.rodata.str  ; CONST2. Same PT_LOAD as .text. The actual alinment is 4, but we specify align=1 so that .data.una can be unaligned if .rodata is empty.
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
    .FreeBSD equ 9

    ELF_PT:  ; ELF PHDR type constants.
    .LOAD equ 1
    .NOTE equ 4

    ; We use ELF_OSABI.FreeBSD, because newer FreeBSD checks it. Linux, NetBSD and SysV SVR4 don't check it.
    Elf32_Ehdr:
		db 0x7F, 'ELF', 1, 1, 1, ELF_OSABI.FreeBSD, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, ERROR_MISSING_F_END
		dd 1, _start, Elf32_Phdr0-Elf32_Ehdr, 0, 0
		dw Elf32_Phdr0-Elf32_Ehdr, Elf32_Phdr1-Elf32_Phdr0, (Elf32_Phdr.end-Elf32_Phdr0)>>5, 0x28, 0, 0
    Elf32_Phdr0:
		dd ELF_PT.LOAD, Elf32_header.end-Elf32_Ehdr, Elf32_header.end, 0, _text_size+_rodatastr_size+_rodata_size-(Elf32_header.end-Elf32_Ehdr), _text_size+_rodatastr_size+_rodata_size-(Elf32_header.end-Elf32_Ehdr), 5, 1<<12
    Elf32_Phdr1:
		dd ELF_PT.LOAD, _text_size+_rodatastr_size+_rodata_size, _datauna_start, 0, _datauna_size+_data_size, _datauna_size+_data_size+_data_endalign_extra+_bss_size, 6, 1<<12
    Elf32_Phdr2:
		dd ELF_PT.NOTE, Elf32_note-Elf32_Ehdr, Elf32_note, 0, Elf32_note.end-Elf32_note, Elf32_note.end-Elf32_note, 4, 1<<2
    Elf32_Phdr.end:
    Elf32_note:  ; NetBSD checks it. Actual value is same as in /bin/echo in NetBSD 1.5.2 (2001-08-18). It also works with NetBSD 10.1 (2024-11-17).
		dd 7, 4, 1  ; Size of the name, size of the value, node type.
		db 'NetBSD', 0  ; 7-byte name.
		db 0  ; Alignment padding to 4.
		dd 199905  ; Some version number in decimal.
		dd 7, 7, 2  ; Size of the name, size of the value, node type.
		db 'NetBSD', 0  ; 7-byte name.
		db 0  ; Alignment padding to 4.
		db 'netbsd', 0
		db 0  ; Alignment padding to 4.
    Elf32_note.end:
    Elf32_header.end:
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
    section .text
  %elif MINIX2I386
    %ifndef MINIX2I386_STACK
      %define MINIX2I386_STACK 0x4000  ; 16 KiB. Minix i386 programs (such as cat(1)) use this value.
    %endif
    %if MINIX2I386_STACK<0x1000  ; Must include argv and environ strings.
      %define MINIX2I386_STACK 0x1000
    %endif
    section .header align=1 valign=1 start=0 vstart=0
    section .text align=1 valign=0x1000 follows=.header vstart=0
    _text_start:
    ; It would be more traditional to group .rodata.str and .rodata together
    ; with .text (rather than .data), just like Linux does it, but that
    ; wouldn't work, because for that we'd have to generate the `cs;' prefix
    ; in assembly instructions reading data from .rodata. That's because in
    ; Minix, CS can be used to access a_text only (no a_data), and DS and ES
    ; can be used to access a_data only (no a_text), and offsets reset to 0
    ; at the beginning of a_text. This is not the flat memory model!
    section .rodata.str align=1 valign=0x1000 follows=.text vstart=0  ; CONST.
    _rodatastr_start:
    section .rodata align=1 valign=1 follows=.rodata.str vfollows=.rodata.str  ; CONST2.
    _rodata_start:
    section .data align=1 valign=1 follows=.rodata vfollows=.rodata
    _data_start:
    section .bss align=4 follows=.data nobits
    _bss_start:
    section .header
    MINIX2I386_exec:  ; `struct exec' in usr/include/a.out.h
    .a_magic: db 1, 3  ; Signature (magic number).
    .a_flags: db 0x20  ; A_SEP == 0x20. Flags.
    .a_cpu: db 0x10  ; CPU ID.
    .a_hdrlen: db .size  ; Length of header.
    .a_unused: db 0  ; Reserved for future use.
    .a_version: dw 0  ; Version stamp (unused by Minix).
    .a_text: dd _text_size  ; Size of text segement in bytes.
    .a_data: dd _rodatastr_size+_rodata_size+_data_size  ; Size of data segment in bytes.
    .a_bss: dd _data_endalign_extra+_bss_size  ; Size of bss segment in bytes.
    .a_entry: dd _start  ; Virtual address (vaddr) of entry point.
    .a_total: dd _data_size+_bss_size+MINIX2I386_STACK  ; Total memory allocated for a_data, a_bss and stack, including argv and environ strings.
    .a_syms: dd 0  ; Size of symbol table.
    .size: equ $-MINIX2I386_exec
    section .text
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
    section .text
  %endif

  %macro f_section__TEXT 0
    section .text
  %endm
  %macro f_section_CONST 0
    section .rodata.str
  %endm
  %macro f_section_CONST2 0
    section .rodata
    times ($$-$)&3 db 0  ; Align to a multiple of 4. We do this to provide proper alingment for the %included() .nasm files.
  %endm
  %macro f_section__DATA 0  ; !! Create and use 1-argument macro so NASM can catch syntax errors as an error (rather than a warning for label-only lines).
    section .data
    times ($$-$)&3 db 0  ; Align to a multiple of 4. We do this to provide proper alingment for the %included() .nasm files.
  %endm
  %macro f_section__DATA_UNA 0  ; Like _DATA, but unaligned.
    %if ELF
      section .data.una
    %else
      section .data
      times ($$-$)&3 db 0  ; Align to a multiple of 4. We do this to provide proper alingment for the %included() .nasm files.
    %endif
  %endm
  %macro f_section__BSS 0
    section .bss
    resb ($$-$)&3  ; Align to a multiple of 4. We do this to provide proper alingment for the %included() .nasm files.
  %endm

  %macro f_end 0
    ERROR_MISSING_F_END equ 0  ; If you are getting ERROR_MISSING_F_END errors, then add `f_end' to the end of your .nasm source file.

    section .text
    _text_endu:
    section .rodata.str
    _rodatastr_endu:
    section .rodata
    _rodata_endu:
    %if ELF
      section .data
      _data_size_before_oscompat equ $-$$
      section .data.una
      %if ($-$$)+_data_size_before_oscompat==0
        ; The reason why we add __oscompat to .data.una here rather
        ; than to anywhere in .bss is to make sure that .data.una+.data is not
        ; empty, which would cause ibcs-us 4.1.6 to segfault.
        %define IS_OSCOMPAT_ADDED 1
        global __oscompat
        __oscompat: db 0  ; Initial value doesn't matter.
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

    %if MINIX2I386
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
      %if _rodata_endu==_rodata_start
        _rodatastr_endalign equ 0
      %else
        _rodatastr_endalign equ (-(_text_endu-_text_start)-(_rodatastr_endu-_rodatastr_start))&3
      %endif
      _rodata_endalign equ 0  ; .data has align=1 valign=0x1000, no need to add any alignment before .data to the file.
      _data_endalign equ (-(_data_endu-_data_start))&3
    %elif ELF
      %if _rodata_endu==_rodata_start
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
    section .text
    _text_end:
    _text_size equ $-$$
    section .rodata.str
    times _rodatastr_endalign db 0
    _rodatastr_end:
    _rodatastr_size equ $-$$
    section .rodata
    times _rodata_endalign db 0
    _rodata_end:
    _rodata_size equ $-$$  ; _rodata_size is a multiple of 4.
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
    _data_size equ $-$$  ; _data_size is a multiple of 4.
    section .bss
    %if ELF && IS_OSCOMPAT_ADDED==0
      %define IS_OSCOMPAT_ADDED 1
      global __oscompat
      __oscompat: resb 1  ; Initial value doesn't matter.  Put it to the end of the .bss, because other parts may need larger alignment.
    %endif
    %if COFF
      %if _text_size+_rodatastr_size+_rodata_size+_data_size+($-$$)<0x1000
        resb 0x1000-(_text_size+_rodatastr_size+_rodata_size+_data_size+($-$$))  ; Workaround to avoid the `<3>mm->brk does not lie within mmap' warning in ibcs-us 4.1.6.
      %endif
    %endif
    resb ($$-$)&3  ; Align end of section to a multiple of 4.
    _bss_end:
    _bss_size equ $-$$  ; _bss_size is a multiple of 4.
    section .text
    %if MINIX2I386
      _data_start_pagemod equ _rodatastr_size+_rodata_size
    %elif S386BSD+V7X86
      _data_start_pagemod equ 0
    %else
      _data_start_pagemod equ _text_size+_rodatastr_size+_rodata_size
    %endif
    %if (_data_start_pagemod&3) && _data_size
      %error ERROR_DATA_NOT_DWORD_ALIGNED
      times -1 nop
    %endif
    _bss_start_pagemod equ _data_start_pagemod+_datauna_size+_data_size+_data_endalign_extra
    %if _bss_start_pagemod&3
      %error ERROR_BSS_NOT_DWORD_ALIGNED
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
%elifidn __OUTPUT_FORMAT__, obj  ; Output of NASM will be sent to a linker: OpenWatcom wlink(1) or GNU ld(1).
  %if ELF==0
    %error ERROR_OUTPUT_FORMAT_OBJ_NEEDS_ELF_EXECUTABLE
    times -1 nop
  %endif
  section _TEXT  USE32 class=CODE align=1
  section CONST  USE32 class=DATA align=1
  section CONST2 USE32 class=DATA align=4
  section _DATA  USE32 class=DATA align=4
  section _BSS   USE32 class=BSS  align=4 NOBITS
  group DGROUP CONST CONST2 _DATA _BSS
  section .text
  extern _edata
  %define _bss_start _edata  ; Both OpenWatcom wlink(1) and GNU ld(1) generate _edata.
  extern _end
  %define _bss_end _end  ; Both OpenWatcom wlink(1) and GNU ld(1) generate _end.

  %macro f_section__TEXT 0
    section _TEXT
  %endm
  %macro f_section_CONST 0
    section _CONST
  %endm
  %macro f_section_CONST2 0
    times ($$-$)&3 db 0  ; Align to a multiple of 4. We do this to provide proper alingment for the %included() .nasm files.
    section _CONST2
  %endm
  %macro f_section__DATA 0
    times ($$-$)&3 db 0  ; Align to a multiple of 4. We do this to provide proper alingment for the %included() .nasm files.
    section _DATA
  %endm
  %macro f_section__DATA_UNA 0
    section _DATA
  %endm
  %macro f_section__BSS 0
    section _BSS
    resb ($$-$)&3  ; Align to a multiple of 4. We do this to provide proper alingment for the %included() .nasm files.
  %endm
  %macro f_end 0
    %if ELF && IS_OSCOMPAT_ADDED==0
      f_section__BSS
      %define IS_OSCOMPAT_ADDED 1
      global __oscompat
      __oscompat: resb 1  ; Initial value doesn't matter.  Put it to the end of the .bss, because other parts may need larger alignment.
    %endif
    f_section__TEXT
  %endm
%elifidn __OUTPUT_FORMAT__, elf  ; Untested. Output of NASM will be sent to a linker: OpenWatcom wlink(1) or GNU ld(1).
  %if ELF==0
    %error ERROR_OUTPUT_FORMAT_ELF_NEEDS_ELF_EXECUTABLE
    times -1 nop
  %endif
  section .text align=1
  section .rodata.str align=1  ; CONST.
  section .rodata align=4
  section .data align=4
  section .bss align=4 nobits
  section .text
  extern _edata
  %define _bss_start _edata  ; Both OpenWatcom wlink(1) and GNU ld(1) generate _edata.

  %macro f_section__TEXT 0
    section .text
  %endm
  %macro f_section_CONST 0
    section .rodata.str
  %endm
  %macro f_section_CONST2 0
    section .rodata
    times ($$-$)&3 db 0  ; Align to a multiple of 4. We do this to provide proper alingment for the %included() .nasm files.
  %endm
  %macro f_section__DATA 0
    section .data
    times ($$-$)&3 db 0  ; Align to a multiple of 4. We do this to provide proper alingment for the %included() .nasm files.
  %endm
  %macro f_section__DATA_UNA 0
    section .data
  %endm
  %macro f_section__BSS
    section .bss
    resb ($$-$)&3  ; Align to a multiple of 4. We do this to provide proper alingment for the %included() .nasm files.
  %endm
  %macro f_end 0
    %if ELF && IS_OSCOMPAT_ADDED==0
      f_section__BSS
      %define IS_OSCOMPAT_ADDED 1
      global __oscompat
      __oscompat: resb 1  ; Initial value doesn't matter.  Put it to the end of the .bss, because other parts may need larger alignment.
    %endif
    f_section__TEXT
  %endm
%else
  %error ERROR_UNSUPPORTED_OUTPUT_FORMAT __OUTPUT_FORMAT__
  times -1 nop
%endif
%define IS_OSCOMPAT_ADDED 0
f_section__TEXT

; --- Including the program source files.
;
; Example invocation: nasm -DINCLUDES="'mysrc1_32.nasm','mysrc2_32.nasm'"

%ifndef INCLUDES
  %error ERROR_MISSING_INCLUDES  ; Specify it like this: nasm -DINCLUDES="'f1.nasm','f2.nasm'"
  times -1 nop
%endif
%macro f_global 1
  %ifdef __GLOBAL_%1
    %error ERROR_MULTIPLE_DEFINITIONS_FOR_%1
    times -1 nop
  %endif
  %define __GLOBAL_%1
  global %1  ; Adds the symbol to the object file (for __OUTPUT_FORMAT__ elf and obj).
%endm
%macro _do_includes 0-*
  %rep %0
    %ifnidn (%1), ()  ; This also does some of the `%define __NEED_...'.
      %include %1
    %endif
    %rotate 1
  %endrep
%endmacro
f_section__TEXT
_do_includes INCLUDES  ; This also does all the `%define __NEED_...'.

; --- Function dependency analysis.

%ifdef __NEED__isatty
  %define __NEED__ioctl_wrapper
%endif
%ifdef __NEED_isatty_
  %define __NEED__ioctl_wrapper
%endif

; --- Defines specific to the operating system.

SYS:  ; Linux i386, FreeBSD i386, SysV SVR3 i386, SysV SVR4 i386, 386BSD syscall numbers.
.exit equ 1
.read equ 3
.write equ 4
.ioctl equ 54  ; Linux i386, FreeBSD i386, SysV SVR3 i386, SysV SVR4 i386, 386BSD, Minix 2.x i386 (in FS).

%if MINIX2I386
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

IOCTL_Minix2:  ; Minix 2.0.4 i386. Minix for 8086 has the low 16 bits only.
.TCGETS     equ 0x80245408  ; _IOR('T',  8, struct termios). sizeof(struct termios) == 36 == 0x24. isatty(3) uses TCGETS.
.TIOCGWINSZ equ 0x80085410  ; _IOR('T', 16, struct winsize). sizeof(struct sinsize) == 8.
.TIOCGPGRP  equ 0x40045412  ; _IOW('T', 18, int). There is a bug, it should be _IOR. sizeof(int) == 4.
.TIOCGETP   equ 0x80087401  ; _IOR('t',  1, struct sgttyb). sizeof(struct sgttyb) == 8.
.TIOCGETC   equ 0x80067403  ; _IOR('t',  3, struct tchars). sizeof(struct tchars) == 6.

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

; !! Add IOCTL_* numbers for Minix 3.2.x, xv6, and Xenix/386.

%if ELF
  SYS_Linux:  ; Linux i386 syscalls. Syscall input: EAX: syscall number; EBX: arg1; ECX: arg2; EDX: arg3.
  .mmap equ 90

  MAP_Linux:  ; Symbolic constants for Linux i386 mmap(2).
  .PRIVATE equ 2
  .FIXED equ 0x10
  .ANONYMOUS equ 0x20

  PROT:  ; Symbolic constants for Linux and FreeBSD mmap(2).
  .READ equ 1
  .WRITE equ 2

  OSCOMPAT:  ; Our platform flags. __oscompat is a bitmask of these.
  .LINUX equ 0  ; (All ELF.) Linux i386 native; Linux i386 running on Linux amd64; Linux (any architecture) qemu-i386; Linux i386 ibcs-us ELF.
  .SYSV equ 1  ; AT&T Unix System V/386 (SysV) Release 3 (SVR3) COFF; AT&T Unix System V/386 (SysV) Release 4 (SVR4) ELF; Coherent 4.x COFF; iBCS2 COFF; Linux i386 ibcs-us COFF.
  .FREENETBSD equ 2  ; (All ELF.) FreeBSD or NetBSD i386. !! Will it work on OpenBSD (probably not, needs extra sections to describe syscall addresses) or DragonFly BSD, or do they mandate conflicting ELF-32 headers?
%endif

f_section__TEXT

; --- Program entry point (_start), system autodetection and exit.

%ifdef __NEED__cstart_
  global _cstart_  ; If there is a main function, the OpenWatcom C compiler generates this symbol.
  _cstart_:
%endif
; Program entry point.
global _start
_start:  ; __noreturn __no_return_address void __cdecl start(int argc, ...);
		; Now the stack looks like (from top to bottom):
		;   dword [esp]: argc
		;   dword [esp+4]: argv[0] pointer
		;   esp+8...: argv[1..] pointers
		;   NULL that ends argv[]
		;   environment pointers
		;   NULL that ends envp[]
		;   ELF Auxiliary Table (auxv): key-value pairs ending with (AT_NULL (== 0), NULL).
		;   argv strings, environment strings, program name etc.
		cld  ; Not all systems set DF := 0, so we play it safe.
%if ELF  ; Auto-detect the operating system (OSCOMPAT) for ELF.
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
		; Linux or FreeBSD >=3.5), FreeBSD 3.5, NetBSD 1.5.2.
		je short .sysv  ; OSCOMPAT.SYSV.
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
		; Now we are running on Linux (OSCOMPAT.LINUX) or some kind
		; of BSD (supported: FreeBSD and NetBSD)
		; (OSCOMPAT.FREENETBSD). Let's distinguish these two cases
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
		int 0x80  ; Linux i386 and FreeBSD i386 syscall. It fails because of the negative fd.
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
		mov eax, _bss_start  ; Symbol provided by the linker.
		add eax, strict dword 0xfff
		and eax, strict dword ~0xfff  ; EDX := _bss_start rounded up to page boundary.
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
		push byte OSCOMPAT.FREENETBSD
		jmp short .detected
  .sysv:
		push byte OSCOMPAT.SYSV
  .detected:
%endif  ; Of %if ELF
%if ELF+S386BSD  ; Not needed for COFF+MINIX2I386+V7X86, because they don't do page-aligned mapping of the executable program file.
		; Now we clear the .bss part of the last page of .data.
		; 386BSD 1.0 and some early Linux kernels put junk there if
		; there is junk at the end of the ELF-32 executable program
		; file after the last PT_LOAD file byte.
		;
		; !! Do we ever have to clear subsequent .bss pages? Not on Linux ELF, because the mmap(...) above has cleared it.
		xor eax, eax
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
		mov [__oscompat], al  ; We must do this after the clearing of the first page of .bss above.
%endif
%ifdef __NEED___argc
		pop eax  ; EAX := argc.
		mov edx, esp  ; EDX := argv.
		lea ebx, [esp+eax*4+4]  ; EBX := envp.
  %ifdef __GLOBAL__main  ; int __cdecl main(int argc, char **argv, char **envp).
		push ebx
		push edx
		push eax
  %elifdef __GLOBAL_main_  ; int __watcall main(int argc, char **argv, char **envp).
  %endif
%endif
%ifdef __GLOBAL__main  ; int __cdecl main(...).
  %ifdef __GLOBAL_main_  ; int __watcall main(...).
    %error ERROR_MULTIPLE_MAIN_FUNCTIONS_CDECL_WATCALL
    times -1 nop
  %endif
		call _main
%elifdef __GLOBAL_main_  ; int __watcall main(...).
		call main_
%else
  %error ERROR_MISSING_MAIN_FUNCTION
  times -1 nop
%endif
		; Fall through to __watcall _exit(...). We already have the return value of main(...) in EAX.

global _exit_
_exit_:  ; __noreturn void __watcall _exit(int exit_code);
global exit_
exit_:  ; __noreturn void __watcall exit(int exit_code);
		push eax  ; exit_code argument of __cdecl _exit(...) below.
		push eax  ; Fake return address.
		; Fall through to __cdecl  _exit(...).

global __exit
__exit:  ; __noreturn void __cdecl _exit(int exit_code);
global _exit
_exit:  ; __noreturn void __cdecl exit(int exit_code);
%if MINIX2I386
		push ebx  ; Save.
		sub esp, byte 36-3*4  ; Message struct.
		push dword [esp+36-3*4+2*4]  ; .m1_i1, at offset 8 of the struct. Set it to exit_code.
		push byte SYS.exit  ; .m_type, at offset 4 of the struct.
		push byte MINIX_WHO.MM  ; .m_source, at offset 0 of the struct. Ignored by sendrec in the Minix kernel.
		mov ebx, esp
		; Fall through to minix_syscall_cont.
%else
		push byte SYS.exit
		; Fall through to simple_syscall3_pop
%endif

%if MINIX2I386
  minix_syscall_cont:
		pop eax  ; MINIX_WHO.FS or MINIX_WHO.MM.
		push eax  ; Put it back to .m_source, at offset 0. We do it just to manage the balance of the stack.
		push byte 3  ; sendrec.
		pop ecx
		int 0x21  ; Minix 2.x i386 syscall. Inputs: EAX, EBX and ECX; returns EAX.
		pop ecx  ; Discard .m_source, at offset 0.
		pop ecx  ; ECX := .m_type, at offset 4. Also discards it.
		add esp, byte 36-2*4  ; Clean up message struct from stack.
		pop ebx  ; Restore.
		test eax, eax
		jnz short .have_status_tested
		or eax, ecx  ; EAX := ECX. Also sets SF for `jns' below.
  .have_status_tested:
		jns short .ret
		; Now we could get errno from the negative of EAX.
		; Fall through to .error.
%else
  ; Calls a single systcall of 0, 1, 2 or 3. arguments.
  ;
  ; Input stack: dword [esp]: syscall number (SYS....); dword [esp+4]: return
  ; address; dword [esp+2*4] syscall argument 1; dword [esp+3*4] syscall
  ; argument 2; dword [esp+4*4] syscall argument 3.
  ;
  ; Output: EAX is -1 on error, otherwise EAX contains the result. The syscall
  ; number and the return address have been popped from the stack, the caller
  ; is responsible for cleaning up the rest.
  simple_syscall3_pop:
		pop eax
		; Fall through to simple_syscall3_eax.

  simple_syscall3_eax:
  ; TOSO(pts): Size optimization: Don't generate the return value porcessing if the program uses only exit(...).
  %if COFF+S386BSD
		call 7:dword 0  ; SysV syscall.
		jnc short .ret
  %elif V7X86
		int 0x30  ; v7x86 syscall.
		jnc short .ret
  %elif ELF
		mov cl, [__oscompat]
		cmp cl, OSCOMPAT.SYSV
		je short .sysv
		test cl, cl  ; OSCOMPAT.LINUX.
		jnz short .freenetbsd  ; OSCOMPAT.FREENETBSD.
    .linux:
		push ebx  ; Save.
		mov ebx, [esp+2*4]  ; Syscall argument 1.
		mov ecx, [esp+3*4]  ; Syscall argument 2.
		mov edx, [esp+4*4]  ; Syscall argument 3.
		int 0x80  ; Linux i386 syscall.
		pop ebx  ; Restore.
		test eax, eax
		jns short .ret
		jmp short .error
    .sysv:
		call 7:dword 0  ; SysV i386 syscall.
		dw 0xb966  ; `mov cx, ...' to skip over the `int 0x80' below.
    .freenetbsd:
		int 0x80  ; FreeBSD i386 syscall.
    %if $-.freenetbsd!=2
      %error ERROR_SKIP_FREEBSD_MUST_BE_2_BYTES  ; For the `dw 0xb966' above.
      times -1 nop
    %endif
    .after_syscall:
                  jnc short .ret
  %else
    %error ERROR_MISSING_SYSCALL_WRAPPER
    times -1 nop
  %endif
%endif
.error:
		;neg eax  ; Negation needed only for Linux and Minix 2.x.
		;mov [errno], eax  ; errno is unused in this program.
		or eax, byte -1  ; Return -1 to indicate error.
.ret:
%ifdef __NEED___argc
  global __argc  ; If there is a main function with argc+argv, the OpenWatcom C compiler generates this symbol.
  __argc:
%endif
		ret

; --- Syscall wrappers (except for SYS.exit, which has been done above).

%ifdef __NEED__write
  global _write
  _write:  ; int __cdecl write(int fd, const void *buf, unsigned count);
  %if MINIX2I386
		push ebx  ; Save.
		sub esp, byte 36-2*4  ; Message struct.
		push byte SYS.write
    .common:
		push byte MINIX_WHO.FS  ; .m_source, at offset 0 of the struct. Ignored by sendrec in the Minix kernel.
		mov ebx, esp
		mov eax, [ebx+36+2*4]  ; fd.
		mov [ebx+8], eax  ; .m1_i1.
		mov eax, [ebx+36+3*4]  ; buf.
		mov [ebx+20], eax  ; .m1_p1.
		mov eax, [ebx+36+4*4]  ; count.
		mov [ebx+12], eax  ; ; .m1_i2.
		jmp short minix_syscall_cont
  %else
		push byte SYS.write
		jmp short simple_syscall3_pop
  %endif
%endif

%ifdef __NEED__read
  global _read
  _read:  ; int __cdecl read(int fd, void *buf, unsigned count);
  %if MINIX2I386
		push ebx  ; Save.
		sub esp, byte 36-2*4  ; Message struct.
		push byte SYS.read  ; .m_type, at offset 4 of the struct.
    %ifdef __NEED__write
		jmp short _write.common
    %else
		push byte MINIX_WHO.FS  ; .m_source, at offset 0 of the struct. Ignored by sendrec in the Minix kernel.
		mov ebx, esp
		mov eax, [ebx+36+2*4]  ; fd.
		mov [ebx+8], eax  ; .m1_i1.
		mov eax, [ebx+36+3*4]  ; buf.
		mov [ebx+20], eax  ; .m1_p1.
		mov eax, [ebx+36+4*4]  ; count.
		mov [ebx+12], eax  ; ; .m1_i2.
		jmp short minix_syscall_cont
    %endif
  %else
		push byte SYS.read
		jmp short simple_syscall3_pop
  %endif
%endif

%ifdef __NEED__ioctl_wrapper
  global _ioctl_wrapper  ; The `request' and `arg' is not system-independent, but still exporting it to aid debugging.
  _ioctl_wrapper:  ; int __cdecl ioctl_wrapper(int fd, unsigned long request, void *arg);
  %if MINIX2I386
		push ebx  ; Save.
		sub esp, byte 36-5*4  ; Message struct.
		push dword [esp+36-5*4+3*4]  ; .m2_i3, at offset 16 of the struct (TTY_REQUEST == COUNT == m2_i3 == m_u.m_m2.m2i3 == 16). Set it to request.
		push eax  ; Put dummy value at offset 12 of the struct.
		push dword [esp+36-3*4+2*4]  ; .m2_i1, at offset 8 of the struct (TTY_LINE == DEVICE == m2_i1 == m_u.m_m2.m2i1 == 8). Set it to fd.
		push byte SYS.ioctl  ; .m_type, at offset 4 of the struct.
		push byte MINIX_WHO.FS  ; .m_source, at offset 0 of the struct. Ignored by sendrec in the Minix kernel.
		mov ebx, esp
		mov eax, [ebx+36+4*4]  ; arg.
		mov [ebx+28], eax  ; .m2_p1, at offset 28 of the struct (ADDRESS == m2_p1 == m_u.m_m2.m2p1 == 28).
		jmp short minix_syscall_cont
  %else
		push byte SYS.ioctl
		jmp short simple_syscall3_pop
  %endif
%endif

; By adding and using the __watcall counterpart for simple_syscall3_pop
; (__NEED_simple_syscall3_WAT), the program file doesn't become shorter,
; because FreeBSD needs the syscall arguments on the stack.

; --- System-specific libc functions.

%ifdef __NEED__isatty
  global _isatty
  _isatty:  ; int __cdecl isatty(int fd);
  %if COFF
    ISATTY_TERMIOS_SIZE equ 18+2  ; sizeof(struct termio) == 18 for SYSV and iBCS2 (we round it up to 20, to dword bounadry).
		sub esp, byte ISATTY_TERMIOS_SIZE
		push esp  ; Argument 3 arg of _ioctl_wrapper.
		push strict dword IOCTL_Linux.TCGETS  ; We need the one which works on SysV. Same value as IOCTL_SysV.TCGETA, IOCTL_Coherent4.TCGETA, IOCTL_iBCS2.TCGETA.
  %elif S386BSD
    ISATTY_TERMIOS_SIZE equ 44  ; sizeof(struct termios) == 44 on 386BSD.
		sub esp, byte ISATTY_TERMIOS_SIZE
		push esp  ; Argument 3 arg of _ioctl_wrapper.
		push strict dword IOCTL_386BSD.TIOCGETA
  %elif MINIX2I386
    ISATTY_TERMIOS_SIZE equ 36  ; sizeof(struct termio) == 36 on MINIX2I386.
		sub esp, byte ISATTY_TERMIOS_SIZE
		push esp  ; Argument 3 arg of _ioctl_wrapper.
		push strict dword IOCTL_Minix2.TCGETS  ; Alternatively, we could use IOCTL_Minix2.TIOCGETP with the smaller sizeof(struct sgttyb) == 8. Or use TIOCGETP (sizeof(int) == 4) everywhere available.
  %elif V7X86
    ISATTY_TERMIOS_SIZE equ 8  ; sizeof(struct agttyb) == 8 on V7X86.
		sub esp, byte ISATTY_TERMIOS_SIZE
		push esp  ; Argument 3 arg of _ioctl_wrapper.
		push strict dword IOCTL_v7x86.TIOCGETP  ; Alternatively, we could use IOCTL_v7x86.TIOCGETD with the smaller sizeof(int) == 3.
  %else  ; ELF.
    ISATTY_TERMIOS_SIZE equ 44  ; Maximum sizeof(struct termios), sizeof(struct termio), sizeof(struct sgttyb) for OSCOMPAT.LINUX, OSCOMPAT.SYSV and OSCOMPAT.FREENETBSD.
		sub esp, byte ISATTY_TERMIOS_SIZE
		push esp  ; Argument 3 arg of _ioctl_wrapper.
		cmp byte [__oscompat], OSCOMPAT.FREENETBSD
		je short .freenetbsd
    .linux_or_sysv:
		push strict dword IOCTL_Linux.TCGETS  ; Same value as IOCTL_SysV.TCGETA, IOCTL_Coherent4.TCGETA, IOCTL_iBCS2.TCGETA.
		jmp .ioctl_number_pushed
    .freenetbsd:
		push strict dword IOCTL_FreeBSD.TIOCGETA  ; Same value as IOCTL_NetBSD.TIOCGETA.
  %endif
  .ioctl_number_pushed:
		push dword [esp+2*4+ISATTY_TERMIOS_SIZE+4]  ; Argument fd of this isatty(...).
		call _ioctl_wrapper
		add esp, byte 3*4+ISATTY_TERMIOS_SIZE  ; Clean up the arguments of _ioctl_wrapper above from the stack, and also clean up the arg struct.
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
		ret
%endif

%ifdef __NEED_isatty_
  global isatty_
  isatty_:  ; int __watcall isatty(int fd);
		push ecx  ; Save.
		push edx  ; Save.
  %if COFF
    ISATTY_TERMIOS_SIZE equ 18+2  ; sizeof(struct termio) == 18 for SYSV and iBCS2 (we round it up to 20, to dword bounadry).
		sub esp, byte ISATTY_TERMIOS_SIZE
		push esp  ; Argument 3 arg of _ioctl_wrapper.
		push strict dword IOCTL_Linux.TCGETS  ; We need the one which works on SysV. Same value as IOCTL_SysV.TCGETA, IOCTL_Coherent4.TCGETA, IOCTL_iBCS2.TCGETA.
  %elif S386BSD
    ISATTY_TERMIOS_SIZE equ 44  ; sizeof(struct termios) == 44 on 386BSD.
		sub esp, byte ISATTY_TERMIOS_SIZE
		push esp  ; Argument 3 arg of _ioctl_wrapper.
		push strict dword IOCTL_386BSD.TIOCGETA
  %elif MINIX2I386
    ISATTY_TERMIOS_SIZE equ 36  ; sizeof(struct termio) == 36 on MINIX2I386.
		sub esp, byte ISATTY_TERMIOS_SIZE
		push esp  ; Argument 3 arg of _ioctl_wrapper.
		push strict dword IOCTL_Minix2.TCGETS  ; Alternatively, we could use IOCTL_Minix2.TIOCGETP with the smaller sizeof(struct sgttyb) == 8. Or use TIOCGETP (sizeof(int) == 4) everywhere available.
  %elif V7X86
    ISATTY_TERMIOS_SIZE equ 8  ; sizeof(struct agttyb) == 8 on V7X86.
		sub esp, byte ISATTY_TERMIOS_SIZE
		push esp  ; Argument 3 arg of _ioctl_wrapper.
		push strict dword IOCTL_v7x86.TIOCGETP  ; Alternatively, we could use IOCTL_v7x86.TIOCGETD with the smaller sizeof(int) == 3.
  %else  ; ELF.
    ISATTY_TERMIOS_SIZE equ 44  ; Maximum sizeof(struct termios), sizeof(struct termio), sizeof(struct sgttyb) for OSCOMPAT.LINUX, OSCOMPAT.SYSV and OSCOMPAT.FREENETBSD.
		sub esp, byte ISATTY_TERMIOS_SIZE
		push esp  ; Argument 3 arg of _ioctl_wrapper.
		cmp byte [__oscompat], OSCOMPAT.FREENETBSD
		je short .freenetbsd
    .linux_or_sysv:
		push strict dword IOCTL_Linux.TCGETS  ; Same value as IOCTL_SysV.TCGETA, IOCTL_Coherent4.TCGETA, IOCTL_iBCS2.TCGETA.
		jmp .ioctl_number_pushed
    .freenetbsd:
		push strict dword IOCTL_FreeBSD.TIOCGETA  ; Same value as IOCTL_NetBSD.TIOCGETA.
  %endif
  .ioctl_number_pushed:
		push eax  ; Argument fd of this isatty(...).
		call _ioctl_wrapper
		add esp, byte 3*4+ISATTY_TERMIOS_SIZE  ; Clean up the arguments of _ioctl_wrapper above from the stack, and also clean up the arg struct.
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
		pop edx  ; Restore.
		pop ecx  ; Restore.
		ret
%endif

; --- System-independent libc functions.
;
%ifdef __NEED__strlen
  global _strlen  ; Longer code than strlen_.
  _strlen:  ; size_t __cdecl strlen(const char *s);
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

%ifdef __NEED_strlen_
  global strlen_
  strlen_:  ; size_t __watcall strlen(const char *s);
  ; unsigned int __cdecl strlen(const char *s);
		push edi  ; Save.
		xchg edi, eax  ; EDI := argument s; EAX : = junk.
		xor eax, eax
		or ecx, byte -1  ; ECX := -1.
		repne scasb
		sub eax, ecx
		dec eax
		dec eax
		pop edi  ; Restore.
		ret
%endif

%ifdef __NEED__memset
  global _memset  ; Longer code than memset_.
  _memset:  ; void * __cdecl memset(void *s, int c, size_t n);
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

%ifdef __NEED_memset_
  global memset_
  memset_:  ; void * __watcall memset(void *s, int c, size_t n);
		push edi  ; Save.
		xchg edi, eax  ; EDI := EAX (argument s); EAX := junk.
		xchg eax, edx  ; EAX := EDX (argument c); EDX := junk.
		xchg ecx, ebx  ; ECX := EBX (argument n); EBX := saved ECX.
		push edi
		rep stosb
		pop eax  ; Result is argument s.
		xchg ecx, ebx  ; ECX := saved ECX; EBX := 0 (unused).
		pop edi  ; Restore.
		ret
%endif

%ifdef __NEED__memcpy
  global _memcpy  ; Longer code than memcpy_.
  _memcpy:  ; void * __cdecl memcpy(void *dest, const void *src, size_t n);
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

%ifdef __NEED_memcpy_
  global memcpy_
  memcpy_:  ; void * __watcall memcpy(void *dest, const void *src, size_t n);
		push edi
		xchg esi, edx
		xchg edi, eax  ; EDI := dest; EAX := junk.
		xchg ecx, ebx
		push edi
		rep movsb
		pop eax  ; Will return dest.
		xchg ecx, ebx  ; Restore ECX from REGARG3. And REGARG3 is scratch, we don't care what we put there.
		xchg esi, edx  ; Restore ESI.
		pop edi
		ret
%endif

f_end

; __END__
