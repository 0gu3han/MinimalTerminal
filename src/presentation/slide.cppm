module;
#include <ncurses.h>
#include <string>
#include <vector>

export module slide;

/// An opening tag and the content up to its closing tag
export struct Element {
  std::string openingTag;
  std::string content;
  size_t contentStart;
  size_t contentEnd;
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
  Slide(const std::string &html, size_t sourceStart = 0);

  /// @brief  Write the content of the slide to the terminal window
  void render();

  /// Whether the slide has code to edit
  bool hasCode() const { return !code.empty(); }

  /// @brief  Let the user type in the code until Esc keeps the changes or
  /// Ctrl-X undoes them
  /// @return Whether the changes were kept
  bool edit();

  /// The code as it goes inside <code>, starting with a line break
  std::string codeHTML() const;

  /// The code as plain text, to run it
  std::string codeText() const;

  size_t codeStart() const { return codeSourceStart; }
  size_t codeEnd() const { return codeSourceEnd; }

  /// The language from class="language-cpp", or empty if none is given
  const std::string &codeLanguage() const { return language; }

  /// Whether the slide shows the output of the code before it
  bool showsOutput() const { return isOutput; }

private:
  std::vector<Line> lines;
  /// The lines of the slide's code block as they show on screen, so they can
  /// be edited
  std::vector<std::string> code;
  size_t codeSourceStart = std::string::npos;
  size_t codeSourceEnd = std::string::npos;
  /// The screen row of the first code line, set when the code is drawn
  int codeRow = 0;
  std::string language;
  bool isOutput = false;

  void addCode(const std::string &html);
  void addOutput(const std::string &html);
  void drawCode();
};
