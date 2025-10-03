#! /bin/sh --
set -ex
test "$0" = "${0%/*}" || cd "${0%/*}"

gcc -m64 -fsanitize=address -g -O2 -ansi -pedantic -W -Wall -Wextra -o luuzcat luuzcat.c unscolzh.c uncompact.c unopack.c
./luuzcat <XFileMgro.sz >XFileMgro
    cmp XFileMgro.good XFileMgro
./luuzcat <test_C1.bin.C >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_pack_old.z >test_C1.bin
    cmp test_C1.good test_C1.bin
# test_C1_pack_old3.z created using pack.c in https://github.com/pts/pts-opack-port
./luuzcat <test_C1_pack_old3.z >test_C1.bin
    cmp test_C1.good test_C1.bin

g++ -m32 -s -O2 -ansi -pedantic -W -Wall -Wextra -o luuzcat luuzcat.c unscolzh.c uncompact.c unopack.c
./luuzcat <XFileMgro.sz >XFileMgro
  cmp XFileMgro.good XFileMgro
./luuzcat <test_C1.bin.C >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_pack_old.z >test_C1.bin
    cmp test_C1.good test_C1.bin

minicc -ansi -pedantic -Wno-n201 -o luuzcat luuzcat.c unscolzh.c uncompact.c unopack.c
./luuzcat <XFileMgro.sz >XFileMgro
  cmp XFileMgro.good XFileMgro
./luuzcat <test_C1.bin.C >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_pack_old.z >test_C1.bin
    cmp test_C1.good test_C1.bin

# We compile with the OpenWatcom C compiler to a DOS 8086 .com program, but we don't use the OpenWatcom libc.
wcc -q -bt=com -D_DOSCOMSTART -os -zl -j -ms -s -W -w4 -wx -we -wcd=201 -za -oi -0 -g=DGROUP -fo=.o luuzcat.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -j -ms -s -W -w4 -wx -we -wcd=201 -za -oi -0 -g=DGROUP -fo=.o unscolzh.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -j -ms -s -W -w4 -wx -we -wcd=201 -za -oi -0 -g=DGROUP -fo=.o uncompact.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -j -ms -s -W -w4 -wx -we -wcd=201 -za -oi -0 -g=DGROUP -fo=.o unopack.c
wlink op q form dos com op d op nored op start=_comstart_ n luuzcatc.com f luuzcat.o f unscolzh.o f uncompact.o f unopack.o
rm -f luuzcat.o unscolzh.o uncompact.o unopack.o
./kvikdos luuzcatc.com <XFileMgro.sz >XFileMgro
    cmp XFileMgro.good XFileMgro
./kvikdos luuzcatc.com <test_C1.bin.C >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatc.com <test_C1_pack_old.z >test_C1.bin
    cmp test_C1.good test_C1.bin

: "$0" OK.
