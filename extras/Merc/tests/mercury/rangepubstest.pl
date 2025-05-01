#!/usr/bin/perl
#
# make sure range pubs are matched correctly
# re-factor the testing code into proper functions .. much of the code is 
# repeated. ugh. 

#
# XXX WILL ONLY WORK WITH ONE ATTRIBUTE

use strict;
require "TestLib.pl";

my $NUM_NODES  = 15;
my $PUBS_PER_NODE = 1;
my $RANGE = 10000;
my $MEAN_AOE = 4000;            # make sure the pub/sub goes to a bunch of places 

##################################################################
####### CHECK RANGES

start_bootstrap( "x" => [0, $RANGE] );
my %node = start_nodes($NUM_NODES, "-l", 50000, "--pubtriggers", "--backpub", "--fanoutpubs" ); #, "-v 20 -D MercuryRouter,RealNet,Message");
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
		if ($node{$sort_ids[$i]}->{range}->{max} != "$RANGE") {
			error("node ranges do not align");
			fail();
		}
	}
}

###################################################################
# send subscriptions

my $guid;

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

	my $x = sprintf "%d", int (rand($RANGE));
    
	$info{$i}->[$j]->{submin} = 
	    sprintf "%d", int (max(0, $x - $MEAN_AOE/2));
	$info{$i}->[$j]->{submax} = 
	    sprintf "%d", int (min($RANGE, $x + $MEAN_AOE/2));
    
	$x = sprintf "%d", int (rand($RANGE));
    
	$info{$i}->[$j]->{pubmin} = 
	    sprintf "%d", int (max(0, $x - $MEAN_AOE/2));
	$info{$i}->[$j]->{pubmax} = 
	    sprintf "%d", int (min($RANGE, $x + $MEAN_AOE/2));
    
	subscribe( $node{$i}, 
		   ("x", $info{$i}->[$j]->{submin}, $info{$i}->[$j]->{submax}));
    }
}

## route all the subs
idle_all(\%node, $NUM_NODES * 10);

for (my $i=1; $i<=$NUM_NODES; $i++) {
    for (my $j = 0; $j<$PUBS_PER_NODE; $j++) {
	$guid{$i}->[$j] = range_publish($node{$i}, 
		("x", $info{$i}->[$j]->{pubmin}, $info{$i}->[$j]->{pubmax}));
    }
}

## route pubs to matchers
idle_all(\%node, $NUM_NODES * 10);

my $failed;

## Check that each pub made to it all its subscribers
for (my $i=1; $i<=$NUM_NODES; $i++) {
    for (my $j = 0; $j<$PUBS_PER_NODE; $j++) {
	my $guid = $guid{$i}->[$j];
	my $min    = $info{$i}->[$j]->{pubmin};
	my $max    = $info{$i}->[$j]->{pubmax};

	my @seen = check_range_matchedpub($guid, $min, $max, \%node);
	my %seen;
	map { $seen{$_} = 1 } @seen;

	 # get all the guys who should have seen this pub -- 
	
	for (my $i2=1; $i2<=$NUM_NODES; $i2++) {
	    for (my $j2 = 0; $j2<$PUBS_PER_NODE; $j2++) {
		my $smin = $info{$i2}->[$j2]->{submin};
		my $smax = $info{$i2}->[$j2]->{submax};

		 # check if the range overlaps!
		if (($smin <= $min && $smax >= $min) ||
			($min <= $smin && $max >= $smin)) 
		{
		    if (! exists $seen{$i2}) {
			error("$i2 didn't get pub $guid,  pub = [$min, $max]; sub = [$smin, $smax]");
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

info("SUCCESS");
stop_all();
