#include "engine/core/FileSystem.h"
#include "engine/core/Handle.h"
#include "engine/core/JobSystem.h"
#include "engine/core/Noise.h"
#include "engine/core/Random.h"
#include "tests/support/TestPaths.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <set>

using namespace br;

TEST_CASE("Rng is deterministic for a given seed", "[core][random]") {
    Rng a(1234), b(1234), c(1235);
    bool anyDifferent = false;
    for (int i = 0; i < 1000; ++i) {
        const u32 va = a.nextU32();
        REQUIRE(va == b.nextU32());
        anyDifferent |= va != c.nextU32();
    }
    REQUIRE(anyDifferent);
}

TEST_CASE("Rng::rangeInt stays within inclusive bounds and covers them", "[core][random]") {
    Rng r(7);
    std::set<int> seen;
    for (int i = 0; i < 5000; ++i) {
        const int v = r.rangeInt(-3, 3);
        REQUIRE(v >= -3);
        REQUIRE(v <= 3);
        seen.insert(v);
    }
    REQUIRE(seen.size() == 7);
}

TEST_CASE("Coordinate hashing is stable and order-sensitive", "[core][random]") {
    REQUIRE(hash2i(42, 1, 2) == hash2i(42, 1, 2));
    REQUIRE(hash2i(42, 1, 2) != hash2i(42, 2, 1));
    REQUIRE(hash2i(42, 1, 2) != hash2i(43, 1, 2));
    // Golden value guards against accidental changes that would alter every world.
    STATIC_REQUIRE(splitmix64(0) == 0xE220A8397B1DCDAFull);
}

TEST_CASE("Gradient noise is bounded, continuous and tileable", "[core][noise]") {
    float maxAbs = 0.0f;
    for (int i = 0; i < 2000; ++i) {
        const glm::vec2 p(static_cast<float>(i) * 0.137f, static_cast<float>(i) * 0.071f);
        maxAbs = std::max(maxAbs, std::abs(noise::gradient2(p, 99)));
        // Continuity: tiny step => tiny change
        REQUIRE(std::abs(noise::gradient2(p, 99) - noise::gradient2(p + glm::vec2(1e-3f), 99)) < 0.02f);
    }
    REQUIRE(maxAbs <= 1.01f);
    REQUIRE(maxAbs > 0.3f);
    const glm::ivec2 period(8, 8);
    for (float x = 0.0f; x < 8.0f; x += 0.5f) {
        REQUIRE(noise::gradient2Tiled({x, 0.25f}, period, 5) == Catch::Approx(noise::gradient2Tiled({x + 8.0f, 8.25f}, period, 5)).margin(1e-5));
    }
}

TEST_CASE("SlotMap detects stale handles", "[core][handle]") {
    struct Tag {};
    SlotMap<Tag, int> map;
    const auto a = map.insert(1);
    const auto b = map.insert(2);
    REQUIRE(*map.get(a) == 1);
    REQUIRE(map.erase(a));
    REQUIRE(map.get(a) == nullptr);
    const auto c = map.insert(3); // reuses a's slot with a new generation
    REQUIRE(c.index == a.index);
    REQUIRE_FALSE(map.contains(a));
    REQUIRE(*map.get(c) == 3);
    REQUIRE(*map.get(b) == 2);
    REQUIRE(map.size() == 2);
}

TEST_CASE("JobSystem runs async work and parallelFor covers every index", "[core][jobs]") {
    JobSystem jobs(3);
    auto f = jobs.async([] { return 6 * 7; });
    REQUIRE(f.get() == 42);

    std::vector<std::atomic<int>> hits(10000);
    jobs.parallelFor(hits.size(), 64, [&](usize b, usize e) {
        for (usize i = b; i < e; ++i) hits[i].fetch_add(1);
    });
    for (auto& h : hits) REQUIRE(h.load() == 1);
}

TEST_CASE("MainThreadQueue drains posted work in order", "[core][jobs]") {
    MainThreadQueue q;
    std::vector<int> order;
    for (int i = 0; i < 5; ++i) q.post([&, i] { order.push_back(i); });
    REQUIRE(q.drain() == 5);
    REQUIRE(order == std::vector<int>{0, 1, 2, 3, 4});
}

TEST_CASE("Atomic file write replaces content and leaves no temp files", "[core][fs]") {
    test::TempDir dir;
    const auto p = dir.path() / "nested" / "file.json";
    REQUIRE(file::writeAtomic(p, std::string_view("first")));
    REQUIRE(file::writeAtomic(p, std::string_view("second")));
    REQUIRE(file::readText(p).value() == "second");
    int count = 0;
    for (const auto& e : fs::directory_iterator(p.parent_path())) {
        (void)e;
        ++count;
    }
    REQUIRE(count == 1);
}
