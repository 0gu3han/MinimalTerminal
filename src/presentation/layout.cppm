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

/// Where each area of a slide goes. right is only used by TwoContent.
export struct Areas {
  Rect title;
  Rect left;
  Rect right;
};

/// The areas of a slide for a layout and screen size
export Areas areasFor(LayoutType layout, int screenHeight, int screenWidth);
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
