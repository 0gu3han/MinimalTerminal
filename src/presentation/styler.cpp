module;
#include <ncurses.h>
#include <string>

module styler;

// same order as the curses colors, so each name's index is its color number
static const std::string colorNames[] = {"black", "red",     "green", "yellow",
                                         "blue",  "magenta", "cyan",  "white"};

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

std::string decodeEntities(std::string text) {
  const std::string entities[][2] = {
      {"&lt;", "<"},  {"&gt;", ">"},  {"&quot;", "\""},
      {"&#39;", "'"}, {"&amp;", "&"}, // &amp; last so &amp;lt; becomes &lt;
  };
  for (const auto &[entity, character] : entities) {
    size_t pos = 0;
    while ((pos = text.find(entity, pos)) != std::string::npos) {
      text.replace(pos, entity.size(), character);
      pos += character.size();
    }
  }
  return text;
}

attr_t colorOf(const std::string &openingTag) {
  size_t colorStart = openingTag.find("color:");
  if (colorStart == std::string::npos)
    return A_NORMAL;

  // skip past "color:" and any spaces after it
  colorStart = openingTag.find_first_not_of(' ', colorStart + 6);
  for (short color = COLOR_BLACK; color <= COLOR_WHITE; ++color)
    if (openingTag.compare(colorStart, colorNames[color].size(),
                           colorNames[color]) == 0)
      return COLOR_PAIR(color + 1);
  return A_NORMAL;
}

void drawStyled(const std::string &html, attr_t style) {
  attrset(style);
  for (size_t textStart = 0; textStart < html.size();) {
    size_t tagStart = html.find('<', textStart);
    // text between tags has no tags in it, so it is safe to decode
    addstr(decodeEntities(html.substr(textStart, tagStart - textStart)).c_str());
    if (tagStart == std::string::npos)
      break;

    // each tag turns bold, italic or underline on or off
    bool closing = html[tagStart + 1] == '/';
    char tag = html[tagStart + 1 + closing];
    attr_t attribute = tag == 'b' ? A_BOLD : tag == 'i' ? A_ITALIC : A_UNDERLINE;
    if (closing)
      attroff(attribute);
    else
      attron(attribute);
    textStart = html.find('>', tagStart) + 1;
  }
  attrset(A_NORMAL);
}
