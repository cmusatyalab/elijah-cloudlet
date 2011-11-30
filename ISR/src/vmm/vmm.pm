#
# vmm.pm - Helper code for OpenISR (R) VMM drivers written in Perl
#
# Copyright (C) 2008-2009 Carnegie Mellon University
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2 of the GNU General Public License as published
# by the Free Software Foundation.  A copy of the GNU General Public License
# should have been distributed along with this program in the file
# LICENSE.GPL.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#

package vmm;
use File::Spec;
use Exporter qw/import/;
use POSIX qw/setpgid/;
use strict;
use warnings;

BEGIN {
	my @import = (qw/NAME CFGDIR UUID SECTORS MEM FULLSCREEN/,
				qw/SUSPENDED COMMAND VERBOSE/);
	our @EXPORT = (qw/main fail find_program run_program $VMNAME/,
				qw/$USES_ROOT %OPTIONS/);
	our $VMNAME = "UnknownVMM";
	our $USES_ROOT = "no";
	our %OPTIONS;
	if (exists $ENV{'OPTIONS'}) {
		for (split(',', $ENV{'OPTIONS'})) {
			my ($k, $v) = split('=', $_, 2);
			$k or next;
			$OPTIONS{$k} = $v || '1';
		}
	}
	foreach my $var (@import) {
		push(@EXPORT, "\$$var");
		eval "our \$$var = \$ENV{'$var'}"
			if exists $ENV{$var};
	}
}

sub main {
	our $VMNAME;
	our $SUSPENDED;
	our $USES_ROOT;
	my $msg;

	undef $@;
	if (@ARGV and $ARGV[0] eq "info") {
		eval {main::info()};
		if ($@) {
			($msg = <<EOF) =~ s/^\s+//gm;
				VMM=$VMNAME
				RUNNABLE=no
				RUNNABLE_REASON=$@
EOF
		} else {
			($msg = <<EOF) =~ s/^\s+//gm;
				VMM=$VMNAME
				USES_ROOT=$USES_ROOT
				RUNNABLE=yes
EOF
		}
	} elsif (@ARGV and $ARGV[0] eq "run") {
		eval {main::run()};
		if ($@) {
			($msg = <<EOF) =~ s/^\s+//gm;
				SUSPENDED=$SUSPENDED
				SUCCESS=no
				ERROR=$@
EOF
		} else {
			($msg = <<EOF) =~ s/^\s+//gm;
				SUSPENDED=$SUSPENDED
				SUCCESS=yes
EOF
		}
	} elsif (@ARGV and $ARGV[0] eq "poweroff") {
		eval {main::poweroff()};
		if ($@) {
			($msg = <<EOF) =~ s/^\s+//gm;
				SUSPENDED=$SUSPENDED
				SUCCESS=no
				ERROR=$@
EOF
		} else {
			($msg = <<EOF) =~ s/^\s+//gm;
				SUSPENDED=$SUSPENDED
				SUCCESS=yes
EOF
		}
	} elsif (@ARGV and $ARGV[0] eq "cleanup") {
		eval {main::cleanup()}
			if exists &main::cleanup;
		if ($@) {
			($msg = <<EOF) =~ s/^\s+//gm;
				SUCCESS=no
				ERROR=$@
EOF
		} else {
			($msg = <<EOF) =~ s/^\s+//gm;
				SUCCESS=yes
EOF
		}
	} else {
		print STDERR "Unknown or no mode specified\n";
		exit 1;
	}
	print $msg;
	exit 0;
}

# Calls die() with newline appended, which prevents file and line information
# from being appended
sub fail {
	my $str = shift;

	die "$str\n";
}

# If $1 is an absolute path and executable, return it.  If it is a relative
# path and executable via the search path, return the absolute path to the
# executable.  If no executable is found, return undef.  The caller may
# override the search path by passing a list reference in $2; otherwise PATH
# is used.
sub find_program {
	my $prog = shift;
	my $search_path = shift;

	my $dir;

	return (-x $prog ? $prog : undef)
		if $prog =~ m:^/:;
	$search_path = [File::Spec->path()]
		unless $search_path;
	foreach $dir (@$search_path) {
		return "$dir/$prog"
			if -x "$dir/$prog";
	}
	return undef;
}

# Run a program and wait for it to exit, redirecting its stdout to stderr so
# that the child can't write key-value pairs back to our calling process.
# The second parameter specifies a subroutine to be called if we receive
# SIGINT; it takes the pid of the child as its first argument.  If the second
# parameter is undefined, we ignore SIGINT while the child is running.  If
# the third parameter is true, we run the child in its own process group
# and redirect its stdin/stdout/stderr to /dev/null.
sub run_program {
	my $cmd = shift;
	my $sigint_sub = shift;
	my $new_pgroup = shift;

	my $pid;

	local $SIG{'INT'};
	if (defined $sigint_sub) {
		$SIG{'INT'} = sub { &$sigint_sub($pid) };
	} else {
		$SIG{'INT'} = sub {}
	}

	defined ($pid = fork)
		or return -1;
	if (!$pid) {
		if ($new_pgroup) {
			open(STDIN, "/dev/null")
				or fail "Couldn't reopen stdin";
			open(STDOUT, ">", "/dev/null")
				or fail "Couldn't reopen stdout";
			open(STDERR, ">", "/dev/null")
				or fail "Couldn't reopen stderr";
			setpgid(0, 0)
				or fail "Couldn't set process group";
		} else {
			open(STDOUT, ">&", *STDERR)
				or fail "Couldn't reopen stdout";
		}
		exec $cmd
			or fail "Couldn't exec $cmd";
	}
	WAIT: {
		if (waitpid($pid, 0) == -1) {
			# Perl seems to return EBADF when it means EINTR.
			redo WAIT
				if $!{EBADF} or $!{EINTR};
			return -1;
		}
	}
	return $?;
}

1;
