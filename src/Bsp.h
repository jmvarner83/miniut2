// MiniUT2 - Bsp.h
// BSP world geometry, modeled on UE2's FBspVertex / FBspVertexStream.
//
// VERIFIED from Engine.dll (Ghidra decompile of FBspVertexStream::GetStride/GetComponents/GetSize):
//   GetStride()      -> 0x28 (40 bytes per vertex)
//   GetComponents()  -> 4 components:
//        {type=1 Position, offset 0}   FVector (3 floats, 12B)
//        {type=1 Tangent,  offset 12}  FVector (3 floats, 12B)
//        {type=2 Texture,  offset 20}  U,V     (2 floats, 8B)
//        {type=5 Lightmap, offset 28}  LmU,LmV (2 floats, 8B)
//   GetSize()        -> count * 0x28
//
// For the box demo we render position + a flat normal (computed per-face) and
// ignore tangents/lightmaps. The serialized brush data in a real .umap also
// carries texture/lightmap indices + pan/align; we stub that here.
#pragma once
#include "Math.h"
#include <vector>
#include <cstdint>

// The GPU vertex format the engine feeds to the render device (40 bytes).
struct FBspVertex {
    FVector Position;   // +0
    FVector TangentBase;// +12
    float   U, V;       // +20
    float   LmU, LmV;   // +28
};
static_assert(sizeof(FBspVertex) == 40, "FBspVertex must be 40 bytes (0x28) to match Engine.dll");

// A convex polygon (a brush face). UE2 stores faces as convex polygons; a box
// face is a 4-vertex quad. We keep the plane + vertex indices.
struct FBspSurf {
    int   TextureIndex = -1;
    FVector Normal;          // face plane normal
    float   PlaneW = 0.f;    // plane distance: Dot(N, P) = PlaneW
    std::vector<int> VertexIndices;
};

// A convex brush = a box in our demo. UE2 worlds are literally built from these.
struct FBrush {
    FVector Center;
    FVector Extent;          // half-size along each axis
    FVector Color;           // demo-only tint (real engine uses textures)
};

// A zone (UE2 AZoneInfo). A single box room = one zone. PVS is skipped for the demo.
struct FZone {
    int   ZoneNumber = 0;
    FVector BspRoot;         // representative point
    std::vector<FBspSurf> Surfaces;
    std::vector<FVector>  Vertices;
};

// The whole world: a set of zones built from box brushes.
struct FWorld {
    std::vector<FZone> Zones;
    std::vector<FBspVertex> Vertices;   // render stream (40B each)
    std::vector<uint32_t>   Indices;    // triangle list
    std::vector<FVector>    FaceNormals;// parallel to triangle count (flat shading)
    std::vector<FVector>    FaceColors; // parallel to triangle count (demo tint)
    std::vector<int>        FaceTex;    // parallel to triangle count: texture id (-1 = none)
};
