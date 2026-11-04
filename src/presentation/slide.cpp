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

// add a blank line, then the code lines padded to the same width so the box is
// a rectangle
void Slide::addCode(const std::string &code) {
  std::vector<std::string> codeLines;
  size_t width = 0;
  // skip the line break after <code> and the indent before </code>
  size_t start = code.find('\n') + 1;
  size_t end;
  while ((end = code.find('\n', start)) != std::string::npos) {
    codeLines.push_back(code.substr(start, end - start));
    // measure the decoded text since that is what shows on screen
    if (decodeEntities(codeLines.back()).size() > width)
      width = decodeEntities(codeLines.back()).size();
    start = end + 1;
  }

  lines.push_back({"", A_NORMAL, 0});
  // the lines stay encoded so drawStyled never reads code as a tag
  for (const std::string &line : codeLines)
    lines.push_back(
        {" " + line + std::string(width - decodeEntities(line).size() + 1, ' '),
         COLOR_PAIR(codePair), 4});
}

// turn each heading, bullet and code block into lines in the order written
Slide::Slide(const std::string &html) {
  for (const Element &element : findAll(html, {"h1", "li", "code"})) {
    const std::string &tag = element.openingTag;
    if (tag.starts_with("<h1"))
      lines.push_back({trim(element.content), A_BOLD | colorOf(tag), 0});
    else if (tag.starts_with("<li"))
      lines.push_back({"* " + trim(element.content), colorOf(tag), 8});
    else
      addCode(element.content);
  }
}

// draw each line on its own row
void Slide::render() {
  int row = 0;
  for (const Line &line : lines) {
    move(row++, line.column);
    drawStyled(line.text, line.style);
  }
}
