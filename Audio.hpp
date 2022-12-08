#pragma once

#include <filesystem>

#include "Audio/Wave.hpp"
#include "Audio/Sound.hpp"

using namespace SnobasteCPP;

class Engine;
class Audio {
public:
	Audio(Engine *engine);

	bool Load(const std::filesystem::path &path);
	void Play();
	void Stop();
	void Reset();

	void SetVolume(float volume);

	float GetDuration() const;

private:
	Engine *engine;

	std::unique_ptr<Wave> wave;
	std::unique_ptr<Sound> sound;
};