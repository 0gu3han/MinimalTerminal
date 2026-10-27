module;
#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

module styler;

// ANSI SGR (Select Graphic Rendition) codes: ESC [ <numbers separated by ;> m
static const std::string ESC = "\033[";
static const std::string RESET = "\033[0m";

// Foreground color numbers: 30-37 are the standard colors, 39 is "default".
static int colorCode(Color color)
{
    switch (color) {
    case Color::Black:   return 30;
    case Color::Red:     return 31;
    case Color::Green:   return 32;
    case Color::Yellow:  return 33;
    case Color::Blue:    return 34;
    case Color::Magenta: return 35;
    case Color::Cyan:    return 36;
    case Color::White:   return 37;
    default:             return 39;
    }
}

Color colorFromName(const std::string& name)
{
    std::string lower;
    for (unsigned char ch : name)
        lower += static_cast<char>(std::tolower(ch));

    const std::pair<const char*, Color> names[] = {
        {"black", Color::Black},     {"red", Color::Red},         {"green", Color::Green},
        {"yellow", Color::Yellow},   {"blue", Color::Blue},       {"magenta", Color::Magenta},
        {"cyan", Color::Cyan},       {"white", Color::White},
    };
    for (const auto& [colorName, color] : names)
        if (lower == colorName)
            return color;
    return Color::Default;
}

// Build the escape sequence that switches a style on.
// Always starts with a reset (0) so styles never leak from the previous text.
static std::string startCodes(const Style& style)
{
    std::string codes = "0";
    if (style.bold)
        codes += ";1";
    if (style.italic)
        codes += ";3";
    if (style.underline)
        codes += ";4";
    if (style.color != Color::Default)
        codes += ";" + std::to_string(colorCode(style.color));
    return ESC + codes + "m";
}

Styler::Styler(Theme theme)
    : theme(theme)
{
}

// Headings are bold and colored; the color is chosen so it stays readable on the theme's background.
Style Styler::headingStyle() const
{
    Style style;
    style.bold = true;
    style.color = (theme == Theme::Dark) ? Color::Cyan : Color::Blue;
    return style;
}

// Body text keeps the terminal's own color, which already suits a light or dark terminal.
Style Styler::bodyStyle() const
{
    return Style{};
}

std::string Styler::apply(const Style& style, const std::string& text) const
{
    return startCodes(style) + text + RESET;
}

// Number of characters on screen. UTF-8 continuation bytes (10xxxxxx) don't add a column.
static size_t textWidth(const std::string& text)
{
    size_t width = 0;
    for (unsigned char ch : text)
        if ((ch & 0xC0) != 0x80)
            ++width;
    return width;
}

std::string Styler::codeBlock(const std::vector<std::string>& lines) const
{
    size_t widest = 0;
    for (const std::string& line : lines)
        widest = std::max(widest, textWidth(line));

    // 256-color background (48;5;N): dark grey on the dark theme, light grey on the light theme.
    const std::string background = (theme == Theme::Dark) ? "48;5;236" : "48;5;254";
    const std::string foreground = (theme == Theme::Dark) ? "37" : "30";
    const std::string start = ESC + "0;" + background + ";" + foreground + "m";

    std::string out;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0)
            out += '\n';
        // 4 spaces of indent outside the box, 1 space of padding inside it
        out += "    " + start + " " + lines[i] + std::string(widest - textWidth(lines[i]), ' ') + " " + RESET;
    }
    return out;
}

std::string Styler::render(const std::string& html, const Style& base) const
{
    std::string out;
    // The top of the stack is the style currently in effect. Opening a tag pushes a
    // modified copy of the top (that is the inheritance); closing a tag pops it.
    std::vector<Style> stack{base};
    std::string pendingText;

    // Emit the text collected so far using the style currently in effect.
    auto flush = [&]() {
        if (!pendingText.empty()) {
            out += apply(stack.back(), pendingText);
            pendingText.clear();
        }
    };

    size_t i = 0;
    while (i < html.size()) {
        if (html[i] != '<') {
            pendingText += html[i++];
            continue;
        }

        size_t end = html.find('>', i);
        if (end == std::string::npos) { // stray "<" with no closing ">", treat as text
            pendingText += html.substr(i);
            break;
        }

        std::string tag = html.substr(i + 1, end - i - 1);
        i = end + 1;

        const bool closing = !tag.empty() && tag[0] == '/';
        if (closing)
            tag.erase(0, 1);

        // Style changes only matter for tags we support; flush first so the
        // text before the tag keeps the old style.
        if (tag != "b" && tag != "i" && tag != "u")
            continue; // unknown tag: drop it, keep its text

        flush();
        if (closing) {
            if (stack.size() > 1) // ignore an unmatched closing tag
                stack.pop_back();
        } else {
            Style child = stack.back();
            if (tag == "b")
                child.bold = true;
            else if (tag == "i")
                child.italic = true;
            else
                child.underline = true;
            stack.push_back(child);
        }
    }

    flush();
    return out;
}
