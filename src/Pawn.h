// MiniUT2 - Pawn.h / .cpp
// Minimal APawn stand-in: position, velocity, gravity, and AABB collision against
// the box room. Replaces UE2's Karma physics for the demo. Movement is WASD +
// mouse-look, like a basic PlayerPawn.
#pragma once
#include "Math.h"
#include "Bsp.h"

struct FPawn {
    FVector Location{0, 0, 64};   // feet/eye origin in unreal units
    FVector Velocity{0, 0, 0};
    float   Yaw = 0.f, Pitch = 0.f;     // radians
    float   EyeHeight = 44.f;           // UE2 default eye height ~44uu
    float   Radius = 16.f;              // collision cylinder radius
    float   Height = 64.f;              // collision cylinder height
    bool    bOnGround = false;

    // Input intent (set from keyboard each frame).
    float  Forward = 0.f, Strafe = 0.f;
    float  Speed = 360.f;               // walk speed uu/s (UE2 default ~360)
    float  JumpZ = 420.f;

    void AddYawPitch(float dy, float dp) { Yaw += dy; Pitch += dp; }
    FVector ViewDir() const {
        float cp = std::cos(Pitch);
        return {std::cos(Yaw) * cp, std::sin(Yaw) * cp, std::sin(Pitch)};
    }
    FVector EyePos() const { return Location + FVector(0, 0, EyeHeight); }
};

// Integrate one frame (dt seconds) with gravity + clamp to the room AABB.
class PawnPhysics {
public:
    static void Tick(FPawn& p, const FWorld& world, float dt);
};
