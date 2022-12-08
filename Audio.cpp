#include "Audio.hpp"

#include "Audio/AudioDevice.hpp"
#include "Filesystem/FileRepository.hpp"

#include "Engine.hpp"

Audio::Audio(Engine *engine) :
	engine(engine) {
	AudioDevice::Initialize();
}

bool Audio::Load(const std::filesystem::path &path) {
	if (!engine->GetMenu()->GetSetting("Audio").value)
		return false;

	wave = std::make_unique<Wave>(FileRepository::registry->GetResourceDirectory() / path);
	sound = std::make_unique<Sound>(*wave);

	return true;
}

void Audio::Play() {
	if (!engine->GetMenu()->GetSetting("Audio").value)
		return;

	SetVolume(1.0f);

	if (sound)
		sound->Play();
}

void Audio::Stop() {
	if (sound)
		sound->Stop();
}

void Audio::Reset() {
	Stop();
	wave.reset();
	sound.reset();
}

float Audio::GetDuration() const {
	if (wave) {
		return static_cast<float>(wave->sampleCount) / wave->channels / wave->sampleRate;
	}

	return 0.0f;
}

void Audio::SetVolume(float volume) {
	if (sound)
		sound->SetVolume(volume);
}