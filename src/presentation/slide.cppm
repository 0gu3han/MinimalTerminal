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

/// One row of the slide and how to draw it
struct Line {
  std::string text;
  attr_t style;
  int column;
};

export class Slide {
public:
  /// html is everything inside the slide's article
  Slide(const std::string &html);

  /// @brief  Write the content of the slide to the terminal window
  void render();

private:
  std::vector<Line> lines;

  void addCode(const std::string &code);
};
