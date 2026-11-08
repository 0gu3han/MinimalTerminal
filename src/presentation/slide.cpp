module;
#include <ncurses.h>
#include <string>
#include <vector>

module slide;
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

// draw the code lines padded to the same width so the box is a rectangle
void Slide::drawCode() {
  size_t width = 0;
  for (const std::string &line : code)
    if (line.size() > width)
      width = line.size();

  // addstr instead of drawStyled so code is never read as a tag
  attrset(COLOR_PAIR(codePair));
  for (const std::string &line : code) {
    move(getcury(stdscr) + 1, 4);
    addstr((" " + line + std::string(width - line.size() + 1, ' ')).c_str());
  }
  attrset(A_NORMAL);
}

// turn each heading, bullet and code block into lines in the order written
Slide::Slide(const std::string &html) {
  for (const Element &element : findAll(html, {"h1", "li", "code"})) {
    const std::string &tag = element.openingTag;
    if (tag.starts_with("<h1"))
      lines.push_back({trim(element.content), A_BOLD | colorOf(tag), 0});
    else if (tag.starts_with("<li"))
      lines.push_back({"* " + trim(element.content), colorOf(tag), 8});
    // only the first code block is kept, since that is the one to edit
    else if (code.empty())
      addCode(element.content);
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
