#include <ncurses.h>
#include <string>
#include <vector>

import editor;
import testing;

using Lines = std::vector<std::string>;

static void type(Editor &editor, const std::string &text) {
  for (char character : text)
    editor.press(character);
}

static void testTyping() {
  Editor editor({}, "cpp");
  expect(editor.lines(), Lines{""}, "empty code still has a line");

  type(editor, "int x;");
  expect(editor.lines(), Lines{"int x;"}, "typing adds text");
  expect(editor.column(), size_t{6}, "cursor after the text");

  editor.press(KEY_LEFT);
  editor.press('\n');
  expect(editor.lines(), Lines{"int x", ";"}, "Enter splits the line");
  expect(editor.row(), size_t{1}, "cursor on the new line");
  expect(editor.column(), size_t{0}, "cursor at its start");

  editor.press(KEY_BACKSPACE);
  expect(editor.lines(), Lines{"int x;"}, "Backspace at the start joins lines");
  expect(editor.column(), size_t{5}, "cursor where the lines met");

  editor.press('\t');
  expect(editor.lines(), Lines{"int x    ;"}, "Tab is four spaces");

  editor.press(KEY_HOME);
  expect(editor.column(), size_t{0}, "Home goes to the start");
  editor.press(KEY_END);
  expect(editor.column(), size_t{10}, "End goes to the end");

  editor.press(1); // Ctrl-A
  expect(editor.lines(), Lines{"int x    ;"}, "control keys are not typed");
}

static void testDelete() {
  Editor editor({"ab", "cd"}, "");
  editor.press(KEY_DC);
  expect(editor.lines(), Lines{"b", "cd"}, "Delete removes the next letter");

  editor.press(KEY_END);
  editor.press(KEY_DC);
  expect(editor.lines(), Lines{"bcd"}, "Delete at the end joins lines");

  editor.press(KEY_END);
  editor.press(KEY_DC);
  expect(editor.lines(), Lines{"bcd"}, "Delete at the very end does nothing");

  editor.press(KEY_HOME);
  editor.press(KEY_BACKSPACE);
  expect(editor.lines(), Lines{"bcd"},
         "Backspace at the very start does nothing");
}

static void testMoving() {
  Editor editor({"a long line", "x", "another"}, "python");
  editor.press(KEY_END);
  editor.press(KEY_DOWN);
  expect(editor.row(), size_t{1}, "Down goes to the next line");
  expect(editor.column(), size_t{1}, "cursor stays inside a shorter line");

  editor.press(KEY_DOWN);
  editor.press(KEY_DOWN);
  expect(editor.row(), size_t{2}, "Down stops at the last line");

  editor.press(KEY_UP);
  editor.press(KEY_UP);
  editor.press(KEY_UP);
  expect(editor.row(), size_t{0}, "Up stops at the first line");

  editor.press(KEY_HOME);
  editor.press(KEY_LEFT);
  expect(editor.column(), size_t{0}, "Left stops at the start");
}

static void testText() {
  Editor editor({"print(1)", "print(2)"}, "python");
  expect(editor.text(), std::string("print(1)\nprint(2)\n"),
         "text is the lines ending in line breaks");
}

auto main() -> int {
  testTyping();
  testDelete();
  testMoving();
  testText();
  return finish("editor");
}
