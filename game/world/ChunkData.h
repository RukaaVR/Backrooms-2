#pragma once

#include "engine/core/Math.h"
#include "engine/core/Types.h"
#include "engine/physics/PhysicsTypes.h"
#include "engine/renderer/RenderTypes.h"
#include "engine/scene/MeshData.h"

#include <compare>
#include <string>
#include <vector>

namespace br::world {

// Integer chunk coordinate. Most levels stream on the XZ plane (y = 0);
// vertical levels (hotel floors, the city) may use y for stacked layers.
struct ChunkCoord {
    i32 x = 0;
    i32 y = 0;
    i32 z = 0;
    friend auto operator<=>(const ChunkCoord&, const ChunkCoord&) = default;
};

struct ChunkCoordHash {
    usize operator()(const ChunkCoord& c) const noexcept {
        u64 h = static_cast<u64>(static_cast<u32>(c.x)) * 0x9E3779B97F4A7C15ull;
        h ^= static_cast<u64>(static_cast<u32>(c.y)) * 0xC2B2AE3D27D4EB4Full + (h << 6);
        h ^= static_cast<u64>(static_cast<u32>(c.z)) * 0x165667B19E3779F9ull + (h >> 2);
        return static_cast<usize>(h);
    }
};

// ---------------------------------------------------------------------------
// Everything below is plain CPU data produced on worker threads by a level's
// chunk generator. The main thread turns it into renderer instances, physics
// bodies, audio emitters and gameplay objects (WorldStreamer). Generators must
// be pure functions of (level seed, chunk coord) so chunks are reproducible.
// ---------------------------------------------------------------------------

// Static render geometry merged per material (few draw calls per chunk).
struct GeometryBatch {
    std::string material; // MaterialRegistry id, e.g. "level0.wallpaper"
    MeshData mesh;        // world-space positions
    bool castShadows = true;
};

struct ColliderBox {
    glm::vec3 center{0.0f};
    glm::vec3 halfExtents{0.5f};
    glm::quat rotation{1, 0, 0, 0};
    physics::Surface surface = physics::Surface::Default;
};

// Only for genuinely irregular static geometry (caves, terrain).
struct ColliderMesh {
    std::vector<glm::vec3> vertices;
    std::vector<u32> indices;
    physics::Surface surface = physics::Surface::Default;
};

enum class FixtureKind : u8 {
    FluorescentPanel, // 2x4 troffer in a drop ceiling (Level 0)
    FluorescentTube,  // bare industrial batten
    Incandescent,
    Emergency,
    Industrial,       // caged bulkhead / sodium
    Streetlight,
    Chandelier,
    Sconce,
    Spotlight,
    Natural,          // window / skylight fill
    Neon,
};

enum class FixtureState : u8 { Normal, Flickering, Failing, Dead };

// A light-emitting fixture. Fixture geometry is rendered as its own instance
// (prefab mesh) so its emissive surface can flicker in sync with the light
// and the hum sound.
struct LightFixture {
    render::LightDesc light;     // world-space emission
    FixtureKind kind = FixtureKind::FluorescentPanel;
    FixtureState state = FixtureState::Normal;
    std::string prefab;          // fixture mesh prefab id ("" = light only)
    Transform fixtureTransform;
    float flickerRate = 0.0f;    // 0..1 chance-per-second scale of flicker bursts
    u32 seed = 0;                // per-fixture deterministic variation
    bool hum = true;             // emits electrical hum audio
    u64 persistentId = 0;        // for persistent state (e.g. player broke it)
};

// Furniture and other prefabs from the PrefabLibrary.
struct PropSpawn {
    std::string prefab;
    Transform transform;
    bool dynamic = false;        // simulated rigid body vs static decoration
    u32 variant = 0;
    u64 persistentId = 0;        // stable id; persistent changes key off this
};

struct ItemSpawn {
    std::string itemId;          // "almond_water", "battery", "flashlight", ...
    Transform transform;
    u64 persistentId = 0;
};

struct DoorSpawn {
    std::string prefab = "door.wood";
    glm::vec3 hingePosition{0.0f}; // bottom of the hinge edge
    float yawRadians = 0.0f;       // closed orientation
    float width = 0.9f;
    float height = 2.05f;
    bool locked = false;
    std::string keyId;
    u64 persistentId = 0;
};

enum class TriggerKind : u8 {
    NoclipAnomaly, // glitching wall/floor that transfers the player to another level
    LevelExit,     // diegetic exit (door, stairwell, ladder)
    Hazard,        // damage / sanity drain zone (steam, electricity, cold water)
    AmbienceZone,  // switches soundscape / reverb
    Event,         // horror director scripted hook
    Checkpoint,
    Document,      // readable note location
};

struct TriggerVolume {
    TriggerKind kind = TriggerKind::Event;
    AABB bounds;
    std::string target;          // level id, event id, hazard type, document id
    float strength = 1.0f;
    u64 persistentId = 0;
};

struct WaterBody {
    AABB bounds;
    float surfaceHeight = 0.0f;
    std::string style;           // "poolrooms", "flooded", "cave", "puddle"
};

struct AudioEmitterSpawn {
    std::string sound;           // sound bank id
    glm::vec3 position{0.0f};
    float volume = 1.0f;
    float radius = 12.0f;
    bool loop = true;
};

struct EntitySpawnHint {
    std::string entityType;      // "hound", "smiler", "partygoer", ...
    glm::vec3 position{0.0f};
    float weight = 1.0f;         // relative likelihood used by the horror director
};

// Coarse walkability grid for AI navigation (cell centres on the XZ plane).
struct NavGrid {
    glm::vec3 origin{0.0f};      // world position of cell (0,0) corner
    float cellSize = 0.5f;
    int width = 0;               // cells along X
    int depth = 0;               // cells along Z
    std::vector<u8> cost;        // 0 = blocked, 1..254 traversal cost, 255 = unknown
    std::vector<float> floorHeight;

    bool valid() const { return width > 0 && depth > 0 && cost.size() == static_cast<usize>(width * depth); }
    u8 at(int x, int z) const { return cost[static_cast<usize>(z * width + x)]; }
};

struct ChunkPayload {
    ChunkCoord coord;
    AABB bounds;
    std::vector<GeometryBatch> geometry;
    std::vector<ColliderBox> boxColliders;
    std::vector<ColliderMesh> meshColliders;
    std::vector<LightFixture> lights;
    std::vector<PropSpawn> props;
    std::vector<ItemSpawn> items;
    std::vector<DoorSpawn> doors;
    std::vector<TriggerVolume> triggers;
    std::vector<WaterBody> water;
    std::vector<AudioEmitterSpawn> emitters;
    std::vector<EntitySpawnHint> entitySpawns;
    std::vector<glm::vec3> playerSpawnPoints;
    NavGrid nav;
};

} // namespace br::world
