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

// extract the heading, code block and bullet points from the raw HTML of the
// slide's <article> element
Slide::Slide(std::string rawHTML) { // get the title of the slide, skipping any
                                    // attributes in the opening tag
  size_t titleStart = rawHTML.find("<h1");
  size_t titleEnd = rawHTML.find("</h1>");
  size_t titleContent = titleStart == std::string::npos
                            ? std::string::npos
                            : rawHTML.find('>', titleStart);
  if (titleContent != std::string::npos && titleEnd != std::string::npos &&
      titleContent < titleEnd) {
    heading =
        trim(rawHTML.substr(titleContent + 1, titleEnd - titleContent - 1));
    headingColor =
        colorOf(rawHTML.substr(titleStart, titleContent - titleStart));
  }
  // get the code block, then remove it so its text can't be mistaken for slide
  // tags
  size_t codeStart = rawHTML.find("<pre>");
  size_t codeEnd = rawHTML.find("</pre>");
  if (codeStart != std::string::npos && codeEnd != std::string::npos &&
      codeStart < codeEnd) {
    codeLines = toCodeLines(rawHTML.substr(
        codeStart + 5, codeEnd - codeStart - 5));      // 5 = length of "<pre>"
    rawHTML.erase(codeStart, codeEnd + 6 - codeStart); // 6 = length of "</pre>"
  }
  // get the bullet points together
  size_t firstBullet = rawHTML.find("<ul");
  size_t lastBullet = rawHTML.find("</ul>");
  if (firstBullet == std::string::npos || lastBullet == std::string::npos)
    return;
  // get each individual bullet point, skipping any attributes, and add it to
  // the bullets vector
  size_t bulletStart = rawHTML.find("<li", firstBullet);
  while (bulletStart != std::string::npos && bulletStart < lastBullet) {
    size_t contentStart = rawHTML.find('>', bulletStart);
    if (contentStart == std::string::npos)
      break;
    size_t bulletEnd = rawHTML.find("</li>", contentStart);
    if (bulletEnd == std::string::npos)
      break;

    bullets.push_back(
        trim(rawHTML.substr(contentStart + 1, bulletEnd - contentStart - 1)));
    bulletColors.push_back(
        colorOf(rawHTML.substr(bulletStart, contentStart - bulletStart)));
    bulletStart = rawHTML.find("<li", bulletEnd + 5); // 5 = length of "</li>"
  }
}
// clear the screen and print the heading, bullet points  and code block of the
// slide
void Slide::render() {
  attron(A_BOLD);
  printw("%s", heading.c_str());
  attroff(A_BOLD);

  for (auto point : bullets) {
    printw("\n\t* %s", point.c_str());
  }

  // for (const auto &line : codeLines) {
  //   printw("%s\n", line.c_str());
  // }
}
