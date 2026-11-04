module;
#include <string>

export module styler;

/// @brief Foreground colors the terminal can show. Default means "use the
/// terminal's own color".
export enum class Color {
  Default,
  Black,
  Red,
  Green,
  Yellow,
  Blue,
  Magenta,
  Cyan,
  White
};

/// @brief Converts a color name such as "red" (case-insensitive) into a Color.
/// @return Color::Default if the name is not one of the 8 standard terminal
/// colors
export Color colorFromName(const std::string &name);
