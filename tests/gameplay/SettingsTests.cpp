#include "game/settings/Settings.h"
#include "tests/support/TestPaths.h"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <SDL3/SDL_scancode.h>

using namespace br;

TEST_CASE("Settings round-trip through JSON and disk", "[settings]") {
    test::TempDir dir;
    game::Settings s;
    s.graphics.fieldOfView = 92.0f;
    s.graphics.render.shadowQuality = 3;
    s.audio.music = 0.25f;
    s.controls.invertY = true;
    s.controls.bindings.bind(Action::Jump, 0, InputBinding::key(SDL_SCANCODE_J));
    s.accessibility.reducedFlashing = true;
    REQUIRE(s.save(dir.path() / "settings.json"));

    game::Settings loaded;
    REQUIRE(loaded.load(dir.path() / "settings.json"));
    REQUIRE(loaded.graphics.fieldOfView == 92.0f);
    REQUIRE(loaded.graphics.render.shadowQuality == 3);
    REQUIRE(loaded.audio.music == 0.25f);
    REQUIRE(loaded.controls.invertY);
    REQUIRE(loaded.controls.bindings.get(Action::Jump, 0) == InputBinding::key(SDL_SCANCODE_J));
    REQUIRE(loaded.accessibility.reducedFlashing);
    REQUIRE(loaded.effectiveRenderSettings().reducedFlashing);
}

TEST_CASE("Corrupt or out-of-range settings fall back safely", "[settings]") {
    test::TempDir dir;
    REQUIRE(file::writeAtomic(dir.path() / "bad.json", std::string_view("{ not json")));
    game::Settings s;
    REQUIRE_FALSE(s.load(dir.path() / "bad.json"));
    REQUIRE(s.graphics.fieldOfView == 75.0f);

    game::Settings clamped;
    REQUIRE(clamped.fromJson(nlohmann::json::parse(R"({"graphics":{"fov":500,"render_scale":"oops"},"audio":{"master":7}})")));
    REQUIRE(clamped.graphics.fieldOfView == 110.0f);
    REQUIRE(clamped.graphics.render.renderScale == 1.0f);
    REQUIRE(clamped.audio.master == 1.0f);
}

TEST_CASE("Binding a key to one action removes it from others", "[input]") {
    ActionBindings b;
    b.bind(Action::Jump, 0, InputBinding::key(SDL_SCANCODE_W));
    REQUIRE_FALSE(b.get(Action::MoveForward, 0).valid());
    REQUIRE(b.get(Action::Jump, 0) == InputBinding::key(SDL_SCANCODE_W));
}
