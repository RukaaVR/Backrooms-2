#pragma once

#include "engine/audio/AudioSystem.h"
#include "engine/renderer/RenderTypes.h"
#include "game/world/ChunkData.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace br::levels {

// Pure, thread-safe chunk generator for one level. Implementations must not
// touch GL, audio or physics: they only produce ChunkPayload data.
class ChunkGenerator {
public:
    virtual ~ChunkGenerator() = default;
    virtual world::ChunkPayload generate(const world::ChunkCoord& coord) const = 0;
    // Edge length of a chunk in metres (XZ). Chunks are square.
    virtual float chunkSize() const = 0;
    // Vertical layers to stream around the player (most levels: 0..0).
    virtual int minLayer() const { return 0; }
    virtual int maxLayer() const { return 0; }
    // Finite levels return their extent in chunks; empty = infinite.
    virtual std::optional<glm::ivec4> chunkBounds() const { return std::nullopt; } // minX, minZ, maxX, maxZ
    // A safe spawn position for a new arrival.
    virtual glm::vec3 spawnPoint() const = 0;
};

// Static description of a level. Registered once at startup.
struct LevelDefinition {
    std::string id;              // "level0", "level37", "levelfun"
    std::string number;          // "0", "37", "FUN"
    std::string name;            // "The Lobby"
    std::string description;     // shown in level select / extras
    int survivalDifficulty = 1;  // 0..5 (Wikidot classification flavour)
    bool unlockedByDefault = false;

    render::EnvironmentDesc environment;
    audio::ReverbPreset reverb = audio::ReverbPreset::Office;
    std::string soundscape;      // soundscape id understood by the audio module
    float streamRadius = 48.0f;  // metres around the player to keep loaded

    // Creates the generator for a world seed.
    std::function<std::unique_ptr<ChunkGenerator>(u64 seed)> createGenerator;
    // Registers this level's materials (called on the main thread on level load).
    std::function<void()> loadMaterials;

    // Where no-clipping / exits from this level may lead (weighted).
    struct Connection {
        std::string levelId;
        float weight = 1.0f;
    };
    std::vector<Connection> exits;
};

class LevelRegistry {
public:
    void add(LevelDefinition def);
    const LevelDefinition* find(std::string_view id) const;
    const std::vector<LevelDefinition>& all() const { return m_levels; }

private:
    std::vector<LevelDefinition> m_levels;
};

} // namespace br::levels
