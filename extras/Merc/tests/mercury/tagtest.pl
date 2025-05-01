#!/usr/bin/perl
#
# Simple regression test.
#
# (1) Test that pubs are correctly matched
#

use strict;
require "TestLib.pl";

my $NUM_NODES  = 6;
my $PUBS_PER_NODE = 1;
my $RANGE = 1000;
my $AOE = 20;
my $absmax_val = $RANGE;

start_bootstrap( "x" => [0, $absmax_val] );
my %node = start_nodes($NUM_NODES, "-l", 25000, "-v 9", "--pubtriggers", "--backpub");
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

info("##### TEST: testing pub matching with tags");

## Set up GUID array
my %guid;
my %info;
for (my $i=1; $i<=$NUM_NODES; $i++) {
    $guid{$i} = [];
    $info{$i} = [];
}

my $tagid = 0;
## Have each node subscribe to several ranges
for (my $i=1; $i<=$NUM_NODES; $i++) {
    for (my $j = 0; $j<$PUBS_PER_NODE; $j++) {
	$info{$i}->[$j] = {};

	$info{$i}->[$j]->{x} = sprintf "%d", int (rand($RANGE));
	$info{$i}->[$j]->{tag} = sprintf "5.5.5.5:2:%d", $tagid++;
    
	$info{$i}->[$j]->{minx} = 
	    sprintf "%d", int (max(0, $info{$i}->[$j]->{x} - $AOE/2));
	$info{$i}->[$j]->{maxx} = 
	    sprintf "%d", int (min($RANGE, $info{$i}->[$j]->{x} + $AOE/2));
    
	subscribe( $node{$i}, 
		   ("x", $info{$i}->[$j]->{minx}, $info{$i}->[$j]->{maxx}, "tag", $info{$i}->[$j]->{tag}));
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

# foreach pub that we sent:
for (my $i=1; $i<=$NUM_NODES; $i++) {
    for (my $j = 0; $j<$PUBS_PER_NODE; $j++) {
	my $guid = $guid{$i}->[$j];
	my $x    = $info{$i}->[$j]->{x};

        # check who all received this pub (we know that set is correct - 3test.pl):
	my @seen = check_matchedpub($guid, $x, \%node);

        # check if those people got the correct tags or not;
	foreach my $pubnode (@seen) {
	    my @expected_tags = ();

            # look at all the subscriptions I sent; if any of them matches the pub, its tag should be expected;
	    for (my $j2 = 0; $j2<$PUBS_PER_NODE; $j2++) {
		my $info = $info{$pubnode}->[$j2];

		if ($x >= $info->{minx} && $x <= $info->{maxx}) {
		    push @expected_tags, $info->{tag};
		}
	    }

	    # find the actual tags seen by this publication at this receiving node;
	    my @found_tags = get_matchedpub_tags($guid, $pubnode, \%node);
	    if (!check_equal(\@found_tags, \@expected_tags)) {
		local $" = ",";
		error("pub $guid (x=$x) for node($pubnode) found-tags = @found_tags, expected_tags = @expected_tags");
		$failed = 1;
	    } else {
                # ok, got correctly matched; did the tags come alright?;
		local $" = ",";
		info("correctly got tags @found_tags for matched pub for $guid");
	    }
	}
    }
}

if ($failed) {
    fail();
}

sub check_equal {
    my ($aref_one, $aref_other) = @_;

    # 8/19 Jeff: they don't have to be equal length to be equal because
    # in some cases, we will have 2 of the identical tag in which case
    # there only needs to be one instance of the tag in the other

    my %one   = map { $_ => 1 } @$aref_one;
    my %other = map { $_ => 1 } @$aref_other;
    
    return 0 if (scalar keys %one != scalar keys %other);
    
    #@$aref_one = sort { $a cmp $b } @$aref_one;
    #@$aref_other = sort { $a cmp $b } @$aref_other;

    #for (my $i = 0; $i < scalar @$aref_one; $i++) {
    #	return 0 if ($aref_one->[$i] ne $aref_other->[$i]);
    #}

    foreach my $k (keys %one) {
	return 0 if !$other{$k};
    }

    return 1;
}

info("SUCCESS");
stop_all();
