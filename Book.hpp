#pragma once

#include <stack>

#include "Filesystem/FileRepository.hpp"
#include "Filesystem/Serial/Node.hpp"
#include "Rendering/OpenGLFont.hpp"
#include "Utils/StringUtils.hpp"

#include "third_party/fpng/fpng.h"

using namespace SnobasteCPP;

class Book : public LoggableClass {
public:
	enum class LoadMode {
		Soft,
		Hard
	};

	class Page {
	public:
		enum class Type {
			Foreward,
			Poem,
			Story
		};

		class Image {
		public:
			std::string relativePath;
			std::vector<unsigned char> data;
			unsigned width, height;
			unsigned scaledWidth, scaledHeight;
			float ratio = 1.0f;
			uint32_t channels = 4;

			void UpdateRatio() {
				scaledWidth = width;
				scaledHeight = height;
				ratio = width / static_cast<float>(height);
			}

			void Scale(float targetWidth, float targetHeight) {
				UpdateRatio();

				float ratio = targetWidth / static_cast<float>(targetHeight);
				if (ratio > this->ratio) {
					scaledHeight = targetHeight;
					scaledWidth = targetHeight * this->ratio;
				} else {
					scaledWidth = targetWidth;
					scaledHeight = targetWidth / this->ratio;
				}
			}
		};

		LoadMode loadMode = LoadMode::Hard;
		Type type = Type::Poem;
		std::optional<std::size_t> entryNumber;
		std::string title;
		std::string date;
		std::vector<std::string> paragraphs;
		std::unique_ptr<Image> image;
		std::string font;
		std::string titleStyle;
		OpenGLFont::Style style;
		std::vector<std::string> sounds;
		bool spicy = false;

		explicit Page(LoadMode loadMode = LoadMode::Hard) :
			loadMode(loadMode) {

		}
		
		friend Node &operator<<(Node &left, const Page &right) {
			left["number"].Set(right.entryNumber);
			left["title"].Set(right.title);
			left["date"].Set(right.date);
			left["paragraphs"].Set(right.paragraphs);
			left["font"].Set(right.font);
			left["titleStyle"].Set(right.titleStyle);
			left["sounds"].Set(right.sounds);
			left["type"].Set(magic_enum::enum_name(right.type));
			left["spicy"].Set(right.spicy);

			if (right.image)
				left["image"].Set(right.image->relativePath);

			return left;
		}

		friend const Node &operator>>(const Node &left, Page &right) {
			left["number"].Get(right.entryNumber);
			left["title"].Get(right.title);
			left["date"].Get(right.date);
			left["paragraphs"].Get(right.paragraphs);
			left["font"].Get(right.font);
			left["titleStyle"].Get(right.titleStyle);
			left["sounds"].Get(right.sounds);
			left["spicy"].Get(right.spicy);
			auto typeString = left["type"].Get<std::string>();
			if (!typeString.empty())
				right.type = magic_enum::enum_cast<Page::Type>(typeString).value();

			if (right.loadMode == Book::LoadMode::Soft) return left;

			for (auto &paragraph : right.paragraphs) {
				paragraph = StringUtils::ReplaceAll(paragraph, "\t", "          ");
			}

			if (left.HasProperty("image")) {
				right.image = std::make_unique<Page::Image>();
				left["image"].Get(right.image->relativePath);

				fpng::fpng_decode_file(
					std::filesystem::absolute(FileRepository::registry->GetResourceDirectory() / right.image->relativePath).string().c_str(),
					right.image->data,
					right.image->width,
					right.image->height,
					right.image->channels,
					4
				);
				right.image->UpdateRatio();
			}

			// Parse markdown out of paragraphs
			std::tuple<uint32_t, std::size_t, std::size_t> currentSpan;
			std::stack<std::size_t> startStack;
			std::vector<std::string> tokenStack;
			std::string token;
			for (auto &[p, paragraph] : Enumerate(right.paragraphs)) {
				std::size_t i = 0;
				for (auto c = paragraph.begin(); c != paragraph.end();) {
					if (*c == '[') {
						token.clear();
						c = paragraph.erase(c);
						while (c != paragraph.end() && *c != ']') {
							token += *c;
							c = paragraph.erase(c);
						}
						if (c != paragraph.end() && *c == ']')
							c = paragraph.erase(c);

						if (*token.begin() == '/') {
							if (!tokenStack.empty() && *tokenStack.rbegin() == token.substr(1)) {
								// Calculate hash of current permutation
								std::string permutationString;
								for (const auto &permutation : tokenStack)
									permutationString += permutation;

								auto hash = StringUtils::fnv1a_32(permutationString);
								right.permutations.emplace(
									std::make_pair(
										hash,
										tokenStack
									)
								);
								std::get<0>(currentSpan) = hash;
								std::get<1>(currentSpan) = startStack.top();
								startStack.pop();
								std::get<2>(currentSpan) = i;
								right.spans[p].emplace_back(currentSpan);

								tokenStack.pop_back();
							}
						} else {
							startStack.push(i);

							tokenStack.emplace_back(token);
						}
					} else {
						++i;
						++c;
					}
				}
			}

			return left;
		}

		const std::map<uint32_t, std::vector<std::string>> &GetPermutations() const { return permutations; }
		const std::map<std::size_t, std::vector<std::tuple<uint32_t, std::size_t, std::size_t>>> &GetSpans() const { return spans; }

	private:
		std::map<uint32_t, std::vector<std::string>> permutations;
		std::map<std::size_t, std::vector<std::tuple<uint32_t, std::size_t, std::size_t>>> spans;
	};

	explicit Book(const std::filesystem::path &path, LoadMode loadMode = LoadMode::Hard) :
		loadMode(loadMode) {
		this->path = path;
		std::ifstream inFile(path);
		
		try {
			Node node;
			node.ParseStream<Json>(inFile);
			node["test"].GetWithFallback(test, false);
			node["title"].Get(title);
			node["front"].Get(front);
			if (loadMode == LoadMode::Hard && node.HasProperty("back")) {
				back = std::make_unique<Page::Image>();
				node["back"].Get(back->relativePath);

				fpng::fpng_decode_file(
					std::filesystem::absolute(FileRepository::registry->GetResourceDirectory() / back->relativePath).string().c_str(),
					back->data,
					back->width,
					back->height,
					back->channels,
					4
				);
				back->UpdateRatio();
			}
			for (const auto &[number, pageNode] : node["pages"].Get<std::map<std::size_t, Node>>()) {
				Page page(loadMode);
				pageNode >> page;
				pages.emplace(
					std::make_pair(
						number,
						std::move(page)
					)
				);
			}

			if (title.empty() || pages.empty()) {
				valid = false;
				return;
			}
		}
		catch (std::exception &e) {
			valid = false;
			return;
		}

		if (loadMode == LoadMode::Soft) return;

		// Collate all of our style permutations
		for (const auto &page : pages) {
			styles.insert(page.second.GetPermutations().begin(), page.second.GetPermutations().end());
		}

		logger.WriteDebug("Styles: ");
		for (auto &[i, permutation] : Enumerate(styles)) {
			logger.WriteDebug("\t", i);
			for (auto &token : permutation.second) {
				logger.WriteDebug("\t\t", token);
			}
		}
	}

	Book(Book &&right) noexcept = default;

	const std::map<std::size_t, Page> &GetPages() const { return pages; }
	std::map<std::size_t, Page> &GetPages() { return pages; }
	const std::map<uint32_t, std::vector<std::string>> &GetStyles() const { return styles; }

	const std::string &GetTitle() const { return title; }
	const std::string &GetFront() const { return front; }
	std::unique_ptr<Page::Image> &GetBack() { return back; }

	const std::filesystem::path &GetPath() const { return path; }

	const bool IsValid() const { return valid; }
	const bool IsTest() const { return test; }

private:
	std::filesystem::path path;

	std::string title;
	std::string front;
	std::unique_ptr<Page::Image> back;

	std::map<std::size_t, Page> pages;
	std::map<uint32_t, std::vector<std::string>> styles;

	LoadMode loadMode = LoadMode::Hard;

	bool valid = true;
	bool test = false;
};