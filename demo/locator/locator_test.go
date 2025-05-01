package main

import (
	"context"
	"net"
	"testing"
	"time"

	"juneyoo/distgamelib/lib/game"
	"juneyoo/distgamelib/lib/locator/proto"

	"github.com/stretchr/testify/assert"
	"google.golang.org/grpc"
)

type TestLocatorServer struct {
	proto.UnimplementedLocatorServer
	locator *Locator
}

func (s *TestLocatorServer) Publish(ctx context.Context, req *proto.PublishRequest) (*proto.PublishResponse, error) {
	return s.locator.Publish(ctx, req)
}

func (s *TestLocatorServer) Subscribe(ctx context.Context, req *proto.SubscribeRequest) (*proto.SubscribeResponse, error) {
	return s.locator.Subscribe(ctx, req)
}

func TestPublish(t *testing.T) {
	// Set up a test gRPC server
	lis, err := net.Listen("tcp", ":0")
	assert.NoError(t, err)
	defer lis.Close()

	grpcServer := grpc.NewServer()
	testServer := &TestLocatorServer{
		locator: MakeLocatorServer(),
	}
	proto.RegisterLocatorServer(grpcServer, testServer)

	go grpcServer.Serve(lis)
	defer grpcServer.Stop()

	// Set up a test gRPC client
	conn, err := grpc.NewClient(lis.Addr().String(), grpc.WithInsecure())
	assert.NoError(t, err)
	defer conn.Close()

	client := proto.NewLocatorClient(conn)

	// Create a test PublishRequest
	pubReq := &proto.PublishRequest{
		Publications: []*proto.StorableMsg{
			{
				GameObject: &proto.GameObjectMsg{
					Guid: 12345,
					X:    10,
					Y:    20,
					W:    30,
					H:    40,
				},
				Ttl: 10000,
			},
		},
	}

	// Call the Publish method
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()

	pubResp, err := client.Publish(ctx, pubReq)
	assert.NoError(t, err)
	assert.NotNil(t, pubResp)
}

func TestSubscribe(t *testing.T) {
	// Set up a test gRPC server
	lis, err := net.Listen("tcp", ":0")
	assert.NoError(t, err)
	defer lis.Close()

	grpcServer := grpc.NewServer()
	testServer := &TestLocatorServer{
		locator: MakeLocatorServer(),
	}
	proto.RegisterLocatorServer(grpcServer, testServer)

	go grpcServer.Serve(lis)
	defer grpcServer.Stop()

	// Set up a test gRPC client
	conn, err := grpc.NewClient(lis.Addr().String(), grpc.WithInsecure())
	assert.NoError(t, err)
	defer conn.Close()

	client := proto.NewLocatorClient(conn)

	// Create a test SubscribeRequest
	subReq := &proto.SubscribeRequest{
		Subscriptions: []*proto.StorableMsg{
			{
				GameObject: &proto.GameObjectMsg{
					Guid: 12345,
					X:    10,
					Y:    20,
					W:    30,
					H:    40,
				},
				Ttl: 10000,
			},
		},
	}

	// Call the Subscribe method
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()

	subResp, err := client.Subscribe(ctx, subReq)
	assert.NoError(t, err)
	assert.NotNil(t, subResp)
}

// Test subscribe returns previously published objects
func TestSubscribeReturnsPublishedObjects(t *testing.T) {
	// Set up a test gRPC server
	lis, err := net.Listen("tcp", ":0")
	assert.NoError(t, err)
	defer lis.Close()

	grpcServer := grpc.NewServer()
	testServer := &TestLocatorServer{
		locator: MakeLocatorServer(),
	}
	proto.RegisterLocatorServer(grpcServer, testServer)

	go grpcServer.Serve(lis)
	defer grpcServer.Stop()

	// Set up a test gRPC client
	conn, err := grpc.NewClient(lis.Addr().String(), grpc.WithInsecure())
	assert.NoError(t, err)
	defer conn.Close()

	client := proto.NewLocatorClient(conn)

	// Create a test PublishRequest
	pubReq := &proto.PublishRequest{
		Publications: []*proto.StorableMsg{},
	}

	for i := 0; i < 10; i++ {
		pubReq.Publications = append(pubReq.Publications, &proto.StorableMsg{
			GameObject: &proto.GameObjectMsg{
				Guid: 12345 + uint32(i),
				Sid:  1,
				X:    10,
				Y:    20,
				W:    4,
				H:    4,
			},
			Ttl: 1000010,
		})
	}

	pubReq.Publications = append(pubReq.Publications, &proto.StorableMsg{
		GameObject: &proto.GameObjectMsg{
			Guid: 12356,
			Sid:  1,
			X:    41,
			Y:    20,
			W:    4,
			H:    4,
		},
		Ttl: 1000110,
	})

	// Call the Publish method
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	pubResp, err := client.Publish(ctx, pubReq)
	assert.NoError(t, err)
	assert.NotNil(t, pubResp)

	// Create a test SubscribeRequest
	subReq := &proto.SubscribeRequest{
		Subscriptions: []*proto.StorableMsg{
			{
				GameObject: &proto.GameObjectMsg{
					Guid: 123,
					Sid:  2,
					X:    10,
					Y:    20,
					W:    30,
					H:    40,
				},
				Ttl: 10000,
			},
		},
	}

	// Call the Subscribe method
	subResp, err := client.Subscribe(ctx, subReq)
	assert.NoError(t, err)
	assert.NotNil(t, subResp)
	assert.Len(t, subResp.Publications, 10)
	assert.Equal(t, uint32(12345), subResp.Publications[0].Guid)
	assert.Equal(t, int32(1), subResp.Publications[0].Sid)
}

// Test cleanup removes expired objects
func TestCleanup(t *testing.T) {
	locator := MakeLocatorServer()

	for i := 0; i < 10; i++ {
		locator.root.Insert(Storable{
			rect: game.Rectangle{X: float32(7 * i), Y: float32(7 * i), Width: 5, Height: 5},
			sid:  int32(i),
			guid: uint32(i),
			ttl:  100000,
		}, locator)
	}

	locator.root.mu.Lock()
	assert.Len(t, locator.root.store, 4)
	locator.root.mu.Unlock()

	locator.mu.Lock()
	locator.lastUpdates[1] = time.Now().Add(time.Second).UnixNano()
	locator.lastUpdates[2] = time.Now().Add(time.Second).UnixNano()
	locator.lastUpdates[3] = time.Now().Add(time.Second).UnixNano()
	locator.mu.Unlock()

	locator.root.Cleanup(locator)

	locator.root.mu.Lock()
	assert.Len(t, locator.root.store, 1)
	locator.root.mu.Unlock()

	locator.mu.Lock()
	for i := 0; i < 10; i++ {
		locator.lastUpdates[uint32(i)] = time.Now().Add(time.Second).UnixNano()
	}
	locator.mu.Unlock()

	locator.root.Cleanup(locator)

	locator.root.mu.Lock()
	assert.Len(t, locator.root.store, 0)
	assert.Equal(t, false, locator.root.divided)
	locator.root.mu.Unlock()
}

// TestInsert tests the Insert method of the Quadtree
func TestInsert(t *testing.T) {
	lastUpdates := map[uint32]int64{}
	boundary := game.Rectangle{X: 0, Y: 0, Width: 100, Height: 100}
	qt := NewQuadtree(boundary, 4)
	locator := &Locator{root: qt, lastUpdates: lastUpdates}

	rect := game.Rectangle{X: 10, Y: 10, Width: 20, Height: 20}
	storable := Storable{rect, 0, 0, 0, false}
	if !qt.Insert(storable, locator) {
		t.Errorf("Failed to insert rectangle: %v", rect)
	}

	if len(qt.store) != 1 {
		t.Errorf("Expected 1 rectangle, got %d", len(qt.store))
	}
}

// TestQuery tests the Query method of the Quadtree
func TestQuery(t *testing.T) {
	lastUpdates := map[uint32]int64{}
	boundary := game.Rectangle{X: 0, Y: 0, Width: 100, Height: 100}
	qt := NewQuadtree(boundary, 4)
	locator := &Locator{root: qt, lastUpdates: lastUpdates}

	rects := []game.Rectangle{}
	rects = append(rects, game.Rectangle{X: 10, Y: 10, Width: 20, Height: 20})
	rects = append(rects, game.Rectangle{X: 30, Y: 30, Width: 20, Height: 20})
	rects = append(rects, game.Rectangle{X: 50, Y: 50, Width: 20, Height: 20})
	rects = append(rects, game.Rectangle{X: 70, Y: 70, Width: 20, Height: 20})

	for i, rect := range rects {
		storable := Storable{rect, 0, uint32(i), 50000, false}
		qt.Insert(storable, locator)
	}

	rangeRect := game.Rectangle{X: 25, Y: 25, Width: 30, Height: 30}
	query := Storable{rangeRect, 1, 1, 0, true}

	found := []Storable{}
	qt.Query(query, &found, locator)

	expectedCount := 3
	if len(found) != expectedCount {
		t.Errorf("Expected %d rectangles, got %d", expectedCount, len(found))
	}

	rangeRect = game.Rectangle{X: 0, Y: 0, Width: 5, Height: 5}
	query = Storable{rangeRect, 1, 1, 0, true}
	found = []Storable{}
	qt.Query(query, &found, locator)

	expectedCount = 0
	if len(found) != expectedCount {
		t.Errorf("Expected %d rectangles, got %d", expectedCount, len(found))
	}

	rangeRect = game.Rectangle{X: 0, Y: 0, Width: 100, Height: 100}
	query = Storable{rangeRect, 1, 1, 0, true}
	found = []Storable{}
	qt.Query(query, &found, locator)

	expectedCount = 4
	if len(found) != expectedCount {
		t.Errorf("Expected %d rectangles, got %d", expectedCount, len(found))
	}
}

// TestExpire tests that storables with short TTL are removed from the Quadtree
func TestExpire(t *testing.T) {
	lastUpdates := map[uint32]int64{}
	boundary := game.Rectangle{X: 0, Y: 0, Width: 100, Height: 100}
	qt := NewQuadtree(boundary, 4)
	locator := &Locator{root: qt, lastUpdates: lastUpdates}

	rect := game.Rectangle{X: 10, Y: 10, Width: 20, Height: 20}
	storable := Storable{rect, 0, 0, 0, false}
	qt.Insert(storable, locator)

	if len(qt.store) != 1 {
		t.Errorf("Expected 1 rectangle, got %d", len(qt.store))
	}

	time.Sleep(100 * time.Millisecond)

	rangeRect := game.Rectangle{X: 10, Y: 10, Width: 20, Height: 20}
	query := Storable{rangeRect, 1, 1, 0, true}
	found := []Storable{}
	qt.Query(query, &found, locator)

	expectedCount := 0
	if len(found) != expectedCount {
		t.Errorf("Expected %d rectangles, got %d", expectedCount, len(found))
	}
}
