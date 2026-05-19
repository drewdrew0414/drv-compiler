#include "incremental.h"
#include <array>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>

namespace drv {

namespace fs = std::filesystem;

// ── CRC32 (ISO 3309 / Ethernet polynomial) ───────────────────────────────────
// Pre-computed at namespace-init via an inline IIFE — thread-safe, no lazy
// flag, no per-call branch.
namespace {
constexpr std::array<uint32_t, 256> kCRC32Table = []() {
    std::array<uint32_t, 256> t{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int k = 0; k < 8; ++k)
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        t[i] = c;
    }
    return t;
}();
}

uint32_t IncrementalCache::computeCRC32(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return 0;

    uint32_t crc = 0xFFFFFFFFu;
    // 64 KiB read buffer — fewer syscalls than the old 4 KiB on big sources.
    static constexpr size_t kBufSize = 64 * 1024;
    std::vector<char> buf(kBufSize);
    while (f.read(buf.data(), kBufSize) || f.gcount() > 0) {
        auto n = static_cast<size_t>(f.gcount());
        const uint8_t* p = reinterpret_cast<const uint8_t*>(buf.data());
        for (size_t i = 0; i < n; ++i)
            crc = kCRC32Table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

uint64_t IncrementalCache::modTimeNs(const std::string& path) {
    std::error_code ec;
    auto ftime = fs::last_write_time(path, ec);
    if (ec) return 0;
    // Convert to nanoseconds since epoch
    auto dur = ftime.time_since_epoch();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count());
}

// ── Constructor ───────────────────────────────────────────────────────────────
IncrementalCache::IncrementalCache(const std::string& project_dir) {
    fs::path cache_dir = fs::path(project_dir) / ".dri_cache";
    std::error_code ec;
    fs::create_directories(cache_dir, ec);
    cache_file_ = cache_dir / "hashes.txt";
    load();
}

// ── Load from disk ────────────────────────────────────────────────────────────
// Format (per non-comment line): "<mtime_ns> <crc32> <path…>"
//   - mtime and crc are leading fixed-width integers
//   - path may contain spaces; consumed up to the line terminator
// Older "<path> <mtime> <crc>" entries fail to parse and are silently dropped,
// triggering one harmless rebuild on first use after upgrade.
void IncrementalCache::load() {
    std::ifstream f(cache_file_);
    if (!f) return;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        uint64_t mtime;
        uint32_t crc;
        if (!(iss >> mtime >> crc)) continue;
        // skip one delimiting space, then take the rest as path
        if (iss.peek() == ' ') iss.get();
        std::string path;
        std::getline(iss, path);
        if (!path.empty()) entries_[path] = {mtime, crc};
    }
}

// ── Save to disk ──────────────────────────────────────────────────────────────
void IncrementalCache::save() const {
    std::ofstream f(cache_file_);
    if (!f) return;
    f << "# dri incremental build cache — do not edit manually\n";
    f << "# format: <mtime_ns> <crc32> <path>\n";
    for (auto& [path, entry] : entries_)
        f << entry.mtime_ns << ' ' << entry.crc32 << ' ' << path << '\n';
}

// ── Check if up-to-date ───────────────────────────────────────────────────────
bool IncrementalCache::isUpToDate(const std::string& src_path,
                                   const std::string& cpp_path) const {
    // Generated .cpp must exist
    if (!fs::exists(cpp_path)) return false;

    auto it = entries_.find(src_path);
    if (it == entries_.end()) return false; // never cached

    const Entry& cached = it->second;

    // Fast check: mtime unchanged → likely unchanged
    uint64_t current_mtime = modTimeNs(src_path);
    if (current_mtime != cached.mtime_ns) return false;

    // Confirm with CRC32 (guards against mtime manipulation / same-second edits)
    uint32_t current_crc = computeCRC32(src_path);
    return current_crc == cached.crc32;
}

// ── Update entry after successful transpile ───────────────────────────────────
void IncrementalCache::updateEntry(const std::string& src_path) {
    Entry e;
    e.mtime_ns = modTimeNs(src_path);
    e.crc32    = computeCRC32(src_path);
    entries_[src_path] = e;
    save(); // persist immediately
}

// ── Remove stale entries ──────────────────────────────────────────────────────
void IncrementalCache::prune() {
    for (auto it = entries_.begin(); it != entries_.end(); ) {
        if (!fs::exists(it->first))
            it = entries_.erase(it);
        else
            ++it;
    }
    save();
}

} // namespace drv
