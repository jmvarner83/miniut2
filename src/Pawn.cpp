// MiniUT2 - Pawn.cpp
#include "Pawn.h"

// For the demo we treat the first zone (the room) as the collision AABB.
// A real engine would sweep the cylinder against BSP planes / Karma.
void PawnPhysics::Tick(FPawn& p, const FWorld& world, float dt) {
    const float G = 2000.f; // gravity uu/s^2 (UE2 default Gravity=2000)

    // Horizontal movement in view space (yaw only).
    FVector fwd{std::cos(p.Yaw), std::sin(p.Yaw), 0};
    FVector right{-std::sin(p.Yaw), std::cos(p.Yaw), 0};
    FVector wish = (fwd * p.Forward + right * p.Strafe);
    float len = wish.Size();
    if (len > 1e-4f) wish = wish * (p.Speed / len);
    p.Velocity.X = wish.X;
    p.Velocity.Y = wish.Y;

    // Gravity.
    p.Velocity.Z -= G * dt;

    // Integrate.
    p.Location = p.Location + p.Velocity * dt;

    // Clamp to room AABB (first zone). Floor/ceiling/walls.
    if (!world.Zones.empty()) {
        const FZone& z = world.Zones[0];
        // We stored the room center as BspRoot; recover extents from vertices.
        // Simpler: use a fixed room from the builder via BspRoot + hardcoded half.
        // Here we clamp using the min/max of the zone's stored vertices if present,
        // else fall back to a 1024uu half-extent around BspRoot.
        FVector c = z.BspRoot;
        float ex = 1024.f, ey = 1024.f, ez = 512.f;
        if (!z.Vertices.empty()) {
            FVector mn = z.Vertices[0], mx = z.Vertices[0];
            for (auto& v : z.Vertices) {
                mn.X = v.X < mn.X ? v.X : mn.X; mx.X = v.X > mx.X ? v.X : mx.X;
                mn.Y = v.Y < mn.Y ? v.Y : mn.Y; mx.Y = v.Y > mx.Y ? v.Y : mx.Y;
                mn.Z = v.Z < mn.Z ? v.Z : mn.Z; mx.Z = v.Z > mx.Z ? v.Z : mx.Z;
            }
            c = (mn + mx) * 0.5f;
            ex = (mx.X - mn.X) * 0.5f; ey = (mx.Y - mn.Y) * 0.5f; ez = (mx.Z - mn.Z) * 0.5f;
        }
        float minX = c.X - ex + p.Radius, maxX = c.X + ex - p.Radius;
        float minY = c.Y - ey + p.Radius, maxY = c.Y + ey - p.Radius;
        float floorZ = c.Z - ez, ceilZ = c.Z + ez - p.Height;

        if (p.Location.X < minX) { p.Location.X = minX; p.Velocity.X = 0; }
        if (p.Location.X > maxX) { p.Location.X = maxX; p.Velocity.X = 0; }
        if (p.Location.Y < minY) { p.Location.Y = minY; p.Velocity.Y = 0; }
        if (p.Location.Y > maxY) { p.Location.Y = maxY; p.Velocity.Y = 0; }
        p.bOnGround = false;
        if (p.Location.Z < floorZ) { p.Location.Z = floorZ; if (p.Velocity.Z < 0) p.Velocity.Z = 0; p.bOnGround = true; }
        if (p.Location.Z > ceilZ)  { p.Location.Z = ceilZ;  if (p.Velocity.Z > 0) p.Velocity.Z = 0; }
    }
}
