module;
#include <string>
#include <vector>

export module layout;

/// A box on the screen
export struct Rect {
  int row;
  int col;
  int height;
  int width;
};

export enum class LayoutType { TitleAndContent, TwoContent, Subsection };

/// The layout from an <article> opening tag, e.g. class="two-content"
export LayoutType layoutFromTag(const std::string &openingTag);

/// Where each area of a slide goes. right is only used by TwoContent, and in
/// Subsection left holds the subtitle.
export struct Areas {
  Rect title;
  Rect left;
  Rect right;
};

/// The areas of a slide for a layout and screen size
export Areas areasFor(LayoutType layout, int screenHeight, int screenWidth);

/// Where each part of the code editor goes: the code on top and what it
/// printed below, each under a one-row header, with the key help on the last
/// row
export struct SplitAreas {
  Rect editorHeader;
  Rect editor;
  Rect outputHeader;
  Rect output;
  Rect status;
};

/// The split editor areas for a screen size
export SplitAreas splitAreas(int screenHeight, int screenWidth);

/// How many columns the text takes on screen. Tags take none and an entity
/// like &amp; takes one.
export int displayWidth(const std::string &html);

/// Break the text into lines no wider than width, only between words unless a
/// word is wider than a whole line. Each line closes the <b>, <i> and <u> tags
/// still open at its end and opens them again on the next line, so every line
/// can be drawn on its own.
export std::vector<std::string> wrap(const std::string &html, int width);

/// Cut the text to fit in width, ending it with … if anything was cut off
export std::string truncate(const std::string &html, int width);

/// Replace each tab with the spaces up to the next multiple of 8 columns, the
/// way a terminal shows it, so the columns can be counted
export std::string expandTabs(const std::string &html);

/// The same for a line of plain text (code, not HTML)
export std::string expandCodeTabs(const std::string &text);

/// How much of a line of plain text (code, not HTML) fits in width columns
export struct Fit {
  /// The bytes of the line to draw
  size_t bytes;
  /// Whether the rest was cut off, so a … goes in the last column
  bool cut;
};

export Fit fitText(const std::string &text, int width);

export enum class BlockKind { Bullet, Code, Output };

/// One bullet, code block or block of program output in an area of a slide
export struct Block {
  BlockKind kind;
  /// The tag it came from, for its color and language. Layout ignores it.
  std::string openingTag;
  /// A bullet has one line of HTML, code has its decoded lines and output has
  /// its lines still encoded
  std::vector<std::string> lines;
};

export enum class RowKind { Blank, Bullet, Code, Output, Overflow };

/// One row of an area and what to draw on it
export struct Row {
  RowKind kind;
  /// The block and line of the block the row shows
  size_t block = 0;
  size_t line = 0;
  /// Where the text starts in the area
  int column = 0;
  /// The HTML of a bullet or output row, or the plain text of a code row
  std::string text{};
  /// Whether this is the first row of a bullet, which has the marker
  bool first = false;
  /// Whether a code row was cut off and ends with …
  bool cut = false;
};

/// Columns between a bullet's marker and its text
export const int bulletIndent = 2;

/// Columns of code background on each side of the code
export const int codePadding = 1;

/// The rows of an area width columns wide and height rows tall. Bullets wrap,
/// code and output lines are cut off with …, and if the rows do not all fit
/// the last one that does is an Overflow row. Tabs in code and output are
/// expanded.
export std::vector<Row> layOut(const std::vector<Block> &blocks, int width,
                               int height);
