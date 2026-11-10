module;
#include <string>

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