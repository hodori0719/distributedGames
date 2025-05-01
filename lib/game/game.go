package game

// Simple event-based toy game with players, projectiles, and shields, and some collision detection
// Application can write in any networked game logic in this module.

import (
	"time"

	"github.com/google/uuid"
	"github.com/sirupsen/logrus"
)

const (
	Player = 1 << iota
	Projectile
	Shield
)

const (
	Up = 1 << iota
	Down
	Left
	Right
	Shoot
	ShieldUp
	Die
)

const PlayerSpeed = 0.001
const ProjectileSpeed = 0.01

const MaxX = 100
const MaxY = 100
const MinX = 0
const MinY = 0

// Rectangle represents a 2D range with a position and size
// We use screen coordinates, with the origin at the top-left corner

type Rectangle struct {
	X, Y, Width, Height float32
}

type GameEvent struct {
	Input     int32 // bitfield of input commands; refer to constants above
	Timestamp int64
}

// GameObjects in our implementation of Colyseus must implement the following interface
/* type GameObject interface {
	GetInterest()
	GetLocation()
	ApplyEvent()
} */

type GameObject struct {
	Guid        uint32
	Sid         int32
	Rect        Rectangle
	Type        int32
	LatestEvent GameEvent
}

// o.speed
// o.expansion

func GenerateGUID() uint32 {
	logrus.Trace("Generating GUID")
	return uuid.New().ID()
}

// GetInterest returns the bounding box that the object is interested in
// predTimeMs is the TTL of the prediction in milliseconds
// pubTimeMs is the estimated time it will take for the event to be published,
// effectively extending the TTL so that there is no gap in publication
func (o *GameObject) GetInterest(predTimeMs int64, pubTimeMs int64) Rectangle {
	// Expanded bounding box for interest.
	// The expansion is a configurable parameter per object type; smaller
	// means less network traffic but you can miss collisions if it is too small
	// and network conditions are bad.

	expansion := float32(1)
	switch o.Type {
	case Player:
		expansion = 3
	case Projectile:
		expansion = 2
	case Shield:
		expansion = 2
	}

	bb := o.GetLocation()

	bb.X -= expansion
	bb.Y -= expansion
	bb.Width += 2 * expansion
	bb.Height += 2 * expansion

	// We expand the bounding volume predictively, based on the object's velocity;
	// described in Section 6.2 of the paper.

	speed := float32(0)
	switch o.Type {
	case Player:
		speed = PlayerSpeed
	case Projectile:
		speed = ProjectileSpeed
	default:
		return bb
	}

	if o.LatestEvent.Input&Left != 0 {
		bb.X -= speed * float32(pubTimeMs+predTimeMs)
		bb.Width += speed * float32(pubTimeMs+predTimeMs)
	}
	if o.LatestEvent.Input&Right != 0 {
		bb.Width += speed * float32(pubTimeMs+predTimeMs)
	}
	if o.LatestEvent.Input&Up != 0 {
		bb.Y -= speed * float32(pubTimeMs+predTimeMs)
		bb.Height += speed * float32(pubTimeMs+predTimeMs)
	}
	if o.LatestEvent.Input&Down != 0 {
		bb.Height += speed * float32(pubTimeMs+predTimeMs)
	}

	return bb
}

func (o *GameObject) GetAdjustedRect(timeDelta int64) Rectangle {
	rect := Rectangle{
		X:      o.Rect.X,
		Y:      o.Rect.Y,
		Width:  o.Rect.Width,
		Height: o.Rect.Height,
	}

	speed := float32(0)
	switch o.Type {
	case Player:
		speed = PlayerSpeed
	case Projectile:
		speed = ProjectileSpeed
	default:
		return rect
	}

	if o.LatestEvent.Input&Left != 0 {
		rect.X -= speed * float32(timeDelta)
		rect.X = max(MinX, rect.X)
	}
	if o.LatestEvent.Input&Right != 0 {
		rect.X += speed * float32(timeDelta)
		rect.X = min(MaxX-rect.Width, rect.X)
	}
	if o.LatestEvent.Input&Up != 0 {
		rect.Y -= speed * float32(timeDelta)
		rect.Y = max(MinY, rect.Y)
	}
	if o.LatestEvent.Input&Down != 0 {
		rect.Y += speed * float32(timeDelta)
		rect.Y = min(MaxY-rect.Height, rect.Y)
	}

	return rect
}

func (o *GameObject) GetLocation() Rectangle {
	timeDelta := time.Now().UnixMilli() - o.LatestEvent.Timestamp

	return o.GetAdjustedRect(timeDelta)
}

func (o *GameObject) ApplyEvent(
	event GameEvent,
	reactions *[]GameEvent,
	createdObjects *[]GameObject,
	deletedObjects *[]uint32,
) {
	switch o.Type {
	case Player:
		// update object state to conclusion of last event
		timeDelta := event.Timestamp - o.LatestEvent.Timestamp

		o.Rect = o.GetAdjustedRect(timeDelta)

		o.LatestEvent = event
		// any start-of-event actions
		if event.Input&Shoot != 0 {
			// create a projectile
			if createdObjects != nil {
				*createdObjects = append(*createdObjects, GameObject{
					Guid: uuid.New().ID(),
					Sid:  o.Sid,
					Rect: Rectangle{
						X:      o.Rect.X, // weird placement, but just so they don't collide with the player
						Y:      o.Rect.Y + o.Rect.Height + 1,
						Width:  1,
						Height: 1,
					},
					Type: Projectile,
					LatestEvent: GameEvent{
						// always drop down
						Input:     Down,
						Timestamp: event.Timestamp,
					},
				})
			}
		}
	case Projectile:
		timeDelta := event.Timestamp - o.LatestEvent.Timestamp
		if o.LatestEvent.Input&Left != 0 {
			o.Rect.X -= PlayerSpeed * float32(timeDelta)
		}
		if o.LatestEvent.Input&Right != 0 {
			o.Rect.X += PlayerSpeed * float32(timeDelta)
		}
		if o.LatestEvent.Input&Up != 0 {
			o.Rect.Y -= PlayerSpeed * float32(timeDelta)
		}
		if o.LatestEvent.Input&Down != 0 {
			o.Rect.Y += PlayerSpeed * float32(timeDelta)
		}
		o.LatestEvent = event
	case Shield:
		break
	}

	if event.Input&Die != 0 {
		if deletedObjects != nil {
			*deletedObjects = append(*deletedObjects, o.Guid)
		}
	}
}

func (r *Rectangle) Intersects(other Rectangle) bool {
	return !(other.X > r.X+r.Width || other.X+other.Width < r.X || other.Y > r.Y+r.Height || other.Y+other.Height < r.Y)
}
