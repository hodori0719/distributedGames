package main

// interactive client which allow user to type in inputs which are sent to the grpc
// server at localhost:port

import (
	"context"
	"flag"
	"fmt"
	"log"
	"math/rand"
	"sort"
	"sync"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"

	"juneyoo/distgamelib/lib/game"
	"juneyoo/distgamelib/lib/server/proto"
)

var (
	port          = flag.Int("port", 9000, "The base server port")
	numServers    = flag.Int("num-servers", 1, "Number of servers to connect to")
	numPlayers    = flag.Int("num-players", 1, "Number of players to create")
	latencyBuffer = make([]time.Duration, 0, 5000000)
	mu            sync.Mutex
)

func recordLatency(d time.Duration) {
	mu.Lock()
	defer mu.Unlock()
	if len(latencyBuffer) < 5000000 {
		latencyBuffer = append(latencyBuffer, d)
	} else {
		latencyBuffer = append(latencyBuffer[1:], d) // Remove the oldest and add the newest
	}
}

func calculateAverage() time.Duration {
	mu.Lock()
	defer mu.Unlock()

	if len(latencyBuffer) == 0 {
		return 0
	}

	var total time.Duration
	for _, latency := range latencyBuffer {
		total += latency
	}
	return total / time.Duration(len(latencyBuffer))
}

func calculatePercentile(percentile float64) time.Duration {
	mu.Lock()
	defer mu.Unlock()

	if len(latencyBuffer) == 0 {
		return 0
	}

	sorted := make([]time.Duration, len(latencyBuffer))
	copy(sorted, latencyBuffer)
	sort.Slice(sorted, func(i, j int) bool {
		return sorted[i] < sorted[j]
	})

	index := int(float64(len(sorted)) * percentile)
	if index >= len(sorted) {
		index = len(sorted) - 1
	}
	return sorted[index]
}

// Set up a connection to the server.
func main() {
	createPlayer := func(servPort int) {
		flag.Parse()
		// Set up a connection to the server.
		log.Printf("Connecting to server on port %d", servPort)
		addr := fmt.Sprintf("localhost:%d", servPort)
		conn, err := grpc.NewClient(addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
		if err != nil {
			log.Fatalf("did not connect: %v", err)
		}
		defer conn.Close()
		c := proto.NewColyseusClient(conn)

		// create a local avatar
		avatar := game.GameObject{
			Rect: game.Rectangle{
				X:      91*rand.Float32() + 5,
				Y:      91*rand.Float32() + 5,
				Width:  2,
				Height: 2,
			},
			Type: game.Player,
		}

		// ask the server to spawn the avatar
		obj, err := c.Spawn(context.Background(), &proto.SpawnRequest{GameObject: &proto.GameObject{
			X:          avatar.Rect.X,
			Y:          avatar.Rect.Y,
			W:          avatar.Rect.Width,
			H:          avatar.Rect.Height,
			ObjectType: avatar.Type,
		}})

		guid := obj.Guid

		if err != nil {
			log.Fatalf("could not spawn: %v", err)
		}

		// reader := bufio.NewReader(os.Stdin)

		// input loop
		for {
			/*
				MANUAL INPUT CODE

				fmt.Print("Enter command (7-bit int): ")
				text, _ := reader.ReadString('\n')
				text = strings.Replace(text, "\n", "", -1)
				if text == "exit" {
					break
				}
				input, err := strconv.Atoi(text)
				if err != nil {
					log.Fatalf("could not convert input to int: %v", err)
				}
			*/
			time.Sleep(500 * time.Millisecond)
			input := rand.Intn(16)
			input = input & 0x7f
			start := time.Now()
			_, err = c.Update(context.Background(), &proto.UpdateRequest{
				TargetGuid: guid,
				Update: &proto.GameEvent{
					Input:     int32(input),
					Timestamp: 0,
				},
			})
			duration := time.Since(start)
			recordLatency(duration)

			if err != nil {
				log.Fatalf("could not greet: %v", err)
			}
		}
	}

	for i := 0; i < *numServers; i++ {
		for j := 0; j < *numPlayers / *numServers; j++ {
			go createPlayer(*port + i)
		}
	}

	for {
		time.Sleep(2 * time.Second)
		avgDuration := calculateAverage()
		p99 := calculatePercentile(0.99)
		log.Printf("Average latency (last %v): %v", len(latencyBuffer), avgDuration)
		log.Printf("99th percentile latency (last 50k): %v", p99)
	}
}
