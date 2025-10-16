#
# wasm2nasm.pl: convert wdis(1) `wdis -a -fi -fu -i=@' disassembly from WASM to NASM syntax
# by pts@fazekas.hu at Mon Oct 13 03:35:21 CEST 2025
#
# Currently only the output of wcc386 (OpenWatcom C compiler, i386 target)
# is supported. Tested only with a limited subset.
#

BEGIN { $^W = 1 }
use integer; use strict;
my $fi = int($ARGV[0]); die("bad fi: $fi\n") if !$fi;
my $ofs;
my %globals;  # Symbol names to be prefixed with "G\@".
# !! TODO(pts): Also convert the local synmbols to have @ in their name.
while (<STDIN>) {
  chomp;
  if (s@^\t(?!\t)@@) {  # Assembly instructions in _TEXT.
    die("bad quote: $_\n") if m@["\x27]@;
    s@\s+@ @g; s@\bFLAT:@@g; s@\@\$(\d+)\b@L\x40$fi\x40$1@g; s@,(?! )@, @g;
    s@\b(byte|d?word) ptr (\w[^,]*)@\U$1\E [$2]@g;
    s@\b(byte|d?word) ptr (\[[^\\[\]]*?\])@\U$1\E $2@g;
    s@\boffset @@g;
    if (m@^(CALL|JMP) near ptr (L\@\d+\@\d+|[_a-zA-Z]\w*)$@) { $_ = "$1 $2" }
    s@\b([_a-zA-Z]\w*)(?!\@)@ exists($globals{$1}) ? "G\x40$1" : $1 @ge;
    #else { die("bad instruction: $_\n") }
    print("\t\t", $_, "\n");
  } elsif (m@^\t\t@) {
    if (m@^\t\tPUBLIC\s+([_a-zA-Z]\w*)$@) { $globals{$1} = 1; print("f_global G\@$1\n") }
    elsif (m@^\t\tEXTRN\s+([_a-zA-Z]\w*):@) { $globals{$1} = 1; print("%define __NEED_G\@$1\n") }
    elsif (m@^\t\tASSUME\s+@) {}
    elsif ($_ eq "\t\tEND") {}
    else { die("bad: $_\n") }
  } else {
    if (m@^([_a-zA-Z]\w*)(?::|\s+LABEL\s)@) { print(exists($globals{$1}) ? "G\@$1:\n" : "$1:\n") }
    elsif (m@^\@\$(\d+)(?::|\s+LABEL\s)@) { print("L\@$fi\@$1:\n") }
    elsif (m@^    D[BWD]\s+@) { s@^\s+@\t\t@; die("bad quote: $_\n") if m@["\x27]@; if (s@\boffset (?:FLAT:)?@@g) { s@\@\$(\d+)\b@L\x40$fi\x40$1@g; s@\b([_a-zA-Z]\w*)(?!\@)@ exists($globals{$1}) ? "G\x40$1" : $1 @ge } print($_, "\n") }
    elsif (m@^    ORG\s+(\d[\da-f]*)(H?)@) { die("bad ofs: $_\n") if !defined($ofs); my $oldofs = $ofs; $ofs = $2 ? hex($1) : int($1); print("\t\tresb $oldofs\n") if ($oldofs = $ofs - $oldofs) > 0; }
    elsif (m@^DGROUP\s+GROUP\t@) {}
    elsif (m@^(_TEXT|CONST2?|_DATA|_BSS)\s+SEGMENT\t@) { print("f_section_$1\n"); $ofs = 0 if $1 eq "_BSS" }
    elsif (m@^[_A-Z0-9]+\s+ENDS$@) { $ofs = undef }
    elsif ($_ eq ".387" or $_ eq ".386p" or $_ eq ".model flat" or !length($_) or m@^YI[BE]?\s+SEGMENT\t@) {}
    else { die("bad: $_\n") }
  }
}
