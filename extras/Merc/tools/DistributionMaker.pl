#!/usr/bin/perl
#
# Script to build the colyseus + mercury distribution. 
#

use strict;
use Getopt::Std;
use vars qw ($opt_c $opt_m $opt_t $opt_T $opt_n);

getopts("cmt:T:n");

our $CLEAN   = defined $opt_c;
our $COMPILE = defined $opt_m;
our $TOPDIR  = defined $opt_t ? $opt_t : ".";
our $TMPDIR  = defined $opt_T ? $opt_T : "$ENV{HOME}/tmp";
our $DONT_REMOVE = defined $opt_n;
our $VERSION = GetVersion();
our $ARCHIVE = "mercury-$VERSION.tar.gz";
our $PWD = $ENV{PWD};
#################################################################

our @allfiles = GetDistFiles($TOPDIR);

my $MERCDIR = "$TMPDIR/Merc";
if (!$DONT_REMOVE) { 
    system ("rm -rf $MERCDIR");
}

system ("mkdir -p $MERCDIR") && die "error while 'mkdir -p Merc': $!";
foreach my $f (@allfiles) {
    my ($d, $b) = ($f =~ m|^(.*)/([^/]+)$|);
    if ($d && ! -d "$MERCDIR/$d") {
	system("mkdir -p $MERCDIR/$d") && die "can't mkdir $d";
    }
    if (!-f "$MERCDIR/$d/$b" or (`diff -q $f $MERCDIR/$d/$b 2>/dev/null` ne "")) {
	print STDERR "*** copying $f\n";
	system("cp -f $f $MERCDIR/$d") && die "can't copy $f";
    }
}

# system ("mkdir -p $MERCDIR/build");
# system ("cp -f $MERCDIR/configs/* $MERCDIR/build");
system ("cp -f $MERCDIR/Makefile.conf-default $MERCDIR/Makefile.conf");  # dont use my local modifications;

if ($CLEAN) {
    chdir ("$TMPDIR/Merc");
    system ("make clean");
}
if ($COMPILE) { 
    chdir ("$TMPDIR/Merc");
    system ("sh distmake.sh") && die "make did not succeed: $!";
}
chdir ($TMPDIR);
system("tar czf $ARCHIVE Merc") && die "can't tar";
chdir ($PWD);
# system("rm -rf colyseus-util");

########################################################################

sub GetVersion() {
    my $vstr = `grep g_MercuryVersion Version.cpp`;
    if ($vstr =~ /"([\d\.]+)"/) { 
	return $1;
    }
    else {
	die "no version found in Version.cpp";
    }
}

sub ScanMakefile($) { 
    my $makefile = shift;
    my ($line, $fstr);

    open F, $makefile or do { 
	print STDERR "** $makefile does not exist\n";
	return ();
    };

    while (<F>) { 
	chomp;
	next if !/DIST_FILES\s+=\s+(.*)$/;
	$line = $fstr = $1;
	$fstr =~ s/\\$//;

	while ($line =~ /\\$/) {
	    $line = <F>;
	    chomp $line;
	    last if (!defined $line or $line =~ /^\s*$/);
	    $fstr .= " $line";
	    $fstr =~ s/\\$//;
	}
    }

    my @files = ("Makefile");
    push @files, split (/\s+/, $fstr);
    return @files;
}

sub GetDistFiles($) {
    my $dir = shift;
    my @files = ();
    
    foreach my $df (ScanMakefile("$dir/Makefile")) {
	foreach my $f (glob "$dir/$df") { 
	    if (-d $f) { 
		push @files, GetDistFiles($f);
	    }
	    else {
		push @files, $f;
	    }
	}
    }
    return @files;
}
    
