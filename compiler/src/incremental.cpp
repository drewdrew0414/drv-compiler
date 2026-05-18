#include "incremental.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>

namespace drv {

namespace fs = std::filesystem;

// ── CRC32 (ISO 3309 / Ethernet polynomial) ───────────────────────────────────
static uint32_t crc32_table[256];
static bool     crc32_table_ready = false;

static void buildCRC32Table() {
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int k = 0; k < 8; ++k)
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        crc32_table[i] = c;
    }
    crc32_table_ready = true;
}

uint32_t IncrementalCache::computeCRC32(const std::string& path) {
    if (!crc32_table_ready) buildCRC32Table();

    std::ifstream f(path, std::ios::binary);
    if (!f) return 0;

    uint32_t crc = 0xFFFFFFFFu;
    char buf[4096];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
        auto n = (size_t)f.gcount();
        for (size_t i = 0; i < n; ++i)
            crc = crc32_table[(crc ^ (uint8_t)buf[i]) & 0xFF] ^ (crc >> 8);
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
void IncrementalCache::load() {
    std::ifstream f(cache_file_);
    if (!f) return;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string path;
        uint64_t mtime;
        uint32_t crc;
        if (iss >> path >> mtime >> crc)
            entries_[path] = {mtime, crc};
    }
}

// ── Save to disk ──────────────────────────────────────────────────────────────
void IncrementalCache::save() const {
    std::ofstream f(cache_file_);
    if (!f) return;
    f << "# dri incremental build cache — do not edit manually\n";
    for (auto& [path, entry] : entries_)
        f << path << " " << entry.mtime_ns << " " << entry.crc32 << "\n";
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
