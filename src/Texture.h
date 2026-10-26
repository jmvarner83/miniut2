// MiniUT2 - Texture.h / .cpp
// Procedural textures that mimic UT2003's material look (metal panel seams,
// tech grating, brick/stone, control panel). The real game loads these from
// .utx packages; we generate equivalent RGBA bitmaps in code so the demo stays
// self-contained (no DXT/BGRA .utx decoder needed). Each texture is a square
// RGBA8 image with a simple 2-level mip chain.
#pragma once
#include <vector>
#include <cstdint>
#include <string>

struct FTexture {
    int   Size = 0;            // width = height (power of two)
    std::vector<uint8_t> RGBA; // Size*Size*4, level 0
    std::vector<uint8_t> Mip1; // Size/2 squared
    std::vector<uint8_t> Mip2; // Size/4 squared
    std::string Name;
};

class TextureGen {
public:
    // Named generators, all 256x256.
    static FTexture MetalPanel();   // brushed metal with panel seams + rivets
    static FTexture TechGrating();  // dark metal floor grating
    static FTexture Brick();        // stone/brick wall
    static FTexture ControlPanel(); // tech wall with lights/vents
    static FTexture Crate();        // industrial crate with edge frame

    // Pick a texture by a stable id (used by the world builder). Names are
    // prefixed to avoid clashing with the generator method names above.
    enum Kind { MatMetal, MatGrating, MatBrick, MatPanel, MatCrate, Count };
    static FTexture Make(Kind k);
};
