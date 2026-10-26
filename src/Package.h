// MiniUT2 - Package.h / Package.cpp
// Minimal UE2 package (.u/.umap) header reader.
//
// VERIFIED from Core.u (rizin hexdump):
//   +0x00  uint32 Tag        = 0x9E2A83C1  (UE2 package magic)
//   +0x04  uint16 VersionLow = 0x0078 (120)
//   +0x06  uint16 VersionHigh= 0x001C (28)
//   +0x08  uint32 PackageFlags
//   +0x0C  uint32 NameCount
//   +0x10  uint32 NameOffset
//   +0x14  uint32 ExportCount
//   +0x18  uint32 ExportOffset
//   +0x1C  uint32 ImportCount
//   +0x20  uint32 ImportOffset
//   +0x24  ... (more tables: depends, activ, export GUIDs, etc.)
//   then a 16-byte package GUID, then the name table (interned strings).
//
// The full FLinkerLoad parses name/import/export tables and the script bytecode
// stored in export data. For the box demo we only validate the header and read
// the package name; geometry is generated procedurally (see WorldBuilder).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct FPackageHeader {
    uint32_t Tag = 0;
    uint16_t VersionLow = 0;
    uint16_t VersionHigh = 0;
    uint32_t PackageFlags = 0;
    uint32_t NameCount = 0, NameOffset = 0;
    uint32_t ExportCount = 0, ExportOffset = 0;
    uint32_t ImportCount = 0, ImportOffset = 0;
    uint8_t  Guid[16] = {};
    std::string PackageName;
    std::vector<std::string> Names;   // interned name table (best-effort)
};

class FPackageReader {
public:
    // Returns true if the file is a recognizable UE2 package.
    static bool ReadHeader(const std::string& path, FPackageHeader& out);
};
