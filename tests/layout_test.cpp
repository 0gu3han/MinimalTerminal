#include <string>
#include <vector>

import layout;
import testing;

using Lines = std::vector<std::string>;

static void expectRect(const Rect &actual, const Rect &expected,
                       const std::string &what) {
  expect(actual.row, expected.row, what + " row");
  expect(actual.col, expected.col, what + " col");
  expect(actual.height, expected.height, what + " height");
  expect(actual.width, expected.width, what + " width");
}

static bool inside(const Rect &rect, int screenHeight, int screenWidth) {
  return rect.row >= 0 && rect.col >= 0 && rect.height >= 0 &&
         rect.width >= 0 && rect.row + rect.height <= screenHeight &&
         rect.col + rect.width <= screenWidth;
}

static void testLayoutFromTag() {
  expect(layoutFromTag("<article>") == LayoutType::TitleAndContent, true,
         "no class is title and content");
  expect(layoutFromTag("<article class=\"two-content\">") ==
             LayoutType::TwoContent,
         true, "two-content class");
  expect(layoutFromTag("<article class=\"subsection\">") ==
             LayoutType::Subsection,
         true, "subsection class");
}

static void testAreas() {
  Areas areas = areasFor(LayoutType::TitleAndContent, 24, 80);
  expectRect(areas.title, {1, 2, 1, 76}, "title");
  expectRect(areas.left, {3, 2, 20, 76}, "content");
  // the bottom row stays free for the slide counter
  expect(areas.left.row + areas.left.height, 23, "content stops above counter");

  areas = areasFor(LayoutType::TwoContent, 24, 80);
  expectRect(areas.left, {3, 2, 20, 36}, "two content left");
  expectRect(areas.right, {3, 42, 20, 36}, "two content right");
  expect(areas.right.col + areas.right.width, 78, "right margin kept");

  // an odd width gives the extra column to the right side
  areas = areasFor(LayoutType::TwoContent, 24, 81);
  expect(areas.right.col + areas.right.width, 79, "odd width right margin");

  areas = areasFor(LayoutType::Subsection, 24, 80);
  expectRect(areas.title, {11, 2, 1, 76}, "subsection title");
  expectRect(areas.left, {12, 2, 1, 76}, "subsection subtitle");

  // the heading is on the same row whatever the screen size
  for (int width : {40, 80, 120})
    for (LayoutType layout :
         {LayoutType::TitleAndContent, LayoutType::TwoContent}) {
      const std::string size = std::to_string(width) + " columns";
      areas = areasFor(layout, 30, width);
      expect(areas.title.row, 1, "heading row at " + size);
      expect(inside(areas.title, 30, width), true, "title fits at " + size);
      expect(inside(areas.left, 30, width), true, "left fits at " + size);
      if (layout == LayoutType::TwoContent) {
        expect(inside(areas.right, 30, width), true, "right fits at " + size);
        expect(areas.left.col + areas.left.width < areas.right.col, true,
               "two content sides do not touch at " + size);
      }
    }

  // a tiny screen gives empty areas rather than negative ones
  for (LayoutType layout : {LayoutType::TitleAndContent, LayoutType::TwoContent,
                            LayoutType::Subsection}) {
    areas = areasFor(layout, 2, 3);
    expect(inside(areas.title, 2, 3) || areas.title.width == 0, true,
           "tiny title");
    expect(areas.left.height >= 0 && areas.left.width >= 0, true,
           "tiny left is not negative");
    expect(areas.right.height >= 0 && areas.right.width >= 0, true,
           "tiny right is not negative");
  }
}

static void testSplitAreas() {
  const SplitAreas areas = splitAreas(24, 80);
  expectRect(areas.editorHeader, {0, 0, 1, 80}, "editor header");
  expectRect(areas.editor, {1, 0, 13, 80}, "editor");
  expectRect(areas.outputHeader, {14, 0, 1, 80}, "output header");
  expectRect(areas.output, {15, 0, 8, 80}, "output");
  expectRect(areas.status, {23, 0, 1, 80}, "key help on the last row");

  // the parts stack with nothing between them and nothing left over
  for (int height : {4, 10, 24, 51}) {
    const SplitAreas split = splitAreas(height, 100);
    const std::string size = std::to_string(height) + " rows";
    expect(split.editor.row, split.editorHeader.row + 1,
           "editor under header " + size);
    expect(split.outputHeader.row, split.editor.row + split.editor.height,
           "output header under editor " + size);
    expect(split.output.row, split.outputHeader.row + 1,
           "output under header " + size);
    expect(split.status.row, split.output.row + split.output.height,
           "key help under output " + size);
    expect(split.editor.height >= split.output.height, true,
           "editor gets the most room " + size);
  }

  const SplitAreas tiny = splitAreas(2, 10);
  expect(tiny.editor.height, 0, "no room for code");
  expect(tiny.output.height, 0, "no room for output");
}

static void testDisplayWidth() {
  expect(displayWidth("plain"), 5, "plain text");
  expect(displayWidth("a <b>bold</b> word"), 11, "tags take no columns");
  expect(displayWidth("x &amp;&lt; y"), 6, "entities take one column");
  expect(displayWidth("héllo"), 5, "UTF-8 letter takes one column");
  expect(displayWidth("AT&T"), 4, "a lone & is just a letter");
}

static void testWrap() {
  expect(wrap("the quick brown fox", 10), Lines{"the quick", "brown fox"},
         "breaks between words");
  expect(wrap("fits", 10), Lines{"fits"}, "short text stays one line");
  expect(wrap("", 10), Lines{""}, "empty text is one empty line");
  expect(wrap("a   b\n    c", 10), Lines{"a b c"}, "spaces and lines collapse");
  expect(wrap("abcdefghij", 4), Lines{"abcd", "efgh", "ij"},
         "long word is split");
  expect(wrap("hi abcdefghij", 4), Lines{"hi", "abcd", "efgh", "ij"},
         "long word starts on its own line");
  expect(wrap("<b>one two three</b>", 7),
         Lines{"<b>one two</b>", "<b>three</b>"},
         "bold carries onto the next line");
  expect(wrap("a <b>b <i>c d</i></b> e", 5),
         Lines{"a <b>b <i>c</i></b>", "<b><i>d</i></b> e"},
         "nested styles carry over in order");
  expect(wrap("a &amp;&amp; b", 3), Lines{"a", "&amp;&amp;", "b"},
         "entities are never split");
  expect(wrap("word", 0), Lines{"w", "o", "r", "d"}, "zero width still works");
}

static void testTruncate() {
  expect(truncate("short", 10), std::string("short"), "fits as is");
  expect(truncate("exactly10!", 10), std::string("exactly10!"),
         "exact width is not cut");
  expect(truncate("abcdefghij", 5), std::string("abcd…"), "cut with …");
  expect(truncate("x&lt;&lt;y z", 3), std::string("x&lt;…"),
         "entities are never cut in half");
  expect(truncate("ééééé", 3), std::string("éé…"),
         "UTF-8 letters are never cut in half");
  expect(truncate("anything", 0), std::string(""), "no room at all");
}

static void testFitText() {
  expect(fitText("short", 10).bytes, size_t{5}, "short code fits");
  expect(fitText("short", 10).cut, false, "short code is not cut");
  expect(fitText("exactly", 7).cut, false, "exact width is not cut");
  expect(fitText("abcdefgh", 5).bytes, size_t{4}, "cut leaves room for …");
  expect(fitText("abcdefgh", 5).cut, true, "long code is cut");
  expect(fitText("a < b && c", 6).bytes, size_t{5},
         "< and & are plain characters in code");
  expect(fitText("ééééé", 3).bytes, std::string("éé").size(),
         "UTF-8 letters are never cut in half");
  expect(fitText("ab", 1).bytes, size_t{0}, "one column only has room for …");
  expect(fitText("ab", 1).cut, true, "one column still shows the cut");
  expect(fitText("abc", 0).cut, false, "no room at all draws nothing");
}

static void testExpandTabs() {
  expect(expandTabs("a\tb"), std::string("a       b"), "tab to column 8");
  expect(expandTabs("12345678\tx"), std::string("12345678        x"),
         "a tab at a stop is a whole stop");
  expect(expandTabs("&lt;\t|"), std::string("&lt;       |"),
         "an entity is one column");
  expect(expandTabs("no tabs"), std::string("no tabs"), "nothing to do");
  expect(expandCodeTabs("&lt;\t|"), std::string("&lt;    |"),
         "in code an entity is just characters");
  expect(expandCodeTabs("é\tx"), std::string("é       x"),
         "a UTF-8 letter is one column");
}

static Block bullet(const std::string &html) {
  return {BlockKind::Bullet, "<li>", {html}};
}

static Block code(const Lines &lines) {
  return {BlockKind::Code, "<code>", lines};
}

static Lines texts(const std::vector<Row> &rows) {
  Lines result;
  for (const Row &row : rows)
    result.push_back(row.text);
  return result;
}

static void testLayOut() {
  // a 200 character bullet at 80 columns
  std::string words;
  while (words.size() < 200)
    words += "lorem ipsum ";
  words.resize(200);
  const int width = 76; // 80 columns less the margins
  std::vector<Row> rows = layOut({bullet(words)}, width, 20);
  expect(rows.size() > 1, true, "long bullet wraps");
  for (size_t index = 0; index < rows.size(); ++index) {
    const Row &row = rows[index];
    expect(row.column, bulletIndent,
           "continuation lines line up under the text " +
               std::to_string(index));
    expect(row.column + displayWidth(row.text) <= width, true,
           "wrapped line fits " + std::to_string(index));
    expect(row.first, index == 0, "only the first row has the marker");
  }

  rows = layOut({bullet("a <b>bold phrase that wraps</b>")}, 2 + 12, 20);
  expect(texts(rows),
         Lines{"a <b>bold</b>", "<b>phrase that</b>", "<b>wraps</b>"},
         "bold stays on across wrapped lines");

  rows = layOut({code({"int main() {", "    return 0;", "}",
                       "std::cout << \"a long line that will not fit\";"})},
                24, 20);
  expect(texts(rows),
         Lines{"int main() {", "    return 0;", "}", "std::cout << \"a long "},
         "code keeps its indentation and long lines are cut");
  expect(rows[1].column, codePadding, "code starts inside its box");
  expect(rows[1].cut, false, "short code line is whole");
  expect(rows[3].cut, true, "long code line gets a …");

  rows = layOut({code({"\treturn 0;", "\t\tlong enough to be cut"})}, 20, 20);
  expect(texts(rows), Lines{"        return 0;", "                l"},
         "tabs in code are expanded before measuring");
  expect(rows[1].cut, true, "tabs count toward the width");

  rows = layOut({bullet("one"), bullet("two"), code({"x"}), bullet("three")},
                40, 20);
  std::vector<bool> blank;
  for (const Row &row : rows)
    blank.push_back(row.kind == RowKind::Blank);
  expect(blank == std::vector<bool>{false, false, true, false, true, false},
         true, "blank rows before and after code, none between bullets");

  rows =
      layOut({{BlockKind::Output, "", {"x &lt; 1 and a long result"}}}, 10, 5);
  expect(texts(rows), Lines{"x &lt; 1 and…"}, "output is cut, not wrapped");
  rows = layOut({{BlockKind::Output, "", {"a\tb"}}}, 40, 5);
  expect(texts(rows), Lines{"a       b"}, "tabs in output are expanded");

  rows = layOut({bullet("x")}, 40, 2);
  expect(rows[0].column, bulletIndent, "bullets start at the left by default");

  Block centered = bullet("hi");
  centered.centered = true;
  rows = layOut({centered}, 20, 5);
  // "• hi" is 4 columns wide, so the marker is at 8 and the text at 10
  expect(rows[0].column, 10, "centered bullet is centered with its marker");

  centered.lines = {"aaaa bbbbbbbb"};
  rows = layOut({centered}, 12, 5);
  expect(texts(rows), Lines{"aaaa", "bbbbbbbb"}, "centered bullet wraps");
  expect(rows[0].column, 5, "first row is centered with its marker");
  expect(rows[1].column, 2, "later rows are centered on their own");

  Lines tall;
  for (int line = 0; line < 30; ++line)
    tall.push_back("line " + std::to_string(line));
  rows = layOut({code(tall)}, 40, 10);
  expect(rows.size(), size_t{10}, "only what fits is shown");
  expect(rows.back().kind == RowKind::Overflow, true,
         "the last row says there is more");
  expect(rows[8].text, std::string("line 8"), "the rows before it are kept");

  rows = layOut({code({"exactly", "fits"})}, 40, 2);
  expect(rows.back().kind == RowKind::Code, true,
         "no overflow row when it all fits");
  expect(layOut({bullet("x")}, 40, 0).empty(), true, "no room, no rows");
}

auto main() -> int {
  testLayoutFromTag();
  testAreas();
  testSplitAreas();
  testDisplayWidth();
  testWrap();
  testTruncate();
  testFitText();
  testExpandTabs();
  testLayOut();
  return finish("layout");
}
