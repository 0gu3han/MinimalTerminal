module;
#include <cctype>
#include <string>
#include <utility>

module styler;

Color colorFromName(const std::string &name) {
  std::string lower;
  for (unsigned char ch : name)
    lower += static_cast<char>(std::tolower(ch));

  const std::pair<const char *, Color> names[] = {
      {"black", Color::Black}, {"red", Color::Red},
      {"green", Color::Green}, {"yellow", Color::Yellow},
      {"blue", Color::Blue},   {"magenta", Color::Magenta},
      {"cyan", Color::Cyan},   {"white", Color::White},
  };
  for (const auto &[colorName, color] : names)
    if (lower == colorName)
      return color;
  return Color::Default;
}
