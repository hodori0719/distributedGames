#!/usr/bin/perl
#
# Quick script to build a library from just the utility components in
# Mercury/Colyseus (crap in util/). Unfortunately it isn't totally
# independent of stuff in mercury/ so include some of that as well
#
# (Jeff: I wanted to use some util stuff for other projects)
#
# Generates libcolyseus-util-$VERSION.tar.gz.
#
# Options:
#
# -m     compile the library also
#

use strict;
use Getopt::Std;
use vars qw ($opt_m);

our $VERSION = "1.0";

# directories we include everything (*.h, *.cpp, Makefile)...
our @dirs   = ("util");
# ... except these files
our @except = ();
# some extra files to include
our @extra  = ("mercury/Makefile", "mercury/common.h", "mercury/IPEndPoint.h",
	       "mercury/IPEndPoint.cpp", "mercury/AutoSerializer.h",
	       "mercury/Packet.h", "mercury/Packet.cpp", 
	       "mercury/NetworkLayer.h", "mercury/ID_common.h", 
	       "mercury/ID.cpp", "mercury/ID.h",
	       "magic.h", "toprules.make", "botrules.make");

getopts("m");

my @files;
foreach my $d (@dirs) {
    push @files, glob("$d/*.{cpp,h}"), glob("$d/Makefile");
}

push @files, @extra;

system("mkdir -p colyseus-util") && die "can't mkdir";

my %except = map { $_ => 1 } @except;
my %dirs;

foreach my $f (@files) {
    next if $except{$f};
    my ($d) = ($f =~ m|^(.*)/[^/]+$|);
    if ($d && ! -d "colyseus-util/$d") {
	system("mkdir -p colyseus-util/$d") && die "can't mkdir $d";
    }
    if ($d) {
	$dirs{$d} = 1;
    }
    system("cp -f $f colyseus-util/$d") && die "can't copy $f";
}

@dirs = keys %dirs;

chdir "colyseus-util" || die "can't chdir";

open (MAKEFILE, ">Makefile") || die "can't create Makefile";
print MAKEFILE <<EOT;
include toprules.make

SUBDIRS = @dirs
srcs = \$(wildcard \$(patsubst \%,\%/*.cpp,@dirs))
objs = \$(shell ls \$(srcs) | perl -pne 's|(.*)/([^/]*)\\.cpp|\$\$1/.\$\$2.o|')
TARGET = libcolyseus-util.so

all: \$(TARGET)

\$(TARGET): \$(SUBDIRS)
\t\@echo "+ Linking \$\@"
\t\@\$(CPP) -shared \$(OPTFLAGS) -Wl,-soname,\$(TARGET) -o \$\@ \$(objs) -lpthread

clean: \$(SUBDIRS)

include botrules.make
EOT

system("echo 'CFLAGS += -DNO_MERC' >> toprules.make");
system("echo 'CPPFLAGS += -DNO_MERC' >> toprules.make");

if ($opt_m) {
    system("make RELEASE=release") && die "make failed";
}

chdir("..");

system("tar czf libcolyseus-util-$VERSION.tar.gz colyseus-util") &&
    die "can't tar";
system("rm -rf colyseus-util");
