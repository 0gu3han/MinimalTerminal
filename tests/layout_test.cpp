#include <iostream>
#include <string>
#include <vector>

import layout;

static int failures = 0;

static std::string show(const std::vector<std::string> &lines) {
  std::string text = "{";
  for (const std::string &line : lines)
    text += " \"" + line + "\"";
  return text + " }";
}

static std::string show(const std::string &text) { return "\"" + text + "\""; }
static std::string show(int number) { return std::to_string(number); }

template <typename T>
static void expect(const T &actual, const T &expected, const std::string &what) {
  if (actual == expected)
    return;
  std::cerr << "FAIL " << what << "\n  expected " << show(expected)
            << "\n  got      " << show(actual) << "\n";
  ++failures;
}

static void expectRect(const Rect &actual, const Rect &expected,
                       const std::string &what) {
  expect(actual.row, expected.row, what + " row");
  expect(actual.col, expected.col, what + " col");
  expect(actual.height, expected.height, what + " height");
  expect(actual.width, expected.width, what + " width");
}

using Lines = std::vector<std::string>;

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
  expect(wrap("<b>one two three</b>", 7), Lines{"<b>one two</b>", "<b>three</b>"},
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

auto main() -> int {
  testLayoutFromTag();
  testAreas();
  testDisplayWidth();
  testWrap();
  testTruncate();

  if (failures == 0)
    std::cout << "All layout tests passed\n";
  return failures == 0 ? 0 : 1;
}
