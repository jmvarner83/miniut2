// MiniUT2 - WorldBuilder.h / .cpp
// Builds a simple box world from convex brushes, the way UE2 does.
// A room = 6 inward-facing quads (floor, ceiling, 4 walls) = one zone.
#pragma once
#include "Bsp.h"

class WorldBuilder {
public:
    // Build a hollow box room of given half-extent centered at `center`.
    // Adds it as one zone and appends triangles to the world render stream.
    static void AddBoxRoom(FWorld& world, const FVector& center, const FVector& extent,
                           const FVector& color);
    // Build a solid box (all faces outward) - used for obstacles/crates.
    static void AddSolidBox(FWorld& world, const FVector& center, const FVector& extent,
                            const FVector& color);
};
