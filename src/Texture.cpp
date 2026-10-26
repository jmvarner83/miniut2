// MiniUT2 - Texture.cpp
#include "Texture.h"
#include <cmath>
#include <cstdlib>

// ---- helpers ----
static inline uint8_t clamp8(float v) { return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v)); }
static inline uint8_t* px(std::vector<uint8_t>& t, int S, int x, int y) { return &t[((y & (S-1)) * S + (x & (S-1))) * 4]; }

// deterministic hash noise
static inline float hashf(int x, int y) {
    uint32_t h = (uint32_t)(x * 374761393 + y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177;
    h = h ^ (h >> 16);
    return (float)(h & 0xFFFFFF) / (float)0xFFFFFF;
}

static FTexture newTex(int S, const char* name) {
    FTexture t; t.Size = S; t.Name = name;
    t.RGBA.assign(S * S * 4, 0);
    t.Mip1.assign((S/2) * (S/2) * 4, 0);
    t.Mip2.assign((S/4) * (S/4) * 4, 0);
    return t;
}

// build mip1/mip2 by box-filtering level 0
static void buildMips(FTexture& t) {
    int S = t.Size;
    for (int y = 0; y < S/2; ++y) for (int x = 0; x < S/2; ++x) {
        uint8_t* d = &t.Mip1[(y * (S/2) + x) * 4];
        for (int c = 0; c < 4; ++c) {
            int s = t.RGBA[((2*y)   * S + (2*x))   * 4 + c] + t.RGBA[((2*y)   * S + (2*x+1)) * 4 + c]
                  + t.RGBA[((2*y+1) * S + (2*x))   * 4 + c] + t.RGBA[((2*y+1) * S + (2*x+1)) * 4 + c];
            d[c] = (uint8_t)(s / 4);
        }
    }
    for (int y = 0; y < S/4; ++y) for (int x = 0; x < S/4; ++x) {
        uint8_t* d = &t.Mip2[(y * (S/4) + x) * 4];
        for (int c = 0; c < 4; ++c) {
            int s = 0;
            for (int j = 0; j < 2; ++j) for (int i = 0; i < 2; ++i)
                s += t.Mip1[((2*y+j) * (S/2) + (2*x+i)) * 4 + c];
            d[c] = (uint8_t)(s / 4);
        }
    }
}

// ---- Metal panel: brushed metal, panel seams, rivets ----
FTexture TextureGen::MetalPanel() {
    const int S = 256; FTexture t = newTex(S, "MetalPanel");
    const int panel = 128;
    for (int y = 0; y < S; ++y) for (int x = 0; x < S; ++x) {
        float base = 120 + 18 * std::sin(x * 0.9f) + 8 * hashf(x, y); // brushed streaks
        // panel seam grid
        int sx = x % panel, sy = y % panel;
        if (sx < 2 || sy < 2) base -= 70;                 // dark seam
        else if (sx == 2 || sy == 2) base += 25;          // highlight edge
        // rivets at panel corners
        int rx = (x % panel) - 10, ry = (y % panel) - 10;
        if (rx*rx + ry*ry < 16) base += 45;
        else if (rx*rx + ry*ry < 25) base -= 30;
        uint8_t* p = px(t.RGBA, S, x, y);
        uint8_t g = clamp8(base);
        p[0] = clamp8(g * 0.95f); p[1] = clamp8(g * 0.97f); p[2] = g; p[3] = 255;
    }
    buildMips(t); return t;
}

// ---- Tech grating: dark floor with slots ----
FTexture TextureGen::TechGrating() {
    const int S = 256; FTexture t = newTex(S, "TechGrating");
    for (int y = 0; y < S; ++y) for (int x = 0; x < S; ++x) {
        float base = 70 + 10 * hashf(x, y);
        int gx = x % 32, gy = y % 32;
        bool slot = (gx > 4 && gx < 26 && gy > 4 && gy < 26);
        if (slot) base = 28 + 6 * hashf(x, y);            // recessed dark
        else { base = 95 + 12 * hashf(x, y); if (gx < 2 || gy < 2) base += 30; } // raised bars
        uint8_t* p = px(t.RGBA, S, x, y);
        uint8_t g = clamp8(base);
        p[0] = clamp8(g * 0.9f); p[1] = clamp8(g * 0.95f); p[2] = g; p[3] = 255;
    }
    buildMips(t); return t;
}

// ---- Brick / stone wall ----
FTexture TextureGen::Brick() {
    const int S = 256; FTexture t = newTex(S, "Brick");
    const int bw = 64, bh = 32;
    for (int y = 0; y < S; ++y) for (int x = 0; x < S; ++x) {
        int row = y / bh;
        int off = (row & 1) ? bw / 2 : 0;
        int bx = (x + off) % bw, by = y % bh;
        float mortar = (bx < 3 || by < 3) ? 1.f : 0.f;
        float grain = 14 * hashf(x / 2, y / 2);
        float r, g, b;
        if (mortar) { r = 90; g = 88; b = 84; }            // grey mortar
        else {
            int cell = ((y / bh) * 7 + ((x + off) / bw) * 13);
            float v = hashf(cell, cell * 3);
            r = 150 + 40 * v + grain; g = 70 + 25 * v + grain; b = 55 + 20 * v + grain;
        }
        uint8_t* p = px(t.RGBA, S, x, y);
        p[0] = clamp8(r); p[1] = clamp8(g); p[2] = clamp8(b); p[3] = 255;
    }
    buildMips(t); return t;
}

// ---- Control panel: tech wall with lights/vents ----
FTexture TextureGen::ControlPanel() {
    const int S = 256; FTexture t = newTex(S, "ControlPanel");
    for (int y = 0; y < S; ++y) for (int x = 0; x < S; ++x) {
        float base = 60 + 10 * hashf(x, y);
        int gx = x % 64, gy = y % 64;
        // vent slats
        if (gy > 8 && gy < 56 && (gy % 8) < 3) base -= 25;
        // glowing indicator lights
        int lx = gx - 16, ly = gy - 16;
        if (lx*lx + ly*ly < 20) { uint8_t* p = px(t.RGBA, S, x, y); p[0]=40; p[1]=220; p[2]=60; p[3]=255; continue; }
        lx = gx - 48; ly = gy - 48;
        if (lx*lx + ly*ly < 20) { uint8_t* p = px(t.RGBA, S, x, y); p[0]=230; p[1]=60; p[2]=40; p[3]=255; continue; }
        // panel border
        if (gx < 3 || gy < 3) base += 30;
        uint8_t* p = px(t.RGBA, S, x, y);
        uint8_t g = clamp8(base);
        p[0] = clamp8(g * 0.85f); p[1] = clamp8(g * 0.9f); p[2] = g; p[3] = 255;
    }
    buildMips(t); return t;
}

// ---- Industrial crate: metal with edge frame + warning stripes ----
FTexture TextureGen::Crate() {
    const int S = 256; FTexture t = newTex(S, "Crate");
    for (int y = 0; y < S; ++y) for (int x = 0; x < S; ++x) {
        float base = 110 + 12 * hashf(x, y);
        int border = 18;
        bool edge = (x < border || y < border || x >= S-border || y >= S-border);
        if (edge) { base = 70 + 10 * hashf(x, y); if ((x+y) % 24 < 12) base += 60; } // frame + bolts
        // diagonal warning stripe band in middle
        int band = (x + y) % 48;
        if (x > 60 && x < 196 && y > 60 && y < 196 && band < 24) {
            uint8_t* p = px(t.RGBA, S, x, y); p[0]=210; p[1]=180; p[2]=40; p[3]=255; continue;
        }
        uint8_t* p = px(t.RGBA, S, x, y);
        uint8_t g = clamp8(base);
        p[0] = clamp8(g * 0.9f); p[1] = clamp8(g * 0.92f); p[2] = g; p[3] = 255;
    }
    buildMips(t); return t;
}

FTexture TextureGen::Make(Kind k) {
    switch (k) {
        case MatMetal:   return MetalPanel();
        case MatGrating: return TechGrating();
        case MatBrick:   return Brick();
        case MatPanel:   return ControlPanel();
        case MatCrate:   return Crate();
        default:         return MetalPanel();
    }
}
