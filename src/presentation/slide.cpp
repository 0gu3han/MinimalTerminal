#include "slide.hpp"

#include <cctype>
#include <iostream>
#include <string>

namespace {
std::string trim(const std::string& text)
{
    const auto first_pos = text.find_first_not_of(" \t\n\r");
    if (first_pos == std::string::npos) {
        return "";
    }

    const auto last_pos = text.find_last_not_of(" \t\n\r");
    return text.substr(first_pos, last_pos - first_pos + 1);
}

std::string strip_tags(const std::string& html)
{
    std::string out;
    out.reserve(html.size());

    // HTML stripping for supported inline tags.
    bool in_tag = false;
    for (const char ch : html) {
        if (ch == '<') {
            in_tag = true;
            continue;
        }

        if (ch == '>') {
            in_tag = false;
            continue;
        }

        if (!in_tag) {
            out.push_back(ch);
        }
    }

    return trim(out);
}

std::string extract_first_tag_content(const std::string& html, const std::string& tag)
{
    const std::string open_tag = "<" + tag + ">";
    const std::string close_tag = "</" + tag + ">";

    const auto open_pos = html.find(open_tag);
    if (open_pos == std::string::npos) {
        return "";
    }

    const auto content_start = open_pos + open_tag.size();
    const auto close_pos = html.find(close_tag, content_start);
    if (close_pos == std::string::npos) {
        return "";
    }

    return strip_tags(html.substr(content_start, close_pos - content_start));
}

std::vector<std::string> extract_all_tag_contents(const std::string& html, const std::string& tag)
{
    std::vector<std::string> values;

    const std::string open_tag = "<" + tag + ">";
    const std::string close_tag = "</" + tag + ">";

    std::size_t cursor = 0;
    while (true) {
        // Walk forward one tag-pair at a time and collect non-empty bodies.
        const auto open_pos = html.find(open_tag, cursor);
        if (open_pos == std::string::npos) {
            break;
        }

        const auto content_start = open_pos + open_tag.size();
        const auto close_pos = html.find(close_tag, content_start);
        if (close_pos == std::string::npos) {
            break;
        }

        const std::string content = strip_tags(html.substr(content_start, close_pos - content_start));
        if (!content.empty()) {
            values.push_back(content);
        }

        cursor = close_pos + close_tag.size();
    }

    return values;
}
} // namespace

Slide::Slide(std::string rawHTML)
{
    // Parse a simple schema: heading from h1, body from li (or p as fallback).
    heading = extract_first_tag_content(rawHTML, "h1");
    bullets = extract_all_tag_contents(rawHTML, "li");

    // Fallback for early demo slides that use paragraphs instead of bullet lists.
    if (bullets.empty()) {
        bullets = extract_all_tag_contents(rawHTML, "p");
    }
}

void Slide::render()
{
    // Render plain terminal-friendly output for now.
    if (!heading.empty()) {
        std::cout << heading << '\n';
    } else {
        std::cout << "(untitled slide)\n";
    }

    std::cout << '\n';

    if (bullets.empty()) {
        std::cout << "- (no content)\n";
        return;
    }

    for (const auto& bullet : bullets) {
        std::cout << "- " << bullet << '\n';
    }
}
