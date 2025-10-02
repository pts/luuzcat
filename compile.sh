#! /bin/sh --
set -ex
test "$0" = "${0%/*}" || cd "${0%/*}"

gcc -m64 -fsanitize=address -g -O2 -ansi -pedantic -W -Wall -Wextra -o luuzcat luuzcat.c unscolzh.c
./luuzcat <XFileMgro.sz >XFileMgro
cmp XFileMgro.good XFileMgro

g++ -m32 -s -O2 -ansi -pedantic -W -Wall -Wextra -o luuzcat luuzcat.c unscolzh.c
./luuzcat <XFileMgro.sz >XFileMgro && cmp XFileMgro.good XFileMgro

minicc -ansi -pedantic -Wno-n201 -o luuzcat luuzcat.c unscolzh.c
./luuzcat <XFileMgro.sz >XFileMgro
cmp XFileMgro.good XFileMgro

wcc -q -bt=com -D_DOSCOMSTART -DUSE_TREE -os -zl -j -ms -s -W -w4 -wx -we -wcd=201 -za -oi -0 -g=DGROUP -fo=.o luuzcat.c
wcc -q -bt=com -D_DOSCOMSTART -DUSE_TREE -os -zl -j -ms -s -W -w4 -wx -we -wcd=201 -za -oi -0 -g=DGROUP -fo=.o unscolzh.c
wlink op q form dos com op d op nored op start=_comstart_ n luuzcatc.com f luuzcat.o f unscolzh.o
./kvikdos luuzcatc.com <XFileMgro.sz >XFileMgro
cmp XFileMgro.good XFileMgro

: "$0" OK.
