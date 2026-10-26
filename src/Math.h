// MiniUT2 - minimal clean-room UE2-style engine (box-world demo)
// Math.h - vector/matrix math. UE2 uses left-handed, Z-up, units = uu (unreal units).
#pragma once
#include <cmath>

struct FVector {
    float X = 0.f, Y = 0.f, Z = 0.f;
    FVector() {}
    FVector(float x, float y, float z) : X(x), Y(y), Z(z) {}
    FVector operator+(const FVector& o) const { return {X + o.X, Y + o.Y, Z + o.Z}; }
    FVector operator-(const FVector& o) const { return {X - o.X, Y - o.Y, Z - o.Z}; }
    FVector operator*(float s) const { return {X * s, Y * s, Z * s}; }
    float Size() const { return std::sqrt(X * X + Y * Y + Z * Z); }
    FVector GetSafeNormal() const {
        float s = Size();
        return s > 1e-6f ? (*this * (1.f / s)) : FVector(0, 0, 0);
    }
};

inline float Dot(const FVector& a, const FVector& b) { return a.X * b.X + a.Y * b.Y + a.Z * b.Z; }
inline FVector Cross(const FVector& a, const FVector& b) {
    return {a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X};
}

// Column-major 4x4, matches OpenGL upload order.
struct FMatrix {
    float m[16];
    static FMatrix Identity() {
        FMatrix r{}; for (int i = 0; i < 16; i += 5) r.m[i] = 1.f; return r;
    }
    static FMatrix Translate(const FVector& t) {
        FMatrix r = Identity(); r.m[12] = t.X; r.m[13] = t.Y; r.m[14] = t.Z; return r;
    }
    static FMatrix Perspective(float fovRad, float aspect, float zn, float zf) {
        float f = 1.f / std::tan(fovRad * 0.5f);
        FMatrix r{};
        r.m[0] = f / aspect; r.m[5] = f;
        r.m[10] = (zf + zn) / (zn - zf); r.m[11] = -1.f;
        r.m[14] = (2.f * zf * zn) / (zn - zf);
        return r;
    }
    // UE2 is left-handed Z-up; this builds a view matrix from eye + yaw/pitch.
    // OpenGL is right-handed and looks down -Z, so the view's forward axis must
    // be -dir. We still derive the right/up basis from the true forward f so the
    // strafe (X) axis keeps its correct sign; only the Z (forward) axis is negated.
    static FMatrix LookAtRH(const FVector& eye, const FVector& dir, const FVector& up) {
        FVector f = dir.GetSafeNormal();
        FVector s = Cross(up, f).GetSafeNormal();
        FVector u = Cross(f, s);
        FMatrix r = Identity();
        r.m[0] = s.X;  r.m[4] = s.Y;  r.m[8]  = s.Z;   // right  = +s
        r.m[1] = u.X;  r.m[5] = u.Y;  r.m[9]  = u.Z;   // up     = +u
        r.m[2] = -f.X; r.m[6] = -f.Y; r.m[10] = -f.Z;  // forward= -f (OpenGL -Z)
        r.m[12] = -Dot(s, eye); r.m[13] = -Dot(u, eye); r.m[14] = Dot(f, eye);
        return r;
    }
    static FMatrix Mul(const FMatrix& a, const FMatrix& b) {
        FMatrix r{};
        for (int c = 0; c < 4; ++c)
            for (int rr = 0; rr < 4; ++rr) {
                float s = 0;
                for (int k = 0; k < 4; ++k) s += a.m[k * 4 + rr] * b.m[c * 4 + k];
                r.m[c * 4 + rr] = s;
            }
        return r;
    }
};
