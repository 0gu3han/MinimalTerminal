module;
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
                            html.substr(contentStart, closing - contentStart)});
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

// keep the decoded code lines, and mark where the code box goes with a blank
// line in the code color
void Slide::addCode(const std::string &html) {
  // skip the line break after <code> and the indent before </code>
  size_t start = html.find('\n') + 1;
  size_t end;
  while ((end = html.find('\n', start)) != std::string::npos) {
    code.push_back(decodeEntities(html.substr(start, end - start)));
    start = end + 1;
  }

  lines.push_back({"", A_NORMAL, 0});
  lines.push_back({"", COLOR_PAIR(codePair), 0});
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
  size_t width = 0;
  for (const std::string &line : code)
    if (line.size() > width)
      width = line.size();

  codeRow = getcury(stdscr) + 1;
  // one character at a time instead of drawStyled, so code is never read as a
  // tag and each character gets its highlight color
  std::string open;
  for (const std::string &line : code) {
    move(getcury(stdscr) + 1, 4);
    attrset(COLOR_PAIR(codePair));
    addch(' ');
    const std::vector<attr_t> styles = highlight(line, language, open);
    for (size_t i = 0; i < line.size(); ++i)
      addch(static_cast<unsigned char>(line[i]) | styles[i]);
    attrset(COLOR_PAIR(codePair));
    addstr(std::string(width - line.size() + 1, ' ').c_str());
  }
  attrset(A_NORMAL);
}

// turn each heading, bullet and code block into lines in the order written
Slide::Slide(const std::string &html) {
  for (const Element &element :
       findAll(html, {"h1", "li", "code", "output"})) {
    const std::string &tag = element.openingTag;
    if (tag.starts_with("<h1"))
      lines.push_back({trim(element.content), A_BOLD | colorOf(tag), 0});
    else if (tag.starts_with("<li"))
      lines.push_back({"* " + trim(element.content), colorOf(tag), 8});
    else if (tag.starts_with("<output"))
      addOutput(element.content);
    // only the first code block is kept, since that is the one to edit
    else if (code.empty()) {
      addCode(element.content);
      // the name runs from "language-" to the closing quote
      size_t name = tag.find("language-");
      if (name != std::string::npos)
        language = tag.substr(name + 9, tag.find_first_of("\" >", name) -
                                            name - 9); // 9 = "language-"
    }
  }
}

// draw each line on its own row
void Slide::render() {
  for (const Line &line : lines) {
    if (line.style == COLOR_PAIR(codePair)) {
      drawCode();
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
    mvaddstr(getmaxy(stdscr) - 1, 0,
             "-- INSERT --   Esc: keep changes   Ctrl-X: undo changes");
    // the code starts after the 4 column indent and 1 space of padding
    move(codeRow + row, 5 + column);
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
