module;
#include <algorithm>
#include <ncurses.h>
#include <string>
#include <utility>
#include <vector>

module editor;
import highlighter;
import styler;

Editor::Editor(std::vector<std::string> lines, std::string language)
    : code(std::move(lines)), language(std::move(language)) {
  // there is always a line for the cursor to be on
  if (code.empty())
    code.emplace_back();
}

std::string Editor::text() const {
  std::string text;
  for (const std::string &line : code)
    text += line + "\n";
  return text;
}

void Editor::press(int key) {
  std::string &line = code[cursorRow];
  switch (key) {
  case KEY_UP:
    if (cursorRow > 0)
      --cursorRow;
    break;
  case KEY_DOWN:
    if (cursorRow + 1 < code.size())
      ++cursorRow;
    break;
  case KEY_LEFT:
    if (cursorColumn > 0)
      --cursorColumn;
    break;
  case KEY_RIGHT:
    if (cursorColumn < line.size())
      ++cursorColumn;
    break;
  case KEY_HOME:
    cursorColumn = 0;
    break;
  case KEY_END:
    cursorColumn = line.size();
    break;
  case KEY_BACKSPACE:
  case 127:
  case 8:
    if (cursorColumn > 0)
      line.erase(--cursorColumn, 1);
    else if (cursorRow > 0) {
      // join this line onto the end of the one above
      cursorColumn = code[cursorRow - 1].size();
      code[cursorRow - 1] += line;
      code.erase(code.begin() + cursorRow--);
    }
    break;
  case KEY_DC:
    if (cursorColumn < line.size())
      line.erase(cursorColumn, 1);
    else if (cursorRow + 1 < code.size()) {
      // join the line below onto the end of this one
      line += code[cursorRow + 1];
      code.erase(code.begin() + cursorRow + 1);
    }
    break;
  case '\n':
  case KEY_ENTER:
    // split the line at the cursor
    code.insert(code.begin() + cursorRow + 1, line.substr(cursorColumn));
    code[cursorRow].erase(cursorColumn);
    ++cursorRow;
    cursorColumn = 0;
    break;
  case '\t':
    // spaces, since a tab would break the line up with the line numbers
    line.insert(cursorColumn, 4, ' ');
    cursorColumn += 4;
    break;
  default:
    if (key >= ' ' && key <= '~')
      line.insert(cursorColumn++, 1, static_cast<char>(key));
  }
  // stay inside the line after moving up or down
  cursorColumn = std::min(cursorColumn, code[cursorRow].size());
}

void Editor::draw(WINDOW *window) {
  const int height = getmaxy(window);
  const int width = getmaxx(window);
  // the line numbers, right aligned with a space on each side
  const int gutter = static_cast<int>(std::to_string(code.size()).size()) + 2;
  const int textWidth = width - gutter;
  if (height <= 0 || textWidth <= 0)
    return;

  // scroll just enough to keep the cursor in view
  const auto rows = static_cast<size_t>(height);
  const auto columns = static_cast<size_t>(textWidth);
  if (cursorRow < topRow)
    topRow = cursorRow;
  else if (cursorRow >= topRow + rows)
    topRow = cursorRow - rows + 1;
  if (cursorColumn < leftColumn)
    leftColumn = cursorColumn;
  else if (cursorColumn >= leftColumn + columns)
    leftColumn = cursorColumn - columns + 1;

  wbkgdset(window, ' ' | COLOR_PAIR(codePair));
  werase(window);

  // highlight every line in order, so a comment or string left open on a line
  // above the window still colors the lines inside it
  std::string open;
  for (size_t row = 0; row < code.size() && row < topRow + rows; ++row) {
    const std::vector<attr_t> styles = highlight(code[row], language, open);
    if (row < topRow)
      continue;

    const int screenRow = static_cast<int>(row - topRow);
    wattrset(window, COLOR_PAIR(commentPair));
    mvwprintw(window, screenRow, 0, "%*zu ", gutter - 1, row + 1);
    const size_t end = std::min(code[row].size(), leftColumn + columns);
    for (size_t column = leftColumn; column < end; ++column) {
      // the cursor counts a tab as one column, so it is drawn as one space
      // rather than letting curses spread it over the characters after it
      const char character = code[row][column] == '\t' ? ' ' : code[row][column];
      mvwaddch(window, screenRow,
               gutter + static_cast<int>(column - leftColumn),
               static_cast<unsigned char>(character) | styles[column]);
    }
  }
  wattrset(window, A_NORMAL);

  wmove(window, static_cast<int>(cursorRow - topRow),
        gutter + static_cast<int>(cursorColumn - leftColumn));
}
