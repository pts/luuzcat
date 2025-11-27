#! /bin/sh --
test "$ZSH_VERSION" && set -y 2>/dev/null  # SH_WORD_SPLIT for zsh(1). It's an invalid option in bash(1), and it's harmful (prevents echo) in ash(1).
set -ex
test "$0" = "${0%/*}" || cd "${0%/*}"

gcc -m64 -fsanitize=address -g -O2 -ansi -pedantic -W -Wall -Wextra -Wstrict-prototypes -Werror-implicit-function-declaration -o luuzcat luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c
# Do a full test.
./luuzcat <XFileMgro.sz >XFileMgro
    cmp XFileMgro.good XFileMgro
./luuzcat <test_C1.bin.C >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_pack.z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_pack_old.z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_pack_old3.z >test_C1.bin  # test_C1_pack_old3.z created using pack.c in https://github.com/pts/pts-opack-port
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
./luuzcat <test_C1_old13.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_old14.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_old15.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_old16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_new16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_new13.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_new14.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_new15.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1_new16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat <test_C1.zip >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat -m <test_C1_split.zip >test_C1.bin  # -m is needed because test_C1.split.zip has multiple archive members.
    cmp test_C1.good test_C1.bin
exit_code=0; ./luuzcat <test_C1_split.zip >test_C1.bin || exit_code="$?"; test "$exit_code" != 0
    cmp test_C1_1of2.good test_C1.bin
# !! Add test for concatenated streams.

~/Downloads/pts_osxcross_10.10/x86_64-apple-darwin14/bin/gcc -m64 -mmacosx-version-min=10.5 -nodefaultlibs -lSystem -O2 -W -Wall -Wextra -Wstrict-prototypes -Werror-implicit-function-declaration -ansi -pedantic -o luuzcat.da6 luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c
~/Downloads/pts_osxcross_10.10/x86_64-apple-darwin14/bin/strip luuzcat.da6
test -s luuzcat.da6  # We'd need a macOS emulator to run it.

~/Downloads/pts_osxcross_10.10/i386-apple-darwin14/bin/gcc   -m32 -mmacosx-version-min=10.5 -nodefaultlibs -lSystem -O2 -W -Wall -Wextra -Wstrict-prototypes -Werror-implicit-function-declaration -ansi -pedantic -o luuzcat.di3 luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c
~/Downloads/pts_osxcross_10.10/x86_64-apple-darwin14/bin/strip luuzcat.di3
test -s luuzcat.di3  # We'd need a macOS emulator to run it.

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
./luuzcat -m <test_C1_split.zip >test_C1.bin
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
./luuzcat -m <test_C1_split.zip >test_C1.bin
    cmp test_C1.good test_C1.bin

# TODO(pts): Try `-os -oh' (more optimizations) here and elsewhere. What difference does it make?
#
# `wcc386 -zp=4' is needed so that a struct containing a double is aligned
# to 4, not 8. Both `wcc -zp=2' and `wcc' (default) align such a struct to
# 2. Also both `wcc386 -zp=4' and `wcc386' (default) align a struct
# containing a `short member[3];' to 2, not 4.
common_wccargs="-q -fr -D_PROGX86 -za -os -s -j -ei -zl -zld -wx -we -wcd=201"  # zsh(1) SH_WORD_SPLIT is needed by $common_wccargs below.
wcc16args="-0"  # zsh(1) SH_WORD_SPLIT is needed by $wcc16args below.
wcc16dosargs="-bt=dos"  # zsh(1) SH_WORD_SPLIT is needed by $wcc16dosargs below.
wcc16minixargs="-bt=dos -U__DOS__ -D__MINIX__ -DLUUZCAT_SMALLBUF -DLUUZCAT_COMPRESS_FORK"  # zsh(1) SH_WORD_SPLIT is needed by $wcc16minixargs below.
wcc32args="-bt=linux -3r -zp=4"  # zsh(1) SH_WORD_SPLIT is needed by $wcc32args below.
dosdefs="-D_PROGX86_ONLY_BINARY -D_PROGX86_CRLF -D_PROGX86_DOSPSPARGV -D_PROGX86_DOSEXIT -D_PROGX86_ISATTYDOSREG -D_PROGX86_REUSE -D_PROGX86_DOSMEM -D_PROGX86_CSEQDS -DLUUZCAT_DUCML"  # zsh(1) SH_WORD_SPLIT is needed by $dosdefs below.
minixi86defs="-D_PROGX86_ONLY_BINARY"  # zsh(1) SH_WORD_SPLIT is needed by $minixi86defs below.
rm -f -- *_16.o *_32.o *_16.wasm *_32.wasm *_16.nasm *_32.nasm

wcc386 $wcc32args $common_wccargs -fo=luuzcat_32.o -D_PROGX86_ONLY_BINARY luuzcat.c
wcc386 $wcc32args $common_wccargs -fo=luuzcatw_32.o -D_PROGX86_ONLY_BINARY -DUSE_WRITE_FIX luuzcat.c
#wcc386 $wcc32args $common_wccargs -fo=luuzcatt_32.o luuzcat.c  # Does \n -> \r\n transformation on stderr. Unused.
wcc386 $wcc32args $common_wccargs -fo=luuzcatr_32.o -D_PROGX86_ONLY_BINARY -D_PROGX86_CRLF luuzcat.c  # Always prints \r\n (CRLF) to stderr.
wcc386 $wcc32args $common_wccargs -fo=unscolzh_32.o unscolzh.c
wcc386 $wcc32args $common_wccargs -fo=uncompact_32.o uncompact.c
wcc386 $wcc32args $common_wccargs -fo=unopack_32.o unopack.c
wcc386 $wcc32args $common_wccargs -fo=unpack_32.o unpack.c
wcc386 $wcc32args $common_wccargs -fo=undeflate_32.o undeflate.c
wcc386 $wcc32args $common_wccargs -fo=uncompress_32.o uncompress.c
wcc386 $wcc32args $common_wccargs -fo=unfreeze_32.o unfreeze.c
#
# TODO(pts): Reenable this for Minix i86 and ELKS.
#wcc    $wcc16args $wcc16dosargs $common_wccargs -fo=luuzcat_16.o -D_PROGX86_ONLY_BINARY luuzcat.c
#wcc    $wcc16args $wcc16dosargs $common_wccargs -fo=luuzcatt_16.o luuzcat.c  # Does \n -> \r\n transformation on stderr. Unused.
#wcc    $wcc16args $wcc16dosargs $common_wccargs -fo=luuzcatd_16.o -D_PROGX86_ONLY_BINARY -D_PROGX86_CRLF -D_PROGX86_DOSPSPARGV -D_PROGX86_DOSEXIT -D_PROGX86_ISATTYDOSREG luuzcat.c  # Always prints \r\n (CRLF) to stderr, gets argv from the DOS PSP.
#wcc    $wcc16args $wcc16dosargs $common_wccargs -fo=unscolzh_16.o unscolzh.c
#wcc    $wcc16args $wcc16dosargs $common_wccargs -fo=uncompact_16.o uncompact.c
#wcc    $wcc16args $wcc16dosargs $common_wccargs -fo=unopack_16.o unopack.c
#wcc    $wcc16args $wcc16dosargs $common_wccargs -fo=unpack_16.o unpack.c
#wcc    $wcc16args $wcc16dosargs $common_wccargs -fo=undeflate_16.o undeflate.c
#wcc    $wcc16args $wcc16dosargs $common_wccargs -fo=uncompress_16.o uncompress.c
#wcc    $wcc16args $wcc16dosargs $common_wccargs -fo=uncompressd_16.o -D_PROGX86_REUSE -D_PROGX86_DOSMEM uncompress.c
#wcc    $wcc16args $wcc16dosargs $common_wccargs -fo=unfreeze_16.o -D_PROGX86_CSEQDS unfreeze.c
#
wcc    $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=luuzcatc_16.o luuzcat.c
wcc    $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=unscolzhc_16.o unscolzh.c
wcc    $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=uncompactc_16.o uncompact.c
wcc    $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=unopackc_16.o unopack.c
wcc    $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=unpackc_16.o unpack.c
wcc    $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=undeflatec_16.o undeflate.c
wcc    $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=uncompressc_16.o uncompress.c
wcc    $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=unfreezec_16.o unfreeze.c
#
wcc    $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=luuzcatm_16.o luuzcat.c
wcc    $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=unscolzhm_16.o unscolzh.c
wcc    $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=uncompactm_16.o uncompact.c
wcc    $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=unopackm_16.o unopack.c
wcc    $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=unpackm_16.o unpack.c
wcc    $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=undeflatem_16.o undeflate.c
wcc    $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=uncompressm_16.o uncompress.c
wcc    $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=unfreezem_16.o unfreeze.c

perl=tools/miniperl-5.004.04.upx
"$perl" -e0 || perl=perl  # Use the system perl(1) if tools is not available.
fi=0
for f in luuzcat_32.o  luuzcatw_32.o luuzcatr_32.o unscolzh_32.o  uncompact_32.o  unopack_32.o  unpack_32.o  undeflate_32.o  uncompress_32.o  unfreeze_32.o \
                                     luuzcatc_16.o unscolzhc_16.o uncompactc_16.o unopackc_16.o unpackc_16.o undeflatec_16.o uncompressc_16.o unfreezec_16.o \
                                     luuzcatm_16.o unscolzhm_16.o uncompactm_16.o unopackm_16.o unpackm_16.o undeflatem_16.o uncompressm_16.o unfreezem_16.o; do
  fi=$((fi+1))  # For local variables.
  wdis -a -fi -fu -i=@ "$f" >"${f%.*}.wasm"
  if test "$f" = uncompressc_16.o; then
    # We pass --CONST=_TEXT so that error messages such as "read error" go to the _TEXT of uncompressc_16.o, thus the rest of the memory image can be overwritten with data.
   "$perl" wasm2nasm.pl --CONST=_TEXT "$fi" <"${f%.*}.wasm" >"${f%.*}.nasm"
  else
   "$perl" wasm2nasm.pl "$fi" <"${f%.*}.wasm" >"${f%.*}.nasm"
  fi
  # objconv 2.54 is buggy, it creates wrong destination for some `call' instructions.
  # tools/objconv-2.54.upx -fnasm "$f" "${f%.*}.nasm"
done
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -DINCLUDES="'luuzcat_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'" -o luuzcatx.o progx86.nasm
# * wlink generates a much larger ELF-32 file than necessary (e.g. it aligns
#   .data to 4096 bytes in the file), but it can add the symbols, so it's useful for debugging.
# * wlink op exportall == op exporta (undocumented flag) keeps symbols (i.e. no run of `strip').
# * `disa 1080` disables the useless message: *Warning! W1080: file luuzcatx.o is a 16-bit object file*.
#   NASM can't generate the relevant COMENT record..
wlink op q form elf ru freebsd disa 1080 op noext op d op nored op start=_start op norelocs op exporta n luuzcatx.elf f luuzcatx.o
./luuzcatx.elf <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
ibcs-us ./luuzcatx.elf <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin

nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o luuzcat_32y.o luuzcat_32.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o luuzcatw_32y.o luuzcatw_32.nasm
#nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o luuzcatt_32y.o luuzcatt_32.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o luuzcatr_32y.o luuzcatr_32.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o unscolzh_32y.o unscolzh_32.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o uncompact_32y.o uncompact_32.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o unopack_32y.o unopack_32.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o unpack_32y.o unpack_32.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o undeflate_32y.o undeflate_32.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o uncompress_32y.o uncompress_32.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o unfreeze_32y.o unfreeze_32.nasm
#
#nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o luuzcat_16y.o luuzcat_16.nasm
#nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o luuzcatt_16y.o luuzcatt_16.nasm
#nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o luuzcatd_16y.o luuzcatd_16.nasm
#nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o unscolzh_16y.o unscolzh_16.nasm
#nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o uncompact_16y.o uncompact_16.nasm
#nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o unopack_16y.o unopack_16.nasm
#nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o unpack_16y.o unpack_16.nasm
#nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o undeflate_16y.o undeflate_16.nasm
#nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o uncompress_16y.o uncompress_16.nasm
#nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o unfreeze_16y.o unfreeze_16.nasm
#
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o luuzcatc_16y.o luuzcatc_16.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o unscolzhc_16y.o unscolzhc_16.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o uncompactc_16y.o uncompactc_16.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o unopackc_16y.o unopackc_16.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o unpackc_16y.o unpackc_16.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o undeflatec_16y.o undeflatec_16.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o uncompressc_16y.o uncompressc_16.nasm
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -o unfreezec_16y.o unfreezec_16.nasm

# As a proof-of-concept, we create luuzcatz.elf with separate object files recompiled with NASM.
dneeds="-D__NEED__write -D__NEED__read -D__NEED_isatty_ -D__NEED___argc -D__NEED__cstart_ -D__NEED_memset_ -D__NEED_memcpy_ -D__NEED_strlen_"
# zsh(1) SH_WORD_SPLIT is needed by $dneeds below.
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -DINCLUDES= -D__GLOBAL_main_=2 $dneeds -o luuzcatz.o progx86.nasm
# !! Examine the output of wlink (even better: analyze the .o files with Perl), autogenerate the `-D__NEED_...'s.
wlink op q form elf ru freebsd disa 1080 op noext op d op nored op start=_start op norelocs op exporta n luuzcatz.elf f luuzcat_32.o f unscolzh_32.o f uncompact_32.o f unopack_32.o f unpack_32.o f undeflate_32.o f uncompress_32.o f unfreeze_32.o f luuzcatz.o
# If we ignore section alignment, luuzcatz.elf is a few dozen bytes smaller than luuzcatx.elf, because NASM was able to optimize the jump instructions better whan wcc386(1) (even than `wcc386 -os -oh').
./luuzcatz.elf <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
ibcs-us ./luuzcatz.elf <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin

dneeds="-D__NEED_G@_write -D__NEED_G@_read -D__NEED_G@isatty_ -D__NEED_G@__argc -D__NEED_G@_cstart_ -D__NEED_G@memset_ -D__NEED_G@memcpy_ -D__NEED_G@strlen_"
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -DINCLUDES= -D__GLOBAL_G@main_=2 $dneeds -o luuzcaty.o progx86.nasm
# !! Examine the output of wlink (even better: analyze the .o files with Perl), autogenerate the `-D__NEED_...'s.
wlink op q form elf ru freebsd disa 1080 op noext op d op nored op start=_start op norelocs op exporta n luuzcaty.elf f luuzcat_32y.o f unscolzh_32y.o f uncompact_32y.o f unopack_32y.o f unpack_32y.o f undeflate_32y.o f uncompress_32y.o f unfreeze_32y.o f luuzcaty.o
./luuzcaty.elf <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
ibcs-us ./luuzcaty.elf <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin

nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'luuzcatw_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'"                                       -o luuzcat.elf   progx86.nasm
chmod +x luuzcat.elf
# Do a full test.
./luuzcat.elf <XFileMgro.sz >XFileMgro
    cmp XFileMgro.good XFileMgro
./luuzcat.elf <test_C1.bin.C >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1_pack.z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1_pack_old.z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1_pack_old3.z >test_C1.bin  # test_C1_pack_old3.z created using pack.c in https://github.com/pts/pts-opack-port
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1.advdef.gz >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1.advdef.qz >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1.advdef.zlib >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf -r <test_C1.advdef.deflate >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf -r <test_C1.advdef.gz >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1_old.F >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1.F >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1_old13.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1_old14.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1_old15.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1_old16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1_new16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1_new13.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1_new14.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1_new15.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1_new16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf <test_C1.zip >test_C1.bin
    cmp test_C1.good test_C1.bin
./luuzcat.elf -m <test_C1_split.zip >test_C1.bin  # -m is needed because test_C1.split.zip has multiple archive members.
    cmp test_C1.good test_C1.bin
exit_code=0; ./luuzcat.elf <test_C1_split.zip >test_C1.bin || exit_code="$?"; test "$exit_code" != 0
    cmp test_C1_1of2.good test_C1.bin

# We pass -DFULL_BSS so that global_insize, global_inptr and global_total_read_size will be zero-initialized.
nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'uncompressm_16.nasm','luuzcatm_16.nasm','unscolzhm_16.nasm','uncompactm_16.nasm','unopackm_16.nasm','unpackm_16.nasm','undeflatem_16.nasm','unfreezem_16.nasm'" -DMINIXI86 -DMINIX_STACK_SIZE=0x600 -o luuzcat.mi8 progx86.nasm
chmod +x luuzcat.mi8  # elksemu needs it.
test -s luuzcat.mi8
# elksemu.mini and elksemu seem to be unreliable: out of 1000 atempts, 6..14 encounter SIGSTOP.
# for i in $(seq 1 1000); do echo -n "$i "; env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1_pack.z >test_C1.bin; done
# So we don't run any of these here.
if false; then
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <XFileMgro.sz >XFileMgro
      cmp XFileMgro.good XFileMgro
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1.bin.C >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1_pack.z >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1_pack_old.z >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1.advdef.gz >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1.advdef.qz >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1.advdef.zlib >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 -r <test_C1.advdef.deflate >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 -r <test_C1.advdef.gz >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1_old.F >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1.F >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1.zip >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini -m luuzcat.mi8 <test_C1_split.zip >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1_new9.Z >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1_new13.Z >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1_new14.Z >test_C1.bin
      cmp test_C1.good test_C1.bin
  #env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1_new15.Z >test_C1.bin  # It won't work in elksemu, because elksemu <=0.8.1 has fork(2) implemented incorrectly, and it doesn't have fmemalloc(3) at all.
  #    cmp test_C1.good test_C1.bin
  #env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1_new16.Z >test_C1.bin  # It won't work in elksemu, because elksemu <=0.8.1 has fork(2) implemented incorrectly, and it doesn't have fmemalloc(3) at all.
  #    cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1_old13.Z >test_C1.bin
      cmp test_C1.good test_C1.bin
  env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1_old14.Z >test_C1.bin
      cmp test_C1.good test_C1.bin
  #env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1_old15.Z >test_C1.bin  # It won't work in elksemu, because elksemu <=0.8.1 has fork(2) implemented incorrectly, and it doesn't have fmemalloc(3) at all.
  #    cmp test_C1.good test_C1.bin
  #env -i FOO=bar ./elksemu.mini luuzcat.mi8 <test_C1_old16.Z >test_C1.bin  # It won't work in elksemu, because elksemu <=0.8.1 has fork(2) implemented incorrectly, and it doesn't have fmemalloc(3) at all.
  #    cmp test_C1.good test_C1.bin
fi

# We pass -DFULL_BSS so that global_insize, global_inptr and global_total_read_size will be zero-initialized.
nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'uncompressc_16.nasm','luuzcatc_16.nasm','unscolzhc_16.nasm','uncompactc_16.nasm','unopackc_16.nasm','unpackc_16.nasm','undeflatec_16.nasm','unfreezec_16.nasm'" -DDOSCOM -DFULL_BSS -o luuzcat.com progx86.nasm
"$perl" shorten_to_bss.pl luuzcat.com  # luuzcat.com allocates extra memory in uncompress.c.
./kvikdos luuzcat.com <XFileMgro.sz >XFileMgro
    cmp XFileMgro.good XFileMgro
./kvikdos luuzcat.com <test_C1.bin.C >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1_pack.z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1_pack_old.z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1.advdef.gz >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1.advdef.qz >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1.advdef.zlib >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com -r <test_C1.advdef.deflate >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com -r <test_C1.advdef.gz >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1_old.F >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1.F >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1.zip >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com -m <test_C1_split.zip >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1_new13.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1_new14.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1_new15.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1_new16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1_old13.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1_old14.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1_old15.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.com <test_C1_old16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin

#nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'luuzcatd_16.nasm','unscolzh_16.nasm','uncompact_16.nasm','unopack_16.nasm','unpack_16.nasm','undeflate_16.nasm','uncompressn_16.nasm','unfreeze_16.nasm'" -DDOSCOM                             -o luuzcatn.com  progx86.nasm
#"$perl" shorten_to_bss.pl luuzcatn.com  # luuzcatn.com doesn't allocate extra memory in uncompress.c, so it supports maxbits <= 13, and it doesn't support 14, 15 or 16. !! Add support for 14 in DOS and Minix i86.
#./kvikdos luuzcatn.com <test_C1_new9.Z >test_C1.bin
#    cmp test_C1.good test_C1.bin
#./kvikdos luuzcatn.com <test_C1_new13.Z >test_C1.bin
#    cmp test_C1.good test_C1.bin
## maxbits >= 14 not supported.
##./kvikdos luuzcatn.com <test_C1_new14.Z >test_C1.bin
##    cmp test_C1.good test_C1.bin
##./kvikdos luuzcatn.com <test_C1_new15.Z >test_C1.bin
##    cmp test_C1.good test_C1.bin
##./kvikdos luuzcatn.com <test_C1_new16.Z >test_C1.bin
##    cmp test_C1.good test_C1.bin
#./kvikdos luuzcatn.com <test_C1_old13.Z >test_C1.bin
#    cmp test_C1.good test_C1.bin
## maxbits >= 14 not supported.
##./kvikdos luuzcatn.com <test_C1_old14.Z >test_C1.bin
##    cmp test_C1.good test_C1.bin
##./kvikdos luuzcatn.com <test_C1_old15.Z >test_C1.bin
##    cmp test_C1.good test_C1.bin
##./kvikdos luuzcatn.com <test_C1_old16.Z >test_C1.bin
##    cmp test_C1.good test_C1.bin
#./kvikdos luuzcatn.com r <test_C1.advdef.deflate >test_C1.bin
#    cmp test_C1.good test_C1.bin

nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'uncompressc_16.nasm','luuzcatc_16.nasm','unscolzhc_16.nasm','uncompactc_16.nasm','unopackc_16.nasm','unpackc_16.nasm','undeflatec_16.nasm','unfreezec_16.nasm'" -DDOSEXE -DFULL_BSS -o luuzcatd.exe progx86.nasm
./kvikdos luuzcatd.exe <test_C1_new16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatd.exe r <test_C1.advdef.deflate >test_C1.bin
    cmp test_C1.good test_C1.bin

# `op st=1024K' is a better general default.
dneeds="-D__NEED_G@_write -D__NEED_G@_read -D__NEED_G@isatty_ -D__NEED_G@__argc -D__NEED_G@_cstart_ -D__NEED_G@memset_ -D__NEED_G@memcpy_ -D__NEED_G@strlen_"
nasm-0.98.39 -O999999999 -w+orphan-labels -f obj -DINCLUDES= -D__GLOBAL_G@main_=2 $dneeds -DWIN32WL -o luuzcatp.o progx86.nasm
# We omit `op norelocs', otherwise WDOSX wouldn't be able to run it.
# We omit `op exporta', because we don't need these symbols.
wlink op q form win nt ru con=3.10 op h=4K com h=0 op st=64K com st=64K disa 1080 op noext op d op nored op start=_start op stub=luuzcatd.exe n luuzcat.exe f luuzcatp.o f luuzcatr_32y.o f unscolzh_32y.o f uncompact_32y.o f unopack_32y.o f unpack_32y.o f undeflate_32y.o f uncompress_32y.o f unfreeze_32y.o
"$perl" -0777 -pi -e 'substr($_, 0xa, 2) = pack("v", unpack("v", substr($_, 0xa, 2)) + 3)' luuzcat.exe  # Hotfix: Add 3 to .minalloc. wlink(1) has kept it intact, incorrectly. !! Come up with a safer hotfix, reading both files.
# We need non-empty command-line because dosbox.nox.static incorrectly reports that stdin is a TTY.
dosbox.nox.static --cmd --mem-mb=2 ~/prg/mwpestub/mwperun.exe luuzcat.exe -cd <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
dosbox.nox.static --cmd luuzcat.exe -cd <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcat.exe <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin

rm -f luuzcat.3b
nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'luuzcat_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'" -DS386BSD                              -o luuzcat.3b    progx86.nasm
chmod +x luuzcat.3b
# We can't run luuzcat.3b on Linux i386 directly, so we just check for the file size.
test -s luuzcat.3b

rm -f luuzcat.mi3
nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'luuzcat_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'" -DMINIXI386                            -o luuzcat.mi3   progx86.nasm
chmod +x luuzcat.mi3
# We can't run luuzcat.mi3 on Linux i386 directly, so we just check for the file size.
test -s luuzcat.mi3

rm -f luuzcat.m3vm
nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'luuzcat_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'" -DMINIX386VM                           -o luuzcat.m3vm  progx86.nasm
chmod +x luuzcat.m3vm
# We can't run luuzcat.m3vm on Linux i386 directly, so we just check for the file size.
test -s luuzcat.m3vm

rm -f luuzcat.v7x
nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'luuzcat_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'" -DV7X86                                -o luuzcat.v7x   progx86.nasm
chmod +x luuzcat.v7x
# We can't run luuzcat.v7x on Linux i386 directly, so we just check for the file size.
test -s luuzcat.v7x

rm -f luuzcat.x63
nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'luuzcat_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'" -DXV6I386                              -o luuzcat.x63   progx86.nasm
chmod +x luuzcat.x63
# We can't run luuzcat.x63 on Linux i386 directly, so we just check for the file size.
test -s luuzcat.x63

nasm-0.98.39 -O999999999 -w+orphan-labels -f bin -DINCLUDES="'luuzcat_32.nasm','unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'" -DCOFF -DCOFF_PROGRAM_NAME="'luuzcat'" -o luuzcat.coff  progx86.nasm
chmod +x luuzcat.coff
ibcs-us ./luuzcat.coff <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin

# We compile with the OpenWatcom C compiler to a DOS 8086 .com program, but we don't use the OpenWatcom libc.
wcc -q -bt=com -D_DOSCOMSTART -os -zl -zld -fr -j -ei -s -wx -we -wcd=201 -za -oi -0 -zp=2 -g=DGROUP -fo=.o -D_DOSCOMSTART_DISTART luuzcat.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -zld -fr -j -ei -s -wx -we -wcd=201 -za -oi -0 -zp=2 -g=DGROUP -fo=.o unscolzh.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -zld -fr -j -ei -s -wx -we -wcd=201 -za -oi -0 -zp=2 -g=DGROUP -fo=.o uncompact.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -zld -fr -j -ei -s -wx -we -wcd=201 -za -oi -0 -zp=2 -g=DGROUP -fo=.o unopack.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -zld -fr -j -ei -s -wx -we -wcd=201 -za -oi -0 -zp=2 -g=DGROUP -fo=.o unpack.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -zld -fr -j -ei -s -wx -we -wcd=201 -za -oi -0 -zp=2 -g=DGROUP -fo=.o undeflate.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -zld -fr -j -ei -s -wx -we -wcd=201 -za -oi -0 -zp=2 -g=DGROUP -fo=.o uncompress.c
wcc -q -bt=com -D_DOSCOMSTART -os -zl -zld -fr -j -ei -s -wx -we -wcd=201 -za -oi -0 -zp=2 -g=DGROUP -fo=.o unfreeze.c
wlink op q form dos com op d op nored op start=_comstart_ n luuzcatc.com f luuzcat.o f unscolzh.o f uncompact.o f unopack.o f unpack.o f undeflate.o f uncompress.o f unfreeze.o
# luuzcatc.com is >100 bytes shorter than luuzcat.com, but luuzcat.com uses less memory for LZW decompression, especially with maxbits <= 14.
"$perl" shorten_to_bss.pl luuzcatc.com
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
./kvikdos luuzcatc.com -m <test_C1_split.zip >test_C1.bin
    cmp test_C1.good test_C1.bin

wcc -q -bt=com -D_DOSCOMSTART -os -zl -zld -fr -j -ei -s -wx -we -wcd=201 -za -oi -0 -zp=2 -g=DGROUP -fo=uncomprc.o -D_DOSCOMSTART_UNCOMPRC -D_DOSCOMSTART_DISTART uncompress.c
wlink op q form dos com op d op nored op start=_comstart_ n uncomprc.com f uncomprc.o
# uncomprc.com is a DOS .com program which can decompress only LZW-compressed data, but it uses less memory than luuzcat.com (and luuzcatc.com). It uses (in addition to the MCB and kernel buffers):
# * For maxbits <= 14, 64 KiB.
# * For maxbits == 15, 127.5 KiB.
# * For maxbits == 16, 255.75 KiB.
"$perl" shorten_to_bss.pl uncomprc.com
rm -f uncomprc.o
./kvikdos uncomprc.com <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos uncomprc.com <test_C1_new13.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos uncomprc.com <test_C1_new14.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos uncomprc.com <test_C1_new15.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos uncomprc.com <test_C1_new16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos uncomprc.com <test_C1_old13.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos uncomprc.com <test_C1_old14.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos uncomprc.com <test_C1_old15.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos uncomprc.com <test_C1_old16.Z >test_C1.bin
    cmp test_C1.good test_C1.bin

# Compile with OpenWatcom to a DOS 8086 .exe program using the OpenWatcom libc.
#
# This is just to prove a point that it compiles without any trickery, and
# to compare the libc size to of luuzcatc.com. The luuzcatc.com linked
# against the custom libc (_DOSCOMSTART) above is smaller, so that should be
# used in production instead.
#
# The default is the small model (`owcc -mcmodel=s').
owcc -bdos -s -Os -fno-stack-check -march=i86 -W -Wall -Wextra -Werror -Wno-n201 -std=c89 -o luuzcatl.exe luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c
./kvikdos luuzcatl.exe <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin
./kvikdos luuzcatl.exe -r <test_C1.advdef.deflate >test_C1.bin
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
./kvikdos ~/Downloads/dosasm2/turboc_1.00/tcc.exe -m$MODEL $ANSI $OPTJUMP $MERGESTR $ALIGN $SPEED $NOFLOAT $WARN $WMSIG $WRCH -el luuzcat.c unscolzh.c un1.c unopack.c unpack.c un2.c un3.c unfreeze.c
mv l.exe luuzcatt.exe
./kvikdos luuzcatt.exe <test_C1_new9.Z >test_C1.bin
    cmp test_C1.good test_C1.bin

: "$0" OK.
