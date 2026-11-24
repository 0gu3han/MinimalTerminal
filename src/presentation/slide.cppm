module;
#include <array>
#include <memory>
#include <ncurses.h>
#include <optional>
#include <string>
#include <vector>

export module slide;

/// An element's tag name, its opening tag and the content up to its closing tag
export struct Element {
  std::string name;
  std::string openingTag;
  std::string content;
  size_t contentStart;
  size_t contentEnd;
};

/// Every element with one of the tag names, in the order they are written,
/// skipping the elements inside them. Throws std::runtime_error if one is not
/// closed.
export std::vector<Element> findAll(const std::string &html,
                                    const std::vector<std::string> &names);

struct WindowDeleter {
  void operator()(WINDOW *window) const { delwin(window); }
};

/// A curses window that is deleted when it goes out of scope
export using Window = std::unique_ptr<WINDOW, WindowDeleter>;

/// A part of the screen to draw in, or none if it is empty
export Window windowAt(int row, int column, int height, int width);

/// A paragraph, bullet, code block or other piece of a slide
struct Block {
  /// The tag name, like "li" or "pre"
  std::string name;
  /// The opening tag, for its color and the code's language
  std::string openingTag{};
  /// The lines of code or output, or the one line of HTML of anything else
  std::vector<std::string> lines{};
  /// What goes in front of a list item, like • or 1.
  std::string marker{};
};

export class Slide {
public:
  /// html is everything inside the slide's article
  /// @param sourceStart Where html starts in the file, to save edited code
  /// @param twoColumns Whether the first <section> goes on the left and the
  /// rest on the right
  Slide(const std::string &html, size_t sourceStart = 0,
        bool twoColumns = false);

  /// Draw the slide on the screen, fitted to its size
  void render() const;

  /// Whether the slide has a code block to edit
  bool hasCode() const { return editable.has_value(); }

  /// The lines of the code block to edit
  const std::vector<std::string> &codeLines() const;

  /// Replace the code block's lines, after they were edited
  void setCode(std::vector<std::string> lines);

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
  /// Which block holds the code to edit
  struct BlockIndex {
    size_t column;
    size_t block;
  };

  bool twoColumns;
  /// How many sections were added, so two columns put the first on the left
  size_t sections = 0;
  std::string title;
  std::string titleTag;
  /// The blocks of each column. Only two columns use the second one.
  std::array<std::vector<Block>, 2> columns;
  std::optional<BlockIndex> editable;
  size_t codeSourceStart = std::string::npos;
  size_t codeSourceEnd = std::string::npos;
  std::string language;
  bool isOutput = false;

  void addBlocks(const std::string &html, size_t sourceStart, size_t column,
                 Block text);
  void addBlock(const Element &element, size_t sourceStart, size_t column);
};
