#pragma once

#include <string>
#include <optional>

class GhostWriter {
public:
	GhostWriter() = default;

	void SetText(std::string &text, float totalTime = -1.0f);
	std::pair<bool, std::string> GetText(float deltaTime);
	std::size_t GetPos() const { return pos; }

private:
	std::optional<std::reference_wrapper<std::string>> string = std::nullopt;
	std::size_t pos = 0;
	std::size_t size = 0;

	float accum = 0.0f;
	float totalTime = -1.0f;
};