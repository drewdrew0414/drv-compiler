#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace drv {

// ─────────────────────────────────────────────────────────────────────────────
// IncrementalCache
//
// Stores per-file modification times + simple CRC32 checksums in a text file
// at <cache_dir>/.dri_cache/hashes.txt.
//
// Workflow in Compiler::compile():
//   1. isUpToDate(src, cpp_out) → true  → skip transpile+compile, reuse .o
//   2. After successful transpile:  updateEntry(src)
//
// Hash format (one entry per line):
//   <src_path> <mtime_ns> <crc32>
// ─────────────────────────────────────────────────────────────────────────────
class IncrementalCache {
public:
    explicit IncrementalCache(const std::string& project_dir);

    // Returns true when the .dri source is unchanged AND the generated .cpp
    // still exists → safe to skip transpilation.
    bool isUpToDate(const std::string& src_path,
                    const std::string& cpp_path) const;

    // Call after successful transpile to record the current file hash.
    void updateEntry(const std::string& src_path);

    // Persist in-memory cache to disk.
    void save() const;

    // Remove stale entries for files that no longer exist.
    void prune();

private:
    struct Entry {
        uint64_t mtime_ns{0};
        uint32_t crc32{0};
    };

    std::filesystem::path                        cache_file_;
    std::unordered_map<std::string, Entry>       entries_;

    void         load();
    static uint32_t computeCRC32(const std::string& path);
    static uint64_t modTimeNs   (const std::string& path);
};

} // namespace drv
