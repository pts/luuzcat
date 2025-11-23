# This is a smiplified example Makefile for compiling luuzcat on a Unix
# system with a C compiler. For the actual release builds, run
# `./compile.sh' on a Linux i386 or Linux amd64 system instead.

.PHONY: all clean

# Uncomment the next line to have some GCC optimization and warning flags enabled for the release build.
#CFLAGS = -s -O2 -ansi -pedantic -W -Wall -Wextra -Wstrict-prototypes -Werror-implicit-function-declaration

all: luuzcat

luuzcat: luuzcat.h luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c
	cc $(CFLAGS) -o luuzcat luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c

clean:
	rm -f luuzcat
