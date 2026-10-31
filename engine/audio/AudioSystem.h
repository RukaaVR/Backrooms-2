#pragma once

#include "engine/core/Handle.h"
#include "engine/core/Math.h"
#include "engine/core/Types.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace br::audio {

struct SoundTag {};
struct VoiceTag {};
using SoundHandle = Handle<SoundTag>;
using VoiceHandle = Handle<VoiceTag>;

enum class Category : u8 { Master, Ambient, Effects, Music, Voice, UI, Count };

// Mono PCM for 3D sources (OpenAL only spatialises mono); stereo for beds/music.
struct PcmData {
    int sampleRate = 48000;
    int channels = 1;
    std::vector<float> samples; // interleaved, [-1, 1]
};

struct PlayParams {
    Category category = Category::Effects;
    bool spatial = true;
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    float volume = 1.0f;
    float pitch = 1.0f;
    bool loop = false;
    float referenceDistance = 1.5f; // full volume inside this radius
    float maxDistance = 40.0f;
    float rolloff = 1.0f;
    int priority = 0;               // higher survives voice stealing
    bool occludable = true;         // participates in occlusion low-pass
    float startOffsetSeconds = 0.0f;
};

// Reverb character applied to the listener's environment (EFX EAX reverb).
enum class ReverbPreset : u8 {
    None,
    SmallRoom,
    Office,          // Level 0 / 4: carpeted, drop ceiling, dry
    Hallway,
    Concrete,        // Level 1 / 3: hard, long
    Pipes,           // Level 2: metallic, ringing
    TiledPool,       // Level 37: very bright, long decay
    Cave,            // Level 8
    Underwater,
    Outdoor,         // Level 9 / 10 / 94
    Hotel,           // Level 5
    Stadium,         // Level 188 courtyard
    Count
};

struct ListenerState {
    glm::vec3 position{0.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec3 velocity{0.0f};
    bool underwater = false;
};

// Callback used for occlusion: returns 0 (clear) .. 1 (fully blocked) between
// the listener and a source. Implemented by the game with physics raycasts.
using OcclusionQuery = std::function<float(glm::vec3 listener, glm::vec3 source)>;

// OpenAL Soft backed mixer. Falls back to a silent null device when no audio
// output exists (CI, headless) so gameplay code never needs to special-case it.
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    bool initialize();
    void shutdown();
    bool hasDevice() const;   // false = null backend
    bool hasEfx() const;

    SoundHandle createSound(const PcmData& pcm, const std::string& debugName = {});
    void destroySound(SoundHandle h);
    float soundDuration(SoundHandle h) const;

    VoiceHandle play(SoundHandle sound, const PlayParams& params);
    void stop(VoiceHandle v, float fadeOutSeconds = 0.0f);
    void stopAll(float fadeOutSeconds = 0.0f);
    bool isPlaying(VoiceHandle v) const;
    void setVoicePosition(VoiceHandle v, glm::vec3 position, glm::vec3 velocity = {});
    void setVoiceVolume(VoiceHandle v, float volume, float fadeSeconds = 0.0f);
    void setVoicePitch(VoiceHandle v, float pitch);

    void setListener(const ListenerState& listener);
    void setCategoryVolume(Category c, float volume); // 0..1 (settings menu)
    float categoryVolume(Category c) const;
    void setReverb(ReverbPreset preset, float blendSeconds = 1.0f);
    void setOcclusionQuery(OcclusionQuery query);
    void setPaused(bool paused); // pauses world sounds; UI/music continue

    // Advances fades, occlusion, voice recycling. Call once per frame.
    void update(float dt);

    u32 activeVoices() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace br::audio
