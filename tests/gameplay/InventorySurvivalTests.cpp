#include "game/inventory/Inventory.h"
#include "game/items/Flashlight.h"
#include "game/items/ItemCatalog.h"
#include "game/items/ItemUseSystem.h"
#include "game/survival/SurvivalSystem.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace br;
using namespace br::game;
using Catch::Approx;

namespace {
void simulate(SurvivalSystem& s, float seconds, const SurvivalInputs& in, float step = 1.0f / 60.0f) {
    for (float t = 0.0f; t < seconds; t += step) {
        s.update(step, in);
    }
}
} // namespace

TEST_CASE("Item catalog resolves standard and templated ids", "[items]") {
    const auto c = ItemCatalog::createDefault();
    for (const char* id : {"flashlight", "battery", "almond_water", "medkit", "bandage", "energy_bar", "fuse", "crowbar", "lighter"}) {
        INFO(id);
        REQUIRE(c.find(id).has_value());
    }
    const auto key = c.find("key_storage_room");
    REQUIRE(key);
    REQUIRE(key->id == "key_storage_room");
    REQUIRE(key->name == "Key: Storage Room");
    REQUIRE(key->category == ItemCategory::Key);
    REQUIRE(c.find("document_lost_wanderer")->category == ItemCategory::Document);
    REQUIRE_FALSE(c.find("unicorn").has_value());
}

TEST_CASE("Inventory stacks, respects capacity and keeps keys outside slots", "[inventory]") {
    const auto c = ItemCatalog::createDefault();
    Inventory inv(c, 2);
    REQUIRE(inv.add("battery", 4).fullyAdded());
    REQUIRE(inv.usedSlots() == 1);
    // battery stacks to 6: 2 more merge, 4 open a second slot, rest rejected
    const auto r = inv.add("battery", 10);
    REQUIRE(r.added == 8);
    REQUIRE(r.rejected == 2);
    REQUIRE(inv.count("battery") == 12);
    REQUIRE(inv.usedSlots() == 2);
    REQUIRE_FALSE(inv.canFit("almond_water"));
    // Keys and documents never consume slots
    REQUIRE(inv.add("key_office").fullyAdded());
    REQUIRE(inv.add("document_note_1").fullyAdded());
    REQUIRE(inv.hasKey("key_office"));
    REQUIRE(inv.has("document_note_1"));
    REQUIRE(inv.usedSlots() == 2);
    REQUIRE(inv.remove("battery", 7) == 7);
    REQUIRE(inv.count("battery") == 5);
    REQUIRE(inv.usedSlots() == 1);
    REQUIRE(inv.canFit("almond_water"));
}

TEST_CASE("Inventory round-trips through JSON and drops unknown items", "[inventory][save]") {
    const auto c = ItemCatalog::createDefault();
    Inventory inv(c);
    inv.add("almond_water", 2);
    inv.add("flashlight", 1, {{"charge", 0.4}});
    inv.add("key_boiler_room");
    auto j = inv.toJson();
    j["slots"].push_back({{"id", "removed_item"}, {"count", 3}});
    Inventory loaded(c);
    REQUIRE(loaded.fromJson(j));
    REQUIRE(loaded.count("almond_water") == 2);
    REQUIRE(loaded.has("flashlight"));
    REQUIRE(loaded.slots()[1].data["charge"].get<double>() == Approx(0.4));
    REQUIRE(loaded.hasKey("key_boiler_room"));
    REQUIRE(loaded.usedSlots() == 2);
    REQUIRE_FALSE(loaded.fromJson(nlohmann::json::array()));
}

TEST_CASE("Sprinting drains stamina, exhaustion gates sprint until partial recovery", "[survival]") {
    SurvivalSystem s;
    SurvivalInputs sprint;
    sprint.sprinting = true;
    simulate(s, 5.0f, sprint);
    REQUIRE(s.state().stamina < 40.0f);
    REQUIRE(s.canSprint());
    simulate(s, 4.0f, sprint);
    REQUIRE(s.state().exhausted);
    REQUIRE_FALSE(s.canSprint());
    SurvivalInputs idle;
    simulate(s, 1.0f, idle); // inside regen delay + a little regen
    REQUIRE_FALSE(s.canSprint());
    simulate(s, 2.0f, idle);
    REQUIRE(s.canSprint());
    simulate(s, 10.0f, idle);
    REQUIRE(s.state().stamina == Approx(100.0f));
}

TEST_CASE("Darkness and fear drain sanity, thresholds emit events once", "[survival]") {
    SurvivalSystem s;
    SurvivalInputs dark;
    dark.darkness = 1.0f;
    simulate(s, 60.0f, dark);
    // base 0.5 + darkness 4.0 per minute
    REQUIRE(s.state().sanity == Approx(95.5f).margin(0.1f));
    SurvivalInputs terror;
    terror.fear = 1.0f;
    s.drainEvents();
    simulate(s, 75.0f, terror); // 1.2/s => from 95.5 crosses 40 then 15
    const auto events = s.drainEvents();
    int low = 0, critical = 0;
    for (const auto& e : events) {
        low += e.type == SurvivalEventType::SanityLow;
        critical += e.type == SurvivalEventType::SanityCritical;
    }
    REQUIRE(low == 1);
    REQUIRE(critical == 1);
    REQUIRE(s.state().health < 100.0f); // critical sanity wears health down
}

TEST_CASE("Breath holding, drowning and recovery", "[survival][water]") {
    SurvivalSystem s;
    SurvivalInputs under;
    under.headUnderwater = true;
    simulate(s, 30.0f, under);
    REQUIRE(s.state().oxygen > 0.0f);
    REQUIRE(s.state().health == Approx(100.0f));
    simulate(s, 15.0f, under);
    REQUIRE(s.state().oxygen == 0.0f);
    REQUIRE(s.state().health < 100.0f);
    simulate(s, 3.0f, SurvivalInputs{});
    REQUIRE(s.state().oxygen == Approx(100.0f));
}

TEST_CASE("Death is final until revive; bleeding blocks regen", "[survival]") {
    SurvivalSystem s;
    s.damage(70.0f, DamageType::Entity);
    s.setBleeding(true);
    simulate(s, 30.0f, SurvivalInputs{});
    REQUIRE(s.state().health < 30.0f); // bled, no regen
    s.setBleeding(false);
    simulate(s, 120.0f, SurvivalInputs{});
    REQUIRE(s.state().health == Approx(50.0f)); // natural regen stops at the cap
    s.damage(500.0f, DamageType::Fall);
    REQUIRE_FALSE(s.state().alive);
    s.heal(50.0f);
    REQUIRE(s.state().health == 0.0f);
    s.revive();
    REQUIRE(s.state().alive);
    REQUIRE(s.state().health == Approx(100.0f));
}

TEST_CASE("Flashlight drains, dims, stutters when low and dies", "[items][flashlight]") {
    FlashlightTuning t;
    t.secondsPerBattery = 100.0f;
    Flashlight f(t);
    f.setOn(true);
    for (int i = 0; i < 50 * 60; ++i) f.update(1.0f / 60.0f);
    REQUIRE(f.charge() == Approx(0.5f).margin(0.01f));
    REQUIRE(f.output() == Approx(1.0f));
    bool stuttered = false;
    for (int i = 0; i < 45 * 60; ++i) {
        f.update(1.0f / 60.0f);
        stuttered |= f.stutteredThisFrame();
    }
    REQUIRE(f.low());
    REQUIRE(stuttered);
    for (int i = 0; i < 10 * 60; ++i) f.update(1.0f / 60.0f);
    REQUIRE(f.empty());
    REQUIRE_FALSE(f.isOn());
    REQUIRE(f.output() == 0.0f);
    f.insertBattery();
    REQUIRE(f.setOn(true));
    f.update(0.016f);
    REQUIRE(f.output() > 0.9f);
}

TEST_CASE("Item use is timed, consumes items and refuses waste", "[items][inventory][survival]") {
    const auto c = ItemCatalog::createDefault();
    Inventory inv(c);
    SurvivalSystem s;
    Flashlight light;
    ItemUseSystem use(c, inv, s, light);
    inv.add("almond_water", 1);
    // full sanity: refuse
    REQUIRE_FALSE(use.begin("almond_water").success);
    s.drainSanity(50.0f);
    REQUIRE(use.begin("almond_water").success);
    REQUIRE(use.busy());
    REQUIRE_FALSE(use.update(1.0f).has_value());
    REQUIRE(use.progress01() == Approx(0.5f));
    const auto done = use.update(1.1f);
    REQUIRE(done);
    REQUIRE(done->success);
    REQUIRE(s.state().sanity == Approx(85.0f));
    REQUIRE_FALSE(inv.has("almond_water"));
    // cancelled actions consume nothing
    inv.add("bandage", 1);
    s.damage(30.0f, DamageType::Generic);
    REQUIRE(use.begin("bandage").success);
    use.cancel();
    REQUIRE(inv.count("bandage") == 1);
    // battery restores the flashlight
    inv.add("battery", 1);
    REQUIRE_FALSE(use.begin("battery").success); // already full
    inv.add("flashlight");
    light.setOn(true);
    for (int i = 0; i < 600; ++i) light.update(1.0f);
    REQUIRE(light.empty());
    REQUIRE(use.begin("battery").success);
    REQUIRE(use.update(2.0f));
    REQUIRE(light.charge() == Approx(1.0f));
    REQUIRE_FALSE(inv.has("battery"));
    // lighter toggles a light without consuming anything
    inv.add("lighter");
    REQUIRE(use.begin("lighter").success);
    REQUIRE(use.lighterLit());
    REQUIRE(inv.has("lighter"));
}
