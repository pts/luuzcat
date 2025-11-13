#! /bin/sh --
set -ex
test "$0" = "${0%/*}" || cd "${0%/*}"

dos="${1:-msdos330}"
at='@'

bakefatdir="$HOME/Downloads/windows_nt_qemu/hdachs"
rm -f fd.img
case "$dos" in
 msdos3[01]0) at=  # MS-DOS 3.00 doesn't support the `@' in `@echo off'.
  cp -a -- "$HOME"/Downloads/windows_nt_qemu/hdachs/boot.msdos300.img fd.img ;;
 msdos320) at=  # MS-DOS 3.20 doesn't support the `@' in `@echo off'.
 "$bakefatdir"/bakefat 1200K fd.img
  dd if="$bakefatdir"/msd320.good.bs.bin of=fd.img conv=notrunc bs=2 skip=27 seek=27 count=229  # The bakefat boot code needs MS-DOS >=3.30, so we use the original MS-DOS 3.20 boot code.
  dd if="$bakefatdir"/msd320.good.bs.bin of=fd.img conv=notrunc bs=11 count=1 ;;
 *)  "$bakefatdir"/bakefat 1200K fd.img ;;
esac
printf '%secho off\r\nprompt $P$G\r\nver' "$at" >autotest.bat
case "$dos" in
 msdos3[01]0) ;;  # Floppy image very small.
 *)
  mcopy -bsomp -i fd.img "$bakefatdir"/IO.SYS."$dos".fat2 ::IO.SYS
  mcopy -bsomp -i fd.img "$bakefatdir"/MSDOS.SYS."$dos" ::MSDOS.SYS
  mcopy -bsomp -i fd.img "$bakefatdir"/COMMAND.COM."$dos" ::COMMAND.COM
  mcopy -bsomp -i fd.img test_C1.advdef.gz ::T.GZ
  mcopy -bsomp -i fd.img test_C1.good ::T.GOO
esac
mcopy -bsomp -i fd.img autotest.bat ::AUTOEXEC.BAT
mcopy -bsomp -i fd.img "$bakefatdir"/crc32.com ::CRC32.COM
mcopy -bsomp -i fd.img "$bakefatdir"/atxoff.com ::O.COM
mcopy -bsomp -i fd.img luuzcat.com ::LUUZCAT.COM
mcopy -bsomp -i fd.img test_C1_new16.Z ::T.Z
rm -f autotest.bat

qemu-system-i386 -M pc-1.0 -m 2 -nodefaults -vga cirrus -drive file=fd.img,format=raw,if=floppy -boot a -debugcon stdio

: "$0" OK.
