package game

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestIntersects(t *testing.T) {
	obj1 := Rectangle{
		X:      0,
		Y:      0,
		Width:  10,
		Height: 10,
	}
	obj2 := Rectangle{
		X:      5,
		Y:      5,
		Width:  10,
		Height: 10,
	}
	assert.True(t, obj1.Intersects(obj2))

	obj2 = Rectangle{
		X:      11,
		Y:      11,
		Width:  10,
		Height: 10,
	}
	assert.False(t, obj1.Intersects(obj2))
}

func TestApplyPlayerEvent(t *testing.T) {
	player := GameObject{
		Guid: 0,
		Rect: Rectangle{
			X:      0,
			Y:      0,
			Width:  10,
			Height: 10,
		},
		LatestEvent: GameEvent{
			Input:     0,
			Timestamp: 0,
		},
		Type: Player,
	}
	event := GameEvent{
		Input:     Right,
		Timestamp: 100,
	}
	createdObjects := []GameObject{}
	player.ApplyEvent(event, nil, &createdObjects, nil)
	assert.Equal(t, float32(0), player.Rect.X)

	event = GameEvent{
		Input:     Right,
		Timestamp: 200,
	}
	player.ApplyEvent(event, nil, &createdObjects, nil)
	assert.Equal(t, float32(0.5), player.Rect.X)

	event = GameEvent{
		Input:     Shoot,
		Timestamp: 300,
	}
	player.ApplyEvent(event, nil, &createdObjects, nil)
	assert.Equal(t, 1, len(createdObjects))
	assert.Equal(t, float32(1), createdObjects[0].Rect.X)
	assert.Equal(t, float32(0), createdObjects[0].Rect.Y)
}

func TestGetAdjustedRect(t *testing.T) {
	player := GameObject{
		Guid: 0,
		Rect: Rectangle{
			X:      0,
			Y:      0,
			Width:  10,
			Height: 10,
		},
		LatestEvent: GameEvent{
			Input:     0,
			Timestamp: 0,
		},
		Type: Player,
	}
	assert.Equal(t, float32(0), player.GetAdjustedRect(0).X)
	assert.Equal(t, float32(0), player.GetAdjustedRect(0).Y)

	event := GameEvent{
		Input:     Right,
		Timestamp: 100,
	}

	player.ApplyEvent(event, nil, nil, nil)
	assert.Less(t, float32(0), player.GetAdjustedRect(100).X)
}

func TestGetLocation(t *testing.T) {
	player := GameObject{
		Guid: 0,
		Rect: Rectangle{
			X:      0,
			Y:      0,
			Width:  10,
			Height: 10,
		},
		LatestEvent: GameEvent{
			Input:     0,
			Timestamp: 0,
		},
		Type: Player,
	}
	assert.Equal(t, float32(0), player.GetLocation().X)
	assert.Equal(t, float32(0), player.GetLocation().X)

	event := GameEvent{
		Input:     Right,
		Timestamp: 100,
	}

	player.ApplyEvent(event, nil, nil, nil)
	assert.Less(t, float32(0), player.GetLocation().X)
}
