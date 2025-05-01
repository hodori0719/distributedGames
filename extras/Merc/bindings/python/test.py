# vim: set sw=4 ts=8 sts=4 noet:

from mercsim import *
import sys
import random

host = "gs203.sp.cs.cmu.edu"
sim  = None

# right now the options are all hard code in the module 
# wrapper code (mercsim.i) at some later point, we shall 
# make stuff accessible from python

def setup_env(argv):
    global sim
    init_environment (argv)
    cvar.g_Preferences.bootstrap = "gs203.sp.cs.cmu.edu:0"
    cvar.SuccessorMaintenanceTimeout = 50
    cvar.PeerPingInterval = 50
    cvar.PeerPongTimeout  = 150

    sim = Simulator ()

    bootstrap = BootstrapNode (sim, sim, IPEndPoint (cvar.g_Preferences.bootstrap), 
                               cvar.g_BootstrapPreferences.schemaFile)
    bootstrap.thisown = 0       # hack to prevent garbage collection!
    sim.AddNode (bootstrap)
    bootstrap.Start ()

def create_node (index, interval):
    addr = IPEndPoint ("%s:%d" % (host, index))
    n = MercuryNode (sim, sim, addr)
    n.thisown = 0               # hack to prevent garbage collection!
    sim.AddNode (n)

    class start:
        def __init__ (self, n):
            self.n = n
            
        def func (self, node, time):
            self.n.Start ()

    sim.add_sim_task (start(n).func, interval)
    return n
            
def msg (*args):
    print >>sys.stderr, args

#e = OMEvent (GUID (SID_NONE, eid), cvar.SID_NONE)
    #if len(nodes) > 0:
    #    pub_node = nodes[random.randint (0, len(nodes) - 1)]
    #    print >>sys.stderr, "\n\n\n:::: publishing from node: %s\n\n" % pub_node.GetAddress ()
    #    pub_node.SendEvent (e)
    

def main(argv):
    setup_env (argv)
    eid = 0;

    nodes = []
    	
    def printer (node, time):
	print >>sys.stderr, ("-------------->>> CURRENT TIME=%s <<<===========" % time)
	for mercnode in nodes:
	    print >>sys.stderr, "  %s: " % mercnode.GetAddress (),
	    mercnode.PrintRanges ()
	print >>sys.stderr, "\n"

	sim.add_sim_task (printer, 1000)
	
    sim.add_sim_task (printer, 1000)

    for i in xrange (cvar.g_DriverPrefs.nodes):
        interval = 100 + i * cvar.g_DriverPrefs.inter_arrival_time
        n = create_node (i + 1, interval)
	nodes.append (n)
        
    class stop:
	def __init__ (self, n): 
	    self.n = n
	def func (self, node, time): 
	    self.n.Stop ()
	    sim.RemoveNode (self.n)
	    nodes.remove (self.n)

    #if len(nodes) > 6:
	#for i in xrange (4): 	
	    #sim.add_sim_task (stop(nodes[i + 1]).func, 1600)

    sim.ProcessFor (10000)

if __name__ == "__main__":
    import signal
    signal.signal(signal.SIGINT, lambda n, f: sys.exit(1))
    argv = None
    if len(sys.argv) <= 1:
        argv = "progname --nodes 4 --arrive-int 100 -v 10 "
    else:
        argv = sys.argv
    
    sys.exit(main(argv))
