#!/usr/bin/perl
#
# (1) Test if pub-triggers get stored
# (2) Test if they are "triggered" when appropriate subscriptions come in
# (3) Test if they expire properly
#

use strict;
require "TestLib.pl";

my $NUM_NODES  = 9;
my $PUBS_PER_NODE = 1;
my $RANGE = 10000;
my $AOE = 20;
my $absmax_val = $RANGE;
my $TTL = 25;       # seconds;

start_bootstrap( "x" => [0, $absmax_val] );
my %node = start_nodes($NUM_NODES, "-l", 25000, "-v 5", "--pubtriggers", "--backpub");
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

info("##### passed TEST0 ");
info("##### TEST: testing storage of triggers");

srand(42);
my $guid;
my $failed;

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
	}
}

# send a bunch of "pubs"; check where they should be stored; 
#         see if they were stored as triggers;

for (my $i = 1; $i <= $NUM_NODES; $i++) {
	for (my $j = 0; $j < $PUBS_PER_NODE; $j++) {
		my $val = $info{$i}->[$j]->{x};
		my $end = belongs_to($val);

		$guid{$i}->[$j] = publish_trigger($TTL, $node{$i}, "x", "$val");
		idle_all(\%node, 2 * $NUM_NODES);
		my @seen = check_triggerstorage("x", $guid{$i}->[$j], $val, \%node);
		my @expected = ($end);

		if (!check_equal(\@seen, \@expected)) {
			local $" = "-";
			error("trigger storage unexpected [ @seen ] versus [ @expected ] for 'x:float:$val'");
			fail();
		}
		else {
			vinfo("### trigger x:float:$val routed and stored successfully");
		}
	}
}

vinfo("### trigger storage OK");
vinfo("### checking if they are triggered properly");

# send a bunch of subs; check if the subscribers get the appropriate triggers;
for (my $i = 1; $i <= $NUM_NODES; $i++) {
	for (my $j = 0; $j < $PUBS_PER_NODE; $j++) {
		subscribe( $node{$i}, 
				("x", $info{$i}->[$j]->{minx},$info{$i}->[$j]->{maxx}));
	}
}

## route all the subs and the triggered pubs
idle_all(\%node, $NUM_NODES * 10);

## Check that each pub made to it all its subscribers
for (my $i = 1; $i <= $NUM_NODES; $i++) {
	for (my $j = 0; $j < $PUBS_PER_NODE; $j++) {
		my $guid = $guid{$i}->[$j];
		my $x    = $info{$i}->[$j]->{x};

		my @seen = check_matchedpub($guid, $x, \%node);
		my %seen;
		map { $seen{$_} = 1 } @seen;

		for (my $i2=1; $i2 <= $NUM_NODES; $i2++) {
			for (my $j2 = 0; $j2 < $PUBS_PER_NODE; $j2++) {
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
vinfo("### pubs got triggered OK");
=start 
vinfo("#### checking if they expire correctly...");

# 
# This test is not correct.. basically because a guy can receive trigger 
# multiple times (because multiple of its subscriptions could have caused a 
# particular trigger to be sent) - so $seen_num{$n} > 1 won't quite work...		

# let triggers expire;
sleep($TTL + 5);

# send a bunch of subs; make sure no-one receives anything.
for (my $i = 1; $i <= $NUM_NODES; $i++) {
	for (my $j = 0; $j < $PUBS_PER_NODE; $j++) {
		subscribe( $node{$i}, 
				("x", ">=", "f:" . $info{$i}->[$j]->{minx}), 
				("x", "<=", "f:" . $info{$i}->[$j]->{maxx}) );
	}
}
idle_all(\%node, $NUM_NODES * 10);

## Check that no pub made to anyone
for (my $i=1; $i <= $NUM_NODES; $i++) {
	for (my $j = 0; $j < $PUBS_PER_NODE; $j++) {
		my $guid = $guid{$i}->[$j];
		my $x    = $info{$i}->[$j]->{x};

		my %seen_num = check_matchedpub_withnum($guid, \%node);
		foreach my $n (keys %seen_num) {
			if ($seen_num{$n} > 1) {
				error("some nodes [$n] got pub $guid");
				$failed = 1;
			}
		}
	}
}

if ($failed) {
	fail();
}
=cut

info("SUCCESS");
stop_all();

sub belongs_to {
	foreach my $id (keys %node) {
		if ($node{$id}->{range}->{min} <= $_[0] and 
				$node{$id}->{range}->{max} > $_[0]) {
			return $id;
		}
	}
	error("belongs_to didn't find a node for value: $_[0]"); fail();
}
