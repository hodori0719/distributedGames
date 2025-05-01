package server

import (
	"context"
	"sync"
	"time"

	"juneyoo/distgamelib/lib/game"
	locator "juneyoo/distgamelib/lib/locator/proto"
	"juneyoo/distgamelib/lib/server/proto"

	"github.com/sirupsen/logrus"

	// replaceable with "github.com/sssgun/grpc-quic" for UDP support
	// however google's grpc is moving to quic natively soon
	// and the above does not seem to be stable.
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

type Interest struct {
	sid int32
	ttl int64
}

type Colyseus struct {
	proto.UnimplementedColyseusServer
	// grpc endpoint to locator
	locatorEndpoint locator.LocatorClient
	sid             int32
	// grpc endpoints to peers
	peerEndpoints   map[int32]proto.ColyseusClient
	Primaries       map[uint32]*game.GameObject
	Replicas        map[uint32]*game.GameObject
	primaryInterest map[uint32][]Interest

	// per-server state
	proactiveReplication bool
	frameRate            int
	predTimeMs           int64
	pubTimeMs            int64
	cleanupTimeMs        int64
	interestTTLMs        int64
	testDelayMs          int64
	logrusInst           *logrus.Entry

	mu sync.RWMutex
}

func MakeColyseusServer(
	sid int32,
	proactiveReplication bool,
	frameRate int,
	predTimeMs int64,
	pubTimeMs int64,
	cleanupTimeMs int64,
	interestTTLMs int64,
	testDelayMs int64,
	logrusInst *logrus.Entry,
) *Colyseus {
	server := &Colyseus{
		peerEndpoints:        make(map[int32]proto.ColyseusClient),
		sid:                  sid,
		Primaries:            make(map[uint32]*game.GameObject),
		Replicas:             make(map[uint32]*game.GameObject),
		primaryInterest:      make(map[uint32][]Interest),
		proactiveReplication: proactiveReplication,
		frameRate:            frameRate,
		predTimeMs:           predTimeMs,
		pubTimeMs:            pubTimeMs,
		cleanupTimeMs:        cleanupTimeMs,
		interestTTLMs:        interestTTLMs,
		testDelayMs:          testDelayMs,
		logrusInst:           logrusInst,
	}
	go server.PublishLoop()
	go server.CleanupLoop()

	return server
}

// Called by the application to advance the game state
func (s *Colyseus) AdvanceGameState(updateGuids []uint32, updates []int32) {
	for i, guid := range updateGuids {
		go func() {
			s.mu.Lock()
			s.HandleUpdate(guid, updates[i])
			s.mu.Unlock()
		}()
	}
}

func (s *Colyseus) Lock() {
	s.mu.Lock()
}

func (s *Colyseus) Unlock() {
	s.mu.Unlock()
}

// Continuously publish area-of-interest subscriptions and object locations to the locator
func (s *Colyseus) PublishLoop() {
	for {
		time.Sleep(time.Duration(s.predTimeMs) * time.Millisecond / 2)

		locations := []*locator.StorableMsg{}
		areaOfInterest := []*locator.StorableMsg{}
		s.mu.Lock()
		for _, primary := range s.Primaries {
			loc := primary.GetLocation()
			locations = append(locations, &locator.StorableMsg{
				GameObject: &locator.GameObjectMsg{
					Guid: primary.Guid,
					Sid:  s.sid,
					X:    loc.X,
					Y:    loc.Y,
					W:    loc.Width,
					H:    loc.Height,
				},
				Ttl: s.predTimeMs,
			})

			aoi := primary.GetInterest(s.predTimeMs, s.pubTimeMs)
			areaOfInterest = append(areaOfInterest, &locator.StorableMsg{
				GameObject: &locator.GameObjectMsg{
					Guid: primary.Guid,
					Sid:  s.sid,
					X:    aoi.X,
					Y:    aoi.Y,
					W:    aoi.Width,
					H:    aoi.Height,
				},
				Ttl: s.predTimeMs,
			})
		}
		s.mu.Unlock()

		// publish our primaries' locations to the locator
		if s.locatorEndpoint != nil {
			go func() {
				s.logrusInst.Trace("Publishing to locator")
				_, err := s.locatorEndpoint.Publish(context.Background(), &locator.PublishRequest{
					Publications: locations,
				})
				if err != nil {
					s.logrusInst.WithError(err).Error("Failed to publish to locator")
				}
			}()

			// subscribe to locator using our combined area of interest
			go func() {
				s.logrusInst.Trace("Subscribing to locator")
				_, err := s.locatorEndpoint.Subscribe(context.Background(), &locator.SubscribeRequest{
					Subscriptions: areaOfInterest,
				})
				if err != nil {
					s.logrusInst.WithError(err).Error("Failed to subscribe to locator")
				}
			}()
		}
	}
}

// assumes caller holds the lock
func (s *Colyseus) isInterestedIn(gameObject *game.GameObject) bool {
	for _, primary := range s.Primaries {
		aoi := primary.GetInterest(s.predTimeMs, s.pubTimeMs)
		rect := gameObject.GetLocation()
		if rect.Intersects(aoi) {
			return true
		}
	}
	return false
}

// Continuously Replicas we are no longer interested in, and refresh interest in the others
func (s *Colyseus) CleanupLoop() {
	for {
		time.Sleep(time.Duration(s.cleanupTimeMs) * time.Millisecond)
		s.mu.Lock()
		stillInterestedGuidsBySid := make(map[int32][]uint32)
		for _, replica := range s.Replicas {
			isInterested := s.isInterestedIn(replica)
			if !isInterested {
				s.logrusInst.Debugf("Cleaning up replica %d", replica.Guid)
				delete(s.Replicas, replica.Guid)
			} else {
				if _, ok := stillInterestedGuidsBySid[replica.Sid]; !ok {
					stillInterestedGuidsBySid[replica.Sid] = make([]uint32, 0)
				}
				stillInterestedGuidsBySid[replica.Sid] = append(stillInterestedGuidsBySid[replica.Sid], replica.Guid)
			}
		}
		s.mu.Unlock()

		go func() {
			for sid, stillInterestedGuids := range stillInterestedGuidsBySid {
				if peer, ok := s.peerEndpoints[sid]; ok {
					s.logrusInst.Debugf("Refreshing interest for %d during cleanup", sid)
					_, err := peer.RegisterInterest(context.Background(), &proto.RegisterInterestRequest{
						TargetGuids: stillInterestedGuids,
						Sid:         s.sid,
						IsRenewal:   true,
					})
					if err != nil {
						s.logrusInst.WithError(err).Error("Failed to register interest")
					}
				}
			}
		}()
	}
}

func MakeConnection(addr string) (*grpc.ClientConn, error) {
	var opts []grpc.DialOption
	opts = append(opts, grpc.WithTransportCredentials(insecure.NewCredentials()))

	channel, err := grpc.NewClient(addr, opts...)
	if err != nil {
		return nil, err
	}
	return channel, nil
}

func (s *Colyseus) ConnectToLocator(addr string) {
	s.logrusInst.Trace("Connecting to locator")

	client, err := MakeConnection(addr)
	if err != nil {
		s.logrusInst.WithError(err).Error("Failed to connect to locator")
		return
	}

	s.mu.Lock()
	s.locatorEndpoint = locator.NewLocatorClient(client)
	s.mu.Unlock()
	s.logrusInst.Trace("Connected to locator")
}

func (s *Colyseus) ConnectToPeer(sid int32, addr string) {
	s.logrusInst.WithField("peerSid", sid).Trace("Connecting to peer")
	client, err := MakeConnection(addr)
	if err != nil {
		s.logrusInst.WithError(err).Error("Failed to connect to locator")
		return
	}

	s.mu.Lock()
	s.peerEndpoints[sid] = proto.NewColyseusClient(client)
	s.mu.Unlock()
	s.logrusInst.WithField("peerSid", sid).Trace("Connected to peer")
}

// SpawnObject adds a new object to the game state; assumes the caller holds the lock
func (s *Colyseus) SpawnObject(gameObject *game.GameObject) uint32 {
	guid := game.GenerateGUID()
	gameObject.Guid = guid
	s.Primaries[guid] = gameObject
	s.primaryInterest[guid] = make([]Interest, 0)
	s.logrusInst.WithField("guid", guid).Trace("Created primary")

	return guid
}

func (s *Colyseus) DeleteObject(guid uint32) {
	delete(s.Primaries, guid)
	delete(s.Replicas, guid)
	delete(s.primaryInterest, guid)
}

// Receive request to spawn a new object
func (s *Colyseus) Spawn(ctx context.Context, request *proto.SpawnRequest) (*proto.SpawnResponse, error) {
	s.logrusInst.WithField("guid", request.GameObject.Guid).Trace("Received spawn request")
	time.Sleep(time.Duration(s.testDelayMs) * time.Millisecond) // SIMULATE LATENCY
	s.mu.Lock()
	defer s.mu.Unlock()
	serializedTimestamp := time.Now().UnixMilli()

	guid := s.SpawnObject(&game.GameObject{
		Sid: s.sid,
		Rect: game.Rectangle{
			X:      request.GameObject.X,
			Y:      request.GameObject.Y,
			Width:  request.GameObject.W,
			Height: request.GameObject.H,
		},
		Type: request.GameObject.ObjectType,
		LatestEvent: game.GameEvent{
			Input:     0,
			Timestamp: serializedTimestamp,
		},
	})
	return &proto.SpawnResponse{
		Guid: guid,
	}, nil
}

// Receive request to register interest in one of our Primaries
// If the request is a renewal, we don't need to send back the initial state, just update TTL
func (s *Colyseus) RegisterInterest(ctx context.Context, request *proto.RegisterInterestRequest) (*proto.RegisterInterestResponse, error) {
	s.logrusInst.WithField("fromSid", request.Sid).Trace("Received register interest request")
	time.Sleep(time.Duration(s.testDelayMs) * time.Millisecond) // SIMULATE LATENCY
	s.mu.Lock()
	defer s.mu.Unlock()

	initialStates := make([]*proto.GameObject, 0)
	latestUpdates := make([]*proto.GameEvent, 0)

	for _, guid := range request.TargetGuids {
		if primary, ok := s.Primaries[guid]; ok {
			// Send back the initial state of the object to the interested server
			initialStates = append(initialStates, &proto.GameObject{
				Guid:       primary.Guid,
				X:          primary.Rect.X,
				Y:          primary.Rect.Y,
				W:          primary.Rect.Width,
				H:          primary.Rect.Height,
				ObjectType: primary.Type,
			})
			latestUpdates = append(latestUpdates, &proto.GameEvent{
				Input:     primary.LatestEvent.Input,
				Timestamp: primary.LatestEvent.Timestamp,
			})
			// Mark the server as interested in this object
			s.primaryInterest[guid] = append(s.primaryInterest[guid], Interest{
				sid: request.Sid,
				ttl: time.Now().UnixMilli() + s.interestTTLMs,
			})
		}
	}

	return &proto.RegisterInterestResponse{
		InitialStates: initialStates,
		LatestUpdates: latestUpdates,
	}, nil
}

// handle update logic ORIGINATING at this node (assumes caller holds lock)
func (s *Colyseus) HandleUpdate(guid uint32, input int32) {
	s.logrusInst.WithFields(logrus.Fields{"guid": guid, "input": input}).Trace("Handling update")
	serializedTimestamp := time.Now().UnixMilli()

	if primary, ok := s.Primaries[guid]; ok {
		s.logrusInst.WithField("guid", guid).Trace("Handling update at primary")

		reactions := make([]game.GameEvent, 0)
		createdObjects := make([]game.GameObject, 0)
		deletedObjects := make([]uint32, 0)
		s.logrusInst.WithField("event", input&game.Shoot).Trace("Applying event to primary")
		s.logrusInst.WithField("prev", primary.LatestEvent.Timestamp).WithField("this", serializedTimestamp).WithField("diff", serializedTimestamp-primary.LatestEvent.Timestamp).Trace("With timestamps")
		primary.ApplyEvent(game.GameEvent{
			Input:     input,
			Timestamp: serializedTimestamp,
		}, &reactions, &createdObjects, &deletedObjects)
		s.logrusInst.WithField("deleted", deletedObjects).Infof("Deleted %d primary objects", len(deletedObjects))

		// whenever a primary object is updated, we need to replicate the update to all interested servers
		// as well as any attached objects if proactive replication is enabled
		initialStates := make([]*proto.GameObject, 0)
		latestUpdates := make([]*proto.GameEvent, 0)
		for _, createdObject := range createdObjects {
			guid := s.SpawnObject(&createdObject)
			if s.proactiveReplication {
				initialStates = append(initialStates, &proto.GameObject{
					Guid:       guid,
					X:          createdObject.Rect.X,
					Y:          createdObject.Rect.Y,
					W:          createdObject.Rect.Width,
					H:          createdObject.Rect.Height,
					ObjectType: createdObject.Type,
				})
				latestUpdates = append(latestUpdates, &proto.GameEvent{
					Input:     createdObject.LatestEvent.Input,
					Timestamp: createdObject.LatestEvent.Timestamp,
				})
			}
		}

		for _, interestedServer := range s.primaryInterest[guid] {
			if interestedServer.sid == s.sid || interestedServer.ttl < time.Now().UnixMilli() {
				// TODO: in theory, we should remove the interest marker completely here,
				// but we'll just let it stay as expired for now
				continue
			}
			if peer, ok := s.peerEndpoints[interestedServer.sid]; ok {
				go func() {
					s.logrusInst.Infof("Propagating update to %d", peer)
					_, err := peer.Update(context.Background(), &proto.UpdateRequest{
						TargetGuid:    guid,
						Update:        &proto.GameEvent{Input: input, Timestamp: serializedTimestamp},
						InitialStates: initialStates,
						LatestUpdates: latestUpdates,
						DeletedGuids:  deletedObjects,
					})
					if err != nil {
						s.logrusInst.WithError(err).Error("Failed to update peer")
					}
				}()
			}
		}

		for _, deletedGuid := range deletedObjects {
			s.DeleteObject(deletedGuid)
		}

		// recursively handle any reactions
		for _, reaction := range reactions {
			s.HandleUpdate(guid, reaction.Input)
		}
	} else if replica, ok := s.Replicas[guid]; ok {
		// if we don't own the object, we ask the owner to update it. When the owner updates the object,
		// the update will eventually be replicated to us.
		owner := replica.Sid
		if peer, ok := s.peerEndpoints[owner]; ok {
			go func() {
				_, err := peer.Update(context.Background(), &proto.UpdateRequest{
					TargetGuid: guid,
					Update:     &proto.GameEvent{Input: input}, // no timestamp here; the primary decides when to serialize.
				})
				if err != nil {
					s.logrusInst.WithError(err).Error("Failed to update peer")
				}
			}()
		}
	}
}

// handle update logic RECEIVED at this node
func (s *Colyseus) Update(ctx context.Context, request *proto.UpdateRequest) (*proto.UpdateResponse, error) {
	time.Sleep(time.Duration(s.testDelayMs) * time.Millisecond) // SIMULATE LATENCY
	s.mu.Lock()

	if _, ok := s.Primaries[request.TargetGuid]; ok {
		s.logrusInst.WithField("guid", request.TargetGuid).Infof("Someone is asking to modify primary")
		// potential update received at the primary.
		// apply the event to the primary and replicate to all Replicas,
		// keeping track of any side effects
		s.HandleUpdate(request.TargetGuid, request.Update.Input)
		s.mu.Unlock()
	} else if replica, ok := s.Replicas[request.TargetGuid]; ok {
		s.logrusInst.WithField("guid", request.TargetGuid).Infof("Primary pushed updates to replica")
		// update received at a replica. apply the event to the replica
		// replica doesn't have permission to create objects or events; those are done by the primary
		// and will be sent to the replica in the next update. However, the replica may immediately
		// delete itself if an event calls for it.

		replica.ApplyEvent(game.GameEvent{
			Input:     request.Update.Input,
			Timestamp: request.Update.Timestamp,
		}, nil, nil, nil)

		// if proactive replication is enabled, we need to replicate the update to all interested servers
		if s.proactiveReplication {
			sid := replica.Sid

			for i, newObject := range request.InitialStates {
				// proactive replication of any new objects
				s.Replicas[newObject.Guid] = &game.GameObject{
					Guid: newObject.Guid,
					Sid:  sid,
					Rect: game.Rectangle{
						X:      newObject.X,
						Y:      newObject.Y,
						Width:  newObject.W,
						Height: newObject.H,
					},
					Type: newObject.ObjectType,
					LatestEvent: game.GameEvent{
						Input:     request.LatestUpdates[i].Input,
						Timestamp: request.LatestUpdates[i].Timestamp,
					},
				}
				// we naively store a replica of this for now. this will either get registered
				// or cleaned up in the next cleanup loop
			}
		}

		for _, deletedGuid := range request.DeletedGuids {
			s.DeleteObject(deletedGuid)
		}

		s.mu.Unlock()
	} else {
		s.mu.Unlock()
	}

	s.logrusInst.WithField("guid", request.TargetGuid).Trace("Handled update")
	return &proto.UpdateResponse{}, nil
}

// NotifySubscriber is called by the locator to notify us of new publications
func (s *Colyseus) NotifySubscriber(ctx context.Context, request *proto.NotifySubscriberRequest) (*proto.NotifySubscriberResponse, error) {
	time.Sleep(time.Duration(s.testDelayMs) * time.Millisecond) // SIMULATE LATENCY
	neededObjectsBySid := make(map[int32][]uint32)

	s.mu.Lock()
	for _, match := range request.Publications {
		if match.Sid == s.sid {
			s.logrusInst.Debugf("Received own publication from sid %d", match.Sid)
		} else {
			if _, ok := s.Replicas[match.Guid]; ok {
				// already have a replica of this object
				continue
			} else {
				if _, ok := neededObjectsBySid[match.Sid]; !ok {
					neededObjectsBySid[match.Sid] = make([]uint32, 0)
				}
				neededObjectsBySid[match.Sid] = append(neededObjectsBySid[match.Sid], match.Guid)
			}
		}
	}
	s.mu.Unlock()

	for sid, neededObjects := range neededObjectsBySid {
		if peer, ok := s.peerEndpoints[sid]; ok {
			resp, err := peer.RegisterInterest(context.Background(), &proto.RegisterInterestRequest{
				Sid:         s.sid,
				TargetGuids: neededObjects,
			})
			if err != nil {
				s.logrusInst.WithError(err).Error("Failed to register interest")
			} else {
				s.mu.Lock()
				for i, initialState := range resp.InitialStates {
					s.Replicas[initialState.Guid] = &game.GameObject{
						Guid: initialState.Guid,
						Sid:  sid,
						Rect: game.Rectangle{
							X:      initialState.X,
							Y:      initialState.Y,
							Width:  initialState.W,
							Height: initialState.H,
						},
						Type: initialState.ObjectType,
						LatestEvent: game.GameEvent{
							Input:     resp.LatestUpdates[i].Input,
							Timestamp: resp.LatestUpdates[i].Timestamp,
						},
					}
				}
				s.mu.Unlock()
			}
		}
	}

	return &proto.NotifySubscriberResponse{}, nil
}

// Setters to modify server configuration

func (s *Colyseus) SetLocatorEndpoint(locatorEndpoint locator.LocatorClient) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.locatorEndpoint = locatorEndpoint
}

func (s *Colyseus) SetPeerEndpoint(sid int32, peerEndpoint proto.ColyseusClient) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.peerEndpoints[sid] = peerEndpoint
}

func (s *Colyseus) SetProactiveReplication(proactiveReplication bool) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.proactiveReplication = proactiveReplication
}

func (s *Colyseus) SetFrameRate(frameRate int) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.frameRate = frameRate
}

func (s *Colyseus) SetPredTimeMs(predTimeMs int64) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.predTimeMs = predTimeMs
}

func (s *Colyseus) SetPubTimeMs(pubTimeMs int64) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.pubTimeMs = pubTimeMs
}

func (s *Colyseus) SetCleanupTimeMs(cleanupTimeMs int64) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.cleanupTimeMs = cleanupTimeMs
}

func (s *Colyseus) SetInterestTTLMs(interestTTLMs int64) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.interestTTLMs = interestTTLMs
}

func (s *Colyseus) SetTestDelayMs(testDelayMs int64) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.testDelayMs = testDelayMs
}

func (s *Colyseus) SetLogrusInst(logrusInst *logrus.Entry) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.logrusInst = logrusInst
}
