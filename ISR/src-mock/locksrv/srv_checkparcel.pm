###################################################################
# srv_checkparcel.pm - check a parcel for consistency
###################################################################

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

###################
# standard prologue
###################
use strict;
use POSIX;
use Cwd;

####################
# begin main routine
####################

# 
# Declare local variables
#

# These are set by command line arguments
my $precommit;
my $contentcheck;
my $verbose;
my $username;
my $parcelname;
my $currver;

# Important variables
my $keyroot;
my $parceldir;
my $lastver;
my $currdir;
my $currkeyring;
my $currkeyring_enc;
my $parcelcfg;
my $predver;
my $preddir;
my $predkeyring;
my $predkeyring_enc;
my $errors;
my $numdirs;
my $totalchunks;
my $chunksperdir;
my $blobtool = LIBDIR . "/blobtool";
my %config = get_config();

# Various temporary variables
my $numchunks;
my $expectchunks;
my $tag;
my $dirname;
my $dirpath;
my $i;
my $chunk;
my $chunkpath;
my $index;
my $dirnum;
my $filenum;
my $dir;
my $filename;
my $rh;
my $fd;

my @files;

# Arrays and list
my @tags;    # array of keyring tags
my %keydiff; # one hash key for every keyring entry that differs

#
# Parse the command line args
#
no strict 'vars';
getopts('Vhcs:u:p:v:');

if ($opt_h) {
    usage();
}
if (!$opt_p) {
    usage("Missing parcel name (-p)");
}
$username = $opt_u;
$username = $ENV{"USER"} if !$username;
$parcelname = $opt_p;
$parceldir = "$config{content_root}/$username/$parcelname";
$currver = $opt_v;
$verbose = $opt_V;
$contentcheck = $opt_c;
$precommit = $opt_s;

use strict 'vars';

#
# Make sure the parcel directory exists
#
(-e $parceldir)
    or errexit("$parceldir does not exist");

#
# Determine the last version that was checked in
# 
opendir(DIR, $parceldir)
    or unix_errexit("Could not open directory $parceldir");
@files = reverse sort grep(/^\d+$/, readdir(DIR));
closedir(DIR);
$lastver = int($files[0]);

#
# Make sure that there is a last link and that it points
# to the most recent version
#
chdir("$parceldir/last")
    or errexit("Parcel misconfigured: missing a last link.");
$dir = cwd();
$dir =~ /.+\/(\d+)$/; # extract the filename from the path
$filename = $1;
if ($filename ne sprintf("%06d", $lastver)) {
    errexit("Parcel misconfigured: last link does not point to last.");
}

#
# Make sure there is a cache directory
#
-d "$parceldir/cache"
    or errexit("Parcel misconfigured: $parceldir/cache does not exist");

# 
# Set the current version (default is the most recent)
#
if (!defined($currver)) {
    $currver = $lastver;
}
if ($currver < 1) {
    errexit("Current version must be greater than 0.");
}

#
# Set the key variables
#
$errors = 0;

# Variables for the current version
$currdir = "$parceldir/" . sprintf("%06d", $currver);
$currkeyring_enc = "$currdir/keyring.enc";
$currkeyring = mktempfile();
$parcelcfg = get_parcelcfg_path($username, $parcelname);

# Variables for the predecessor version (if any)
if ($precommit) {
    $predver = "checkin";
    $preddir = "$parceldir/cache/$precommit";
    -d $preddir
        or errexit("Cache directory $preddir does not exist");
} else {
    $predver = $currver - 1; 
    $preddir = "$parceldir/" . sprintf("%06d", $predver);
}
$predkeyring_enc = "$preddir/keyring.enc";
$predkeyring = mktempfile();

#
# Make sure that the other files we will need are available
#
(-e $currdir)
    or errexit("$currdir does not exist.");
(-e "$currkeyring_enc")
    or errexit("$currkeyring_enc does not exist.");

if (-e $preddir) {
    (-e "$predkeyring_enc")
	or errexit("$predkeyring_enc does not exist.");
}
 
# 
# Bail out if there's nothing to do
#
if (! -e $preddir && $currver != $lastver && !$contentcheck) {
    errexit("Version $currver has no predecessors, is not last, and -c was not specified.  Nothing to do.");
}

#
# Decrypt the current and predecessor keyrings
#
if (-e $preddir) {
    print "Checking versions $currver and $predver.\n"
	if $verbose;
}
else {
    print "Checking version $currver.\n"
	if $verbose;
}

$keyroot = get_value($parcelcfg, "KEYROOT");

($rh, $fd) = keyroot_pipe($keyroot);
system("$blobtool -ed -i $currkeyring_enc -o $currkeyring -k $fd") == 0
    or system_errexit("Unable to decode $currkeyring_enc");

if (-e $preddir) {
    ($rh, $fd) = keyroot_pipe($keyroot);
    system("$blobtool -ed -i $predkeyring_enc -o $predkeyring -k $fd") == 0
	or system_errexit("Unable to decode $predkeyring_enc");
}

#
# Check that current keyring size is consistent with parcel.cfg
#
open(TAGS, "-|", LIBDIR . "/query", $currkeyring, "SELECT tag FROM keys ORDER BY chunk ASC")
    or system_errexit("Unable to read tags from $currkeyring");

@tags = ();
while ($tag = <TAGS>) {
    chomp($tag);
    push @tags, $tag;
}

close TAGS;
$? == 0
    or unix_errexit("$currkeyring query failed");

# There better be a keyring entry for each block
$totalchunks = get_value($parcelcfg, "NUMCHUNKS");
if (@tags != $totalchunks) {
    err("Version $currver keyring has " . scalar(@tags) . " chunks while the disk has $totalchunks.");
    $errors++;
}

$chunksperdir = get_value($parcelcfg, "CHUNKSPERDIR");
$numdirs = ceil($totalchunks / $chunksperdir);

#
# Check the current and predecessor keyrings for relative consistency
#
if (-e $preddir) {
    print "Comparing keyrings $currver and $predver for differences...\n"
	if $verbose;
    open(DIFFS, "-|", LIBDIR . "/query", $currkeyring, "-a", "pred:$predkeyring", "SELECT main.keys.chunk FROM main.keys JOIN pred.keys ON main.keys.chunk == pred.keys.chunk WHERE main.keys.tag != pred.keys.tag")
	or system_errexit("Unable to compare $currkeyring and $predkeyring");

    while ($chunk = <DIFFS>) {
	chomp($chunk);
	$keydiff{$chunk} = 1;
    }

    close DIFFS;
    $? == 0
	or unix_errexit("Keyring comparison failed");
    
    # 
    # Check that the blocks in the predecessor correspond to the differing
    # entries in the keyring
    #
    print "Checking version $predver against its keyring...\n"
    	if $verbose;
    for ($i = 0; $i < $totalchunks; $i++) {
	$dirnum = floor($i / $chunksperdir);
	$filenum = $i % $chunksperdir;
	$chunk = sprintf("%04d/%04d", $dirnum, $filenum);
	$chunkpath = "$preddir/hdk/$chunk";
	if (-e $chunkpath && !defined($keydiff{$i})) {
	    print "Error: [$i] file $chunk exists, but entries are the same.\n";
	    $errors++;
	} elsif (! -e $chunkpath && defined($keydiff{$i})) {
	    print "Error: [$i] file $chunk does not exist, but entries differ.\n";
	    $errors++;
	}
    }
}

#
# If the current directory is also the most recent directory, then do a 
# simple consistency check to ensure that it is fully populated.
#
if ($currver == $lastver) {
    print "Scanning $numdirs version $currver dirs ($chunksperdir chunks/dir, $totalchunks chunks) for completeness...\n"
	if $verbose;
    
    # Iterate through the complete list of possible subdirectories
    $expectchunks = $chunksperdir;
    for ($i = 0; $i < $numdirs; $i++) {
	$dirname = sprintf("%04d", $i);
	$dirpath = "$currdir/hdk/$dirname"; 
	if ($i == $numdirs - 1) {
	    $expectchunks = $totalchunks % $chunksperdir;
	    $expectchunks = $chunksperdir
	        if $expectchunks == 0;
	}

	# Check the directory contents
	if (opendir(DIR, $dirpath)) {
	    @files = grep(!/^[\._]/, readdir(DIR)); # filter out "." and ".."
	    closedir(DIR);

	    # Count the number of files in the subdirectory
	    $numchunks = scalar(@files);
	    if ($numchunks != $expectchunks) {
		print "Error: Directory $dirname has $numchunks blocks. Expected $expectchunks.\n";
		$errors++;
	    }
	} else {
	    print "Error: Directory $dirname does not exist.\n";
	    $errors++;
	}
    }
}    

#
# If the user has asked for a content consistency check, then verify that
# each encrypted disk chunk has a valid key
# 
if ($contentcheck) {
    print "Performing content consistency check...\n"
	if $verbose;

    # Iterate through the complete list of possible subdirectories
    for ($i = 0; $i < $numdirs; $i++) {
	$dirname = sprintf("%04d", $i);
	$dirpath = "$currdir/hdk/$dirname"; 

	# If the directory exists, then check its chunks
	if (opendir(DIR, $dirpath)) {
	    print "$dirname "
		if $verbose;

	    @files = grep(!/^[\._]/, readdir(DIR)); # filter out "." and ".."
	    closedir(DIR);

	    # Check each chunk in the directory
	    foreach $chunk (sort @files) {
		$index = $i*$chunksperdir + $chunk;
		if ($index <= scalar(@tags)) {
		    # Check that the keyring entry tag is correct
		    $tag = `$blobtool -hi $dirpath/$chunk`;
		    chomp($tag);
		    if (lc($tag) ne lc($tags[$index])) {
			if ($verbose) {
			    print "Error: [$index] Computed tag (", uc($tag), ") <> keyring tag (", uc($tags[$index]), ").\n";
			}
			else {
			    print("Error: [$index] Computed tag <> keyring tag.\n");
			}
			$errors++;
		    }
		}
	    }
	}
    }
    print "\n"
	if $verbose;
}

#
# Print a final status message
#
if ($errors == 0) {
    print "Success: Parcel appears to be consistent.\n"
	if $verbose;
    exit 0;
} 
else {
    print "Error: Parcel appears to be inconsistent.\n";
    exit 1;
}

##################
# end main routine
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
        print "Error: $msg\n\n";
    }

    print "Usage: $progname [-hcV] [-u <username>] -p <parcel> [-s <nonce>] [-v <version>]\n";
    print "Options:\n";
    print "  -c           Perform content consistency check\n";
    print "  -h           Print this message\n";
    print "  -u <user>    User for this parcel (default is $ENV{'USER'})\n";
    print "  -p <parcel>  Parcel name\n";    
    print "  -s <nonce>   Run pre-commit check against cache with the given nonce\n";
    print "  -v <ver>     Parcel version to check (default is last)\n";
    print "  -V           Be verbose\n";
    print "\n";
    exit 0;
}
