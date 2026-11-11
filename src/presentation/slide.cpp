module;
#include <algorithm>
#include <ncurses.h>
#include <string>
#include <vector>

module slide;
import highlighter;
import styler;

std::vector<Element> findAll(const std::string &html,
                             const std::vector<std::string> &tags) {
  std::vector<Element> elements;
  for (size_t start = html.find('<'); start != std::string::npos;
       start = html.find('<', start + 1))
    for (const std::string &tag : tags)
      // check if one of the tags opens here
      if (html.compare(start + 1, tag.size(), tag) == 0) {
        size_t contentStart = html.find('>', start) + 1;
        size_t closing = html.find("</" + tag + ">", contentStart);
        elements.push_back({html.substr(start, contentStart - start),
                            html.substr(contentStart, closing - contentStart),
                            contentStart, closing});
        // jump past the content so tags inside it are not matched
        start = closing;
        break;
      }
  return elements;
}

// remove spaces and tabs from both ends of the text
static std::string trim(const std::string &text) {
  size_t first = text.find_first_not_of(" \t");
  if (first == std::string::npos)
    return "";

  size_t last = text.find_last_not_of(" \t");
  return text.substr(first, last - first + 1);
}

static std::vector<std::string> codeLines(const std::string &html) {
  std::string text = decodeEntities(html);
  const bool startsWithBoundary = !text.empty() && text.front() == '\n';
  if (startsWithBoundary)
    text.erase(0, 1);

  const size_t lastLineBreak = text.rfind('\n');
  if (lastLineBreak != std::string::npos &&
      text.find_first_not_of(" \t", lastLineBreak + 1) == std::string::npos)
    text.erase(lastLineBreak);
  else if (startsWithBoundary &&
           text.find_first_not_of(" \t") == std::string::npos)
    text.clear();

  std::vector<std::string> result;
  size_t start = 0;
  size_t end;
  while ((end = text.find('\n', start)) != std::string::npos) {
    result.push_back(text.substr(start, end - start));
    start = end + 1;
  }
  result.push_back(text.substr(start));
  return result;
}

// Keep the decoded code lines, and mark where the code box goes with a blank
// line in the code color.
void Slide::addCode(const std::string &html) {
  code = codeLines(html);

  lines.push_back({"", A_NORMAL, 0});
  lines.push_back({"", COLOR_PAIR(codePair), 0});
}

void Slide::addReadOnlyCode(const std::string &html) {
  lines.push_back({"", A_NORMAL, 0});
  for (const std::string &line : codeLines(html))
    lines.push_back({line, COLOR_PAIR(codePair), 4});
}

// add a blank line, then each line the code printed
void Slide::addOutput(const std::string &html) {
  isOutput = true;
  lines.push_back({"", A_NORMAL, 0});
  // skip the line break after <output> and the indent before </output>
  size_t start = html.find('\n') + 1;
  size_t end;
  // the lines stay encoded so drawStyled never reads output as a tag
  while ((end = html.find('\n', start)) != std::string::npos) {
    lines.push_back({html.substr(start, end - start), A_NORMAL, 4});
    start = end + 1;
  }
}

// draw the code lines padded to the same width so the box is a rectangle
void Slide::drawCode() {
  const int height = getmaxy(stdscr);
  const int width = getmaxx(stdscr);
  if (height > 1)
    codeRow = std::min(getcury(stdscr) + 1, height - 2);
  else
    codeRow = 0;
  const size_t visibleRows =
      height > 1 ? static_cast<size_t>(height - 1 - codeRow) : 0;
  const size_t visibleColumns = width > 5 ? static_cast<size_t>(width - 5) : 0;

  std::string open;
  for (size_t row = 0; row < code.size(); ++row) {
    const std::vector<attr_t> styles = highlight(code[row], language, open);
    if (row < codeViewportRow || row - codeViewportRow >= visibleRows)
      continue;

    const int screenRow =
        codeRow + static_cast<int>(row - codeViewportRow);
    move(screenRow, 4);
    attrset(COLOR_PAIR(codePair));
    for (int column = 4; column < width; ++column)
      addch(' ');

    const size_t end =
        std::min(code[row].size(), codeViewportColumn + visibleColumns);
    for (size_t column = codeViewportColumn; column < end; ++column) {
      move(screenRow, 5 + static_cast<int>(column - codeViewportColumn));
      addch(static_cast<unsigned char>(code[row][column]) | styles[column]);
    }
  }
  attrset(A_NORMAL);
}

// turn each heading, bullet and code block into lines in the order written
Slide::Slide(const std::string &html, size_t sourceStart) {
  bool foundCode = false;
  for (const Element &element :
       findAll(html, {"h1", "li", "code", "output"})) {
    const std::string &tag = element.openingTag;
    if (tag.starts_with("<h1"))
      lines.push_back({trim(element.content), A_BOLD | colorOf(tag), 0});
    else if (tag.starts_with("<li"))
      lines.push_back({"* " + trim(element.content), colorOf(tag), 8});
    else if (tag.starts_with("<output"))
      addOutput(element.content);
    else if (!foundCode) {
      foundCode = true;
      hasCodeBlock = true;
      addCode(element.content);
      codeSourceStart = sourceStart + element.contentStart;
      codeSourceEnd = sourceStart + element.contentEnd;
      // the name runs from "language-" to the closing quote
      size_t name = tag.find("language-");
      if (name != std::string::npos) {
        const size_t languageStart = name + 9;
        const size_t languageEnd =
            tag.find_first_of("\"' \t\r\n\f>", languageStart);
        language = tag.substr(
            languageStart,
            languageEnd == std::string::npos ? std::string::npos
                                              : languageEnd - languageStart);
      }
    } else {
      addReadOnlyCode(element.content);
    }
  }
}

// draw each line on its own row
void Slide::render() {
  for (const Line &line : lines) {
    if (line.style == COLOR_PAIR(codePair)) {
      if (line.column == 0) {
        drawCode();
      } else {
        move(getcury(stdscr) + 1, line.column);
        attrset(COLOR_PAIR(codePair));
        addnstr(line.text.c_str(), std::max(0, getmaxx(stdscr) - line.column));
        attrset(A_NORMAL);
      }
      continue;
    }
    move(getcury(stdscr) + 1, line.column);
    drawStyled(line.text, line.style);
  }
}

std::string Slide::codeHTML() const {
  std::string html;
  for (const std::string &line : code)
    html += "\n" + encodeEntities(line);
  return html;
}

std::string Slide::codeText() const {
  std::string text;
  for (const std::string &line : code)
    text += line + "\n";
  return text;
}

bool Slide::edit() {
  const std::vector<std::string> original = code;
  size_t row = 0;
  size_t column = 0;
  curs_set(1);

  while (true) {
    erase();
    render();
    const size_t visibleRows =
        getmaxy(stdscr) > 1 && codeRow < getmaxy(stdscr) - 1
            ? static_cast<size_t>(getmaxy(stdscr) - 1 - codeRow)
            : 0;
    const size_t visibleColumns =
        getmaxx(stdscr) > 5 ? static_cast<size_t>(getmaxx(stdscr) - 5) : 0;
    bool viewportChanged = false;
    if (visibleRows) {
      if (row < codeViewportRow) {
        codeViewportRow = row;
        viewportChanged = true;
      } else if (row >= codeViewportRow + visibleRows) {
        codeViewportRow = row - visibleRows + 1;
        viewportChanged = true;
      }
    }
    if (visibleColumns) {
      if (column < codeViewportColumn) {
        codeViewportColumn = column;
        viewportChanged = true;
      } else if (column >= codeViewportColumn + visibleColumns) {
        codeViewportColumn = column - visibleColumns + 1;
        viewportChanged = true;
      }
    }
    if (viewportChanged) {
      erase();
      render();
    }
    mvaddstr(getmaxy(stdscr) - 1, 0,
             "-- INSERT --   Esc: keep changes   Ctrl-X: undo changes");
    if (visibleRows && visibleColumns)
      move(codeRow + static_cast<int>(row - codeViewportRow),
           5 + static_cast<int>(column - codeViewportColumn));
    refresh();

    int key = getch();
    std::string &line = code[row];
    switch (key) {
    case 27: // Esc
      curs_set(0);
      return true;
    case 24: // Ctrl-X
      code = original;
      curs_set(0);
      return false;
    case KEY_UP:
      if (row > 0)
        --row;
      break;
    case KEY_DOWN:
      if (row + 1 < code.size())
        ++row;
      break;
    case KEY_LEFT:
      if (column > 0)
        --column;
      break;
    case KEY_RIGHT:
      if (column < line.size())
        ++column;
      break;
    case KEY_BACKSPACE:
    case 127:
    case 8:
      if (column > 0) {
        line.erase(--column, 1);
      } else if (row > 0) {
        // join this line onto the end of the one above
        column = code[row - 1].size();
        code[row - 1] += line;
        code.erase(code.begin() + row--);
      }
      break;
    case '\n':
      // split the line at the cursor
      code.insert(code.begin() + row + 1, line.substr(column));
      code[row].erase(column);
      ++row;
      column = 0;
      break;
    case '\t':
      // spaces, since a tab would break the box alignment
      line.insert(column, 4, ' ');
      column += 4;
      break;
    default:
      if (key >= ' ' && key <= '~')
        line.insert(column++, 1, key);
    }
    // stay inside the line after moving up or down
    if (column > code[row].size())
      column = code[row].size();
  }
}
