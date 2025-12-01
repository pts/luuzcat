#
# elksemu.pl: runs elksemu, retrying it upon a SIGSTOP
# by pts@fazekas.hu at Sun Nov 30 23:17:02 CET 2025
#

BEGIN { $^W = 1 }
use integer;
use strict;

my($elksemu, $prog, $stdinfn, $stdoutfn);
{ my $i = 0;
  while ($i < @ARGV) {
    my $arg = $ARGV[$i++];
    if ($arg eq "--") { last }
    elsif ($arg !~ m@^-@) { --$i; last }
    elsif ($arg =~ m@^--in=(.*)@s) { $stdinfn = $1 }
    elsif ($arg =~ m@^--out=(.*)@s) { $stdoutfn = $1 }
    else { die("fatal: unknown command-line flag: $arg\n") }
  }
  die("fatal: missing elksemu argument\n") if $i >= @ARGV;
  $elksemu = $ARGV[$i++];
  die("fatal: missing prog argument\n") if $i >= @ARGV;
  $prog = $ARGV[$i++];
  splice(@ARGV, 0, $i);
}
die("fatal: elksemu not an executable program file: $elksemu\n") if !-f($elksemu) or !-x($elksemu);
die("fatal: prog not an executable program file: $prog\n") if !-f($prog) or !-x($prog);

my $stdinfnq = (!defined($stdinfn) or $stdinfn =~ m@^[\w/]@) ? $stdinfn : "./$stdinfn";  # Escape for Perl open.
my $stdoutfnq = (!defined($stdoutfn) or $stdoutfn =~ m@^[\w/]@) ? $stdoutfn : "./$stdoutfn";  # Escape for Perl open.
# $SIG{CHLD} = "IGNORE";  # Causes segfault.
%ENV = ("FOO" => "bar");  # Avoid the NoMem error upon a too large Linux environment.
for (;;) {
  my $pid = fork();
  die("fatal: fork: $!\n") if !defined($pid);
  if (!$pid) {  # Child.
    die("fatal: error opening --in= file: $stdinfn\n") if defined($stdinfnq) and !open(STDIN, "< $stdinfnq");
    die("fatal: error opening --out= file: $stdoutfn\n") if defined($stdoutfnq) and !open(STDOUT, "> $stdoutfnq");
    die("fatal: exec with elksemu $prog: $!\n") if !defined(exec($elksemu, $prog, @ARGV));
    exit(127);
  }
  #my $wpid = (wait() or 0);  # Exit status is returned in $?.
  my $wpid = (waitpid($pid, 2) or 0);  # Exit status is returned in $?. 2 == WUNTRACED == WSTOPPED on Linux.
  die("fatal: bad child wait PID: expected=$pid got=$wpid\n") if $pid != $wpid;
  last if $? != (19 << 8 | 0x7f); # 19 == SIGSTOP on Linux. elksemu is unreliable (gets SIGSTOP 6..14 times out of 1000). Retry it.
  print("fatal: error sending SIGKILL: $!\n") if !kill(9, $pid);
  waitpid($pid, 0);
}
exit($? >> 8) if !($? & 0xff);  # Regular exit(...).
exit(127);  # Some kind of signal exit.
