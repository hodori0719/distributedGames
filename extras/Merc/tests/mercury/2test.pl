#!/usr/bin/perl
#
# Regression test for multiple hub routing...
#

use strict;
require "TestLib.pl";

my $num_nodes = shift;
$num_nodes = 5 if (not defined $num_nodes or $num_nodes eq '');

my %absmax_val = ("x" => 10000, "y" => 12000);
my %attr_index_map = ( "x", 0, "y", 1);

start_bootstrap( 
		"x" => [0, $absmax_val{x}], 
		"y" => [0, $absmax_val{y}] 
		);

my %node = start_nodes($num_nodes, "-l", 15000, "--pubtriggers", "-v 5");
idle_all(\%node, $num_nodes * 20);

foreach my $id (keys %node) {
	$node{$id}->{all_ranges} = get_ranges($node{$id});
	foreach my $attr (keys %{$node{$id}->{all_ranges}}) {
		$node{$id}->{all_ranges}->{$attr}->{min} =~ s/real://;
		$node{$id}->{all_ranges}->{$attr}->{max} =~ s/real://;
		$node{$id}->{all_ranges}->{$attr}->{amin} =~ s/real://;
		$node{$id}->{all_ranges}->{$attr}->{amax} =~ s/real://;
		vinfo("node($id) = $attr: [" . $node{$id}->{all_ranges}->{$attr}->{min} .   "," .  $node{$id}->{all_ranges}->{$attr}->{max} . "]\n");
	}
}

info("##### TEST 0: check ranges are disjoint and exhaustive");
my %sort_ids;
foreach my $attr ("x", "y") {
	$sort_ids{$attr} = [];

	foreach my $id (keys %node) {
		if (exists $node{$id}->{all_ranges}->{$attr}) {
			push (@{$sort_ids{$attr}}, $id);
		}
	}

	@{$sort_ids{$attr}} = 
		sort { $node{$a}->{all_ranges}->{$attr}->{min} <=> $node{$b}->{all_ranges}->{$attr}->{min} } @{$sort_ids{$attr}};

	my $len = scalar @{$sort_ids{$attr}};
	info ("attribute $attr");
	for (my $i = 0; $i < $len; $i++) {
	    print "node-", $sort_ids{$attr}->[$i], " ", $node{$sort_ids{$attr}->[$i]}->{all_ranges}->{$attr}->{min};
	    print " ";
	    print $node{$sort_ids{$attr}->[$i]}->{all_ranges}->{$attr}->{max};
	    print "\n";

		if ($i != $len - 1) { 
			if ($node{$sort_ids{$attr}->[$i]}->{all_ranges}->{$attr}->{max} 
					!= $node{$sort_ids{$attr}->[$i + 1]}->{all_ranges}->{$attr}->{min}) {
				error("node ranges for $attr do not align");
				fail();
			}
		}
		else {
			if ($node{$sort_ids{$attr}->[$i]}->{all_ranges}->{$attr}->{max} != $absmax_val{$attr}) {
				error("node ranges for $attr do not align");
				fail();
			}
		}
	}
}
info("##### passed TEST 0");

my $guid;
info("##### TEST 1: publication routing (without subs)");
srand(42);
sub belongs_to {
	my $attr = shift;

	foreach my $id (@{$sort_ids{$attr}}) {
		if ($node{$id}->{all_ranges}->{$attr}->{min} <= $_[0] and 
				$node{$id}->{all_ranges}->{$attr}->{max} > $_[0]) {
			return $id;
		}
	}
	error("belongs_to didn't find a node for value: $_[0]"); fail();
}

=start
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
	for (my $i = 0; $i < $num_nodes; $i++) {
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
		my $left = get_distance($node{$sort_ids[($start_ind + $num_nodes - 1) % $num_nodes]}->{range}->{min}, $val);
		my $right = get_distance($node{$sort_ids[($start_ind + 1) % $num_nodes]}->{range}->{min}, $val);

		if ($left <= $right) {     # ComputeNextHop prefers the predecessor over the successor;
			for (my $i = $start_ind; $i != $end_ind; $i = ($i + $num_nodes - 1) % $num_nodes) {
				push @expected, $sort_ids[$i];
			}
			push @expected, $end;
		}
		else {
			for (my $i = $start_ind; $i != $end_ind; $i = ($i + 1) % $num_nodes) {
				push @expected, $sort_ids[$i];
			}
			push @expected, $end;
		}
	}

	return @expected;
}

=cut
sub get_substorage {
	my $attr = shift;
	my $sub_hash_ref = shift;
	my @expected = ();

	my $sub_ref = $sub_hash_ref->{$attr};

	foreach my $id (@{$sort_ids{$attr}}) {
		if ($sub_ref->[0] eq ">") {
			if ($node{$id}->{all_ranges}->{$attr}->{max} >  $sub_ref->[1]) {
				push @expected, $id;
			}
		}
		elsif ($sub_ref->[0] eq "<") {
			if ($node{$id}->{all_ranges}->{$attr}->{min} <  $sub_ref->[1]) {
				push @expected, $id;
			}
		}
		else {
		    my @range = ($node{$id}->{all_ranges}->{$attr}->{min}, $node{$id}->{all_ranges}->{$attr}->{max});
		    if (($sub_ref->[0] >= $range[0] and $sub_ref->[0] < $range[1]) or
			    ($range[0] >= $sub_ref->[0] and $range[0] <= $sub_ref->[1])) 
		    {
			push @expected, $id;
		    }
		}
	}
	return @expected;
}

my $test_pubs = [];
my $ref;

# 0 => "x", 1 => "y"
push @$test_pubs, { "x" => int (rand($absmax_val{x})) };
push @$test_pubs, { "y" => int (rand($absmax_val{x})) };
push @$test_pubs, { "x" => int (rand($absmax_val{x})) };
push @$test_pubs, { "y" => int (rand($absmax_val{y})) };
push @$test_pubs, { "x" => int (rand($absmax_val{x})), "y" => int (rand($absmax_val{y})) };
push @$test_pubs, { "x" => int (rand($absmax_val{x})), "y" => int (rand($absmax_val{y})) };

foreach my $pub (@$test_pubs) {
	my %expected = ();
	foreach my $attr (keys %$pub) {
		my $val = $pub->{$attr}; $val =~ s/f://;
		$expected{$attr} = belongs_to($attr, $val);
		
	}

	my $n   = 1 + int(rand($num_nodes));
	
	my @fp = ();
	foreach my $attr (keys %$pub) {
	    push @fp, ($attr_index_map{$attr}, $pub->{$attr});
	}
	$guid = publish($node{$n}, @fp);
	idle_all(\%node, $num_nodes * 2);

	foreach my $attr (keys %$pub) {
		my @seen = check_pubstorage($attr, $guid, \%node);
		my @expect = ($expected{$attr});

		if (!check_equal(\@seen, \@expect)) {
			local $" = "-";
			my @arr;
			error("publication route (check for attr: \"$attr\") unexpected [ @seen ] versus [ @expect ] for '@{[%$pub]}' starting from $n");
			fail();
		}
	}
	{
		local $" = ":";
		vinfo("### pub @{[%$pub]} routed successfully from node($n)");
	}
}
vinfo("##### passed TEST 1");

info("##### TEST 2: subscription storage");
my $test_subs = [];
my $ref;
push @$test_subs, { "x" => [ 3000, $absmax_val {x} ] }; 
push @$test_subs, { "y" => [ 0, 7300 ] };
push @$test_subs, { "x" => [ 1000, 5000 ] };
push @$test_subs, { "y" => [ 3000, 5969 ] };
push @$test_subs, { "x" => [ 1000, 5000 ],  "y" => [ 3000, 5969 ] };
push @$test_subs, { "x" => [ 1000, $absmax_val {x} ], "y" => [ 0, 4000 ] };

sub create_sub {
	my $hooky = shift;
	my @s = ();
	
	foreach my $attr (keys %$hooky) {
		my $sub = $hooky->{$attr};
=start		
		if ($sub->[0] eq ">") { 
			@s = ($attr, ">", "f:" . $sub->[1]);
		}
		elsif ($sub->[0] eq "<") { 
			@s = ($attr, "<", "f:" . $sub->[1]);
		}
		else {
			@s = ($attr, ">", "f:" . $sub->[0], $attr, "<", "f:" . $sub->[1]);
		}
=cut
                push @s, ($attr_index_map{$attr}, $sub->[0], $sub->[1]);
	}
	return @s;
}

foreach my $sub (@$test_subs)
{
	my $n = 1 + int(rand($num_nodes));
	my @s = create_sub($sub);

	{
	    local $" = "--";
	    vinfo( "sub=@s\n");
	}
	$guid = subscribe($node{$n}, @s);

	local $" = ",";
	vinfo("* node($n) sending sub @s (guid=$guid)");

	idle_all(\%node, $num_nodes * 2);

	my $hubs_seen = 0;
	foreach my $attr (keys %$sub) {
		my @seen = check_substorage($attr, $guid, \%node);
		if ($#seen >= 0) {    # saw somewhere; so this MUST be the only place i should see it...;
			$hubs_seen++;
			my @expected = get_substorage($attr, $sub);
			if (!check_equal(\@seen, \@expected)) {
				local $" = ",";
				error("subscription storage unexpected @seen versus @expected for '@s'");
				fail();
			}
		}
	}
	if ($hubs_seen != 1) { 
		error("subscription went into $hubs_seen hubs instead of 1!");
		fail();
	}
	else 
	{
		local $" = ",";
		vinfo("--- sub @s stored successfully after routing from node($n)");
	}
}

vinfo("##### passed TEST 3");
info("SUCCESS");
stop_all();

