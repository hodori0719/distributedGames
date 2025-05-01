#!/usr/bin/perl
#
# Test to see if caching is working reasonably...
# At this point, this aint an automatic test. We just output a bunch of pub 
# routes and see if that matches our intuition. (Also, output the avg#hops 
# taken by a pub...)		
#

use strict;
require "TestLib.pl";

my $NUM_NODES  = 50;
my $absmax_val = 10000;

start_bootstrap( "x" => [0, $absmax_val] );
my %node = start_nodes($NUM_NODES, "-l", 15000, "--cache", "--cachesize 3"); #, "-v 20 -D MercuryRouter,RealNet,Message");
# my %node = start_nodes($NUM_NODES, "-l", 15000); #, "-v 20 -D MercuryRouter,RealNet,Message");
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
		if ($node{$sort_ids[$i]}->{range}->{max} ne $node{$sort_ids[$i + 1]}->{range}->{min}) {
			error("node ranges do not align");
			fail();
		}
	}
	else {
		if ($node{$sort_ids[$i]}->{range}->{max} ne "$absmax_val") {
			error("node ranges do not align");
			fail();
		}
	}
}

my $guid;

# send a bunch of publications from a known guy. to a known value. 
# all except the first one should take small number of routes...

info("##### TEST 2: simple cache check ");

srand(42);
my $begin = 1;
my $av = 0;
my $npubs = 20;
for (my $j = 0; $j  < $npubs; $j++) { 
	my $val = int (rand($absmax_val));   # the value to route to...
#		my $end = belongs_to($val);
#	my $end_index = get_sortid_index($end); 
#	my $begin_index = ($end_index + ($NUM_NODES / 2 )) % $NUM_NODES;  # try to pick the longest route!
#		my $begin = $sort_ids[$begin_index];

	vinfo("routing pub [x:$val] from $begin");
#	for (my $i = 0; $i < 5; $i++) {
		$guid = publish($node{$begin}, "x", $val);
		idle_all(\%node, $NUM_NODES * 2);

		my @seen = check_pub($guid, \%node); vinfo("hop len: $#seen");
		$av += $#seen;
#	vinfo("******* pub-[$i] route: @seen");
#	}
}
vinfo ("average hop length: ". ($av/$npubs));

sub belongs_to {
	foreach my $id (keys %node) {
		if ($node{$id}->{range}->{min} <= $_[0] and 
				$node{$id}->{range}->{max} > $_[0]) {
			return $id;
		}
	}
	error("belongs_to didn't find a node for value: $_[0]"); fail();
}

sub get_distance {
	my $d = abs($_[0] - $_[1]);
	if ($d < ($absmax_val - $d)) {
		return $d;
	}
	else {
		return $absmax_val - $d;
	}
}

sub get_sortid_index {
	for (my $i = 0; $i < $NUM_NODES; $i++) {
		if ($sort_ids[$i] == $_[0]) {
			return $i;
		}
	}
	return -1;
}

sub get_pubroute {
	my ($val, $start) = @_;
	my $end = belongs_to($val);
	my @expected = ();

	if ($start == $end) {
		push @expected, $start;
	}
	else {
		my ($start_ind, $end_ind) = (get_sortid_index($start), get_sortid_index($end));
		if ($start_ind == -1 || $end_ind == -1) { error("internal dysfunction in script; start: $start, end: $end, val: $val "); fail(); }
		my $left = get_distance($node{$sort_ids[($start_ind + $NUM_NODES - 1) % $NUM_NODES]}->{range}->{min}, $val);
		my $right = get_distance($node{$sort_ids[($start_ind + 1) % $NUM_NODES]}->{range}->{min}, $val);

		if ($left <= $right) {     # ComputeNextHop prefers the predecessor over the successor;
			for (my $i = $start_ind; $i != $end_ind; $i = ($i + $NUM_NODES - 1) % $NUM_NODES) {
				push @expected, $sort_ids[$i];
			}
			push @expected, $end;
		}
		else {
			for (my $i = $start_ind; $i != $end_ind; $i = ($i + 1) % $NUM_NODES) {
				push @expected, $sort_ids[$i];
			}
			push @expected, $end;
		}
	}

	return @expected;
}

sub get_substorage {
	my $sub_ref = shift;
	my @expected = ();

	foreach my $id (keys %node) {
		if ($sub_ref->[0] eq ">") {
			if ($node{$id}->{range}->{max} >=  $sub_ref->[1]) {
				push @expected, $id;
			}
		}
		elsif ($sub_ref->[0] eq "<") {
			if ($node{$id}->{range}->{min} <=  $sub_ref->[1]) {
				push @expected, $id;
			}
		}
		else {
			if ($node{$id}->{range}->{max} >= $sub_ref->[0] and
					$node{$id}->{range}->{min} <= $sub_ref->[1]) {
				push @expected, $id;
			}
		}
	}
	return @expected;
}

info("SUCCESS");
stop_all();

sub check_equal {
	my ($aref_one, $aref_other) = @_;

	return 0 if (scalar @$aref_one != scalar @$aref_other);
	@$aref_one = sort { $a <=> $b } @$aref_one;
	@$aref_other = sort { $a <=> $b } @$aref_other;

	for (my $i = 0; $i < scalar @$aref_one; $i++) {
		return 0 if ($aref_one->[$i] != $aref_other->[$i]);
	}
	return 1;
}
