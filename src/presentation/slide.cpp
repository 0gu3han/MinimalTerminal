module;
#include <algorithm>
#include <limits>
#include <ncurses.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

module slide;
import highlighter;
import layout;
import styler;

std::vector<Element> findAll(const std::string &html,
                             const std::vector<std::string> &tags) {
  std::vector<Element> elements;
  for (size_t start = html.find('<'); start != std::string::npos;
       start = html.find('<', start + 1))
    for (const std::string &tag : tags)
      // check if one of the tags opens here
      if (html.compare(start + 1, tag.size(), tag) == 0) {
        const size_t tagEnd = html.find('>', start);
        if (tagEnd == std::string::npos)
          throw std::runtime_error("<" + tag + " is missing its >");
        const size_t contentStart = tagEnd + 1;
        const size_t closing = html.find("</" + tag + ">", contentStart);
        if (closing == std::string::npos)
          throw std::runtime_error("<" + tag + "> is missing its </" + tag +
                                   ">");
        elements.push_back({html.substr(start, contentStart - start),
                            html.substr(contentStart, closing - contentStart),
                            contentStart, closing});
        // jump past the content so tags inside it are not matched
        start = closing;
        break;
      }
  return elements;
}

// the decoded lines of a code block, without the line breaks right after
// <code> and right before </code>
static std::vector<std::string> splitCode(const std::string &html) {
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

// the lines between <output> and </output>, skipping the line break after
// <output> and the indent before </output>. They stay encoded so drawStyled
// never reads output as a tag.
static std::vector<std::string> outputLines(const std::string &html) {
  std::vector<std::string> lines;
  size_t start = html.find('\n') + 1;
  size_t end;
  while ((end = html.find('\n', start)) != std::string::npos) {
    lines.push_back(html.substr(start, end - start));
    start = end + 1;
  }
  return lines;
}

// "cpp" from <code class="language-cpp">, or empty if there is none
static std::string languageOf(const std::string &openingTag) {
  const size_t name = openingTag.find("language-");
  if (name == std::string::npos)
    return "";
  const size_t start = name + 9; // 9 = length of "language-"
  const size_t end = openingTag.find_first_of("\"' \t\r\n\f>", start);
  return openingTag.substr(start, end == std::string::npos ? std::string::npos
                                                           : end - start);
}

// the text on one line, with runs of spaces and line breaks made one space
static std::string oneLine(const std::string &html) {
  return wrap(html, std::numeric_limits<int>::max()).front();
}

Window windowFor(const Rect &rect) {
  if (rect.height <= 0 || rect.width <= 0)
    return nullptr;
  // newwin gives nullptr if the rect does not fit on the screen
  return Window(newwin(rect.height, rect.width, rect.row, rect.col));
}

// a row of code on its grey box, highlighted, with … if it was cut off
static void drawCode(WINDOW *window, int screenRow, const Row &row,
                     const std::vector<attr_t> &styles) {
  wattrset(window, COLOR_PAIR(codePair));
  mvwhline(window, screenRow, 0, ' ', getmaxx(window));
  wmove(window, screenRow, row.column);
  for (size_t byte = 0; byte < row.text.size(); ++byte)
    waddch(window, static_cast<unsigned char>(row.text[byte]) | styles[byte]);
  if (row.cut) {
    wattrset(window, COLOR_PAIR(codePair));
    waddstr(window, "…");
  }
  wattrset(window, A_NORMAL);
}

void drawBlocks(WINDOW *window, const std::vector<Block> &blocks) {
  const std::vector<Row> rows =
      layOut(blocks, getmaxx(window), getmaxy(window));
  // what a comment or string left open on one line of code means for the next
  std::string open;
  for (int screenRow = 0; const Row &row : rows) {
    switch (row.kind) {
    case RowKind::Blank:
      break;
    case RowKind::Bullet: {
      const attr_t style = colorOf(blocks[row.block].openingTag);
      if (row.first) {
        wattrset(window, style);
        mvwaddstr(window, screenRow, row.column - bulletIndent, "•");
      }
      wmove(window, screenRow, row.column);
      drawStyled(window, row.text, style);
      break;
    }
    case RowKind::Code: {
      const Block &code = blocks[row.block];
      if (row.line == 0)
        open.clear();
      // the row's text has its tabs expanded, so highlight the line that way
      // too for each style to stay on its character
      drawCode(window, screenRow, row,
               highlight(expandCodeTabs(code.lines[row.line]),
                         languageOf(code.openingTag), open));
      break;
    }
    case RowKind::Output:
      wmove(window, screenRow, row.column);
      drawStyled(window, row.text, A_NORMAL);
      break;
    case RowKind::Overflow:
      wattrset(window, A_DIM);
      mvwaddstr(window, screenRow, 0, row.text.c_str());
      wattrset(window, A_NORMAL);
      break;
    }
    ++screenRow;
  }
}

// a heading on the first row of its area, cut off with … if it is too wide
static void drawHeading(const Rect &area, const std::string &html, attr_t style,
                        bool centered) {
  const Window window = windowFor(area);
  if (!window || html.empty())
    return;

  const std::string text = truncate(html, area.width);
  const int column =
      centered ? std::max((area.width - displayWidth(text)) / 2, 0) : 0;
  wmove(window.get(), 0, column);
  drawStyled(window.get(), text, style);
  wnoutrefresh(window.get());
}

static void drawArea(const Rect &area, const std::vector<Block> &blocks) {
  const Window window = windowFor(area);
  if (!window)
    return;
  drawBlocks(window.get(), blocks);
  wnoutrefresh(window.get());
}

// sort the headings, bullets, code and output into the areas of the layout
Slide::Slide(const std::string &html, size_t sourceStart, LayoutType layout)
    : layout(layout) {
  size_t sections = 0;
  for (const Element &element :
       findAll(html, {"h1", "h2", "section", "li", "code", "output"})) {
    const std::string &tag = element.openingTag;
    if (tag.starts_with("<h1")) {
      if (title.empty()) {
        title = oneLine(element.content);
        titleTag = tag;
      }
    } else if (tag.starts_with("<h2")) {
      if (subtitle.empty()) {
        subtitle = oneLine(element.content);
        subtitleTag = tag;
      }
    } else if (tag.starts_with("<section")) {
      // two content puts its first section on the left and the rest on the
      // right; the other layouts have one area for everything
      const size_t column = layout == LayoutType::TwoContent
                                ? std::min(sections++, size_t{1})
                                : 0;
      addBlocks(element.content, sourceStart + element.contentStart, column);
    } else
      addBlock(element, sourceStart, 0);
  }
}

void Slide::addBlocks(const std::string &html, size_t sourceStart,
                      size_t column) {
  for (const Element &element : findAll(html, {"li", "code", "output"}))
    addBlock(element, sourceStart, column);
}

void Slide::addBlock(const Element &element, size_t sourceStart,
                     size_t column) {
  const std::string &tag = element.openingTag;
  std::vector<Block> &blocks = columns[column];
  if (tag.starts_with("<li"))
    blocks.push_back(
        {BlockKind::Bullet, tag, {element.content}, isCentered(tag)});
  else if (tag.starts_with("<output")) {
    isOutput = true;
    blocks.push_back({BlockKind::Output, tag, outputLines(element.content)});
  } else {
    // the first code block is the one to edit
    if (!editable) {
      editable = BlockIndex{column, blocks.size()};
      codeSourceStart = sourceStart + element.contentStart;
      codeSourceEnd = sourceStart + element.contentEnd;
      language = languageOf(tag);
    }
    blocks.push_back({BlockKind::Code, tag, splitCode(element.content)});
  }
}

void Slide::render() const {
  const Areas areas = areasFor(layout, getmaxy(stdscr), getmaxx(stdscr));
  const bool subsection = layout == LayoutType::Subsection;
  drawHeading(areas.title, title, A_BOLD | colorOf(titleTag),
              subsection || isCentered(titleTag));

  if (subsection)
    drawHeading(areas.left, subtitle, colorOf(subtitleTag), true);
  else {
    drawArea(areas.left, columns[0]);
    if (layout == LayoutType::TwoContent)
      drawArea(areas.right, columns[1]);
  }
}

const std::vector<std::string> &Slide::codeLines() const {
  static const std::vector<std::string> none;
  return editable ? columns[editable->column][editable->block].lines : none;
}

void Slide::setCode(std::vector<std::string> lines) {
  if (editable)
    columns[editable->column][editable->block].lines = std::move(lines);
}

std::string Slide::codeHTML() const {
  std::string html;
  for (const std::string &line : codeLines())
    html += "\n" + encodeEntities(line);
  return html;
}

std::string Slide::codeText() const {
  std::string text;
  for (const std::string &line : codeLines())
    text += line + "\n";
  return text;
}
