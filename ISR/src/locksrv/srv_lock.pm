###################################################################
# srv_lock.pm - Acquires, releases, or checks a parcel lock
###################################################################

#
#                     Internet Suspend/Resume (R)
#           A system for capture and transport of PC state
#
#              Copyright (c) 2002-2004, Intel Corporation
#         Copyright (c) 2004-2007, Carnegie Mellon University
#
# This software is distributed under the terms of the Eclipse Public
# License, Version 1.0 which can be found in the file named LICENSE.Eclipse.
# ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS SOFTWARE CONSTITUTES
# RECIPIENT'S ACCEPTANCE OF THIS AGREEMENT
#

###################
# Standard prologue
###################
use strict;
use Sys::Hostname;
use Fcntl;

##################
# Helper functions
##################

sub stored_nonce ($) {
    my $lockfile = shift;
    my $found;
    
    open(LOCK, $lockfile)
	or errexit("Unable to open lock file $lockfile");
    $found = <LOCK>;
    chomp($found);
    close(LOCK)
	or errexit("Unable to close lock file $lockfile");
    return $found;
}

####################
# Begin main routine
####################

# 
# Variables
#
my $parceldir;
my $parcelname;
my $serverhostname;
my $userid;
my $clienthostname;
my $verbose;
my $lockfile;
my $logfile;
my $server_nonce;
my $datestring;
my $acquire;
my $release;
my $hard_release;
my $check;
my $check_nonce;
my $line;
my $action;
my $unused;
my %config = get_config();

#
# Parse the command line args
#
no strict 'vars';
getopts('hVu:p:n:ar:RcC:');

if ($opt_h) {
    usage();
}
if (!$opt_u) {
    $opt_u = $ENV{"USER"};
}
if (!$opt_p) {
    usage("Missing parcel name (-p)");
}
if (($opt_a or $opt_r or $opt_R) and !$opt_n) {
    usage("Missing client host name (-n)");
}
if (!$opt_a and !$opt_r and !$opt_R and !$opt_c and !$opt_C) {
    usage("Must specify either -a, -r <nonce>, -R, -c, or -C <nonce>.");
}
$userid = $opt_u;
$parcelname = $opt_p;
$clienthostname = $opt_n;
$acquire = $opt_a;
$release = $opt_r;
$hard_release = $opt_R;
$check = $opt_c;
$check_nonce = $opt_C;
$verbose = $opt_V;
use strict 'vars';

#
# Make sure the parcel directory exists
#
$parceldir = "$config{content_root}/$userid/$parcelname";
(-e $parceldir)
    or errexit("Parcel $parceldir does not exist");

#
# Set some variables that we'll need later
#
$lockfile = "$parceldir/LOCK";
$logfile = "$parceldir/lockholder.log";
$serverhostname = hostname();

#
# If the logfile doesn't exist then create an empty one
#
if (!-e $logfile) {
    open(LOGFILE, ">$logfile")
	or errexit("Unable to open log file $logfile.");
    close(LOGFILE) 
	or errexit("Unable to close log file $logfile.");
}
    

################
# Acquire a lock
################
if ($acquire) {
    # Create a nonce [1..MAXNONCE] that can be used to validate releases
    srand();  # Generate a different seed each time
    $server_nonce = int(rand(Server::MAXNONCE)) + 1;

    # Try to acquire the lock
    if (!sysopen(LOCK, $lockfile, O_WRONLY|O_CREAT|O_EXCL)) {
	# If we can't acquire the lock, try to print an informative
	# message that explains exactly why the request failed.
	$line = get_last_acquired_entry($logfile);
	($serverhostname, $datestring, $action, $userid, $parcelname, $clienthostname) = split('\|', $line); # NOTE: single quotes are important here
	if (-e $lockfile and $action eq "acquired") {
	    errexit("Unable to acquire lock for $parcelname because lock is currently held by $userid on $clienthostname since $datestring.");
	} else {
	    errexit("Unable to lock $userid/$parcelname (reason unknown).");
	}
    }

    # Save the nonce
    print LOCK "$server_nonce\n";
    close(LOCK)
	or errexit("Error: Unable to close $lockfile");

    # Log the successful result
    open(LOGFILE, ">>$logfile")
	or errexit("Error: Unable to open $logfile");
    $datestring = localtime();
    print LOGFILE "$serverhostname|$datestring|acquired|$userid|$parcelname|$clienthostname\n";
    close(LOGFILE)
	or errexit("Error: Unable to close $logfile");

    # Return the nonce to the caller via stdout
    print "$server_nonce\n";
    exit 0;
}

################
# Release a lock
################
if ($release or $hard_release) {
    # If no one holds the lock and we were asked for a hard release, we
    # silently succeed.  On the other hand, if we were asked for a soft
    # release, we fail.
    if (-e $lockfile) {
	# If we were asked for a soft release, compare nonce stored on
	# the server with the one passed on command line
	if ($release and stored_nonce($lockfile) != $release) {
	    errexit("Unable to release lock because the nonce passed on the command line does not match the nonce stored on the server.");
	}

	# Release the lock
	unlink($lockfile)
	    or errexit("couldn't remove lockfile $lockfile: $!\n");
    } elsif ($release) {
	errexit("Cannot perform soft release when lock is not held");
    }

    # Log the successful result
    open(LOGFILE, ">>$logfile")
	or errexit("Error: Unable to open $logfile");
    $datestring = localtime();
    print LOGFILE "$serverhostname|$datestring|released|$userid|$parcelname|$clienthostname\n";
    close(LOGFILE)
	or errexit("Error: Unable to close $logfile");

    # Done
    exit 0;
}

##############
# Check a lock
##############

# Returns exit code that indicates the current lock status.
# Also, return most recent acquire or release log entry via stdout
# if running in verbose mode.
#
# Note: We return 0 if lock exists (success), 1 if lock does not
# exist (failure). 
#
if ($check or $check_nonce) {
    $line = get_last_entry($logfile);
    print("$line\n")
	if $verbose;

    if (-e $lockfile) {
	exit 1
	    if $check_nonce and stored_nonce($lockfile) != $check_nonce;
	exit 0;
    } else {
	exit 1;
    }
}

# Control should never reach here.
exit 1;

##################
# End main routine
##################

#
# get_last_acquired_entry - Return the log entry from the last time
# the lock was acquired.
#
sub get_last_acquired_entry {
    my $logfile = shift;

    my $line;
    my $last_acquire_line = "";
    my $unused;
    my $action;

    open(INFILE, $logfile) 
	or errexit("Unable to open $logfile for reading.");
    while ($line = <INFILE>) {
	chomp($line);
	($unused, $unused, $action, $unused, $unused, $unused, $unused) 
	    = split('\|', $line); # NOTE: the single quotes are important here
	if ($action eq "acquired") {
	    $last_acquire_line = $line;
	}
    }
    close(INFILE)
	or errexit("Unable to close $logfile.");
    return $last_acquire_line;
}    

#
# get_last_entry - Return the log entry from the last time
# the lock was acquired or released. The reason we don't simply
# return the last line in the file is that we decide at some
# point to log other lock functions besides acquire and release,
# such as checking the lock, or failing to acquire or release.
#
sub get_last_entry {
    my $logfile = shift;

    my $line;
    my $last_line = "";
    my $unused;
    my $action;

    open(INFILE, $logfile) 
	or errexit("Unable to open $logfile for reading.");
    while ($line = <INFILE>) {
	chomp($line);
	($unused, $unused, $action, $unused, $unused, $unused, $unused) 
	    = split('\|', $line); # NOTE: the single quotes are important here
	if (($action eq "acquired") or ($action eq "released")) {
	    $last_line = $line;
	}
    }
    close(INFILE)
	or errexit("Unable to close $logfile.");
    return $last_line;
}    


#
# usage - print help message and terminate
#
sub usage
{
    my $msg = shift;
    my $progname;

    # Strip any path information from the program name
    ($progname = $0) =~ s#.*/##s; 

    if ($msg) {
        print "$progname: $msg\n";
    }

    print "Usage: $progname [-hV] [-u <userid>] -p <parcel> [-n <hostname>] -a|-r <nonce>|-R|-c|-C <nonce>\n";
    print "Options:\n";
    print "  -h          Print this message\n";
    print "  -V          Be verbose\n";
    print "  -u <user>   Username for this parcel (default is $ENV{'USER'})\n";
    print "  -p <parcel> Parcel name\n";    
    print "  -n <name>   Client host name (required in acquire/release modes)\n";
    print "Specify exactly one of the following commands:\n";
    print "  -a          Acquire lock and return nonce on stdout\n";
    print "  -r <nonce>  Release lock after checking <nonce>\n";
    print "  -R          Release lock without checking nonce\n";
    print "  -c          Check lock\n";
    print "  -C <nonce>  Check lock against <nonce>\n";
    print "\n";
    exit 0;
}


