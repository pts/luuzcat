;
; hello.nasm: simple test program for porting progi386.nasm to new systems
; by pts@fazekas.hu at Tue Oct 14 20:27:02 CEST 2025
;
; Compile with: nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'hello.nasm'" -o hello.elf progi386.nasm && chmod +x hello.elf
; Compile with: nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'hello.nasm'" -DCOFF -DCOFF_PROGRAM_NAME="'hello'" -o hello.coff progi386.nasm && chmod +x hello.coff
; Compile with: nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'hello.nasm'" -DS386BSD -o hello.3b progi386.nasm && printf ThereIsJunk >>hello.3b && chmod +x hello.3b
; Compile with: nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'hello.nasm'" -DMINIX2I386 -o hello.m23 progi386.nasm && printf ThereIsJunk >>hello.m23 && chmod +x hello.m23
; Compile with: nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'hello.nasm'" -DV7X86 -o hello.v7x progi386.nasm && printf ThereIsJunk >>hello.v7x && chmod +x hello.v7x

;%define __NEED___argc
%define __NEED__write
%define __NEED_isatty_
%define __NEED__exit
f_global main_
main_:

		push strict byte msg.size
		push strict dword msg
		push strict byte 1  ; STDOUT_FILENO.
		call _write
		add esp, 3*4  ; Clean up arguments of _write above from the stack.
		cmp dword [bssvar], byte 0
		je short .ok
		push byte 7  ; Indicate failure with exit(7) if there was junk in the file after the last .data byte in the header.
		call _exit  ; Doesn't return.
.ok:
		xor eax, eax  ; EAX := STDIN_FILENO == 0.
		call isatty_  ; isatty(STDIN_FILENO).
		ret  ; exit(1) if stdin is a TTY, 0 otherwise.

f_section_CONST
;f_section__DATA_UNA
;f_section_CONST2
msg:
		db 'Hello, World!', 10
.size equ $-msg
		;times ($$-$)&3 db '_'  ; Alingment padding to avoid NULs.

f_section__BSS
bssvar:
		resb 4

; __END__
