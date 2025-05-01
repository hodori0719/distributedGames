package main

import (
	"context"
	"flag"
	"fmt"
	"net"
	"sync"
	"time"

	"github.com/sirupsen/logrus"

	"juneyoo/distgamelib/lib/game"
	"juneyoo/distgamelib/lib/locator/proto"
	colyseus "juneyoo/distgamelib/lib/server/proto"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

var (
	port            = flag.Int("port", 8999, "port to listen on")
	cleanupInterval = flag.Int("cleanup-ms", 2000, "interval in milliseconds to clean up stale objects")
	logLevel        = flag.String("log-level", "info", "log level")
	numServers      = flag.Int("num-servers", 8, "number of servers to connect to")
)

type Locator struct {
	proto.UnimplementedLocatorServer
	root        *Quadtree
	lastUpdates map[uint32]int64
	servers     map[int32]colyseus.ColyseusClient
	mu          sync.RWMutex
	shutdown    chan struct{}
}

type Storable struct {
	rect           game.Rectangle
	sid            int32
	guid           uint32
	ttl            int64
	isSubscription bool
}

func MakeLocatorServer() *Locator {
	logrus.WithField("port", *port).Trace("Creating locator server")
	server := Locator{
		root:        NewQuadtree(game.Rectangle{X: 0, Y: 0, Width: 100, Height: 100}, 4),
		lastUpdates: map[uint32]int64{},
		servers:     map[int32]colyseus.ColyseusClient{},
	}
	go server.CleanupLoop()
	return &server
}

// ConnectToServer connects to a server
func (l *Locator) ConnectToServer(sid int32, addr string) {
	logrus.WithField("sid", sid).Trace("Connecting to server")

	var opts []grpc.DialOption
	opts = append(opts, grpc.WithTransportCredentials(insecure.NewCredentials()))

	channel, err := grpc.NewClient(addr, opts...)
	if err != nil {
		logrus.WithError(err).Error("Failed to connect to server")
		return
	}
	l.mu.Lock()
	l.servers[sid] = colyseus.NewColyseusClient(channel)
	l.mu.Unlock()
}

// CleanupLoop periodically reduces the quadtree
func (l *Locator) CleanupLoop() {
	for {
		select {
		case <-l.shutdown:
			return
		default:
			logrus.Trace("Cleaning up stale objects")
			l.root.Cleanup(l)
		}
		time.Sleep(time.Duration(*cleanupInterval) * time.Millisecond)
	}
}

// Handle Publish requests. Publications are stored in the quadtree for a TTL as soft state.
// Matching subscriptions are notified of the publication(s).
func (l *Locator) Publish(ctx context.Context, request *proto.PublishRequest) (*proto.PublishResponse, error) {
	logrus.Tracef("Received Publish request: %v", request)

	matches := map[int32][]*colyseus.MatchMsg{}

	for _, pub := range request.Publications {
		gameObject := pub.GameObject
		found := []Storable{}
		l.root.Query(Storable{
			rect:           game.Rectangle{X: gameObject.X, Y: gameObject.Y, Width: gameObject.W, Height: gameObject.H},
			sid:            gameObject.Sid,
			guid:           gameObject.Guid,
			ttl:            pub.Ttl,
			isSubscription: false,
		}, &found, l)

		logrus.Tracef("Found %d matches", len(found))

		for _, f := range found {
			if _, ok := matches[f.sid]; !ok {
				matches[f.sid] = []*colyseus.MatchMsg{}
			}

			matches[f.sid] = append(matches[f.sid], &colyseus.MatchMsg{
				Guid: gameObject.Guid,
				Sid:  gameObject.Sid,
			})
		}

		l.root.Insert(Storable{
			rect:           game.Rectangle{X: gameObject.X, Y: gameObject.Y, Width: gameObject.W, Height: gameObject.H},
			sid:            gameObject.Sid,
			guid:           gameObject.Guid,
			ttl:            pub.Ttl,
			isSubscription: false,
		}, l)
	}

	logrus.WithField("matches", matches).Trace("Matches found")

	for sid, sMatches := range matches {
		logrus.WithField("sid", sid).Trace("Notifying subscriber")
		server := l.servers[sid]
		_, err := server.NotifySubscriber(ctx, &colyseus.NotifySubscriberRequest{
			Publications: sMatches,
		})

		if err != nil {
			logrus.WithError(err).Error("Failed to notify subscriber")
		}
	}

	return &proto.PublishResponse{}, nil
}

// Handle Subscribe requests. Subscribes are stored in the quadtree for a TTL as soft state.
// Matching publications are returned to the client.
func (l *Locator) Subscribe(ctx context.Context, request *proto.SubscribeRequest) (*proto.SubscribeResponse, error) {
	logrus.Tracef("Received Subscribe request: %v", request)
	found := []Storable{}
	for _, sub := range request.Subscriptions {
		gameObject := sub.GameObject
		l.root.Query(Storable{
			rect:           game.Rectangle{X: gameObject.X, Y: gameObject.Y, Width: gameObject.W, Height: gameObject.H},
			sid:            gameObject.Sid,
			guid:           gameObject.Guid,
			ttl:            sub.Ttl,
			isSubscription: true,
		}, &found, l)

		l.root.Insert(Storable{
			rect:           game.Rectangle{X: gameObject.X, Y: gameObject.Y, Width: gameObject.W, Height: gameObject.H},
			sid:            gameObject.Sid,
			guid:           gameObject.Guid,
			ttl:            sub.Ttl,
			isSubscription: true,
		}, l)
	}
	matches := []*proto.MatchMsg{}
	for _, f := range found {
		matches = append(matches, &proto.MatchMsg{
			Guid: f.guid,
			Sid:  f.sid,
		})
	}

	return &proto.SubscribeResponse{Publications: matches}, nil
}

func main() {
	flag.Parse()

	level, err := logrus.ParseLevel(*logLevel)
	if err != nil {
		logrus.Fatal(err)
	}
	logrus.SetLevel(level)

	lis, err := net.Listen("tcp", fmt.Sprintf(":%d", *port))
	if err != nil {
		logrus.Fatal(err)
	}

	server := grpc.NewServer()
	locator := MakeLocatorServer()

	for i := 0; i <= *numServers; i++ {
		locator.ConnectToServer(int32(i), fmt.Sprintf("localhost:%d", 9000+i))
	}

	proto.RegisterLocatorServer(
		server,
		locator,
	)

	logrus.Infof("server listening at %v", lis.Addr())
	if err := server.Serve(lis); err != nil {
		logrus.Fatalf("failed to serve: %v", err)
	}
}
