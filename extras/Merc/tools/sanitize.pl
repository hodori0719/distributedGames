#!/usr/bin/perl
use strict;

our $FMT_HDR = <<EOT;
vim: set sw=4 sts=4 ts=8 noet: 
Local Variables:
Mode: c++
c-basic-offset: 4
tab-width: 8
indent-tabs-mode: t
End:
EOT

our $XXX_PAT = "XXX_ERASE_ME_XXX";
our $LICENSE = <<EOT;
Mercury and Colyseus Software Distribution 

Copyright (C) 2004-2005 Ashwin Bharambe (ashu\@cs.cmu.edu)
              2004-2005 Jeffrey Pang    (jeffpang\@cs.cmu.edu)
                   2004 Mukesh Agrawal  (mukesh\@cs.cmu.edu)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA

EOT

my $file = shift @ARGV;
defined $file or exit 1;

modify_file ($file);

#######################################################################

sub modify_file {
    my $file = shift;
    system "cp -f $file $file~";

    open F, $file or die "could not open $file for reading...\n";
    my @lines = <F>;
    close F;

    my ($sl, $el, $ic) = (-1, -1, 0);
    for (my $i = 0; $i < scalar @lines; $i++) {
	my $line = $lines[$i];
	
	if ($line =~ /vim: /) { 
	    $lines[$i] = "$XXX_PAT";
	}
	
	if ($line =~ /Mercury and Colyseus Software Distribution/) { 
	    $sl = $i;
	}
	if ($line =~ /USA/) {
	    $el = $i if ($el < 0);
	}

	if ($line =~ /Local Variables:/) { 
	    $ic = 1;
	}
	if ($line =~ /End:/) { 
	    $lines[$i] = $XXX_PAT;
	    $ic = 0;
	}
	if ($ic) {
	    $lines[$i] = $XXX_PAT;
	}
    }

    if ($sl >= 0) { 	
	splice (@lines, $sl - 1, $el - $sl + 3); # two additional lines for the delimiter lines
    }
	
    my $head = $lines[0];
    chomp $head;

    open G, ">$file" or die "could not open $file for writing: $!";    
    if ($head =~ /^#!/) { 
	print G "$head\n";
	shift @lines;
    }

    my $cc = get_comment_character ($file);
    my $nx = int( 80 / length $cc);
    print G $cc x $nx, "\n";
    foreach my $ll (split (/\n/, $LICENSE)) { 
	print G "$cc $ll\n";
    }
    print G $cc x $nx, "\n";
    foreach my $line (@lines) { 
	next if ($line =~ /$XXX_PAT/);
	print G $line;
    }
	    
    foreach my $line (split (/\n/, $FMT_HDR)) {
	print G "$cc $line\n";
    }
    close G;

    # system "indent -kr -ts4 -ut $file";
}

sub get_comment_character {
    my $file = shift;
    my $head = `head -1 $file`;
    chomp $head;

    if (-x $file) {
	return "#";       # for #! perl and #! sh, the comment char is same
    }
    
    if ($file =~ /\.pl$/ or $file =~ /\.pm$/ or $file =~ /\.py$/) {
	return "#";
    }
    elsif ($file =~ /\.sh$/) { 
	return "#";
    }
    elsif ($file =~ /\.cpp$/ or $file =~ /\.c$/ or $file =~ /\.h$/ or $file =~ /\.cxx$/) {
	return "//";
    }
    elsif ($file =~ /\.tex$/) {
	return "%";
    }
}

