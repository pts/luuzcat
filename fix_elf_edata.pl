#
# wasm2nasm.pl: fix broken values of _edata and _end in ELF-32 executable created by broken OpenWatcomw wlink(1) 2023-03-04
# by pts@fazekas.hu at Thu Oct 16 19:35:46 CEST 2025
#

BEGIN { $^W = 1 }
use integer;
use strict;
die("Usage: $0 <prog.elf>\n") if @ARGV != 1;
my $fn = $ARGV[0];
my $fnoq = ($fn =~ m@^[\w/]@) ? $fn : "./$fn";  # Escape for Perl open.
die("fatal: open: $fn\n") if !open(F, "+< $fnoq");
my $header;
die("fatal: error reading ELF header: $fn\n") if (sysread(F, $header, 0x1000) or -1) < 0;
# https://en.wikipedia.org/wiki/Executable_and_Linkable_Format#ELF_header
die("fatal: not an ELF-32 i386 executable: $fn\n") if length($header) < 0x34 or $header !~ m@^\x7fELF\x01\x01\x01.{9}\x02\0\x03\0\x01\0\0\0@s;
my($e_entry, $e_phoff, $e_shoff, $e_flags, $e_ehsize, $e_phentsize, $e_phnum, $e_shentsize, $e_shnum, $e_shstrndx) =
    unpack("VVVVvvvvvv", substr($header, 0x18, 0x34 - 0x18));
$e_shentsize = 0x28 if !$e_shentsize;
die("fatal: missing section headers, file has been stripped too much: $fn\n") if !$e_shnum;
die("fatal: bad e_phentsize: $fn\n") if $e_phentsize != 0x20;
die("fatal: bad e_phentsize: $fn\n") if $e_shentsize != 0x28;
die("fatal: bad e_shstrndx: $fn\n") if $e_shstrndx >= $e_shnum;
die("fatal: program header too far: $fn\n") if $e_phoff + ($e_phnum << 5) > length($header);
my $good_edata = 0; my $good_end = 0;
my($first_p_vaddr, $first_p_offset, $first_p_filesz);
for (my $phi = 0; $phi < $e_phnum; ++$phi) {
  my($p_type, $p_offset, $p_vaddr, $p_paddr, $p_filesz, $p_memsz, $p_flags, $p_align) = unpack("V8", substr($header, $e_phoff + ($phi << 5)));
  next if $p_type != 1;  # PT_LOAD.
  die("fatal: bad ordering of p_filesz and p_memsz: $fn\n") if $p_memsz < $p_filesz;
  ($first_p_vaddr, $first_p_offset, $first_p_filesz) = ($p_vaddr, $p_offset, $p_filesz) if !defined($first_p_vaddr);
  $good_edata = $p_vaddr + $p_filesz if $good_edata < $p_vaddr + $p_filesz;
  $good_end = $p_vaddr + $p_memsz if $good_end < $p_vaddr + $p_memsz;
}
die("fatal: _edata and _end not found in PT_LOAD header: $fn\n") if !$good_edata or !$good_end;
die("fatal: error seeking to section header: $fn\n") if (sysseek(F, $e_shoff, 0) or 0) != $e_shoff;
my $sheader;
die("fatal: error reading section header: $fn\n") if (sysread(F, $sheader, $e_shnum * 0x28) or 0) != $e_shnum * 0x28;
my($symtab_fofs, $symtab_count);  my($strtab_fofs, $strtab_size);
for (my $shi = 0; $shi < $e_shnum; ++$shi) {
  my($sh_name, $sh_type, $sh_flags, $sh_addr, $sh_offset, $sh_size, $sh_link, $sh_info, $sh_addralign, $sh_entsize) = unpack("V10", substr($sheader, $shi * 0x28, 0x28));
  if ($sh_type == 2) {  # SH_SYMTAB.
    die("fatal: multiple symbol tables found: $fn\n") if defined($symtab_fofs);
    die("fatal: bad sh_entsize for symbol table: $fn\n") if $sh_entsize != 0x10;
    die("fatal: bad sh_size for symbol table: $fn\n") if $sh_size & 0xf;
    $symtab_fofs = $sh_offset; $symtab_count = $sh_size >> 4;
  } elsif ($sh_type == 3 and $shi != $e_shstrndx) {  # SH_STRTAB.
    die("fatal: multiple string tables found: $fn\n") if defined($strtab_fofs);
    die("fatal: bad sh_entsize for string table: $fn\n") if $sh_entsize > 1;
    $strtab_fofs = $sh_offset; $strtab_size = $sh_size;
  }
}
die("fatal: missing symbol table: $fn\n") if !defined($symtab_fofs);
die("fatal: missing string table: $fn\n") if !defined($strtab_fofs);
die("fatal: error seeking to symbol table: $fn\n") if (sysseek(F, $symtab_fofs, 0) or 0) != $symtab_fofs;
my $symtab;
die("fatal: error reading symbol table: $fn\n") if (sysread(F, $symtab, $symtab_count << 4) or 0) != ($symtab_count << 4);
die("fatal: error seeking to string table: $fn\n") if (sysseek(F, $strtab_fofs, 0) or 0) != $strtab_fofs;
my $strtab;
die("fatal: error reading string table: $fn\n") if (sysread(F, $strtab, $strtab_size) or 0) != $strtab_size;
die("fatal: bad string table: $fn\n") if vec($strtab, 0, 8) or vec($strtab, length($strtab) - 1, 8);
#$strtab .= "#";  # To prevent split(...) below from truncating the final, empty strings.
#my @strings = split(m@\0@, $strtab);
#die("fatal: assert: bad string pop\n") if pop(@strings) ne "#";
#$strtab =~ y@\0@,@;
my(@vaddrs_to_patch);
for (my $symi = 0; $symi < $symtab_count; ++$symi) {
  my($st_name, $st_value, $st_size, $st_info, $st_other, $st_shnidx) = unpack("VVVCCv", substr($symtab, $symi << 4, 0x10));
  die("fatal: bad symbol name index: $st_name: $fn\n") if $st_name >= length($strtab) - 1;
  my $st_type = $st_info & 0xf;
  my $st_bind = $st_info >> 4;
  next if $st_type > 2 or $st_bind > 1;
  my $end_idx = index($strtab, "\0", $st_name);
  die("fatal: assert: bad st_name: $fn\n") if $end_idx < 0;
  my $symbol_name = substr($strtab, $st_name, $end_idx - $st_name);
  if (!$st_type and $st_bind == 1 and ($symbol_name eq "_end" or $symbol_name eq "__end")) {  # NOTYPE, GLOBAL.
    # wlink(1) seems to fill it correctly here (but it doesn't subtitute it in the code).
    die(printf("fatal: conflicting values for _end: 0x%08x vs 0x%08x\n", $good_end, $st_value)) if $good_end != $st_value;
  } elsif (!$st_type and $st_bind == 1 and ($symbol_name eq "_edata" or $symbol_name eq "__edata")) {  # NOTYPE, GLOBAL.
    # wlink(1) seems to fill it correctly here (but it doesn't subtitute it in the code).
    my $good_edata_up = ($good_edata + 0xfff) & ~0xfff;
    die(printf("fatal: conflicting values for _edata: 0x%08x vs 0x%08x\n", $good_edata, $st_value)) if $good_edata != $st_value and $good_edata_up != $st_value;
    $good_edata = $st_value if $good_edata < $st_value;
  } elsif ($symbol_name =~ m@^_start[.](start|end)ref\d*\Z(?!\n)@) {  # NOTYPE, GLOBAL.
    # This test is a bit restrictive. We could also apply it to subsequent PT_LOADs if we remembered their locations.
    die("fatal: offset to patch is not in the first PT_LOAD: $fn\n") if $st_value < $first_p_vaddr or $st_value > $first_p_vaddr + $first_p_filesz - 4;
    push @vaddrs_to_patch, (($1 eq "end") ? 1 : 0), $st_value;
  }
}
die("fatal: no patch locations found\n") if !@vaddrs_to_patch;
for (my $i = 0; $i < @vaddrs_to_patch; $i += 2) {  # Typically 3 patches.
  my $value = ($vaddrs_to_patch[$i]) ? $good_end : $good_edata;
  my $vaddr = $vaddrs_to_patch[$i + 1];
  my $fofs = $vaddr - $first_p_vaddr + $first_p_offset;
  die("fatal: error seeking to patch location: $fn\n") if (sysseek(F, $fofs, 0) or 0) != $fofs;
  my $b;
  die("fatal: error reading patch instruction: $fn\n") if (sysread(F, $b, 1) or 0) != 1;
  die("Fatal: bad patch instruction: $fn\n") if ord($b) < 0xb0 or ord($b) > 0xbf;  # `mov reg, imm32'.
  die("fatal: error seeking to patch location again: $fn\n") if (sysseek(F, $fofs + 1, 0) or 0) != $fofs + 1;
  die("fatal: error reading patch instruction: $fn\n") if (syswrite(F, pack("V", $value), 4) or 0) != 4;
}

__END__
