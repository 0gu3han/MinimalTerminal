module;
#include <cctype>
#include <ncurses.h>
#include <string>
#include <utility>

module styler;

void startColors() {
  start_color();
  // let -1 mean the terminal's own color
  use_default_colors();
  // pair n + 1 draws color n
  for (short color = COLOR_BLACK; color <= COLOR_WHITE; ++color)
    init_pair(color + 1, color, -1);
  // white on dark grey for code
  init_pair(codePair, COLOR_WHITE, 236);
}

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
