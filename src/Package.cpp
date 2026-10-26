// MiniUT2 - Package.cpp
#include "Package.h"
#include <fstream>
#include <cstring>

static uint32_t rd32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
static uint16_t rd16(const uint8_t* p) { return uint16_t(p[0]) | (uint16_t(p[1]) << 8); }

bool FPackageReader::ReadHeader(const std::string& path, FPackageHeader& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<uint8_t> buf(0x400);
    f.read((char*)buf.data(), buf.size());
    std::streamsize got = f.gcount();
    if (got < 0x40) return false;

    out.Tag          = rd32(&buf[0x00]);
    if (out.Tag != 0x9E2A83C1u) return false;   // UE2 package magic (verified)
    out.VersionLow   = rd16(&buf[0x04]);
    out.VersionHigh  = rd16(&buf[0x06]);
    out.PackageFlags = rd32(&buf[0x08]);
    out.NameCount    = rd32(&buf[0x0C]);
    out.NameOffset   = rd32(&buf[0x10]);
    out.ExportCount  = rd32(&buf[0x14]);
    out.ExportOffset = rd32(&buf[0x18]);
    out.ImportCount  = rd32(&buf[0x1C]);
    out.ImportOffset = rd32(&buf[0x20]);
    std::memcpy(out.Guid, &buf[0x24], 16);

    // Best-effort: read the name table strings (UE2 names are length-prefixed).
    // We stop at the buffer we loaded; a real linker seeks to NameOffset.
    if (out.NameOffset && out.NameOffset < (uint32_t)got) {
        uint32_t p = out.NameOffset;
        for (uint32_t i = 0; i < out.NameCount && p + 4 <= (uint32_t)got; ++i) {
            uint32_t len = rd32(&buf[p]); p += 4;
            if (len == 0 || len > 1024 || p + len > (uint32_t)got) break;
            std::string s((char*)&buf[p], len);
            p += len;
            out.Names.push_back(s);
        }
    }
    if (!out.Names.empty()) out.PackageName = out.Names[0];
    return true;
}
