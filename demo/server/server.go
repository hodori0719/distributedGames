package main

import (
	"flag"
	"fmt"
	"juneyoo/distgamelib/lib/game"
	"juneyoo/distgamelib/lib/server"
	"juneyoo/distgamelib/lib/server/proto"
	"net"
	"time"

	"github.com/sirupsen/logrus"
	"google.golang.org/grpc"
)

var (
	sid                  = flag.Int("sid", 0, "server id")
	basePort             = flag.Int("base-port", 9000, "port of first server")
	numServers           = flag.Int("num-servers", 4, "number of servers to connect to")
	logLevel             = flag.String("log-level", "info", "log level")
	proactiveReplication = flag.Bool("proactive-repl", true, "enable proactive replication")
	frameRate            = flag.Int("frame-rate", 10, "rate at which to update the game state")
	predTimeMs           = flag.Int64("pred-time", 500, "prediction time")
	pubTimeMs            = flag.Int64("pub-time", 100, "publication time (buffer)")
	cleanupTimeMs        = flag.Int64("cleanup-time", 1000, "cleanup time")
	interestTTLMs        = flag.Int64("interest-ttl", 2000, "interest TTL")

	testDelayMs = flag.Int64("test-delay", 10, "delay for testing")

	logrusInst *logrus.Entry = logrus.New().WithField("sid", *sid)
)

// Top level catalyst of events in the server. Other events occur in response to these events
// in handleUpdate.
// For the sake of convenience, this contains some game logic that isn't purely the Colyseus system itself.
// When multiple objects are involved in an event, the decision of which object should implement
// that logic is often application specific. For instance, when a player picks up a shield, we probably
// want the shield to tell the player that it collected it and then destroy itself, while the player
// should handle the resulting logic. If the player were to handle the collision logic,
// then we might have multiple players claiming they all picked up the shield, etc.
func GameLoop(s *server.Colyseus) {
	for {
		time.Sleep(time.Second / time.Duration(*frameRate))
		updateGuids := make([]uint32, 0)
		updates := make([]int32, 0)
		s.Lock()
		for _, primary := range s.Primaries {
			switch primary.Type {
			case game.Player:
				loc := primary.GetLocation()
				logrusInst.WithField("x", loc.X).WithField("y", loc.Y).Infof("Rendering player %d", primary.Guid)
			case game.Projectile:
				loc := primary.GetLocation()
				logrusInst.WithField("x", loc.X).WithField("y", loc.Y).Infof("Rendering projectile %d", primary.Guid)
				// rudimentary logic: check for collisions with players across primaries and replicas.
				targetFound := false
				for _, primary2 := range s.Primaries {
					if primary2.Type == game.Player {
						if loc.Intersects(primary2.GetLocation()) {
							logrusInst.WithField("x", loc.X).WithField("y", loc.Y).Infof("Projectile %d hit player %d", primary.Guid, primary2.Guid)
							updateGuids = append(updateGuids, primary2.Guid)
							updates = append(updates, game.Die)
							updateGuids = append(updateGuids, primary.Guid)
							updates = append(updates, game.Die)
							targetFound = true
							break
						}
					}
				}
				if !targetFound {
					for _, replica := range s.Replicas {
						if replica.Type == game.Player {
							if loc.Intersects(replica.GetLocation()) {
								logrusInst.WithField("x", loc.X).WithField("y", loc.Y).Infof("Projectile %d hit player %d", primary.Guid, replica.Guid)
								updateGuids = append(updateGuids, replica.Guid)
								updates = append(updates, game.Die)
								updateGuids = append(updateGuids, primary.Guid)
								updates = append(updates, game.Die)
								targetFound = true
								break
							}
						}
					}
				}
			case game.Shield:
				loc := primary.GetLocation()
				logrusInst.WithField("x", loc.X).WithField("y", loc.Y).Infof("Rendering shield %d", primary.Guid)
			}
		}

		for _, replica := range s.Replicas {
			switch replica.Type {
			case game.Player:
				loc := replica.GetLocation()
				logrusInst.WithField("x", loc.X).WithField("y", loc.Y).WithField("replSid", replica.Sid).Infof("Rendering repl player %d", replica.Guid)
			case game.Projectile:
				// Notice here we don't care if a replica projectile hits something. The primary copy will detect the collision and update us on what to do.
				loc := replica.GetLocation()
				logrusInst.WithField("x", loc.X).WithField("y", loc.Y).WithField("replSid", replica.Sid).Infof("Rendering repl projectile %d", replica.Guid)
			case game.Shield:
				loc := replica.GetLocation()
				logrusInst.WithField("x", loc.X).WithField("y", loc.Y).WithField("replSid", replica.Sid).Infof("Rendering repl shield %d", replica.Guid)
			}
		}
		s.Unlock()

		s.AdvanceGameState(updateGuids, updates)
	}
}

func main() {
	flag.Parse()

	// Set up logging
	level, err := logrus.ParseLevel(*logLevel)
	if err != nil {
		logrusInst.Fatal(err)
	}
	logrus.SetLevel(level)

	// Calculate the port for this server
	port := *basePort + *sid

	// Start listening on the calculated port
	lis, err := net.Listen("tcp", fmt.Sprintf(":%d", port))
	if err != nil {
		logrusInst.Fatal(err)
	}

	// Create the gRPC server
	grpcServ := grpc.NewServer()
	logrusInst.WithField("port", port).Trace("Creating colyseus server")

	// Create and configure the Colyseus server
	colyseus := server.MakeColyseusServer(
		int32(*sid),
		*proactiveReplication,
		*frameRate,
		*predTimeMs,
		*pubTimeMs,
		*cleanupTimeMs,
		*interestTTLMs,
		*testDelayMs,
		logrusInst,
	)
	colyseus.ConnectToLocator("localhost:8999")
	for i := 0; i < *numServers; i++ {
		if i == *sid {
			continue
		}
		colyseus.ConnectToPeer(int32(i), fmt.Sprintf("localhost:%d", *basePort+i))
	}

	// Register the Colyseus server with gRPC
	proto.RegisterColyseusServer(grpcServ, colyseus)

	go GameLoop(colyseus)

	// Start serving
	logrusInst.Infof("server listening at %v", lis.Addr())
	if err := grpcServ.Serve(lis); err != nil {
		logrusInst.Fatalf("failed to serve: %v", err)
	}
}
