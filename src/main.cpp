// MiniUT2 - main.cpp
// A minimal clean-room UE2-style "render device" + game loop for a box world.
//
// Design mirrors the real engine's pluggable subsystem idea (D3DDrv.dll implements
// URenderDevice). Here we implement a tiny GL1.1 device and drive a Pawn + BSP
// world. No external libs: Win32 + opengl32/glu32 loaded dynamically.
//
// Build (MSVC):   cl /EHsc /O2 main.cpp Package.cpp WorldBuilder.cpp Pawn.cpp
//                     /link opengl32.lib user32.lib gdi32.lib
// Build (MinGW):  g++ -O2 main.cpp Package.cpp WorldBuilder.cpp Pawn.cpp -lopengl32 -lgdi32 -o miniut2.exe
//
// Controls: WASD move, mouse look, Space jump, Esc quit.
#include <windows.h>
#include <windowsx.h>
#include <gl/gl.h>
#include <cmath>
#include <cstdio>
#include <string>
#include "Math.h"
#include "Bsp.h"
#include "Package.h"
#include "WorldBuilder.h"
#include "Pawn.h"
#include "Texture.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// ---- tiny GL device (the "URenderDevice" of the demo) ----
struct GLDevice {
    HWND   hwnd = nullptr;
    HDC    hdc = nullptr;
    HGLRC  hgl = nullptr;
    int    width = 1280, height = 720;

    bool Create(const wchar_t* title, HINSTANCE hInst);
    void Destroy();
    void Swap();
    void SetRes(int w, int h);
};

// ---- game state ----
struct Game {
    FWorld   world;
    FPawn    pawn;
    GLDevice dev;
    bool     running = true;
    bool     mouseGrabbed = false;
    float    sens = 0.0025f;
    GLuint   texIds[TextureGen::Count] = {};   // GL texture objects per material

    void Init(const std::string& pkgPath);
    void Render();
    void Tick(float dt);
    void UploadTextures();
};

static Game g_game;

// ---- input ----
static void OnKey(WPARAM vk, bool down) {
    switch (vk) {
        case 'W': g_game.pawn.Forward = down ? 1.f : 0.f; break;
        case 'S': g_game.pawn.Forward = down ? -1.f : 0.f; break;
        case 'A': g_game.pawn.Strafe  = down ? -1.f : 0.f; break;
        case 'D': g_game.pawn.Strafe  = down ? 1.f : 0.f; break;
        case VK_SPACE:
            if (down && g_game.pawn.bOnGround) { g_game.pawn.Velocity.Z = g_game.pawn.JumpZ; g_game.pawn.bOnGround = false; }
            break;
        case VK_ESCAPE: g_game.running = false; break;
    }
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
        case WM_CLOSE: PostQuitMessage(0); return 0;
        case WM_KEYDOWN: OnKey(w, true); return 0;
        case WM_KEYUP:   OnKey(w, false); return 0;
        case WM_SIZE: {
            int wd = LOWORD(l), ht = HIWORD(l);
            if (wd && ht) g_game.dev.SetRes(wd, ht);
            return 0;
        }
        case WM_LBUTTONDOWN:
            if (!g_game.mouseGrabbed) {
                RECT r; GetClientRect(h, &r);
                POINT c{(r.left + r.right) / 2, (r.top + r.bottom) / 2};
                ClientToScreen(h, &c);
                SetCursorPos(c.x, c.y);
                ShowCursor(FALSE);
                g_game.mouseGrabbed = true;
            }
            return 0;
        case WM_MOUSEMOVE:
            if (g_game.mouseGrabbed) {
                POINT c{GET_X_LPARAM(l), GET_Y_LPARAM(l)};
                RECT r; GetClientRect(h, &r);
                POINT mid{(r.left + r.right) / 2, (r.top + r.bottom) / 2};
                ClientToScreen(h, &mid);
                POINT cur; GetCursorPos(&cur);
                float dx = (float)(cur.x - mid.x), dy = (float)(cur.y - mid.y);
                g_game.pawn.AddYawPitch(dx * g_game.sens, -dy * g_game.sens);
                SetCursorPos(mid.x, mid.y);
            }
            return 0;
    }
    return DefWindowProc(h, m, w, l);
}

// ---- GLDevice impl ----
bool GLDevice::Create(const wchar_t* title, HINSTANCE hInst) {
    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc; wc.hInstance = hInst; wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); wc.lpszClassName = L"MiniUT2";
    RegisterClassW(&wc);
    hwnd = CreateWindowW(L"MiniUT2", title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                        width, height, nullptr, nullptr, hInst, nullptr);
    if (!hwnd) return false;
    hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd); pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32; pfd.cDepthBits = 24; pfd.iLayerType = PFD_MAIN_PLANE;
    int pf = ChoosePixelFormat(hdc, &pfd);
    if (!pf || !SetPixelFormat(hdc, pf, &pfd)) return false;
    hgl = wglCreateContext(hdc);
    if (!hgl) return false;
    wglMakeCurrent(hdc, hgl);
    ShowWindow(hwnd, SW_SHOW);
    return true;
}
void GLDevice::Destroy() {
    if (hgl) { wglMakeCurrent(nullptr, nullptr); wglDeleteContext(hgl); }
    if (hdc && hwnd) ReleaseDC(hwnd, hdc);
    if (hwnd) DestroyWindow(hwnd);
}
void GLDevice::Swap() { SwapBuffers(hdc); }
void GLDevice::SetRes(int w, int h) {
    width = w; height = h;
    glViewport(0, 0, w, h);
}

// ---- Game impl ----
void Game::Init(const std::string& pkgPath) {
    // Try to read a real UE2 package header to prove the loader works.
    FPackageHeader hdr;
    if (FPackageReader::ReadHeader(pkgPath, hdr)) {
        printf("Package OK: tag=0x%08X ver=%d.%d names=%u exports=%u imports=%u\n",
               hdr.Tag, hdr.VersionLow, hdr.VersionHigh, hdr.NameCount, hdr.ExportCount, hdr.ImportCount);
        if (!hdr.Names.empty()) printf("  first names:");
        for (size_t i = 0; i < hdr.Names.size() && i < 6; ++i) printf(" %s", hdr.Names[i].c_str());
        printf("\n");
    } else {
        printf("No package loaded (using procedural box world).\n");
    }

    // Build the box world: a big room + a couple of crates resting on the floor.
    // Room floor is at Z = 256 - 512 = -256, so a crate of half-height h rests
    // at center Z = -256 + h.
    WorldBuilder::AddBoxRoom(world, {0, 0, 256}, {1024, 1024, 512}, {0.5f, 0.5f, 0.55f});
    WorldBuilder::AddSolidBox(world, {300, 200, -192}, {64, 64, 64}, {0.8f, 0.4f, 0.2f});   // floor -256 + 64
    WorldBuilder::AddSolidBox(world, {-400, -300, -128}, {128, 128, 128}, {0.3f, 0.6f, 0.3f}); // floor -256 + 128

    pawn.Location = {0, 0, -256};  // feet on the floor
    pawn.Yaw = 0; pawn.Pitch = -0.15f;  // look slightly down so floor/wall boundary shows

    printf("World: %d zones, %d verts, %d tris\n",
           (int)world.Zones.size(), (int)world.Vertices.size(), (int)world.Indices.size() / 3);

    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.05f, 0.06f, 0.09f, 1.f);

    UploadTextures();
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // face color tints texture

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) printf("GL error after init: 0x%X\n", (unsigned)err);
    else printf("GL init OK, %d textures uploaded\n", TextureGen::Count);
    fflush(stdout);
}

void Game::Tick(float dt) { PawnPhysics::Tick(pawn, world, dt); }

// Upload each procedural texture to a GL texture object. Uses the most
// software-renderer-compatible settings (GL_RGB internal, GL_LINEAR, no mipmaps)
// because the classic opengl32.dll fallback is strict about GL_RGBA/mipmaps.
void Game::UploadTextures() {
    glGenTextures(TextureGen::Count, texIds);
    for (int k = 0; k < TextureGen::Count; ++k) {
        FTexture t = TextureGen::Make((TextureGen::Kind)k);
        glBindTexture(GL_TEXTURE_2D, texIds[k]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        int S = t.Size;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, S, S, 0, GL_RGBA, GL_UNSIGNED_BYTE, t.RGBA.data());
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Game::Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    double aspect = (double)dev.width / (double)(dev.height ? dev.height : 1);
    double fov = 90.0 * 3.14159265 / 180.0;
    // gluPerspective-free: build our own.
    float f = 1.f / std::tan((float)fov * 0.5f);
    float proj[16] = {0};
    proj[0] = f / (float)aspect; proj[5] = f; proj[11] = -1.f;
    float zn = 4.f, zf = 20000.f;
    proj[10] = (zf + zn) / (zn - zf); proj[14] = (2.f * zf * zn) / (zn - zf);
    glLoadMatrixf(proj);

    FVector eye = pawn.EyePos(), dir = pawn.ViewDir();
    FMatrix view = FMatrix::LookAtRH(eye, dir, {0, 0, 1});
    glMatrixMode(GL_MODELVIEW); glLoadMatrixf(view.m);

    // Draw BSP triangles: bind per-face texture, pass normal + color + UV.
    // Bind the first texture BEFORE glBegin so we never call glEnd() on an
    // empty glBegin (the software renderer rejects that and aborts the draw).
    int curTex = -1;
    size_t i = 0;
    while (i < world.Indices.size()) {
        int tex = world.FaceTex[i];
        if (tex != curTex) {
            if (curTex != -1) glEnd();           // close previous batch only if open
            if (tex >= 0) glBindTexture(GL_TEXTURE_2D, texIds[tex]);
            else          glBindTexture(GL_TEXTURE_2D, 0);
            curTex = tex;
            glBegin(GL_TRIANGLES);
        }
        const FBspVertex& v = world.Vertices[world.Indices[i]];
        const FVector& n = world.FaceNormals[i];
        const FVector& col = world.FaceColors[i];
        glNormal3f(n.X, n.Y, n.Z);
        glColor3f(col.X, col.Y, col.Z);
        glTexCoord2f(v.U, v.V);
        glVertex3f(v.Position.X, v.Position.Y, v.Position.Z);
        ++i;
    }
    if (curTex != -1) glEnd();

    dev.Swap();
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    AllocConsole(); freopen("CONOUT$", "w", stdout);
    if (!g_game.dev.Create(L"MiniUT2 - box world", hInst)) { printf("GL init failed\n"); return 1; }
    g_game.Init("C:\\UT2003\\System\\Core.u");

    LARGE_INTEGER freq, prev, cur;
    QueryPerformanceFrequency(&freq); QueryPerformanceCounter(&prev);
    MSG msg{};
    int frame = 0;
    while (g_game.running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) g_game.running = false;
            TranslateMessage(&msg); DispatchMessage(&msg);
        }
        QueryPerformanceCounter(&cur);
        float dt = (float)((cur.QuadPart - prev.QuadPart) / (double)freq.QuadPart);
        prev = cur;
        if (dt > 0.05f) dt = 0.05f;
        g_game.Tick(dt);
        g_game.Render();
        if ((frame++ % 30) == 0) {
            printf("frame %d pawn=(%.1f,%.1f,%.1f) vel=(%.1f,%.1f,%.1f) onGround=%d\n",
                   frame, g_game.pawn.Location.X, g_game.pawn.Location.Y, g_game.pawn.Location.Z,
                   g_game.pawn.Velocity.X, g_game.pawn.Velocity.Y, g_game.pawn.Velocity.Z,
                   (int)g_game.pawn.bOnGround);
            fflush(stdout);
        }
    }
    g_game.dev.Destroy();
    return 0;
}
