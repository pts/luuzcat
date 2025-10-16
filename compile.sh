#! /bin/sh --
set -ex
test "$0" = "${0%/*}" || cd "${0%/*}"

gcc -m64 -fsanitize=address -g -O2 -ansi -pedantic -W -Wall -Wextra -Wstrict-prototypes -Werror-implicit-function-declaration -o luuzcat luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c
./luuzcat <XFileMgro.sz >XFileMgro
    cmp XFileMgro.good XFileMgro
./luuzcat <test_C1.bin.C >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_pack.z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_pack_old.z >test_C1.bin
    cmp test_C1.good test_C1.bin
# test_C1_pack_old3.z created using pack.c in https://github.com/pts/pts-opack-port
./luuzcat <test_C1_pack_old3.z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.advdef.gz >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.advdef.qz >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.advdef.zlib >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat -r <test_C1.advdef.deflate >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat -r <test_C1.advdef.gz >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_old.F >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.F >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_old16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_new16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.zip >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_split.zip >test_C1.bin
    cmp test_C1.good test_C1.bin

# !! Add test for concatenated streams.

g++ -m32 -s -O2 -ansi -pedantic -W -Wall -Wextra -o luuzcat luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c
./luuzcat <XFileMgro.sz >XFileMgro
  cmp XFileMgro.good XFileMgro
./luuzcat <test_C1.bin.C >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_pack.z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_pack_old.z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.advdef.gz >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.advdef.qz >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.advdef.zlib >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat -r <test_C1.advdef.deflate >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat -r <test_C1.advdef.gz >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_old.F >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.F >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_old16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_new16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.zip >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_split.zip >test_C1.bin
    cmp test_C1.good test_C1.bin

# TODO(pts): Compile with --gcc=4.8 and extra warnings?
minicc -ansi -pedantic -march=i386 -Wno-n201 -o luuzcat luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c
./luuzcat <XFileMgro.sz >XFileMgro
  cmp XFileMgro.good XFileMgro
./luuzcat <test_C1.bin.C >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_pack.z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_pack_old.z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.advdef.gz >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.advdef.qz >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.advdef.zlib >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat -r <test_C1.advdef.deflate >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat -r <test_C1.advdef.gz >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_old.F >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.F >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_old16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_new16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.zip >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_split.zip >test_C1.bin
    cmp test_C1.good test_C1.bin

wcc386 -q -bt=linux -D_NOSYS32 -os -s -j -ei -of+ -ec -fr -zl -zld -zp=4 -3r -za -wx -wce=308 -wcd=201 -fo=.o luuzcat.c
wcc386 -q -bt=linux -D_NOSYS32 -os -s -j -ei -of+ -ec -fr -zl -zld -zp=4 -3r -za -wx -wce=308 -wcd=201 -fo=.o unscolzh.c
wcc386 -q -bt=linux -D_NOSYS32 -os -s -j -ei -of+ -ec -fr -zl -zld -zp=4 -3r -za -wx -wce=308 -wcd=201 -fo=.o uncompact.c
wcc386 -q -bt=linux -D_NOSYS32 -os -s -j -ei -of+ -ec -fr -zl -zld -zp=4 -3r -za -wx -wce=308 -wcd=201 -fo=.o unopack.c
wcc386 -q -bt=linux -D_NOSYS32 -os -s -j -ei -of+ -ec -fr -zl -zld -zp=4 -3r -za -wx -wce=308 -wcd=201 -fo=.o unpack.c
wcc386 -q -bt=linux -D_NOSYS32 -os -s -j -ei -of+ -ec -fr -zl -zld -zp=4 -3r -za -wx -wce=308 -wcd=201 -fo=.o undeflate.c
wcc386 -q -bt=linux -D_NOSYS32 -os -s -j -ei -of+ -ec -fr -zl -zld -zp=4 -3r -za -wx -wce=308 -wcd=201 -fo=.o uncompress.c
wcc386 -q -bt=linux -D_NOSYS32 -os -s -j -ei -of+ -ec -fr -zl -zld -zp=4 -3r -za -wx -wce=308 -wcd=201 -fo=.o unfreeze.c
perl=tools/miniperl-5.004.04.upx
"$perl" -e0 || perl=perl  # Use the system perl(1) if tools is not available.
fi=0
for f in luuzcat.o unscolzh.o uncompact.o unopack.o unpack.o undeflate.o uncompress.o unfreeze.o; do
  fi=$((fi+1))  # For local variables.
  wdis -a -fi -fu -i=@ "$f" >"${f%.*}_32.wasm"
  "$perl" wasm2nasm.pl "$fi" <"${f%.*}_32.wasm" >"${f%.*}_32.nasm"
  # objconv 2.54 is buggy, it creates wrong destination for some `call' instructions.
  # tools/objconv-2.54.upx -fnasm "$f" "${f%.*}_32.nasm"
done
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -DINCLUDES="'luuzcat_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'" -o luuzcatx.o progi386.nasm
# * wlink generates a much larger ELF-32 file than necessary (e.g. it aligns
#   .data to 4096 bytes in the file), but it can add the symbols, so it's useful for debugging.
# * wlink op exportall == op exporta (undocumented flag) keeps symbols (i.e. no run of `strip').
# * `disa 1080` disables the useless message: *Warning! W1080: file luuzcatx.o is a 16-bit object file*.
#   NASM can't generate the relevant COMENT record..
wlink op q form elf ru freebsd disa 1080 op noext op d op nored op start=_start op norelocs op exporta n luuzcatx.elf f luuzcatx.o
"$perl" fix_elf_edata.pl luuzcatx.elf  # Fix buggy _end and _edata generated by wlink(1).
./luuzcatx.elf <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
ibcs-us ./luuzcatx.elf <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin

nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'luuzcat_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'"                                        -o luuzcat.elf  progi386.nasm
chmod +x luuzcat.elf
./luuzcat.elf <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
ibcs-us ./luuzcat.elf <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin

rm -f luuzcat.3b
nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'luuzcat_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'" -DS386BSD                              -o luuzcat.3b   progi386.nasm
chmod +x luuzcat.3b
# We can't run luuzcat.3b on Linux i386 directly, so we just check for the file size.
test -s luuzcat.3b

rm -f luuzcat.m23
nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'luuzcat_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'" -DMINIX2I386                           -o luuzcat.m23   progi386.nasm
chmod +x luuzcat.m23
# We can't run luuzcat.m23 on Linux i386 directly, so we just check for the file size.
test -s luuzcat.m23

rm -f luuzcat.v7x
nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'luuzcat_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'" -DV7X86                                -o luuzcat.v7x   progi386.nasm
chmod +x luuzcat.v7x
# We can't run luuzcat.v7x on Linux i386 directly, so we just check for the file size.
test -s luuzcat.v7x

rm -f luuzcat.x63
nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'luuzcat_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'" -DXV6I386                              -o luuzcat.x63   progi386.nasm
chmod +x luuzcat.x63
# We can't run luuzcat.x63 on Linux i386 directly, so we just check for the file size.
test -s luuzcat.x63

nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'luuzcat_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'" -DCOFF -DCOFF_PROGRAM_NAME="'luuzcat'" -o luuzcat.coff progi386.nasm
chmod +x luuzcat.coff
ibcs-us ./luuzcat.coff <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin

# We compile with the OpenWatcom C compiler to a DOS 8086 .com program, but we don't use the OpenWatcom libc.
wcc -q -bt=com -D_DOSCOMSTART -os -zl -j -ms -s -W -w4 -wx -we -wcd=201 -za -oi -0 -g=DGROUP -fo=.o luuzcat.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -j -ms -s -W -w4 -wx -we -wcd=201 -za -oi -0 -g=DGROUP -fo=.o unscolzh.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -j -ms -s -W -w4 -wx -we -wcd=201 -za -oi -0 -g=DGROUP -fo=.o uncompact.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -j -ms -s -W -w4 -wx -we -wcd=201 -za -oi -0 -g=DGROUP -fo=.o unopack.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -j -ms -s -W -w4 -wx -we -wcd=201 -za -oi -0 -g=DGROUP -fo=.o unpack.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -j -ms -s -W -w4 -wx -we -wcd=201 -za -oi -0 -g=DGROUP -fo=.o undeflate.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -j -ms -s -W -w4 -wx -we -wcd=201 -za -oi -0 -g=DGROUP -fo=.o uncompress.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -j -ms -s -W -w4 -wx -we -wcd=201 -za -oi -0 -g=DGROUP -fo=.o unfreeze.c
wlink op q form dos com op d op nored op start=_comstart_ n luuzcatc.com f luuzcat.o f unscolzh.o f uncompact.o f unopack.o f unpack.o f undeflate.o f uncompress.o f unfreeze.o
rm -f luuzcat.o unscolzh.o uncompact.o unopack.o unpack.o undeflate.o unfreeze.o
./kvikdos luuzcatc.com <XFileMgro.sz >XFileMgro
    cmp XFileMgro.good XFileMgro
./kvikdos luuzcatc.com <test_C1.bin.C >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com <test_C1_pack.z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com <test_C1_pack_old.z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com <test_C1.advdef.gz >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com <test_C1.advdef.qz >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com <test_C1.advdef.zlib >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com -r <test_C1.advdef.deflate >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com -r <test_C1.advdef.gz >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com <test_C1_old.F >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com <test_C1.F >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com <test_C1_old16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com <test_C1_new16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com <test_C1.zip >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com <test_C1_split.zip >test_C1.bin
    cmp test_C1.good test_C1.bin

# Compile with OpenWatcom to a DOS 8086 .exe program using the OpenWatcom libc.
#
# This is just to prove a point that it compiles without any trickery, and
# to compare the libc size to of luuzcatc.com. The luuzcatc.com linked
# against the custom libc (_DOSCOMSTART) above is smaller, so that should be
# used in production instead.
#
# The default is the small model (`owcc -mcmodel=s').
owcc -bdos -s -Os -fno-stack-check -march=i86 -W -Wall -Wextra -Werror -Wno-n201 -std=c89 -o luuzcatd.exe luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c
./kvikdos luuzcatd.exe <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatd.exe -r <test_C1.advdef.deflate >test_C1.bin
    cmp test_C1.good test_C1.bin

# Compile with OpenWatcom to a Win32 .exe program using the mmlibc386 libc.
~/Downloads/windows_nt_qemu/hdachs/mmlibcc.sh -bwin32 -o luuzcatm.exe luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c
# We need non-empty command-line because dosbox.nox.static incorrectly reports that stdin is a TTY.
dosbox.nox.static --cmd --mem-mb=2 ~/prg/mwpestub/mwperun.exe luuzcatm.exe -cd <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
dosbox.nox.static --cmd --mem-mb=2 ~/prg/mwpestub/mwperun.exe luuzcatm.exe -r <test_C1.advdef.deflate >test_C1.bin
    cmp test_C1.good test_C1.bin

# Compile with OpenWatcom to a Win32 .exe program using the OpenWatcom libc.
#
# This is just to prove a point that it compiles without any trickery, and
# to compare the libc size to of luuzcatm.exe. The luuzcatm.exe linked
# against mmlibc386 above is smaller, so that should be used in production
# instead.
owcc -bwin32 -Wl,runtime -Wl,console=3.10 -s -Os -fno-stack-check -march=i386 -W -Wall -Wextra -Werror -Wno-n201 -std=c89 -o luuzcatw.exe luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c
# We need non-empty command-line because dosbox.nox.static incorrectly reports that stdin is a TTY.
dosbox.nox.static --cmd --mem-mb=2 ~/prg/mwpestub/mwperun.exe luuzcatw.exe -cd <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
dosbox.nox.static --cmd --mem-mb=2 ~/prg/mwpestub/mwperun.exe luuzcatw.exe -r <test_C1.advdef.deflate >test_C1.bin
    cmp test_C1.good test_C1.bin

# Compile with Borland Turbo C++ 1.x to a DOS 8086 .exe program.
#
# This is just to prove a point that it compiles with Borland Turbo C++ 1.00
# and 1.01. The luuzcatc.com compiled with the OpenWatcom C compiler is
# smaller and faster, so that should be used in production instead. And it
# is also easier to cross-compile.
MODEL=s
#ANSI=-A  # This wouldn't work because we use far pointers.
ANSI=
OPTJUMP=-O
MERGESTR=-d
ALIGN=-a
SPEED=
#SPEED=-G  # Bloats up nasm.exe by 6 kb for questionable benefit.
NOFLOAT=-f-
WARN=-w
#WMSIG=-w-sig
WMSIG=
#WRCH=
WRCH=-w-rch  # Don't warn about unreachable code (e.g. `if (sizeof(unsigned int) > 2) { ... }').
cat uncompact.c  >un1.c  # Avoid long filenames in DOS.
cat undeflate.c  >un2.c  # Avoid long filenames in DOS.
cat uncompress.c >un3.c  # Avoid long filenames in DOS.
# Creates luuzcat.exe.
./kvikdos ~/Downloads/dosasm2/turboc_1.00/tcc.exe -m$MODEL $ANSI $OPTJUMP $MERGESTR $ALIGN $SPEED $NOFLOAT $WARN $WMSIG $WRCH luuzcat.c unscolzh.c un1.c unopack.c unpack.c un2.c un3.c unfreeze.c
./kvikdos luuzcat.exe <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin

: "$0" OK.
