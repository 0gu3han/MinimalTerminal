module;
#include <ncurses.h>
#include <string>
#include <vector>

export module slide;

/// An opening tag and the content up to its closing tag
export struct Element {
  std::string openingTag;
  std::string content;
};

/// Every element with one of the tags, in the order they are written
export std::vector<Element> findAll(const std::string &html,
                                    const std::vector<std::string> &tags);

export class Slide {
public:
  /// @brief
  /// @param rawHTML The contents of the slide's <article> element
  Slide(std::string rawHTML);

  /// @brief  Write the content of the slide to the terminal window
  void render();

private:
  std::string heading;
  attr_t headingColor = A_NORMAL;
  std::vector<std::string> bullets;
  std::vector<attr_t> bulletColors;
  std::vector<std::string> codeLines;
};