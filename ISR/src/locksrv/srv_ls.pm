#################################################################
# srv_ls.pm - List information about a user's parcels
#################################################################

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

##########
# Prologue
##########
use strict;
use File::stat;
use Term::ANSIColor qw(:constants);
use Socket;
use Sys::Hostname;

####################
# begin main routine
####################

# Globals
our $userdir;
our $username;
our $longvers;
our %config = get_config();

# Variables
my $parcel;
my $isrdir;
my $verbose;

#
# Parse the command line args
#
no strict 'vars';
getopts('hu:p:L:v');

if ($opt_h) {
    usage();
}
if (!$opt_u) {
    $opt_u = $ENV{"USER"};
}
$longvers = $opt_L;  # Use long format using specific number of versions
$username = $opt_u;
$parcel = $opt_p;
$verbose = $opt_v;
$userdir = "$config{content_root}/$username";
use strict 'vars';

$isrdir = (getpwnam($username))[7] . "/.isr/";

if ($parcel) {
    if (-e "$isrdir/$parcel/parcel.cfg") {
	print_one($parcel);
    } else {
	print "Parcel not found.\n";
	exit 1;
    }
} else {
    opendir(DIR, $isrdir)
	or unix_errexit("Could not open directory $isrdir");
    # filter out any dot files
    foreach $parcel (sort grep(!/^[\.]/, readdir(DIR))) {
	# A parcel dir is a directory that contains a parcel.cfg file
	if (-d "$isrdir/$parcel" and -e "$isrdir/$parcel/parcel.cfg") {
	    print_one($parcel);
	}
    }
    closedir(DIR);
}

exit 0;


##################################################
# print_one - print a status line for one parcel #
##################################################

sub print_one {
    my $parcel = shift;
    
    my $unlocked;
    my $logentry;
    my $unused;
    my $action;
    my $client;
    my $date;
    my $user;
    my $state;
    my $version;
    my $memory;
    my $disk;
    my $size;
    my $line;
    my $count;
    my %data;
    
    my @versions;
    
    #
    # Determine the last time this parcel was acquired or released
    #
    $logentry = `isr_runserv lock -u $username -p $parcel -Vc`;
    $unlocked = $?;
    chomp($logentry);
    
    #
    # Determine it's present state
    # 
    if ($unlocked) {
	$state = "released";
    }
    else {
	$state = "acquired";
    }
    
    #
    # Parse the log entry
    #
    ($unused, $date, $action, $user, $unused, $client, $unused) = 
	split('\|', $logentry);
    if ($logentry and 
	(($unlocked and $action ne "released") or
	(!$unlocked and $action ne "acquired")) ) {
	errexit("System error: inconsistent log entry: unlocked=$unlocked action=$action\nlogentry=$logentry");
    }
    
    #
    # We don't need the day of the week on the date
    #
    $date =~ s/^\w+\s//;
    
    #
    # Use only the hostname portion of the client's FQDN
    #
    $client =~ /^([^\.\s]+)\.?/; # at least 1 alphanum char followed by 0 or 1 dots
    $client = $1;                # (i wanted '-' to match also so i changed the regexp -mtoups)
    
    #
    # Print the main output line
    #
    if ($logentry and $unlocked) {
	print GREEN;
	printf("%s %s by %s on %s\n", $parcel, $state, $client, $date);
	print RESET;
    }
    elsif ($logentry and !$unlocked) {
	print RED;
	printf("%s %s by %s on %s\n", $parcel, $state, $client, $date);
	print RESET;
    }
    else {
	print("$parcel has never been checked out.\n");
    }
    
    #
    # Print extra information about the parcel if requested.
    #
    if ($verbose) {
        %data = get_values(get_parcelcfg_path($username, $parcel));
        $memory = "unknown";
        $memory = "$data{'MEM'} MB"
            if exists $data{'MEM'};
        $disk = (($data{'CHUNKSIZE'} >> 10) * $data{'NUMCHUNKS'}) >> 10;
        print "$data{'UUID'}, $data{'VMM'}, memory $memory, disk $disk MB\n";
    }
    
    # 
    # If the user wants to see the available versions of the parcel, print 
    # those too.
    #
    if ($longvers) {
	opendir(DIR, "$userdir/$parcel")
	    or unix_errexit("Could not open directory $userdir/$parcel");
	@versions = reverse sort grep(/^\d+$/, readdir(DIR)); # numbers only
    
	closedir(DIR);
    
	$count = 0;
	foreach $version (@versions) {
	    $count++;
	    if ($count > $longvers) {
		last;
	    }
    
	    if (-e "$userdir/$parcel/$version/keyring.enc") {
		$date = localtime(stat("$userdir/$parcel/$version/keyring.enc")->mtime);
	    }
	    else {
		$date = "[not available]";
	    }
    
	    $line = `du -h -s $userdir/$parcel/$version`;
	    ($size, $unused) = split(" ", $line);
	    printf("  %s %6s  %s\n", $version, $size, $date);
	}
    }
}

############################################
# usage - print help message and terminate #
############################################

sub usage
{
    my $msg = shift;
    my $progname;

    # Strip any path information from the program name
    ($progname = $0) =~ s#.*/##s; 
    
    if ($msg) {
        print "$progname: $msg\n\n";
    }
    
    print "Usage: $progname [-h] [-L <n>] [-u <username>] [-p <parcel>] [-v]\n";
    print "Options:\n";
    print "  -h              Print this message\n";
    print "  -L <n>          List the <n> most recent versions\n";  
    print "  -u <username>   User name (default is $ENV{'USER'})\n";
    print "  -p <parcel>     Parcel name\n";
    print "  -v              Show extra information about each parcel\n";
    print "\n";
    exit 0;
}
