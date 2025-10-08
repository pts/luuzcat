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

# TODO(pts): Compile with --gcc=4.8 and extra warnings?
minicc -ansi -pedantic -Wno-n201 -o luuzcat luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c
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

owcc -bwin32 -Wl,runtime -Wl,console=3.10 -s -Os -fno-stack-check -march=i386 -W -Wall -Wextra -Werror -Wno-n201 -std=c89 -o luuzcatw.exe luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c
dosbox.nox.static --cmd --mem-mb=2 ~/prg/mwpestub/mwperun.exe luuzcatw.exe <test_C1_new9.Z >test_C1.bin
cmp test_C1.good test_C1.bin

~/Downloads/windows_nt_qemu/hdachs/mmlibcc.sh -bwin32 -o luuzcatm.exe luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c
dosbox.nox.static --cmd --mem-mb=2 ~/prg/mwpestub/mwperun.exe luuzcatm.exe <test_C1_new9.Z >test_C1.bin
cmp test_C1.good test_C1.bin

: "$0" OK.
