#!/usr/bin/perl

use strict;
# use lib "$ENV{HOME}/id-source/scripts/";
use lib "$ENV{PERL5LIB}/id-source/scripts/emulab";
use Travertine;
use EmulabConf;
use Getopt::Std;
use vars qw($opt_E $opt_n);

getopts("E:n:");

our $EXP_NAME      = defined $opt_E ? $opt_E : "50lan";
our $MAX_NODES     = defined $opt_n ? $opt_n : 50;

sub Run
{
    my ($TOPDIR, $prog, $args) = @_;

    chdir($TOPDIR);
    $ENV{LD_LIBRARY_PATH} .= ":./Merc";

    # force a time sync
    #psystem("sudo /etc/init.d/ntpd restart >/dev/null 2>&1");
    #sleep(10);

    psystem("killall $prog >/dev/null 2>&1");
    #print `pwd`;
    #print "$prog $args\n";
    psystem("$prog $args");
}

## start master
tinfo "starting master...";
my $ssh = new Travertine::SSHTunnel($USERNAME, 
				    "node0.$EXP_NAME.$GROUP.emulab.net");
my $bg = ExecRemoteFunc($ssh,
			\&Run, 
			[ $TOPDIR,
			  "./Merc/util/time-test/server", 
			  "node0:55555" ],
			-background => 1,
			-log => "/tmp/time-test.master.log");

# Jeff: on my home machine, many parallel DNS requests seem to fail :P
my @ips = map { my $ip =
		    join(".", unpack 'C4', 
			 (gethostbyname 
			  "node$_.$EXP_NAME.$GROUP.emulab.net")[4]);
		tdie "can't resolve node$_" if !$ip;
		[$_, $ip];
	    } (0..($MAX_NODES-1));

## start slaves
tinfo "starting slaves...";
my @ret = ParallelExec2(sub {
    my $i  = shift;
    my $ip = shift;

    my $ssh = new Travertine::SSHTunnel($USERNAME, $ip);
    my ($out, $err, $stat) =
	ExecRemoteFunc($ssh,
		       \&Run, 
		       [ $TOPDIR,
			 "./Merc/util/time-test/client", 
			 "node$i:44444 node0:55555" ]);

    if (!$out) {
	tinfo "node$i failed!";
	#twarn "node$i: ERROR: no output";
	return "node$i\tERROR: no output\n";
    }

    tinfo "node$i completed";
    return "node$i\t$out";

}, @ips);

foreach my $o (@ret) {
    print $o;
}

## stop master
tinfo "killing master...";
rsystem($USERNAME, "node0.$EXP_NAME.$GROUP.emulab.net", sub {
	psystem("killall -9 server >/dev/null 2>&1");
});

tinfo "done";
