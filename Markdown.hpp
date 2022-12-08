#pragma once

#include "third_party/magic_enum/magic_enum.hpp"
#include "Rendering/OpenGLFont.hpp"

using namespace SnobasteCPP;

class Markdown {
public:
	static std::optional<OpenGLFont::SpanItem> GetSpanForMarkdown(const std::vector<std::string> &permutation) {
		std::optional<OpenGLFont::Style> style = std::nullopt;
		std::optional<Color> color = std::nullopt;
		std::optional<float> scale = std::nullopt;

		for (const auto &token : permutation) {
			if (auto possible = magic_enum::enum_cast<OpenGLFont::Style>(token); possible.has_value())
				style = possible.value();
			if (auto possible = Color::colors.find(token); possible != Color::colors.end())
				color = possible->second;
		}

		if (style) {
			if (color) {
				return OpenGLFont::SpanItem(*style, *color);
			} else {
				return OpenGLFont::SpanItem(*style);
			}
		} else if (color) {
			return OpenGLFont::SpanItem(OpenGLFont::Style::Regular, *color);
		} else {
			return std::nullopt;
		}
	}
};