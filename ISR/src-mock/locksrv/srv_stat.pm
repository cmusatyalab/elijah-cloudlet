#################################################################
# srv_stat.pm - Return file metadata (from stat)
#################################################################

#
#                     Internet Suspend/Resume (R)
#           A system for capture and transport of PC state
#
#              Copyright (c) 2002-2004, Intel Corporation
#         Copyright (c) 2004-2009, Carnegie Mellon University
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
use Socket;
use Sys::Hostname;
use sigtrap qw(die normal-signals);

####################
# begin main routine
####################

#
# Variables
#
my $filepath;
my $item;
my $metadata;
my $blobtool = LIBDIR . "/blobtool";
my %config = get_config();

#
# Parse the command line args
#
no strict 'vars';
getopts('hf:');

if ($opt_h) {
    usage();
}
if (!$opt_f) {
    usage("Missing file path (-f)");
}
$filepath = "$config{content_root}/$opt_f";
use strict 'vars';

#
# Make sure the file exists
#
(-e $filepath)
    or errexit("$filepath does not exist.");

#
# Return the stat metadata as a list of key-value pairs
#
$metadata = stat($filepath);
print "DEV=", $metadata->dev, "\n";
print "INO=", $metadata->ino, "\n";
print "SIZE=", $metadata->size, "\n";
print "MODE=", $metadata->mode, "\n";
print "NLINK=", $metadata->nlink, "\n";
print "UID=", $metadata->uid, "\n";
print "GID=", $metadata->gid, "\n";
print "RDEV=", $metadata->rdev, "\n";
print "SIZE=", $metadata->size, "\n";
print "ATIME=", $metadata->atime, "\n";
print "MTIME=", $metadata->mtime, "\n";
print "CTIME=", $metadata->ctime, "\n";
print "BLKSSIZE=", $metadata->blksize, "\n";
print "BLOCKS=", $metadata->blocks, "\n";
print "SHA1=", `$blobtool -hi $filepath`;
#
# Clean up and exit
#
exit 0;


##################
# end main routine
##################


#
# usage - print help message and terminate
#
sub usage {
    my $msg = shift;
    my $progname;

    # Strip any path information from the program name
    ($progname = $0) =~ s#.*/##s; 

    if ($msg) {
        print "$progname: $msg\n";
    }

    print "Usage: $progname [-hV] -f <path>\n";
    print "Options:\n";
    print "  -h        Print this message\n";
    print "  -f <path> File path (relative to $config{content_root})\n";
    print "\n";
    exit 0;
}
