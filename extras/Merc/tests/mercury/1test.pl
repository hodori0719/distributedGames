#!/usr/bin/perl
#
# Simple regression test.
#
# (0) Test that the mercury nodes set up routing tables correctly.
# (1) Test that a publication gets routed correctly. 
# (2) Test that a subscription gets routed correctly.
# (3) Test the publications get matched correctly.
#

use strict;
require "TestLib.pl";

my $NUM_NODES = shift;
$NUM_NODES = 10 if (not defined $NUM_NODES or $NUM_NODES eq '');
my $absmax_val = 10000;

start_bootstrap( "x" => [0, $absmax_val] );
my %node = start_nodes($NUM_NODES, "-l", 15000, "--pubtriggers", "--measurement"); #, "-v 20 -D MercuryRouter,RealNet,Message");
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

	if ($node{$id}->{range}->{min} eq "") { fail (); }
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

info("##### TEST 1: dumping routing tables and peers");
foreach my $id (keys %node) {
	dump_rt($node{$id});
}

info("##### TEST 2: publication routing (without subs)");
srand(42);
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

for (my $i = 0; $i < 3; $i++) {
	my $val = int(rand($absmax_val));
	my $end = belongs_to($val);
	my $n   = 1 + int(rand($NUM_NODES));

	$guid = publish($node{$n}, "x", "$val");
	idle_all(\%node, $NUM_NODES * 2);

	my @seen = check_pubstorage("x", $guid, \%node);
	my @expected = ($end);

	if (!check_equal(\@seen, \@expected)) {
		local $" = "-";
		error("publication route unexpected [ @seen ] versus [ @expected ] for 'x:float:$val' starting from $n");
		fail();
	}
	else {
		vinfo("### pub x:float:$val routed successfully from node($n)");
	}
}

info("##### TEST 2: subscription storage");
foreach my $sub ( 
		[ 1000, 6000 ],
		[ 3000, 10000 ],
		[ 0, 7400 ])
{
	my $n = 1 + int(rand($NUM_NODES));
	my @s = "x";
	push @s , @$sub;

	vinfo ("node:$n subscription=@s");

	$guid = subscribe($node{$n}, @s);
	idle_all(\%node, $NUM_NODES * 2);

	my @seen = check_substorage("x", $guid, \%node);
	my @expected = get_substorage($sub);
	if (!check_equal(\@seen, \@expected)) {
		local $" = ",";
		error("subscription storage unexpected @seen versus @expected for '@s'");
		fail();
	}
	else {
		$" = ",";
		vinfo("--- sub @s stored successfully after routing from node($n)");
	}
}

info("SUCCESS");
stop_all();
