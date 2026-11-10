module;
#include <string>

module layout;

const int MARGIN = 2;      // empty columns on the left and right
const int TITLE_ROW = 1;
const int GAP = 1;         // blank rows between the title and the content
const int COLUMN_GAP = 4;  // columns between the two areas in two content

LayoutType layoutFromTag(const std::string &openingTag) {
  if (openingTag.find("two-content") != std::string::npos)
    return LayoutType::TwoContent;
  if (openingTag.find("subsection") != std::string::npos)
    return LayoutType::Subsection;
  return LayoutType::TitleAndContent;
}

Areas areasFor(LayoutType layout, int screenHeight, int screenWidth) {
  const int width = screenWidth - 2 * MARGIN;
  // the content starts below the title and the gap, and stops above the
  // counter on the last row
  const int contentRow = TITLE_ROW + 1 + GAP;
  const int contentHeight = screenHeight - 1 - contentRow;

  Areas areas{};
  if (layout == LayoutType::Subsection) {
    // the title in the middle of the screen, the subtitle right under it
    const int middle = screenHeight / 2 - 1;
    areas.title = {middle, MARGIN, 1, width};
    areas.left = {middle + 1, MARGIN, 1, width};
    return areas;
  }

  areas.title = {TITLE_ROW, MARGIN, 1, width};
  if (layout == LayoutType::TwoContent) {
    // the right side gets any leftover column so nothing is lost on odd widths
    const int leftWidth = (width - COLUMN_GAP) / 2;
    areas.left = {contentRow, MARGIN, contentHeight, leftWidth};
    areas.right = {contentRow, MARGIN + leftWidth + COLUMN_GAP, contentHeight,
                   width - leftWidth - COLUMN_GAP};
  } else
    areas.left = {contentRow, MARGIN, contentHeight, width};
  return areas;
}