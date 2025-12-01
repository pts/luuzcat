#!/bin/sh --
# by pts@fazekas.hu at Sun Nov 30 16:20:26 CET 2025

argv0="${1#--sh-script=}"
if test "$1" != "$argv0"; then
  shift
elif test "$1" = --noenv; then
  shift; test "$0" = "${0%/*}" || cd "${0%/*}" || exit 2
  argv0="$0"; test "$0" != "${0%/*}" || argv0="./$0"
  exec env -i sh ./"${0##*/}" --sh-script="$argv0" "$@" || exit 2
else
  test "$0" = "${0%/*}" || cd "${0%/*}" || exit 2
  argv0="$0"; test "$0" != "${0%/*}" || argv0="./$0"
  exec tools/busybox-minicc-1.21.1.upx env -i PATH=/dev/null/missing sh ./"${0##*/}" --sh-script="$argv0" "$@" || exit 2
fi
test "$ZSH_VERSION" && set -y 2>/dev/null  # SH_WORD_SPLIT for zsh(1). It's an invalid option in bash(1), and it's harmful (prevents echo) in ash(1).
set -e +x  # Exit of first non-successful command. Don't echo.
nl="
"
crlf="$(printf '\r')$nl"
IFS="$nl"

perl=tools/miniperl-5.004.04.upx
nasm=tools/nasmf-0.98.39.upx
wcc=tools/wcc-ow2023-03-04.upx
wcc386=tools/wcc386-ow2023-03-04.upx
wlink=tools/wlink-ow2023-03-04.upx
wdis=tools/wdis-ow2023-03-04.upx
ibcsus=tools/ibcs-us-4.1.6.upx
ptsosxcrossdir=pts_osxcross_10.10
kvikdos=tools/kvikdos-v1.upx
emu2lc=tools/emu2-lc-2021.01.upx
elksemu=tools/elksemu-2023-01-12.upx
mwperunexe=tools/mwperun.exe
dosboxnox=tools/dosbox-nox-v1.upx

all="luuzcat.elf${nl}luuzcat.coff${nl}luuzcat.3b${nl}luuzcat.mi3${nl}luuzcat.m3vm${nl}luuzcat.v7x${nl}luuzcat.x63${nl}luuzcat.da6${nl}luuzcat.di3${nl}luuzcat.com${nl}luuzcat.mi8${nl}luuzcatd.exe${nl}luuzcat.exe"
common_wccargs="-q$nl-fr$nl-D_PROGX86$nl-za$nl-os$nl-s$nl-j$nl-ei$nl-zl$nl-zld$nl-wx$nl-we$nl-wcd=201"
wcc16args="-0"
wcc16dosargs="-bt=dos"
wcc16minixargs="-bt=dos$nl-U__DOS__$nl-D__MINIX__$nl-DLUUZCAT_SMALLBUF$nl-DLUUZCAT_COMPRESS_FORK"
wcc32args="-bt=linux$nl-3r$nl-zp=4"
dosdefs="-D_PROGX86_ONLY_BINARY$nl-D_PROGX86_CRLF$nl-D_PROGX86_DOSPSPARGV$nl-D_PROGX86_DOSEXIT$nl-D_PROGX86_ISATTYDOSREG$nl-D_PROGX86_REUSE$nl-D_PROGX86_DOSMEM$nl-D_PROGX86_CSEQDS$nl-DLUUZCAT_DUCML"
minixi86defs="-D_PROGX86_ONLY_BINARY"
common_includes386=",'unscolzh_32.nasm','uncompact_32.nasm','unopack_32.nasm','unpack_32.nasm','undeflate_32.nasm','uncompress_32.nasm','unfreeze_32.nasm'"
common_includesdos="'luuzcatc_16.nasm','unscolzhc_16.nasm','uncompactc_16.nasm','unopackc_16.nasm','unpackc_16.nasm','undeflatec_16.nasm','unfreezec_16.nasm'"
common_includesminixi86="'luuzcatm_16.nasm','unscolzhm_16.nasm','uncompactm_16.nasm','unopackm_16.nasm','unpackm_16.nasm','undeflatem_16.nasm','unfreezem_16.nasm'"
nasms32y="luuzcat_32.nasm${nl}luuzcatw_32.nasm${nl}luuzcatr_32.nasm${nl}unscolzh_32.nasm${nl}uncompact_32.nasm${nl}unopack_32.nasm${nl}unpack_32.nasm${nl}undeflate_32.nasm${nl}uncompress_32.nasm${nl}unfreeze_32.nasm"
dneeds="-D__NEED_G@_write$nl-D__NEED_G@_read$nl-D__NEED_G@isatty_$nl-D__NEED_G@__argc$nl-D__NEED_G@_cstart_$nl-D__NEED_G@memset_$nl-D__NEED_G@memcpy_$nl-D__NEED_G@strlen_"

cmdfailok() {
  stdinfn="${1#<}"
  if test "$1" != "$stdinfn"; then  # Redirection of stdin. Only recognized in $1.
    stdoutfn="${2#>}"
    if test "$2" != "$stdoutfn"; then  # Redirection of stdout. Only recognized in $2. `>>' etc. are not recognized.
      shift; shift; cmdq="$(exec 2>&1 && set -x && : "$@")"; stdinfnq="$(exec 2>&1 && set -x && : "$stdinfn")"; stdoutfnq="$(exec 2>&1 && set -x && : "$stdoutfn")"
      echo "+ ${cmdq#+ : } <${stdinfnq#+ : } >${stdoutfnq#+ : }" >&2; "$@" <"$stdinfn" >"$stdoutfn"
    else
      shift; cmdq="$(exec 2>&1 && set -x && : "$@")"; stdinfnq="$(exec 2>&1 && set -x && : "$stdinfn")"
      echo "+ ${cmdq#+ : } <${stdinfnq#+ : }" >&2; "$@" <"$stdinfn"
    fi
  else
    cmdq="$(exec 2>&1 && set -x && : "$@")"
    echo "+ ${cmdq#+ : }" >&2; "$@"  # This "$@" can run built-in commands and external commands (with `/' in their name or on $PATH).
  fi
}

cmd() {
  cmdfailok "$@" || exit 3
}

nasmfi=0
wasm2nasm() {
  for f in "$@"; do
    nasmfi=$((nasmfi+1))  # For local variables.
    cmd "$wdis" -a -fi -fu -i=@ "$f" >"${f%.*}.wasm"
    wasm2nasmargs=
    # We pass --CONST=_TEXT so that error messages such as "read error" go to the _TEXT of uncompressc_16.o, thus the rest of the memory image can be overwritten with data.
    test "$f" != uncompressc_16.o || wasm2nasmargs="--CONST=_TEXT"
    cmd \<"${f%.*}.wasm" \>"${f%.*}.nasm" "$perl" wasm2nasm.pl $wasm2nasmargs "$nasmfi"
    # objconv 2.54 is buggy, it creates wrong destination for some `call' instructions.
    # tools/objconv-2.54.upx -fnasm "$f" "${f%.*}.nasm"
  done
}

objsi386="luuzcat_32.o${nl}luuzcatw_32.o${nl}luuzcatr_32.o${nl}unscolzh_32.o${nl}uncompact_32.o${nl}unopack_32.o${nl}unpack_32.o${nl}undeflate_32.o${nl}uncompress_32.o${nl}unfreeze_32.o"
dowcc386() {
  test -z "$done_wcc386" || return 0; done_wcc386=1
  nasmfi="$1"
  cmd "$wcc386" $wcc32args $common_wccargs -fo=luuzcat_32.o -D_PROGX86_ONLY_BINARY luuzcat.c
  cmd "$wcc386" $wcc32args $common_wccargs -fo=luuzcatw_32.o -D_PROGX86_ONLY_BINARY -DUSE_WRITE_FIX luuzcat.c
  #cmd "$wcc386" $wcc32args $common_wccargs -fo=luuzcatt_32.o luuzcat.c  # Does \n -> \r\n transformation on stderr. Unused.
  cmd "$wcc386" $wcc32args $common_wccargs -fo=luuzcatr_32.o -D_PROGX86_ONLY_BINARY -D_PROGX86_CRLF luuzcat.c  # Always prints \r\n (CRLF) to stderr.
  cmd "$wcc386" $wcc32args $common_wccargs -fo=unscolzh_32.o unscolzh.c
  cmd "$wcc386" $wcc32args $common_wccargs -fo=uncompact_32.o uncompact.c
  cmd "$wcc386" $wcc32args $common_wccargs -fo=unopack_32.o unopack.c
  cmd "$wcc386" $wcc32args $common_wccargs -fo=unpack_32.o unpack.c
  cmd "$wcc386" $wcc32args $common_wccargs -fo=undeflate_32.o undeflate.c
  cmd "$wcc386" $wcc32args $common_wccargs -fo=uncompress_32.o uncompress.c
  cmd "$wcc386" $wcc32args $common_wccargs -fo=unfreeze_32.o unfreeze.c
  wasm2nasm $objsi386
}

objsdos="luuzcatc_16.o${nl}unscolzhc_16.o${nl}uncompactc_16.o${nl}unopackc_16.o${nl}unpackc_16.o${nl}undeflatec_16.o${nl}uncompressc_16.o${nl}unfreezec_16.o"
dowccdos() {
  test -z "$done_wccdos" || return 0; done_wccdos=1
  nasmfi="$1"
  cmd "$wcc" $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=luuzcatc_16.o luuzcat.c
  cmd "$wcc" $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=unscolzhc_16.o unscolzh.c
  cmd "$wcc" $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=uncompactc_16.o uncompact.c
  cmd "$wcc" $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=unopackc_16.o unopack.c
  cmd "$wcc" $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=unpackc_16.o unpack.c
  cmd "$wcc" $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=undeflatec_16.o undeflate.c
  cmd "$wcc" $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=uncompressc_16.o uncompress.c
  cmd "$wcc" $wcc16args $wcc16dosargs $common_wccargs $dosdefs -fo=unfreezec_16.o unfreeze.c
  wasm2nasm $objsdos
}

objsminixi86="luuzcatm_16.o${nl}unscolzhm_16.o${nl}uncompactm_16.o${nl}unopackm_16.o${nl}unpackm_16.o${nl}undeflatem_16.o${nl}uncompressm_16.o${nl}unfreezem_16.o"
dowccminixi86() {
  test -z "$done_wccminixi86" || return 0; done_wccminixi86=1
  nasmfi="$1"
  cmd "$wcc" $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=luuzcatm_16.o luuzcat.c
  cmd "$wcc" $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=unscolzhm_16.o unscolzh.c
  cmd "$wcc" $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=uncompactm_16.o uncompact.c
  cmd "$wcc" $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=unopackm_16.o unopack.c
  cmd "$wcc" $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=unpackm_16.o unpack.c
  cmd "$wcc" $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=undeflatem_16.o undeflate.c
  cmd "$wcc" $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=uncompressm_16.o uncompress.c
  cmd "$wcc" $wcc16args $wcc16minixargs $common_wccargs $minixi86defs -fo=unfreezem_16.o unfreeze.c
  wasm2nasm $objsminixi86
}

doobjs32y() {
  test -z "$done_objs32y" || return 0; done_objs32y=1
  for f in $nasms32y; do
    cmd "$nasm" -O999999999 -w+orphan-labels -f obj -o "${f%.nasm}y.o" "$f"
  done
}

doibcsus() {
  test "$need_test" || return 0
  test "$use_ibcsus" || return 0
  test -z "$done_ibcsus" || return 0; done_ibcsus=1
  cmd "$nasm" -O999999999 -w+orphan-labels -f bin -DTRUEPROG -DCOFF -DCOFF_PROGRAM_NAME="'true'" -o true.coff progx86.nasm; chmod +x true.coff
  if ! cmdfailok "$ibcsus" ./true.coff; then
    cmdname="${argv0%/*}/$ibcsus"
    test "$cmdname" = "${cmdname%[-+]}" || cmdname="./$cmdname"
    cmdq1="$(exec 2>&1 && set -x && : sudo "$cmdname" -s)"
    cmdq2="$(exec 2>&1 && set -x && : sudo setcap cap_sys_rawio+ep "$cmdname")"
    echo "fatal: specify noibcsus, or run this first: ${cmdq1#*: }" >&2
    echo "fatal:                   or run this first: ${cmdq2#*: }" >&2; exit 5
  fi
}

# We can't run "1" on Linux i386 directly or in emulation, so we just check for the file size.
dotest_xsize() {
  cmd test -f "$1"
  cmd test -x "$1"
  cmd test -s "$1"
}

doosxcross() {
  test "$need_osxcross" || return 0
  test -z "$done_osxcross" || return 0; done_osxcross=1
  if ! test -f "$ptsosxcrossdir"/x86_64-apple-darwin14/bin/gcc || ! test -x "$ptsosxcrossdir"/x86_64-apple-darwin14/bin/gcc; then
    if ! test -f pts_osxcross_10.10.v2.sfx.7z; then
      cmdq1="$(exec 2>&1 && set -x && : wget -O "${argv0%/*}"/pts_osxcross_10.10.v2.sfx.7z.tmp https://github.com/pts/pts-osxcross/releases/download/v2/pts_osxcross_10.10.v2.sfx.7z)"
      cmdq2="$(exec 2>&1 && set -x && : mv "${argv0%/*}"/pts_osxcross_10.10.v2.sfx.7z.tmp "${argv0%/*}"/pts_osxcross_10.10.v2.sfx.7z)"
      echo "fatal: specify nomacos, or run this first to download pts-osxcross: ${cmdq1#*: } && ${cmdq2#*: }" >&2; exit 7
    fi
    if ! cmdfailok test -f "$ptsosxcrossdir"/x86_64-apple-darwin14/bin/gcc; then
      cmd chmod +x pts_osxcross_10.10.v2.sfx.7z
      ./pts_osxcross_10.10.v2.sfx.7z  # Creates pts_osxcross_10.10/x86_64-apple-darwin14/bin/gcc etc.
    fi
    cmd test -f "$ptsosxcrossdir"/x86_64-apple-darwin14/bin/gcc
  fi
  echo 'int main(void) { return 0; }' >true.c
  rm -f true.da6
  cmd "$ptsosxcrossdir"/x86_64-apple-darwin14/bin/gcc -m64 -mmacosx-version-min=10.5 -nodefaultlibs -lSystem -Os -W -Wall -Wextra -Wstrict-prototypes -Werror-implicit-function-declaration -ansi -pedantic -o true.da6 true.c  # Test compile.
  dotest_xsize true.da6
}

dosmyemu=///  # Nonempty for `test ... =' comparisons.
dodosmyemu() {
  test "$need_test" || return 0
  test "$use_dosmyemu" || return 0
  test -z "$done_dosmyemu" || return 0; done_dosmyemu=1
  if cmdfailok "$kvikdos" --kvm-check; then dosmyemu="$kvikdos"
  else dosmyemu="$emu2lc"  # If KVM is not available on the host, use emu2-lc for DOS program emulation.
  fi
}

# Do a test of 1 input only.
dotest1() {
  prog="./$1"; shift
  if test "$1" = "$ibcsus" && test -z "$use_ibcsus"; then dotest_xsize "$prog"; return 0; fi
  cmdq="$(exec 2>&1 && set -x && : "$@" "$prog")"; echo "+ ${cmdq#+ : }" >&2
  is_crlf=; cdargs=
  if test "$1" = "$dosboxnox"; then
    # cdargs=-cd is needed to force decompression even though "$dosboxnox" falsely indicates that stdin is a TTY.
    is_crlf=1; cdargs=-cd; exit_code=0; "$@" "$prog" >usage.tmp || exit_code="$?"; cat usage.tmp; test "$exit_code" = 1
  else
    test "$1" != "$dosmyemu" || is_crlf=1
    exit_code=0; "$@" "$prog" 2>usage.tmp || exit_code="$?"; cat usage.tmp; test "$exit_code" = 1
  fi
  if test -z "$is_crlf" && test "$(cat usage.tmp; echo .)" = "Usage: luuzcat [-][r][m] <input.gz >output${nl}https://github.com/pts/luuzcat${nl}."; then :
  elif test "$is_crlf" && test "$(cat usage.tmp; echo .)" = "Usage: luuzcat [-][r][m] <input.gz >output${crlf}https://github.com/pts/luuzcat${crlf}."; then :
  else echo "fatal: unexpected usage message" >&2; exit 6
  fi
  cmd \<testdata/test_C1_new9.Z \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1.advdef.deflate \>test_C1.tmp "$@" "$prog" -r; cmp testdata/test_C1.good test_C1.tmp
}

# Do a full test.
dotest() {
  dotest1 "$@"
  prog="./$1"; shift
  cdargs=; test "$1" != "$dosboxnox" || cdargs=-cd  # cdargs=-cd is needed to force decompression even though "$dosboxnox" falsely indicates that stdin is a TTY.
  cmd \<testdata/XFileMgro.sz \>XFileMgro.tmp "$@" "$prog" $cdargs; cmp testdata/XFileMgro.good XFileMgro.tmp
  cmd \<testdata/test_C1.bin.C \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1_pack.z \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1_pack_old.z \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1_pack_old3.z \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp  # test_C1_pack_old3.z created using pack.c in https://github.com/pts/pts-opack-port
  cmd \<testdata/test_C1.advdef.gz \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1.advdef.qz \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1.advdef.zlib \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1.advdef.deflate \>test_C1.tmp "$@" "$prog" -r; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1.advdef.gz \>test_C1.tmp "$@" "$prog" $cdargs-r; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1_old.F \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1.F \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1_old13.Z \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1_old14.Z \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1_old15.Z \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1_old16.Z \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1_new9.Z \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1_new13.Z \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1_new14.Z \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1_new15.Z \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1_new16.Z \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1.zip \>test_C1.tmp "$@" "$prog" $cdargs; cmp testdata/test_C1.good test_C1.tmp
  cmd \<testdata/test_C1_split.zip \>test_C1.tmp "$@" "$prog" -m ; cmp testdata/test_C1.good test_C1.tmp  # -m is needed because test_C1.split.zip has multiple archive members.
  if test "$1" = "$dosboxnox"; then
    cat testdata/test_C1_1of2.good >splitexp.tmp
    printf 'multiple archive members\r\n' >>splitexp.tmp  # "$dosboxnox" incorrectly sends the program's stderr to the stdout.
    expfn=splitexp.tmp
  else
    expfn=testdata/test_C1_1of2.good
  fi
  exit_code=0; cmdfailok \<testdata/test_C1_split.zip \>test_C1.tmp "$@" "$prog" $cdargs || exit_code="$?"; test "$exit_code" = 1; cmp "$expfn" test_C1.tmp
  # !! Add more tests for concatenated streams.
}

# Do a full test which is supported by elksemu.
# !! Remove duplication with with dotest().
dotest_elksemu() {
  prog="./$1"; shift
  cmdq="$(exec 2>&1 && set -x && : "$perl" elksemu.pl "$elksemu" "$prog" "$@")"; echo "+ ${cmdq#+ : }" >&2
  exit_code=0; "$perl" elksemu.pl "$elksemu" "$prog" "$@" 2>usage.tmp || exit_code="$?"; cat usage.tmp; test "$exit_code" = 1
  if test "$(cat usage.tmp; echo .)" = "Usage: luuzcat [-][r][m] <input.gz >output${nl}https://github.com/pts/luuzcat${nl}."; then :
  else  echo "fatal: unexpected usage message" >&2; exit 6
  fi
  cmd "$perl" elksemu.pl --in=testdata/XFileMgro.sz --out=XFileMgro.tmp "$elksemu" "$prog" "$@"; cmp testdata/XFileMgro.good XFileMgro.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1.bin.C --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1_pack.z --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1_pack_old.z --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1_pack_old3.z --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp  # test_C1_pack_old3.z created using pack.c in https://github.com/pts/pts-opack-port
  cmd "$perl" elksemu.pl --in=testdata/test_C1.advdef.gz --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1.advdef.qz --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1.advdef.zlib --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1.advdef.deflate --out=test_C1.tmp "$elksemu" "$prog" "$@" -r; cmp testdata/test_C1.good test_C1.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1.advdef.gz --out=test_C1.tmp "$elksemu" "$prog" "$@" -r; cmp testdata/test_C1.good test_C1.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1_old.F --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1.F --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1_old13.Z --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1_old14.Z --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp
  #cmd "$perl" elksemu.pl --in=testdata/test_C1_old15.Z --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp  # It won't work in elksemu, because elksemu <=0.8.1 has fork(2) implemented incorrectly, and it doesn't have fmemalloc(3) at all.
  #cmd "$perl" elksemu.pl --in=testdata/test_C1_old16.Z --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp  # It won't work in elksemu, because elksemu <=0.8.1 has fork(2) implemented incorrectly, and it doesn't have fmemalloc(3) at all.
  cmd "$perl" elksemu.pl --in=testdata/test_C1_new9.Z --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1_new13.Z --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1_new14.Z --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp
  #cmd "$perl" elksemu.pl --in=testdata/test_C1_new15.Z --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp  # It won't work in elksemu, because elksemu <=0.8.1 has fork(2) implemented incorrectly, and it doesn't have fmemalloc(3) at all.
  #cmd "$perl" elksemu.pl --in=testdata/test_C1_new16.Z --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp  # It won't work in elksemu, because elksemu <=0.8.1 has fork(2) implemented incorrectly, and it doesn't have fmemalloc(3) at all.
  cmd "$perl" elksemu.pl --in=testdata/test_C1.zip --out=test_C1.tmp "$elksemu" "$prog" "$@"; cmp testdata/test_C1.good test_C1.tmp
  cmd "$perl" elksemu.pl --in=testdata/test_C1_split.zip --out=test_C1.tmp "$elksemu" "$prog" "$@" -m ; cmp testdata/test_C1.good test_C1.tmp  # -m is needed because test_C1.split.zip has multiple archive members.
  exit_code=0; cmdfailok "$perl" elksemu.pl --in=testdata/test_C1_split.zip --out=test_C1.tmp "$elksemu" "$prog" "$@" || exit_code="$?"; test "$exit_code" = 1; cmp testdata/test_C1_1of2.good test_C1.tmp
  # !! Add more tests for concatenated streams.
}

if test -z "$1" || test "$1" = --help; then
  echo "Usage: $argv0 <target> [...]${nl}Example <target>: all" >&2
  exit 1
fi

for arg in "$@"; do if test "$arg" = all; then set -- "$@" $all; break; fi; done
for arg in "$@"; do if test "$arg" = luuzcat.exe; then set -- luuzcatd.exe "$@"; break; fi; done

rm -f -- *_16.o *_32.o *_32y.o *_16.wasm *_32.wasm *_16.nasm *_32.nasm
for arg in "$@"; do case "$arg" in luuzcat.* | luuzcatd.exe) rm -f "$arg" ;; esac; done

need_test=; use_ibcsus=1; use_dosmyemu=1; use_elksemu=1; need_osxcross=1; for arg in "$@"; do
  if test "$arg" = test; then need_test=1
  elif test "$arg" = ibcsus; then use_ibcsus=1
  elif test "$arg" = noibcsus; then use_ibcsus=
  elif test "$arg" = dosmyemu; then use_dosmyemu=1
  elif test "$arg" = nodosmyemu; then use_dosmyemu=
  elif test "$arg" = elksemu; then use_elksemu=1
  elif test "$arg" = noelksemu; then use_elksemu=  # This may be useful if "$elksemu" is unreliable on the host system even if elksemu.pl restarts it.
  elif test "$arg" = macos; then need_osxcross=1
  elif test "$arg" = nomacos; then need_osxcross=  # This may be useful on a Linux i386 system, on which pts-osxcross doesn't run (bacause it needs Linux amd64).
  fi
done

for arg in "$@"; do case "$arg" in
  luuzcat.elf | luuzcat.coff) test -z "$need_test" || doibcsus ;;
  luuzcat.da6 | luuzcat.di3) doosxcross ;;
  luuzcat.com | luuzcatd.exe) test -z "$need_test" || dodosmyemu ;;
esac; done

for arg in "$@"; do case "$arg" in
  luuzcat.elf | luuzcat.coff | luuzcat.3b | luuzcat.mi3 | luuzcat.m3vm | luuzcat.v7x | luuzcat.x63) dowcc386 0 ;;
  luuzcat.com | luuzcatd.exe) dowccdos 20 ;;
  luuzcat.mi8) dowccminixi86 30 ;;
  luuzcat.exe) dowcc386 0; doobjs32y ;;
esac; done

for arg in "$@"; do case "$arg" in
  luuzcat.elf)  cmd "$nasm" -O999999999 -w+orphan-labels -DINCLUDES="'luuzcatw_32.nasm'$common_includes386" -o luuzcat.elf progx86.nasm; cmd chmod +x luuzcat.elf ;;
  luuzcat.coff) cmd "$nasm" -O999999999 -w+orphan-labels -DINCLUDES="'luuzcat_32.nasm'$common_includes386" -DCOFF -DCOFF_PROGRAM_NAME="'luuzcat'" -o luuzcat.coff progx86.nasm; cmd chmod +x luuzcat.coff ;;
  luuzcat.3b)   cmd "$nasm" -O999999999 -w+orphan-labels -DINCLUDES="'luuzcat_32.nasm'$common_includes386" -DS386BSD                              -o luuzcat.3b   progx86.nasm; cmd chmod +x luuzcat.3b   ;;
  luuzcat.mi3)  cmd "$nasm" -O999999999 -w+orphan-labels -DINCLUDES="'luuzcat_32.nasm'$common_includes386" -DMINIXI386                            -o luuzcat.mi3  progx86.nasm; cmd chmod +x luuzcat.mi3  ;;
  luuzcat.m3vm) cmd "$nasm" -O999999999 -w+orphan-labels -DINCLUDES="'luuzcat_32.nasm'$common_includes386" -DMINIX386VM                           -o luuzcat.m3vm progx86.nasm; cmd chmod +x luuzcat.m3vm ;;
  luuzcat.v7x)  cmd "$nasm" -O999999999 -w+orphan-labels -DINCLUDES="'luuzcat_32.nasm'$common_includes386" -DV7X86                                -o luuzcat.v7x  progx86.nasm; cmd chmod +x luuzcat.v7x  ;;
  luuzcat.x63)  cmd "$nasm" -O999999999 -w+orphan-labels -DINCLUDES="'luuzcat_32.nasm'$common_includes386" -DXV6I386                              -o luuzcat.x63  progx86.nasm; cmd chmod +x luuzcat.x63  ;;
  luuzcat.com)  done_luuzcatdcom=1; cmd "$nasm" -O999999999 -w+orphan-labels -f bin -DINCLUDES="'uncompressc_16.nasm',$common_includesdos" -DDOSCOM -DFULL_BSS -o luuzcat.com progx86.nasm; cmd "$perl" shorten_to_bss.pl luuzcat.com ;;  # luuzcat.com allocates extra memory in uncompress.c.
  luuzcatd.exe) if test -z "$done_luuzcatdexe"; then done_luuzcatexe=1; cmd "$nasm" -O999999999 -w+orphan-labels -f bin -DINCLUDES="'uncompressc_16.nasm',$common_includesdos" -DDOSEXE -DFULL_BSS -o luuzcatd.exe progx86.nasm; fi ;;  # luuzcatd.exe allocates extra memory in uncompress.c.
  luuzcat.mi8)  cmd "$nasm" -O999999999 -w+orphan-labels -f bin -DINCLUDES="'uncompressm_16.nasm',$common_includesminixi86" -DMINIXI86 -DMINIX_STACK_SIZE=0x600 -o luuzcat.mi8 progx86.nasm; cmd chmod +x luuzcat.mi8 ;;  # elksemu needs the `chmod +x'.
  luuzcat.da6)  if test "$need_osxcross"; then cmd "$ptsosxcrossdir"/x86_64-apple-darwin14/bin/gcc -m64 -mmacosx-version-min=10.5 -nodefaultlibs -lSystem -Os -W -Wall -Wextra -Wstrict-prototypes -Werror-implicit-function-declaration -ansi -pedantic -o luuzcat.da6 luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c; "$ptsosxcrossdir"/x86_64-apple-darwin14/bin/strip luuzcat.da6; fi ;;
  luuzcat.di3)  if test "$need_osxcross"; then cmd "$ptsosxcrossdir"/i386-apple-darwin14/bin/gcc   -m32 -mmacosx-version-min=10.5 -nodefaultlibs -lSystem -Os -W -Wall -Wextra -Wstrict-prototypes -Werror-implicit-function-declaration -ansi -pedantic -o luuzcat.di3 luuzcat.c unscolzh.c uncompact.c unopack.c unpack.c undeflate.c uncompress.c unfreeze.c; "$ptsosxcrossdir"/i386-apple-darwin14/bin/strip   luuzcat.di3; fi ;;
  luuzcat.exe)
    cmd "$nasm" -O999999999 -w+orphan-labels -f obj -DINCLUDES= -D__GLOBAL_G@main_=2 $dneeds -DWIN32WL -o luuzcatp.o progx86.nasm
    # `op st=1024K' is a better general default.
    # We omit `op norelocs', otherwise WDOSX wouldn't be able to run it.
    # We omit `op exporta', because we don't need these symbols.
    # !! Add our own NASM-based linker for creating Win32 PE .exe files (rather than using wlink).
    cmd "$wlink" op q form win nt ru con=3.10 op h=4K com h=0 op st=64K com st=64K disa 1080 op noext op d op nored op start=_start op stub=luuzcatd.exe n luuzcat.exe f luuzcatp.o f luuzcatr_32y.o f unscolzh_32y.o f uncompact_32y.o f unopack_32y.o f unpack_32y.o f undeflate_32y.o f uncompress_32y.o f unfreeze_32y.o
    # Hotfix: Add 3 to .minalloc. wlink(1) has kept it intact, incorrectly. !! Come up with a safer hotfix, reading both files.
    # Hotfix: For reproducible builds, change the PE TimeDateStamp header field to 0.
    cmd "$perl" -0777 -wpi -e 'substr($_, 0xa, 2) = pack("v", unpack("v", substr($_, 0xa, 2)) + 3); my $pe_ofs = unpack("v", substr($_, 0x3c, 4)); substr($_, $pe_ofs + 8, 4) = "\0\0\0\0"' luuzcat.exe ;;
  all | test | ibcsus | noibcsus | dosmyemu | nodosmyemu | elksemu | noelksemu | macos | nomacos) ;;
  *) echo "fatal: unknown <target>: $arg" >&2; exit 4
esac; done

if test "$need_test"; then for arg in "$@"; do case "$arg" in
  luuzcat.elf) dotest "$arg"; dotest1 "$arg" "$ibcsus" ;;
  luuzcat.coff) dotest "$arg" "$ibcsus" ;;
  luuzcat.3b | luuzcat.mi3 | luuzcat.m3vm | luuzcat.v7x | luuzcat.x63) dotest_xsize "$arg" ;;
  luuzcat.da6 | luuzcat.di3) if test "$need_osxcross"; then dotest_xsize "$arg"; fi ;;
  luuzcat.com) dotest "$arg" "$dosmyemu" --cmd ;;
  luuzcatd.exe) if test "$done_luuzcatdcom"; then dotest1 "$arg" "$dosmyemu" --cmd; else dotest "$arg" "$dosmyemu" --cmd; fi ;;
  luuzcat.exe) dotest "$arg" "$dosboxnox" --cmd --mem-mb=2 "$mwperunexe" ;;
  luuzcat.mi8) if test "$use_elksemu"; then dotest_elksemu "$arg"; else dotest_xsize "$arg"; fi ;;
esac; done; fi 

if false; then
  # !! We need non-empty command-line because dosbox.nox.static incorrectly reports that stdin is a TTY.
  dosbox.nox.static --cmd --mem-mb=2 ~/prg/mwpestub/mwperun.exe luuzcat.exe -cd <test_C1_new9.Z >test_C1.bin
      cmp test_C1.good test_C1.bin
  dosbox.nox.static --cmd luuzcat.exe -cd <test_C1_new9.Z >test_C1.bin
      cmp test_C1.good test_C1.bin
  ./kvikdos luuzcat.exe <test_C1_new9.Z >test_C1.bin
      cmp test_C1.good test_C1.bin
fi

# !! Add some missing 16-bit targets such as luuzcatc.com.

echo "$(exec 2>&1 && set -x && : "$argv0" OK.)"
