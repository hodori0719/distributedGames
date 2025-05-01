package main

import (
	"juneyoo/distgamelib/lib/game"
	"sync"
	"time"

	"github.com/sirupsen/logrus"
)

// Quadtree node
type Quadtree struct {
	boundary       game.Rectangle
	capacity       int
	store          []Storable
	divided        bool
	nw, ne, sw, se *Quadtree
	mu             sync.RWMutex
}

// NewQuadtree creates a new Quadtree node
func NewQuadtree(boundary game.Rectangle, capacity int) *Quadtree {
	return &Quadtree{
		boundary: boundary,
		capacity: capacity,
		store:    []Storable{},
	}
}

// Subdivide subdivides the quadtree node into four children. Assumes mutex is locked
func (qt *Quadtree) Subdivide() {
	x, y, w, h := qt.boundary.X, qt.boundary.Y, qt.boundary.Width/2, qt.boundary.Height/2
	qt.nw = NewQuadtree(game.Rectangle{X: x, Y: y, Width: w, Height: h}, qt.capacity)
	qt.ne = NewQuadtree(game.Rectangle{X: x + w, Y: y, Width: w, Height: h}, qt.capacity)
	qt.sw = NewQuadtree(game.Rectangle{X: x, Y: y + h, Width: w, Height: h}, qt.capacity)
	qt.se = NewQuadtree(game.Rectangle{X: x + w, Y: y + h, Width: w, Height: h}, qt.capacity)
	qt.divided = true
}

// Insert inserts a rectangle into the quadtree
func (qt *Quadtree) Insert(trigger Storable, l *Locator) bool {
	qt.mu.Lock()
	if !qt.boundary.Intersects(trigger.rect) {
		qt.mu.Unlock()
		return false
	}

	store := []Storable{}
	l.mu.Lock()
	for _, stored := range qt.store {
		// expire stale objects
		if (stored.ttl > 0 && time.Now().UnixMilli()-stored.ttl > 0) ||
			(l.lastUpdates[stored.guid] > 0 && l.lastUpdates[stored.guid] > stored.ttl) {
			continue
		}
		store = append(store, stored)
	}

	qt.store = store

	if len(qt.store) < qt.capacity {
		trigger.ttl = time.Now().UnixMilli() + trigger.ttl
		qt.store = append(qt.store, trigger)
		l.lastUpdates[trigger.guid] = trigger.ttl

		l.mu.Unlock()
		qt.mu.Unlock()
		logrus.WithField("trigger", trigger).Trace("Inserted trigger")
		return true
	}
	l.mu.Unlock()

	if !qt.divided {
		qt.Subdivide()
	}

	if qt.nw.Insert(trigger, l) || qt.ne.Insert(trigger, l) ||
		qt.sw.Insert(trigger, l) || qt.se.Insert(trigger, l) {
		qt.mu.Unlock()
		return true
	}

	qt.mu.Unlock()
	return false
}

// Reduce the quadtree by coalescing empty children and removing stale objects
func (qt *Quadtree) Cleanup(l *Locator) {
	qt.mu.Lock()

	// coalesce empty children
	if qt.divided {
		qt.nw.Cleanup(l)
		qt.ne.Cleanup(l)
		qt.sw.Cleanup(l)
		qt.se.Cleanup(l)

		if len(qt.nw.store) == 0 && len(qt.ne.store) == 0 && len(qt.sw.store) == 0 && len(qt.se.store) == 0 &&
			!qt.nw.divided && !qt.ne.divided && !qt.sw.divided && !qt.se.divided {
			qt.nw = nil
			qt.ne = nil
			qt.sw = nil
			qt.se = nil
			qt.divided = false
		}
	}

	// remove stale objects
	store := []Storable{}
	l.mu.Lock()
	for _, stored := range qt.store {
		// expire stale objects
		if (stored.ttl > 0 && time.Now().UnixMilli()-stored.ttl > 0) ||
			(l.lastUpdates[stored.guid] > 0 && l.lastUpdates[stored.guid] > stored.ttl) {
			continue
		}
		store = append(store, stored)
	}
	l.mu.Unlock()

	qt.store = store
	qt.mu.Unlock()
}

// Query finds all rectangles that intersect with the given rectangle
func (qt *Quadtree) Query(query Storable, found *[]Storable, l *Locator) {
	qt.mu.Lock()
	if !qt.boundary.Intersects(query.rect) {
		qt.mu.Unlock()
		return
	}

	store := []Storable{}
	l.mu.Lock()
	for _, stored := range qt.store {
		// expire stale objects
		if (stored.ttl > 0 && time.Now().UnixMilli()-stored.ttl > 0) ||
			(l.lastUpdates[stored.guid] > 0 && l.lastUpdates[stored.guid] > stored.ttl) {
			continue
		}
		store = append(store, stored)

		if query.rect.Intersects(stored.rect) &&
			query.isSubscription != stored.isSubscription &&
			query.sid != stored.sid {
			*found = append(*found, stored)
		}
	}

	l.mu.Unlock()

	qt.store = store
	divided := qt.divided

	if divided {
		qt.nw.Query(query, found, l)
		qt.ne.Query(query, found, l)
		qt.sw.Query(query, found, l)
		qt.se.Query(query, found, l)
	}
	qt.mu.Unlock()
}
