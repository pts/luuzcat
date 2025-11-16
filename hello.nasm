;
; hello.nasm: simple test program for porting progx86.nasm to new systems
; by pts@fazekas.hu at Tue Oct 14 20:27:02 CEST 2025
;
; Compile with: nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'hello.nasm'" -o hello.elf progx86.nasm && chmod +x hello.elf
; Compile with: nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'hello.nasm'" -DCOFF -DCOFF_PROGRAM_NAME="'hello'" -o hello.coff progx86.nasm && chmod +x hello.coff
; Compile with: nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'hello.nasm'" -DS386BSD -o hello.3b progx86.nasm && printf ThereIsJunk >>hello.3b && chmod +x hello.3b
; Compile with: nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'hello.nasm'" -DMINIXI386 -o hello.mi3 progx86.nasm && printf ThereIsJunk >>hello.m23 && chmod +x hello.m23
; Compile with: nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'hello.nasm'" -DV7X86 -o hello.v7x progx86.nasm && printf ThereIsJunk >>hello.v7x && chmod +x hello.v7x
; Compile with: nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'hello.nasm'" -DXV6I386 -o hello.x63 progx86.nasm && printf ThereIsJunk >>hello.x63 && chmod +x hello.x63
; Compile with: nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -DINCLUDES="'hello.nasm'" -DWIN32WL -o hellop.o progx86.nasm && wlink op q form win nt ru con=3.10 op h=4K com h=0 op st=64K com st=64K disa 1080 op noext op d op nored op start=_start n hellop.exe f hellop.o
; Compile with: nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'hello.nasm'" -DDOSCOM -o hello.com progx86.nasm && printf ThereIsJunk >>hello.com
; Compile with: nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'hello.nasm'" -DDOSEXE -o hellod.exe progx86.nasm && printf ThereIsJunk >>hellod.exe

__prog_default_cpu_and_bits  ; Not needed, just to check that progx86.nasm is included.
;cpu 386
;bits 32

%if DOSCOM+DOSEXE
  extern write_
%else
  extern _write
%endif
;extern __argc
%if 0
  extern _isatty
%else
  extern isatty_
%endif
extern _exit
%if DOSCOM+DOSEXE
%elif 0
  extern setmode_
%endif

global main_
main_:
%if DOSCOM+DOSEXE
		mov bx, msg.size
		mov dx, msg
		mov ax, 1  ; STDOUT_FILENO.
		call write_
  %if 0
		push ax
		call _exit
  %endif
		cmp word [bssvar], byte 0
		je short .ok
		mov ax, 7  ; Indicate failure with exit(7) if there was junk in the file after the last .data byte in the header.
		push ax
		call _exit  ; Doesn't return.
  .ok:
		xor ax, ax  ; EAX := STDIN_FILENO == 0.
  %if 0
		push ax
		call _isatty
		pop cx  ; Clean up argument of _isatty above from the stack.
  %else
		call isatty_  ; isatty(STDIN_FILENO).
  %endif
%else
  %if 0
		xor eax, eax
		inc eax  ; EAX := STDOUT_FILENO.
		mov dl, 4  ; O_BINARY
		call setmode_
  %endif
		push strict byte msg.size
		push strict dword msg
		push strict byte 1  ; STDOUT_FILENO.
		call _write
		add esp, 3*4  ; Clean up arguments of _write above from the stack.
  %if 0
		push eax
		call _exit
  %endif
		cmp dword [bssvar], byte 0
		je short .ok
		push byte 7  ; Indicate failure with exit(7) if there was junk in the file after the last .data byte in the header.
		call _exit  ; Doesn't return.
  .ok:
		xor eax, eax  ; EAX := STDIN_FILENO == 0.
  %if 0
		push eax
		call _isatty
		pop ecx  ; Clean up argument of _isatty above from the stack.
  %else
		call isatty_  ; isatty(STDIN_FILENO).
  %endif
%endif
		ret  ; exit(1) if stdin is a TTY, 0 otherwise.

%idefine foo bar
%ifndef FOO
%error FOO1
%endif
%undef foo
%ifdef FOO
%error FOO1
%endif

section .rodata.str  ; Synonym of CONST .
;section CONST align=8  ; Error, not allowed to specify alignment.
;section  ; Error: missing argument.
;section a, b  ; Error too many arguments.
;section CONST
;section _DATA_UNA
;section CONST2
msg:
		db 'Hello, '  ; , 10
		db 'World!', 10
.size equ $-msg
		;times ($$-$)&3 db '_'  ; Alingment padding to avoid NULs.

section _BSS
bssvar:
		resb 4

