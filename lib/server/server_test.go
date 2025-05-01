package server

import (
	"context"
	"net"
	"testing"

	"juneyoo/distgamelib/lib/game"
	"juneyoo/distgamelib/lib/server/proto"

	"github.com/stretchr/testify/assert"
	"google.golang.org/grpc"
)

type TestColyseusServer struct {
	proto.UnimplementedColyseusServer
	col *Colyseus
}

func (s *TestColyseusServer) Spawn(ctx context.Context, req *proto.SpawnRequest) (*proto.SpawnResponse, error) {
	return s.col.Spawn(ctx, req)
}

func (s *TestColyseusServer) RegisterInterest(ctx context.Context, req *proto.RegisterInterestRequest) (*proto.RegisterInterestResponse, error) {
	return s.col.RegisterInterest(ctx, req)
}

func (s *TestColyseusServer) Update(ctx context.Context, req *proto.UpdateRequest) (*proto.UpdateResponse, error) {
	return s.col.Update(ctx, req)
}

func (s *TestColyseusServer) NotifySubscriber(ctx context.Context, req *proto.NotifySubscriberRequest) (*proto.NotifySubscriberResponse, error) {
	return s.col.NotifySubscriber(ctx, req)
}

func TestSpawn(t *testing.T) {
	// Set up a test gRPC server
	lis, err := net.Listen("tcp", ":0")
	assert.NoError(t, err)
	defer lis.Close()

	grpcServer := grpc.NewServer()
	testServer := &TestColyseusServer{
		col: MakeColyseusServer(0, true, 10, 500, 100, 1000, 2000, 10, nil),
	}
	proto.RegisterColyseusServer(grpcServer, testServer)

	go grpcServer.Serve(lis)
	defer grpcServer.Stop()

	// Set up a test gRPC client
	conn, err := grpc.Dial(lis.Addr().String(), grpc.WithInsecure())
	assert.NoError(t, err)
	defer conn.Close()

	client := proto.NewColyseusClient(conn)

	// Create a test SpawnRequest
	spawnReq := &proto.SpawnRequest{
		GameObject: &proto.GameObject{
			X:          10,
			Y:          20,
			W:          30,
			H:          40,
			ObjectType: game.Player,
		},
	}

	// Call the Spawn method
	resp, err := client.Spawn(context.Background(), spawnReq)
	assert.NoError(t, err)
	assert.NotNil(t, resp)
	assert.NotEqual(t, 0, resp.Guid)
}

func TestUpdate(t *testing.T) {
	// Set up a test gRPC server
	lis, err := net.Listen("tcp", ":0")
	assert.NoError(t, err)
	defer lis.Close()

	grpcServer := grpc.NewServer()
	testServer := &TestColyseusServer{
		col: MakeColyseusServer(0, true, 10, 500, 100, 1000, 2000, 10, nil),
	}
	proto.RegisterColyseusServer(grpcServer, testServer)

	go grpcServer.Serve(lis)
	defer grpcServer.Stop()

	// Set up a test gRPC client
	conn, err := grpc.Dial(lis.Addr().String(), grpc.WithInsecure())
	assert.NoError(t, err)
	defer conn.Close()

	client := proto.NewColyseusClient(conn)

	// Create a test UpdateRequest
	updateReq := &proto.UpdateRequest{
		TargetGuid: 12345,
		Update: &proto.GameEvent{
			Input:     0,
			Timestamp: 0,
		},
	}

	// Call the Update method
	resp, err := client.Update(context.Background(), updateReq)
	assert.NoError(t, err)
	assert.NotNil(t, resp)
}

func TestRegisterInterest(t *testing.T) {
	// Set up a test gRPC server
	lis, err := net.Listen("tcp", ":0")
	assert.NoError(t, err)
	defer lis.Close()

	grpcServer := grpc.NewServer()
	testServer := &TestColyseusServer{
		col: MakeColyseusServer(0, true, 10, 500, 100, 1000, 2000, 10, nil),
	}
	proto.RegisterColyseusServer(grpcServer, testServer)

	go grpcServer.Serve(lis)
	defer grpcServer.Stop()

	// Set up a test gRPC client
	conn, err := grpc.Dial(lis.Addr().String(), grpc.WithInsecure())
	assert.NoError(t, err)
	defer conn.Close()

	client := proto.NewColyseusClient(conn)

	// Create a test RegisterInterestRequest
	regReq := &proto.RegisterInterestRequest{
		TargetGuids: []uint32{12345},
		Sid:         0,
	}

	// Call the RegisterInterest method
	resp, err := client.RegisterInterest(context.Background(), regReq)
	assert.NoError(t, err)
	assert.NotNil(t, resp)
}

func TestNotifySubscriber(t *testing.T) {
	// Set up a test gRPC server
	lis, err := net.Listen("tcp", ":0")
	assert.NoError(t, err)
	defer lis.Close()

	grpcServer := grpc.NewServer()
	testServer := &TestColyseusServer{
		col: MakeColyseusServer(0, true, 10, 500, 100, 1000, 2000, 10, nil),
	}
	proto.RegisterColyseusServer(grpcServer, testServer)

	go grpcServer.Serve(lis)
	defer grpcServer.Stop()

	// Set up a test gRPC client
	conn, err := grpc.Dial(lis.Addr().String(), grpc.WithInsecure())
	assert.NoError(t, err)
	defer conn.Close()

	client := proto.NewColyseusClient(conn)

	// Create a test NotifySubscriberRequest
	notifyReq := &proto.NotifySubscriberRequest{
		Publications: []*proto.MatchMsg{},
	}

	// Call the NotifySubscriber method
	resp, err := client.NotifySubscriber(context.Background(), notifyReq)
	assert.NoError(t, err)
	assert.NotNil(t, resp)
}

// Test spawning an object, registering interest in it from a second client, and updating the client
// from the first server. Both clients should see the update.
func TestSpawnRegisterInterestUpdate(t *testing.T) {
	// Set up a test gRPC server
	lis, err := net.Listen("tcp", ":0")
	assert.NoError(t, err)
	defer lis.Close()

	grpcServer := grpc.NewServer()
	testServer := &TestColyseusServer{
		col: MakeColyseusServer(0, true, 10, 500, 100, 1000, 2000, 10, nil),
	}
	proto.RegisterColyseusServer(grpcServer, testServer)

	go grpcServer.Serve(lis)
	defer grpcServer.Stop()

	// Set up a test gRPC client
	conn, err := grpc.Dial(lis.Addr().String(), grpc.WithInsecure())
	assert.NoError(t, err)
	defer conn.Close()

	client := proto.NewColyseusClient(conn)

	// Create a test SpawnRequest
	spawnReq := &proto.SpawnRequest{
		GameObject: &proto.GameObject{
			X:          10,
			Y:          20,
			W:          30,
			H:          40,
			ObjectType: game.Player,
		},
	}

	// Call the Spawn method
	resp, err := client.Spawn(context.Background(), spawnReq)
	assert.NoError(t, err)
	assert.NotNil(t, resp)
	assert.NotEqual(t, 0, resp.Guid)
	assert.Equal(t, 1, len(testServer.col.Primaries))

	// Create a test UpdateRequest
	updateReq := &proto.UpdateRequest{
		TargetGuid: resp.Guid,
		Update: &proto.GameEvent{
			Input:     42,
			Timestamp: 100,
		},
	}

	// Call the Update method
	updateResp, err := testServer.col.Update(context.Background(), updateReq)
	assert.NoError(t, err)
	assert.NotNil(t, updateResp)

	assert.Equal(t, 1, len(testServer.col.Primaries))
	assert.Equal(t, int32(42), testServer.col.Primaries[resp.Guid].LatestEvent.Input)
	assert.NotEqual(t, int64(100), testServer.col.Primaries[resp.Guid].LatestEvent.Timestamp)
	// Set up a second test client

	client2 := proto.NewColyseusClient(conn)

	// Create a test RegisterInterestRequest
	regReq := &proto.RegisterInterestRequest{
		TargetGuids: []uint32{resp.Guid},
		Sid:         1,
	}

	// Call the RegisterInterest method
	regResp, err := client2.RegisterInterest(context.Background(), regReq)
	assert.NoError(t, err)
	assert.NotNil(t, regResp)
	assert.Equal(t, int32(42), regResp.LatestUpdates[0].Input)

	updateReq = &proto.UpdateRequest{
		TargetGuid: resp.Guid,
		Update: &proto.GameEvent{
			Input:     43,
			Timestamp: 100,
		},
	}

	updateResp, err = testServer.col.Update(context.Background(), updateReq)
	assert.NoError(t, err)
	assert.NotNil(t, updateResp)
	assert.Equal(t, 1, len(testServer.col.Primaries))
	assert.Equal(t, int32(43), testServer.col.Primaries[resp.Guid].LatestEvent.Input)
	assert.Equal(t, int32(42), regResp.LatestUpdates[0].Input)
}
