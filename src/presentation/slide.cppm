module;
#include <array>
#include <memory>
#include <ncurses.h>
#include <optional>
#include <string>
#include <vector>

export module slide;
import layout;

/// An opening tag and the content up to its closing tag
export struct Element {
  std::string openingTag;
  std::string content;
  size_t contentStart;
  size_t contentEnd;
};

/// Every element with one of the tags, in the order they are written. Throws
/// std::runtime_error if one is missing its > or its closing tag.
export std::vector<Element> findAll(const std::string &html,
                                    const std::vector<std::string> &tags);

struct WindowDeleter {
  void operator()(WINDOW *window) const { delwin(window); }
};

/// A curses window that is deleted when it goes out of scope
export using Window = std::unique_ptr<WINDOW, WindowDeleter>;

/// A window covering the rect, or none if the rect is empty or off the screen
export Window windowFor(const Rect &rect);

/// Draw the blocks in the window, laid out to fit it
export void drawBlocks(WINDOW *window, const std::vector<Block> &blocks);

export class Slide {
public:
  /// html is everything inside the slide's article
  /// @param sourceStart Where html starts in the file, to save edited code
  /// @param layout From the article's class
  Slide(const std::string &html, size_t sourceStart = 0,
        LayoutType layout = LayoutType::TitleAndContent);

  /// Draw each area of the slide in its own window, sized for the screen.
  /// The windows are only copied to the screen, so call doupdate() after.
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

  LayoutType layout;
  std::string title;
  std::string titleTag;
  /// The <h2> under the title of a subsection slide
  std::string subtitle;
  std::string subtitleTag;
  /// The bullets, code and output of each area. Only TwoContent uses the
  /// second one.
  std::array<std::vector<Block>, 2> columns;
  std::optional<BlockIndex> editable;
  size_t codeSourceStart = std::string::npos;
  size_t codeSourceEnd = std::string::npos;
  std::string language;
  bool isOutput = false;

  void addBlocks(const std::string &html, size_t sourceStart, size_t column);
  void addBlock(const Element &element, size_t sourceStart, size_t column);
};
