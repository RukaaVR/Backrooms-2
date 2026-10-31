// TEMPORARY SCAFFOLD (Technical Director, wave 0): silent null mixer.
// OWNER: Audio Engineer — replace with the OpenAL Soft implementation.
#include "engine/audio/AudioSystem.h"

namespace br::audio {

struct AudioSystem::Impl {
    SlotMap<SoundTag, PcmData> sounds;
    float volumes[static_cast<int>(Category::Count)] = {1, 1, 1, 1, 1, 1};
};

AudioSystem::AudioSystem() : m_impl(std::make_unique<Impl>()) {}
AudioSystem::~AudioSystem() = default;
bool AudioSystem::initialize() { return true; }
void AudioSystem::shutdown() {}
bool AudioSystem::hasDevice() const { return false; }
bool AudioSystem::hasEfx() const { return false; }
SoundHandle AudioSystem::createSound(const PcmData& pcm, const std::string&) { return m_impl->sounds.insert(pcm); }
void AudioSystem::destroySound(SoundHandle h) { m_impl->sounds.erase(h); }
float AudioSystem::soundDuration(SoundHandle h) const {
    const PcmData* p = m_impl->sounds.get(h);
    return p ? static_cast<float>(p->samples.size()) / static_cast<float>(p->sampleRate * p->channels) : 0.0f;
}
VoiceHandle AudioSystem::play(SoundHandle, const PlayParams&) { return {}; }
void AudioSystem::stop(VoiceHandle, float) {}
void AudioSystem::stopAll(float) {}
bool AudioSystem::isPlaying(VoiceHandle) const { return false; }
void AudioSystem::setVoicePosition(VoiceHandle, glm::vec3, glm::vec3) {}
void AudioSystem::setVoiceVolume(VoiceHandle, float, float) {}
void AudioSystem::setVoicePitch(VoiceHandle, float) {}
void AudioSystem::setListener(const ListenerState&) {}
void AudioSystem::setCategoryVolume(Category c, float v) { m_impl->volumes[static_cast<int>(c)] = v; }
float AudioSystem::categoryVolume(Category c) const { return m_impl->volumes[static_cast<int>(c)]; }
void AudioSystem::setReverb(ReverbPreset, float) {}
void AudioSystem::setOcclusionQuery(OcclusionQuery) {}
void AudioSystem::setPaused(bool) {}
void AudioSystem::update(float) {}
u32 AudioSystem::activeVoices() const { return 0; }

} // namespace br::audio
