#pragma once

#include "engine/core/Math.h"
#include "engine/core/Types.h"

#include <memory>
#include <vector>

namespace br::physics {

// Object layers. Collision filtering (who collides with whom) is defined in
// PhysicsWorld.cpp; keep this list and that table in sync.
enum class Layer : u8 {
    Static,     // world geometry (never moves)
    Dynamic,    // simulated props, doors, debris that matters for gameplay
    Debris,     // small cosmetic bodies; collide with Static only
    Character,  // player / entity proxies (kinematic)
    Sensor,     // triggers: no collision response, overlap events only
    Count
};

enum class MotionType : u8 { Static, Kinematic, Dynamic };

// Acoustic / footstep surface identity attached to bodies and static geometry.
enum class Surface : u8 {
    Default,
    Carpet,
    WetCarpet,
    Concrete,
    Tile,
    Wood,
    Metal,
    Grass,
    Dirt,
    Gravel,
    Water,
    Glass,
    Plastic,
    Cardboard,
    Rock,
    Asphalt,
    Count
};

struct BodyId {
    u32 value = 0xFFFFFFFFu;
    bool valid() const { return value != 0xFFFFFFFFu; }
    friend bool operator==(const BodyId&, const BodyId&) = default;
};

struct ShapeDesc {
    enum class Kind : u8 { Box, Sphere, Capsule, Cylinder, ConvexHull, TriangleMesh };
    Kind kind = Kind::Box;
    glm::vec3 halfExtents{0.5f};   // Box
    float radius = 0.5f;           // Sphere / Capsule / Cylinder
    float halfHeight = 0.5f;       // Capsule (cylindrical part) / Cylinder
    std::vector<glm::vec3> points; // ConvexHull points or TriangleMesh vertices
    std::vector<u32> indices;      // TriangleMesh indices (static bodies only)
    glm::vec3 localOffset{0.0f};   // centre-of-shape offset from body origin
    glm::quat localRotation{1, 0, 0, 0};

    static ShapeDesc box(glm::vec3 halfExtents) {
        ShapeDesc s;
        s.kind = Kind::Box;
        s.halfExtents = halfExtents;
        return s;
    }
    static ShapeDesc sphere(float r) {
        ShapeDesc s;
        s.kind = Kind::Sphere;
        s.radius = r;
        return s;
    }
    static ShapeDesc capsule(float halfHeight, float r) {
        ShapeDesc s;
        s.kind = Kind::Capsule;
        s.halfHeight = halfHeight;
        s.radius = r;
        return s;
    }
    static ShapeDesc cylinder(float halfHeight, float r) {
        ShapeDesc s;
        s.kind = Kind::Cylinder;
        s.halfHeight = halfHeight;
        s.radius = r;
        return s;
    }
};

struct BodyDesc {
    std::vector<ShapeDesc> shapes; // >1 creates a compound shape
    glm::vec3 position{0.0f};
    glm::quat rotation{1, 0, 0, 0};
    MotionType motion = MotionType::Static;
    Layer layer = Layer::Static;
    float mass = 0.0f;             // kg; 0 = derive from density
    float density = 500.0f;        // kg/m^3 when mass == 0
    float friction = 0.6f;
    float restitution = 0.1f;
    float linearDamping = 0.05f;
    float angularDamping = 0.1f;
    float gravityFactor = 1.0f;
    bool isSensor = false;
    bool allowSleeping = true;
    bool continuousCollision = false; // thrown objects
    Surface surface = Surface::Default;
    u64 userData = 0;              // game-side owner id
};

struct RayHit {
    BodyId body;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    float distance = 0.0f;
    Surface surface = Surface::Default;
    u64 userData = 0;
};

// Emitted for new contacts above a small impulse threshold; drives impact
// sounds and gameplay reactions (e.g. entities hearing a thrown bottle).
struct ContactEvent {
    BodyId a, b;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
    float impactSpeed = 0.0f; // relative normal velocity, m/s
    Surface surfaceA = Surface::Default, surfaceB = Surface::Default;
    u64 userDataA = 0, userDataB = 0;
};

struct SensorEvent {
    BodyId sensor, other;
    bool entered = true; // false = exited
    u64 sensorUserData = 0, otherUserData = 0;
};

struct QueryFilter {
    bool hitStatic = true;
    bool hitDynamic = true;
    bool hitDebris = false;
    bool hitCharacters = false;
    bool hitSensors = false;
    BodyId ignore;
};

} // namespace br::physics
