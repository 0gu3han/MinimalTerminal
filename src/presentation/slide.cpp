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
  if (first == std::string::npos) // empty or all whitespace
    return "";

  size_t last = text.find_last_not_of(" \t");
  return text.substr(first, last - first + 1);
}

// split the text inside <pre> into lines, keeping the indentation of each line
static std::vector<std::string> toCodeLines(std::string code) {
  // the <code> tags are only a wrapper, drop them
  for (const std::string tag : {"<code>", "</code>"}) {
    size_t pos;
    while ((pos = code.find(tag)) != std::string::npos)
      code.erase(pos, tag.size());
  }
  code = decodeEntities(code);

  std::vector<std::string> lines;
  size_t start = 0;
  while (start <= code.size()) {
    size_t end = code.find('\n', start);
    if (end == std::string::npos)
      end = code.size();

    std::string line = code.substr(start, end - start);
    if (!line.empty() && line.back() == '\r')
      line.pop_back();

    // a tab would break the box alignment
    size_t tab;
    while ((tab = line.find('\t')) != std::string::npos)
      line.replace(tab, 1, "    ");
    lines.push_back(line);
    start = end + 1;
  }

  // blank lines right after <pre> and right before </pre> come from formatting
  // the HTML
  auto isBlank = [](const std::string &line) {
    return line.find_first_not_of(" ") == std::string::npos;
  };
  while (!lines.empty() && isBlank(lines.back()))
    lines.pop_back();
  size_t firstReal = 0;
  while (firstReal < lines.size() && isBlank(lines[firstReal]))
    ++firstReal;
  lines.erase(lines.begin(), lines.begin() + firstReal);
  return lines;
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
      codeLines = toCodeLines(element.content);
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
