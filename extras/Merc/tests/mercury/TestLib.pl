# Scripting tools.
#
# To use:
#
# require "TestLib.pl";
#
# See usage details below.

use strict;
use IPC::Open2;
use IO::Socket::INET;
use IO::Select;
use Fcntl;
use POSIX "sys_wait_h";

our $VALGRIND = 0;
our $VERBOSE = 1;
our $VG = 0;
our $TIMEOUT = $VG ? 10 : 1;
$TIMEOUT = 5;

our $ENV_PREFIX = "LD_LIBRARY_PATH=../..";
our $PROG = "$ENV_PREFIX " . ($VG ? "valgrind --tool=memcheck --num-callers=10 " : "") . " ./merctest";
our $BOOTSTRAP = "$ENV_PREFIX " . ($VG ? "valgrind --tool=memcheck --num-callers=4 " : "") . " ../../build/bootstrap";
our $start_port = 25000;

# did the test succeed?
our $SUCCESS;

# nodeid => { nodeid     => $,
#             pid        => $,
#             out        => $,
#             in         => $,
#             killed     => $
#           }

$SIG{INT} = $SIG{TERM} = $SIG{QUIT} = sub {
	fail("Received SIG $_[0]; cleaning up");
};

$SIG{PIPE} = 'IGNORE';

system ("rm -f TEST.*.LOG bootstrap.log MercEvents.* core.*");
system ("killall -9 bootstrap");
our $CONF = "/tmp/conf.$$";

sub max($$) {
	my ($a, $b) = @_;
	return $a > $b ? $a : $b;
}

sub min($$) {
	my ($a, $b) = @_;
	return $a < $b ? $a : $b;
}

###############################################################################

# start_bootstrap(@conf)
#
# @conf = (attr_name => [ type, min_value, max_value ])
#
# Description: start the bootstrap server with the specified configuration.
# attr_name is the attribute name, type is one of {int, float, etc.}.
sub start_bootstrap(%)
{
	fail("no attributes!") if !@_;
	my %attributes = @_;
	open(CFG, ">$CONF") || fail("can't open $CONF");
	foreach my $k (sort keys %attributes) {
		my @args = @{$attributes{$k}};
		print CFG "$k\t" . join("\t", @args) . "\n";
	}
	close(CFG);

# system("$ENV_PREFIX xterm -hold -e valgrind --tool=memcheck " . ($VALGRIND ? "valgrind " : "") . "../../build/bootstrap --schema $CONF --histograms --buckets 35 -v 6 2>bootstrap.log 1>&2 &") && fail("couldn't start bootstrap");
	system(($VALGRIND ? "valgrind " : "") . "$BOOTSTRAP --schema $CONF --histograms --buckets 35 -v 5 2>bootstrap.log 1>&2 &") && fail("couldn't start bootstrap");
	vinfo("started bootstrap server with conf:\n" . `cat /$CONF`);
	Sleep(0.25);

#system("cat $CONF");
}

# $noderef = start_node($nodeid, @args)
#
# nodeid - a unique number for the node
# args   - arguments to start the node with (port is predefined)
#
# Description: start up a node running TestApp. Note, you MUST call the
# appropriate sequence of "Send()" "Recv()" on the nodes already in
# Mercury in order for this node to join properly before calling
# any commands on it. Use start_nodes(...) to do this automatically.
sub start_node($@) {
	my ($nodeid, @args, $node);

	$nodeid     = shift;
	@args       = @_;

	@args = (@args, "-C", "-R");
	my $hostname;
	chomp ($hostname = `hostname -i`);
	unshift @args, "-v 5 -D '' -H $hostname", '-p', $start_port+$nodeid;

	my $log = "TEST.NODE${nodeid}.LOG";

	vinfo("starting node $nodeid (@args) => $log");

	$node = {};

	$node->{nodeid} = $nodeid;

	my($in, $out);
	$node->{pid} = open2($in, $out, ($VALGRIND ? "valgrind " : "") . "$PROG @args 2>$log");
	$node->{in} = $in;
	$node->{out} = $out;

	if ($node->{pid} <= 0) {
		fail("Couldn't start node $nodeid!");
	}

	Sleep(0.1);
	return $node;
}

# %nodes = start_nodes($number, @args)
#
# Description: start several nodes with the specified arguments 
# (port predefined). The return value is a hash of all the node references
# indexed by node id (1, 2, ..., $number).
sub start_nodes($@) {
	my %nodes;
	my $num = shift @_;

	for (my $i=1; $i<=$num; $i++) {
		$nodes{$i} = start_node($i, @_);

# Jeff 6/10/04:
# XXX Mercury join process is broken. must force everyone to
# join synchronously. :P
# XXX Fuck. Its still broken. This sucks. Mercury just doesn't work. :(
		idle_all(\%nodes, 10);
		}

		return %nodes;
		}

# stop_node($noderef)
#
# Description: kill the node.
		sub stop_node($) {
		my $node = shift;
		fail("bad node") if !$node || !$node->{out};

		vinfo("stopping node " . $node->{nodeid});

		kill 15, $node->{pid};
		}

# stop_all()
#
# Description: kill everything. This is kind of hacky.
sub stop_all() {
	vinfo("stopping everything"); 

	system("killall bootstrap");
	system("killall merctest");
	unlink $CONF;
}


###############################################################################

# $guid = publish($node, @pub_args) 
#   Description: sends a publication from a node; returns a unique GUID 
#   for that publication so you can track it later. pub_args is a hash
#   of [attr, val] pairs, e.g., [ "x", "f:30" ]
#              
sub publish($%) {
	my $node = shift;
	my @pub_args = @_;

	my $resp = cmd($node, "publish", @pub_args);
	my $guid;
	if ($resp =~ /guid=([^ \t\n]+)/) {
		$guid = $1;
	} else {
		error("publish error: $resp");
	}   

	return $guid;
}

# $guid = range_publish($node, @pub_args) 
#   Description: sends a publication from a node; returns a unique GUID 
#   for that publication so you can track it later. 
#   pub_args contains a range pub [ "x", "<=", "f:30" ]
#              
sub range_publish($%) {
	my $node = shift;
	my @pub_args = @_;

	my $resp = cmd($node, "range_publish", @pub_args);
	my $guid;
	if ($resp =~ /guid=([^ \t\n]+)/) {
		$guid = $1;
	} else {
		error("publish error: $resp");
	}   

	return $guid;
}

# $guid = publish_trigger($ttl, $node, @pub_args)
#  Description: sends a trigger from a node. TTL is the ttl in seconds;
#  returns a unique GUID for the trigger
#
sub publish_trigger($$%) {
	my $ttl = shift;
	my $node = shift;
	my @pub_args = @_;

	my $resp = cmd($node, "trigger", $ttl, @pub_args);
	my $guid;
	if ($resp =~ /guid=([^ \t\n]+)/) {
		$guid = $1;
	} else {
		error("trigger error: $resp");
	}   

	return $guid;
}

# $guid = publish_guid_trigger($guid, $seqnum, $ttl, $node, @pub_args)
#  Description: sends a trigger from a node. TTL is the ttl in seconds;
#  returns a unique GUID for the trigger
#
sub publish_guid_trigger($$%) {
        my $guid = shift;
	my $seqnum = shift;
	my $ttl = shift;
	my $node = shift;
	my @pub_args = @_;

	my $resp = cmd($node, "guid_trigger", $guid, $seqnum, $ttl, @pub_args);
	if ($resp =~ /guid=([^ \t\n]+)/) {
	    if ($guid ne $1) {
		error("guid_trigger error: resp guid $1 different from original guid $guid");
	    }
	    $guid = $1;
	} else {
	    error("trigger error: $resp");
	}   
	
	return $guid;
}

# $guid = subscribe($node, @sub_args)
#  Description: sends a subscription from a node; returns a unique GUID 
#  for the subscription for later tracking. sub_args is just an array of 
#  the form: [ "attrib" , "min", "max" ]
#
sub subscribe($@) {
	my $node = shift;
	my @sub_args = @_;

	vinfo(" subscribe: @sub_args ");
	my $resp = cmd($node, "subscribe", @sub_args);
	my $guid;
	if ($resp =~ /guid=([^ \t\n]+)/) {
		$guid = $1;
	} else {
		error("subscribe error: $resp");
	}   

	return $guid;
}

# %hubsubs = getsubs($node)
#  Description: retrieves subscriptions stored at the node. The return value
#  is a hash consistign of a list of subscriptions for each hub. 
#      { "x" => [ { "x" => [ "<", "f:400" ] }, { "x" => [ ">", "i:200" ] } ] }
#
sub getsubs($) {
	vinfo($_[0]->{nodeid} . ": getsubs");

	my $resp = cmd($_[0], "getsubs");
	vinfo($resp);

	my %hubsubs = ();
	if (!$resp || $resp =~ /OK/) {
		my @lines = split(/\n/, $resp);
		shift @lines;     # ignore the first line (contains OK);
		foreach my $l (@lines) {
			if ($l =~ /hub (\w+) ::(.*)::/) {
				my ($name, $subs) = ($1, $2);
				$hubsubs{$name} = [];
				for my $s (split(/\#\#/, $subs)) {
					if ($s =~ /Interest guid=([^ \t\n]+) cons=\[(.*)\]/) {
						my $hs = {};

						my ($guid, $act_sub) = ($1, $2);

						$hs->{guid} = $guid;         # just check the guid for now;
#			for my $c (split(/,/, $act_sub)) {
#			    my ($k, $o, $t, $v) = split(/:/, $c);
#			    $hs->{$k} = [$o, $t, $v];
#			}

						push( @{$hubsubs{$name}}, $hs );
					}
					else {
						goto error;
					}
				}
			}   
			else 
			{
				goto error;
			}
		}
	}
	return %hubsubs;
error:
	error("getsubs error: $resp");
	return undef;
}

# $range_ref = get_ranges($node)
#
# Description: retrieves ranges for all attributes for a node
# range_ref = (attr => range)
#   The range itself is a reference to a hash.
#
sub get_ranges($) {
	my ($node) = @_;

	my $all_ranges = {};
	my $resp = cmd($node, "get_allranges");
	if ($resp =~ /OK/) {
		my @lines = split(/\n/, $resp);
		foreach my $l (@lines) {
#		    vinfo($l);
			if ($l =~ /OK: (\w+):\[([^ \t\n]+),([^ \t\n]+)\] abs_min=([^ \t\n]+) abs_max=([^ \t\n]+)/) {
				my $range = {};
				my ($name) = ($1);
				($range->{min}, $range->{max}, 
				 $range->{amin}, $range->{amax}) = ($2, $3, $4, $5);

				 $all_ranges->{$name} = $range;
			}
			elsif ($l =~ /OK: not joined null range (\w+)/) { 
				vinfo("node($node->{nodeid}) did not join hub $1");
			}
			else {
				error("get_ranges error: $resp");
				return undef;
			}
		}
		return $all_ranges;
	} else {
		error("get_ranges error: $resp");
		return undef;
	}
}

#   %hubpeers = dump_rt($noderef)
#
# Description: dump the routing table of node $noderef. The output is a 
#  a hash containing a list of peers for each hub; the first member is 
#  the predecessor, second = successor
#
sub dump_rt($) {
	my %ret;

	vinfo($_[0]->{nodeid} . ": dump_rt"); 

	my $resp = cmd($_[0], "dump_rt");
	my %hubpeers = ();
	vinfo($resp);

	if (!$resp || $resp =~ /OK/) {
		my @lines = split(/\n/, $resp);
		shift @lines;     # ignore the first line (contains OK);
		foreach my $l (@lines) {
			if ($l =~ /hub (\w+) (.*)/) {
				my ($name, $peers) = ($1, $2);
				$hubpeers{$name} = [];
				for my $p (split(/,/, $peers)) {
					push (@{$hubpeers{$name}}, $p);
				}
			}   
			else 
			{
				goto error;
			}
		}
	}
	return %hubpeers;
error:
	error("dump_rt error: $resp");
	return undef;
}

# @seen = check_pub($guid)
# 
# returns the nodes which have registered this publication in their logs.
sub check_pub($%) {
	my $guid = shift;
	my $nodes_ref = shift;
	my @seen = ();

	foreach my $id (keys %$nodes_ref)
	{
		my $node = $nodes_ref->{$id};
		my $file = sprintf("MercEvents.%d", $start_port + $id);

		my $grep = `cat $file | grep $guid | grep 'routing event'`;
		push (@seen, $id) if (defined $grep and $grep ne '');
	}
	return @seen;
}

# @seen = check_sub($guid)
# 
# returns the nodes which have registered this subscription in their logs.
sub check_sub($%) {
	my $guid = shift;
	my $nodes_ref = shift;
	my @seen = ();

	foreach my $id (keys %$nodes_ref)
	{
		my $node = $nodes_ref->{$id};
		my $file = sprintf("MercEvents.%d", $start_port + $id);

		my $grep = `cat $file | grep $guid | grep 'routing sub'`;
		push (@seen, $id) if (defined $grep and $grep ne '');
	}
	return @seen;
}

# @seen = check_substorage($attr, $guid, $nodes_ref)
# 
# returns the nodes which have stored this subscription.
sub check_substorage($$$) {
	my $attr = shift;
	my $guid = shift;
	my $nodes_ref = shift;
	my @seen = ();

	vinfo ("attr=$attr");
	foreach my $id (keys %$nodes_ref)
	{
		my $node = $nodes_ref->{$id};
		my $file = sprintf("MercEvents.%d", $start_port + $id);

		my $grep = `cat $file | grep -- "==hub== $attr" | grep $guid | grep 'storing sub'`;
		push (@seen, $id) if (defined $grep and $grep ne '');
	}
	return @seen;
}

# @seen = check_pubstorage($attr, $guid, $nodes_ref)
# 
# returns the nodes which have stored this publication.
sub check_pubstorage($$%) {
	my $attr = shift;
	my $guid = shift;
	my $nodes_ref = shift;
	my @seen = ();

	foreach my $id (keys %$nodes_ref)
	{
		my $node = $nodes_ref->{$id};
		my $file = sprintf("MercEvents.%d", $start_port + $id);

		my $grep = `cat $file | grep -- "==hub== $attr" | grep $guid | grep 'matching event'`;
		push (@seen, $id) if (defined $grep and $grep ne '');
	}
	return @seen;
}
# @seen = check_triggerstorage($attr, $guid, $val, $nodes_ref)
# 
# returns the nodes which have stored this publication.
sub check_triggerstorage($$%) {
	my $attr = shift;
	my $guid = shift;
	my $val = shift;
	my $nodes_ref = shift;
	my @seen = ();

	$val = sprintf("%d", int ($val));
	foreach my $id (keys %$nodes_ref)
	{
		my $node = $nodes_ref->{$id};
		my $file = sprintf("MercEvents.%d", $start_port + $id);

		my $grep = `cat $file | grep -- "==hub== $attr" | grep $guid | grep 'storing trigger' | grep '$val'`;
=start		
		if (defined $grep and $grep ne '') {
		    $grep =~ m/real:([\d\.]+)/;
		    my $seenval = $1; 
		    print "seenval = $seenval\n";

		    if (abs($seenval - $val) < 0.0001) {
			push (@seen, $id);
		    }
		}
=cut
                push (@seen, $id) if (defined $grep and $grep ne '');
	}
	return @seen;
}


# @seen = check_matchedpub($guid, $val, $nodes_ref)
# 
# returns the nodes which have matched this publication.
sub check_matchedpub($%) {
    	my $guid = shift;
	my $val = shift;
	my $nodes_ref = shift;
	my @seen = ();

	$val = sprintf("%d", int ($val));
	foreach my $id (keys %$nodes_ref)
	{
	    my $node = $nodes_ref->{$id};
	    my $file = sprintf("MercEvents.%d", $start_port + $id);
	    
	    my $grep = `cat $file | grep -- "received matchedpub" | grep $guid | grep '$val'`;
=start		 
	    if (defined $grep and $grep ne '') {
		$grep =~ m/real:([\d\.]+)/;
		my $seenval = $1; 
		print "seenval = $seenval\n";

		if (abs($seenval - $val) < 0.0001) {
		    push (@seen, $id);
		}
	    }
=cut
	    push (@seen, $id) if (defined $grep and $grep ne '');
	}
	return @seen;
}

# @seen = check_range_matchedpub($guid, $min, $max, $nodes_ref)
# 
# returns the nodes which have received this publication as a match for their subscriptions.
sub check_range_matchedpub($%) {
    	my $guid = shift;
	my $min = shift;
	my $max = shift;
	my $nodes_ref = shift;
	my @seen = ();

	$min = sprintf("%d", int ($min));
	$max = sprintf("%d", int ($max));
	
	foreach my $id (keys %$nodes_ref)
	{
	    my $node = $nodes_ref->{$id};
	    my $file = sprintf("MercEvents.%d", $start_port + $id);
	    
	    my $grep = `cat $file | grep -- "received matchedpub" | grep $guid | grep '$min' | grep '$max'`;
	    push (@seen, $id) if (defined $grep and $grep ne '');
	}
	return @seen;
}

# @seen = check_matchedpub_withnum($guid, $nodes_ref)
# 
# returns the nodes which have matched this publication.
sub check_matchedpub_withnum($%) {
	my $guid = shift;
	my $nodes_ref = shift;
	my %seen = ();

	foreach my $id (keys %$nodes_ref)
	{
		my $node = $nodes_ref->{$id};
		my $file = sprintf("MercEvents.%d", $start_port + $id);

		my $grep = `cat $file | grep -- "received matchedpub" | grep $guid | wc -l`;
		chomp $grep;
		$seen{$id} = $grep;
	}

	return %seen;
}
# returns the tags for the matched publication
sub get_matchedpub_tags($$$) {
	my $guid = shift;
	my $id   = shift;
	my $nodes_ref = shift;
	my @tags = ();

	my $node = $nodes_ref->{$id};
	my $file = sprintf("MercEvents.%d", $start_port + $id);

	chomp(my $grep = `cat $file | grep -- "received matchedpub" | grep $guid`);
	if (defined $grep and $grep ne '') {
		my $matched;

		if ($grep =~ /tags=\[(.*)\]/) {
			$matched = $1;
			@tags = split(/,/, $matched);
		}
	}
	return @tags;
}

#  $does_overlap = range_overlap($range1, $range2, $attr)
#
# returns "true" if the ranges overlap. for the given attribute
#
sub range_overlap($$$) {
	my ($r1, $r2, $attr) = @_;

# ah, fuck this.... too bloody complex and i have implemented 
# this stuff already in C++... :-(
		}

# idle($noderef, $loops)
#
# Description: Calls Send(), Recv() $loops times on the node.
		sub idle($$) {
		my $node  = shift;
		my $loops = shift; 
		if (!$loops) { $loops = 5 }

#vinfo($node->{nodeid} . ": idling for $loops cycles"); 

		while ($loops-- > 0) {
		cmd($node, "idle");
		}   
		}

# idle_all(\%noderefs, $loops)
#
# Description: Calls Send(), Recv() $loops times on each node in the hash.
# Calls it once on each node in order and repeats until finished.
sub idle_all($$) {
	my $nodes = shift;
	my $loops = shift;
	if (!$loops) { $loops = 5 }

	vinfo("idling all for $loops cycles");

	while ($loops-- > 0) {
# HACK: if we are in the process of joining, then we need
# to process each guy's join messages in turn. only the first
# guy to join actually is able to handle messages so far.
# The other guys will be blocked waiting
		foreach my $k (sort keys %$nodes) {
			cmd($nodes->{$k}, "idle");
		}
	}
}


###############################################################################

sub cmd($@) {
	my $node = shift;
	fail("bad node") if !$node || !$node->{out};

	my $out = $node->{out};
	my $cmd = join(" ", @_) . "\n";
	print $out $cmd;
	my $line;
	my $ret;

	my $in = $node->{in};
	$line = read_line($in, $TIMEOUT, $cmd, $node);
	if ($line && $line !~ /==BEGIN_RESP/) {
		error("response to @_ didn't begin with ==BEGIN_RESP: $line");
	}
	while($line = read_line($in, $TIMEOUT, "**$cmd", $node)) {
		last if $line =~ /==END_RESP/;

		$ret .= $line;
	}

	return $ret;
}

###############################################################################

# Report an error
# Params:
# $0: message
# $1: file handle to print to (optional, default=STDOUT)
sub error($) {
	my $fh = *STDOUT;
	if (defined $_[1]) {
		$fh = $_[1];
	}

	$SUCCESS = 0;

	print $fh (" [EE] $_[0]\n");
}

# Report information
# Params:
# $0: message
# $1: file handle to print to (optional, default=STDOUT)
sub info($) {
	my $fh = *STDOUT;
	if (defined $_[1]) {
		$fh = $_[1];
	}
	print $fh ( " [II] $_[0]\n");
}

sub vinfo($) {
	info($_[0]) if $VERBOSE;
}

our $FAILED = 0;

# Reports failure to run and complete test
# Params:
# $0: message
# $1: file handle to print to (optional, default=STDOUT)
sub fail(@) {
	my $fh = *STDOUT;
	if (defined $_[1]) {
		$fh = $_[1];
	}
	$_[0] = "FAILED" if ! defined $_[0];
	print $fh (_time() . " [FF] $_[0]\n");
# don't recursively fail again!
	if ($FAILED == 1) {
		return;
	}
	$FAILED = 1;
#	sleep(100000); # to debug
	stop_all();
	info("UNKNOWN");
	exit(255);
}

# true if no errors reported
sub success() {
	return $SUCCESS;
}

sub _time() {
	return time();
}

# Read line with timeout
# $0: the file descriptor to read from
# $1: timeout in sec (optional, default=1)
# $2: current command 
sub read_line($$$) {

	my $fh = shift;
	my $timeout = shift;
	my $cmd = shift; 
	my $node = shift;

	$timeout = 1 if (!defined $timeout);
	my $res = undef;

	eval {
		local $SIG{ALRM} = sub { die "alarm clock restart" };
		alarm $timeout;
		eval {
			$res = <$fh>;
			$res =~ s/\r\n/\n/g;
		};
		alarm 0;

		if ($@ && $@ =~ /alarm clock restart/) {
			die "timeout failure";
		}
	};
	alarm 0;

	if ($@ && $@ =~ /timeout failure/) {
		error("Timed out waiting for response for cmd: $cmd; node: $node->{nodeid}");
		return undef;
	}
	return $res;
}

sub Sleep($) {
	select undef, undef, undef, $_[0];
}

###############################################################################

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

sub nuke_logs {
	my $node_ref = shift;
	
	foreach my $id (keys %$node_ref) {
		my $file = sprintf("MercEvents.%d", $start_port + $id);
		system "rm -f $file";
	}
}

1;
