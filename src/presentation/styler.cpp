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
  init_pair(keywordPair, COLOR_MAGENTA, 236);
  init_pair(stringPair, COLOR_GREEN, 236);
  // a lighter grey so comments are quieter than the code
  init_pair(commentPair, 245, 236);
  init_pair(numberPair, COLOR_YELLOW, 236);
  init_pair(preprocessorPair, COLOR_CYAN, 236);
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

std::string encodeEntities(std::string text) {
  const std::string entities[][2] = {
      {"&", "&amp;"}, // & first so the & of &lt; is not encoded again
      {"<", "&lt;"},
      {">", "&gt;"},
  };
  for (const auto &[character, entity] : entities) {
    size_t pos = 0;
    while ((pos = text.find(character, pos)) != std::string::npos) {
      text.replace(pos, character.size(), entity);
      pos += entity.size();
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

// the style a tag name turns on, or none if it does not style text
static attr_t attributeOf(const std::string &name) {
  if (name == "b" || name == "strong")
    return A_BOLD;
  if (name == "i" || name == "em")
    return A_ITALIC;
  if (name == "u")
    return A_UNDERLINE;
  return A_NORMAL;
}

void drawStyled(WINDOW *window, const std::string &html, attr_t style) {
  wattrset(window, style);
  for (size_t textStart = 0; textStart < html.size();) {
    size_t tagStart = html.find('<', textStart);
    // text between tags has no tags in it, so it is safe to decode
    waddstr(
        window,
        decodeEntities(html.substr(textStart, tagStart - textStart)).c_str());
    if (tagStart == std::string::npos)
      break;

    // <b>, <i> and <u> turn bold, italic or underline on or off; any other
    // tag, like <span> or <br>, changes nothing
    const size_t tagEnd = html.find('>', tagStart);
    const bool closing = html[tagStart + 1] == '/';
    const size_t nameStart = tagStart + 1 + closing;
    const std::string name = html.substr(
        nameStart, html.find_first_of(" />", nameStart) - nameStart);
    if (const attr_t attribute = attributeOf(name)) {
      if (closing)
        wattroff(window, attribute);
      else
        wattron(window, attribute);
    }
    if (tagEnd == std::string::npos)
      break;
    textStart = tagEnd + 1;
  }
  wattrset(window, A_NORMAL);
}
