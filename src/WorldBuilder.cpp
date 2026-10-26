// MiniUT2 - WorldBuilder.cpp
#include "WorldBuilder.h"
#include "Texture.h"
#include <initializer_list>
#include <cmath>

// Texture world scale: one texture tile per this many unreal units (UE2 default
// pan/scale is 1.0 uu/texel; a 256px texture at scale 1.0 = 256uu per tile).
static const float TEX_SCALE = 1.0f / 256.0f;

// Planar UV projection: pick the two axes perpendicular to the face normal and
// map world coords -> UV. This is how UE2 brushes get their texture coords.
static void FaceUV(const FVector& n, const FVector& p, float& u, float& v) {
    float ax = std::fabs(n.X), ay = std::fabs(n.Y), az = std::fabs(n.Z);
    if (az >= ax && az >= ay)      { u = p.X; v = p.Y; }   // floor/ceiling
    else if (ax >= ay && ax >= az) { u = p.Y; v = p.Z; }   // +/-X walls
    else                           { u = p.X; v = p.Z; }   // +/-Y walls
    u *= TEX_SCALE; v *= TEX_SCALE;
}

// Each face: 4 vertices (with UVs), a normal, a texture id, and 2 triangles.
static void EmitFace(FWorld& w, std::initializer_list<FVector> verts, const FVector& n,
                     const FVector& color, int texId) {
    uint32_t base = (uint32_t)w.Vertices.size();
    for (const FVector& c : verts) {
        FBspVertex v;
        v.Position = c;
        v.TangentBase = n;     // demo: stash normal in tangent slot
        FaceUV(n, c, v.U, v.V);
        v.LmU = v.LmV = 0.f;
        w.Vertices.push_back(v);
    }
    // two triangles: (0,1,2) (0,2,3)
    int tris[6] = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < 6; ++i) {
        w.Indices.push_back(base + tris[i]);
        w.FaceNormals.push_back(n);
        w.FaceColors.push_back(color);
        w.FaceTex.push_back(texId);
    }
}

void WorldBuilder::AddBoxRoom(FWorld& w, const FVector& c, const FVector& e, const FVector& color) {
    (void)color;
    FZone zone; zone.ZoneNumber = (int)w.Zones.size();
    zone.BspRoot = c;
    const float x0 = c.X - e.X, x1 = c.X + e.X;
    const float y0 = c.Y - e.Y, y1 = c.Y + e.Y;
    const float z0 = c.Z - e.Z, z1 = c.Z + e.Z;

    // UT2003-style material assignment: metal ceiling, tech-grating floor,
    // brick + control-panel + metal walls.
    EmitFace(w, {{x0,y0,z1},{x1,y0,z1},{x1,y1,z1},{x0,y1,z1}}, {0,0,-1}, {1,1,1}, TextureGen::MatMetal);    // ceiling
    EmitFace(w, {{x0,y0,z0},{x0,y1,z0},{x1,y1,z0},{x1,y0,z0}}, {0,0, 1}, {1,1,1}, TextureGen::MatGrating);   // floor
    EmitFace(w, {{x0,y0,z0},{x0,y0,z1},{x1,y0,z1},{x1,y0,z0}}, {0, 1,0}, {1,1,1}, TextureGen::MatBrick);     // -Y wall
    EmitFace(w, {{x1,y1,z0},{x1,y1,z1},{x0,y1,z1},{x0,y1,z0}}, {0,-1,0}, {1,1,1}, TextureGen::MatPanel);     // +Y wall
    EmitFace(w, {{x1,y0,z0},{x1,y0,z1},{x1,y1,z1},{x1,y1,z0}}, {-1,0,0}, {1,1,1}, TextureGen::MatMetal);     // +X wall
    EmitFace(w, {{x0,y0,z0},{x0,y1,z0},{x0,y1,z1},{x0,y0,z1}}, { 1,0,0}, {1,1,1}, TextureGen::MatPanel);     // -X wall

    w.Zones.push_back(zone);
}

void WorldBuilder::AddSolidBox(FWorld& w, const FVector& c, const FVector& e, const FVector& color) {
    (void)color;
    FZone zone; zone.ZoneNumber = (int)w.Zones.size(); zone.BspRoot = c;
    const float x0 = c.X - e.X, x1 = c.X + e.X;
    const float y0 = c.Y - e.Y, y1 = c.Y + e.Y;
    const float z0 = c.Z - e.Z, z1 = c.Z + e.Z;
    int tex = TextureGen::MatCrate;
    // Outward-facing crate faces.
    EmitFace(w, {{x0,y0,z1},{x1,y0,z1},{x1,y1,z1},{x0,y1,z1}}, {0,0, 1}, {1,1,1}, tex);
    EmitFace(w, {{x0,y0,z0},{x1,y0,z0},{x1,y1,z0},{x0,y1,z0}}, {0,0,-1}, {1,1,1}, tex);
    EmitFace(w, {{x0,y0,z0},{x1,y0,z0},{x1,y0,z1},{x0,y0,z1}}, {0,-1,0}, {1,1,1}, tex);
    EmitFace(w, {{x1,y1,z0},{x0,y1,z0},{x0,y1,z1},{x1,y1,z1}}, {0, 1,0}, {1,1,1}, tex);
    EmitFace(w, {{x1,y0,z0},{x1,y1,z0},{x1,y1,z1},{x1,y0,z1}}, { 1,0,0}, {1,1,1}, tex);
    EmitFace(w, {{x0,y0,z0},{x0,y0,z1},{x0,y1,z1},{x0,y1,z0}}, {-1,0,0}, {1,1,1}, tex);
    w.Zones.push_back(zone);
}
