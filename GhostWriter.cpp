#include "GhostWriter.hpp"

constexpr float CharTime = 0.05f;

void GhostWriter::SetText(std::string &text, float totalTime) { 
	string = text;
	accum = 0.0f;
	pos = 0;
	this->totalTime = totalTime;

	// Get the real size of the string,
	// accounting for Unicode characters
	size = 0;
	for (auto i = 0; i < string->get().size(); ++i) {
		while (string->get()[i] & 0x80 && i < string->get().size())
			++i;
		++size;
	}
}

std::pair<bool, std::string> GhostWriter::GetText(float deltaTime) { 
	if (!string) return { false, "" };

	accum += deltaTime;
	if (accum >= (totalTime >= 0.0f ? totalTime / size : CharTime) && pos < string->get().size()) {
		// Did we hit a Unicode character?
		// Make sure to grab every byte.
		if (string->get()[pos] & 0x80) {
			while (string->get()[++pos] & 0x80);
		} else {
			++pos;
		}
		accum = accum - (totalTime >= 0.0f ? totalTime / size : CharTime);
	}

	auto test = string->get().substr(0, pos);

	return { pos == string->get().size(),  string->get().substr(0, pos) };
}