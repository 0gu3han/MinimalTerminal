module;
#include <string>
#include <vector>

export module styler;

/// @brief Foreground colors the terminal can show. Default means "use the terminal's own color".
export enum class Color { Default, Black, Red, Green, Yellow, Blue, Magenta, Cyan, White };

/// @brief Converts a color name such as "red" (case-insensitive) into a Color.
/// @return Color::Default if the name is not one of the 8 standard terminal colors
export Color colorFromName(const std::string& name);

/// @brief How a piece of text should look on screen.
export struct Style
{
    Color color = Color::Default;
    bool bold = false;
    bool italic = false;
    bool underline = false;
};

/// @brief Built-in themes, see "Themes" in the README.
export enum class Theme { Light, Dark };

/// @brief Works out how text should look and turns that into ANSI escape codes.
export class Styler
{
public:
    Styler(Theme theme = Theme::Dark);

    /// @return The style for a slide heading in the current theme
    Style headingStyle() const;

    /// @return The style for bullet text in the current theme
    Style bodyStyle() const;

    /// @brief Wraps plain text in the ANSI codes for a style, followed by a reset.
    std::string apply(const Style& style, const std::string& text) const;

    /// @brief Converts text containing inline <b>, <i> and <u> tags into styled terminal text.
    /// Nested tags inherit the style of their parent; unknown tags are dropped.
    /// @param html The text between a block's opening and closing tag
    /// @param base The style of the surrounding block (e.g. the bullet's style)
    std::string render(const std::string& html, const Style& base) const;

    /// @brief Draws lines of code as a shaded box with a one-space inner margin.
    /// Whitespace is kept as-is and every line is padded to the widest one so the box is rectangular.
    /// Inline tags are not interpreted. Lines are separated by '\n' (no trailing newline).
    std::string codeBlock(const std::vector<std::string>& lines) const;

private:
    Theme theme;
};
