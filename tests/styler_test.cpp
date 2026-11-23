#include <ncurses.h>

import styler;
import testing;

static void testIsCentered() {
  expect(isCentered("<h1 style=\"text-align: center\">"), true, "centered");
  expect(isCentered("<li style=\"text-align:center\">"), true,
         "centered without a space");
  expect(isCentered("<li style=\"color: red; text-align: center\">"), true,
         "centered after another property");
  expect(isCentered("<li style=\"text-align: left\">"), false, "left aligned");
  expect(isCentered("<li>"), false, "no style");
}

static void testColorOf() {
  expect(colorOf("<li style=\"color: red\">") == COLOR_PAIR(COLOR_RED + 1),
         true, "red");
  expect(colorOf("<li style=\"text-align: center; color:green\">") ==
             COLOR_PAIR(COLOR_GREEN + 1),
         true, "green after text-align");
  expect(colorOf("<li>") == A_NORMAL, true, "no color");
}

auto main() -> int {
  testIsCentered();
  testColorOf();
  return finish("styler");
}
