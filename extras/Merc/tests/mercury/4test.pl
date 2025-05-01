#!/usr/bin/perl
#
# (1) Test that softstate subscriptions expire properly
#

use strict;
require "TestLib.pl";


my $num_nodes = shift;
$num_nodes = 10 if (not defined $num_nodes or $num_nodes eq '');

my $PUBS_PER_NODE = 1;
my $RANGE = 10000;
my $AOE = 20;
my $absmax_val = $RANGE;
my $TTL = 30;

#
# XXX WILL ONLY WORK WITH ONE ATTRIBUTE

start_bootstrap( "x" => [0, $absmax_val] );
my %node = start_nodes($NUM_NODES, "-l", $TTL * 1000, "-v 9", "--pubtriggers", "--backpub");
idle_all(\%node, $NUM_NODES * 20);

# all the time and check things accordingly.
foreach my $id (keys %node) {
    my $all_ranges = get_ranges($node{$id});
    
    $node{$id}->{range} = $all_ranges->{"x"};
    $node{$id}->{range}->{min} =~ s/real://;
    $node{$id}->{range}->{max} =~ s/real://;
    $node{$id}->{range}->{amin} =~ s/real://;
    $node{$id}->{range}->{amax} =~ s/real://;
    vinfo("node($id) = [" . $node{$id}->{range}->{min} .   "," .  $node{$id}->{range}->{max} . "]\n");
}

info("##### TEST 0: check ranges are disjoint and exhaustive");
my @sort_ids = sort { $node{$a}->{range}->{min} <=> $node{$b}->{range}->{min} } (keys %node);
for (my $i = 0; $i < $NUM_NODES; $i++) {
    if ($i != $NUM_NODES - 1) { 
	if ($node{$sort_ids[$i]}->{range}->{max} != $node{$sort_ids[$i + 1]}->{range}->{min}) {
	    error("node ranges do not align");
	    fail();
	}
    }
    else {
	if ($node{$sort_ids[$i]}->{range}->{max} != "$absmax_val") {
	    error("node ranges do not align");
	    fail();
	}
    }
}

my $guid;

info("##### TEST: testing soft subs");

## Set up GUID array
my %guid;
my %info;
for (my $i=1; $i<=$NUM_NODES; $i++) {
    $guid{$i} = [];
    $info{$i} = [];
}

## Have each node subscribe to several ranges
for (my $i=1; $i<=$NUM_NODES; $i++) {
    for (my $j = 0; $j<$PUBS_PER_NODE; $j++) {
	$info{$i}->[$j] = {};

	$info{$i}->[$j]->{x} = sprintf "%d", int (rand($RANGE));
    
	$info{$i}->[$j]->{minx} = 
	    sprintf "%d", int (max(0, $info{$i}->[$j]->{x} - $AOE/2));
	$info{$i}->[$j]->{maxx} = 
	    sprintf "%d", int (min($RANGE, $info{$i}->[$j]->{x} + $AOE/2));
    
	subscribe( $node{$i}, 
		   ("x", $info{$i}->[$j]->{minx}, $info{$i}->[$j]->{maxx}) );
    }
}

## route all the subs
idle_all(\%node, $NUM_NODES * 10);

## send pubs
for (my $i=1; $i<=$NUM_NODES; $i++) {
    for (my $j = 0; $j<$PUBS_PER_NODE; $j++) {
		$guid{$i}->[$j] = publish($node{$i}, "x", $info{$i}->[$j]->{x});
	}
}

## route pubs to matchers
idle_all(\%node, $NUM_NODES * 10);

my $failed;

## Check that each pub made to it all its subscribers
for (my $i=1; $i<=$NUM_NODES; $i++) {
    for (my $j = 0; $j<$PUBS_PER_NODE; $j++) {
	my $guid = $guid{$i}->[$j];
	my $x    = $info{$i}->[$j]->{x};

	my @seen = check_matchedpub($guid, $x, \%node);
	my %seen;
	map { $seen{$_} = 1 } @seen;

	for (my $i2=1; $i2<=$NUM_NODES; $i2++) {
	    for (my $j2 = 0; $j2<$PUBS_PER_NODE; $j2++) {
		my $info = $info{$i2}->[$j2];

		if ($x >= $info->{minx} && $x <= $info->{maxx}) {

		    if (! exists $seen{$i2}) {
			error("$i2 didn't get pub for $guid should have: x=" . 
				$x . " range=[" . $info->{minx} . "," . 
				$info->{maxx} . "]");
			$failed = 1;
		    } else {
			info("$i2 correctly got matched pub for $guid");
		    }
		} else {

#if (exists $seen{$i2}) {
#   error("$i2 got pub for $guid but should not have: x=" .
#         $x . " range=[" . $info->{minx} . "," . 
#         $info->{maxx} . "]");
#   $failed = 1;
#}

		}
	    }
	}
    }
}

if ($failed) {
    fail();
}

sleep($TTL);

# let subs expire
idle_all(\%node, $NUM_NODES * 2);

## send pubs
for (my $i=1; $i<=$NUM_NODES; $i++) {
    for (my $j = 0; $j<$PUBS_PER_NODE; $j++) {
	$guid{$i}->[$j] = publish($node{$i}, "x", $info{$i}->[$j]->{x});
    }
}

## route pubs to matchers
idle_all(\%node, $NUM_NODES * 10);

## Check that no pub made to anyone
for (my $i=1; $i<=$NUM_NODES; $i++) {
    for (my $j = 0; $j<$PUBS_PER_NODE; $j++) {
	my $guid = $guid{$i}->[$j];
	my $x    = $info{$i}->[$j]->{x};

	my @seen = check_matchedpub($guid, $x, \%node);

	if (@seen) {
	    error("some nodes got pub $guid: @seen");
	    $failed = 1;
	}
    }
}

if ($failed) {
    fail();
}

info("SUCCESS");
stop_all();
