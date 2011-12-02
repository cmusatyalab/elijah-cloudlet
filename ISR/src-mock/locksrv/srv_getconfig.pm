###################################################################
# srv_getconfig.pm - Fetch the parcel.cfg file from server
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
use File::Basename;
use Sys::Hostname;

####################
# Begin main routine
####################
my $parcelpath;
my $parceldir;
my $parcelname;
my $username;
my $configfile;

#
# Parse the command line args
#
no strict 'vars';
getopts('hp:');

if ($opt_h) {
    usage();
}

if (!$opt_p) {
    usage("Missing parcel name (-p)");
}
$parcelname = $opt_p;
use strict 'vars';

#
# Set some variables that we'll need later
#
$configfile = get_parcelcfg_path($ENV{"USER"}, $parcelname);

#
# Return the config file to the caller via stdout
#
open(INFILE, $configfile) 
    or unix_errexit("Unable to open $configfile.");
while (<INFILE>) {
    print $_;
}
close (INFILE) 
    or unix_errexit("Unable to close $configfile.");

exit 0;

##################
# End main routine
##################

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

    print "Usage: $progname [-hV] -p <parcel>\n";
    print "Options:\n";
    print "  -h    Print this message\n";
    print "  -p    Parcel name\n";    
    print "\n";
    exit 0;
}


