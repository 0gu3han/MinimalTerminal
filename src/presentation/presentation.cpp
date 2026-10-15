#include "presentation.hpp"

#include <string>

namespace {
std::vector<std::string> extract_tag_blocks(const std::string& html, const std::string& tag)
{
	std::vector<std::string> blocks;

	const std::string open_tag = "<" + tag + ">";
	const std::string close_tag = "</" + tag + ">";

	std::size_t cursor = 0;
	while (true) {
		const auto open_pos = html.find(open_tag, cursor);
		if (open_pos == std::string::npos) {
			break;
		}

		const auto content_start = open_pos + open_tag.size();
		const auto close_pos = html.find(close_tag, content_start);
		if (close_pos == std::string::npos) {
			break;
		}

		blocks.push_back(html.substr(content_start, close_pos - content_start));
		cursor = close_pos + close_tag.size();
	}

	return blocks;
}
} // namespace

Presentation::Presentation(std::string rawHTML)
	: currSlide(0)
{
	auto slide_blocks = extract_tag_blocks(rawHTML, "article");

	for (const auto& slide_html : slide_blocks) {
		slides.emplace_back(slide_html);
	}

	if (slides.empty()) {
		currSlide = -1;
	}
}

void Presentation::start()
{
}
