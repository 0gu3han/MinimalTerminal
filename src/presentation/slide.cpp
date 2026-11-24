module;
#include <algorithm>
#include <cctype>
#include <ncurses.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

module slide;
import highlighter;
import styler;

// empty columns between the screen's edges and the slide, and between two
// columns
const int margin = 2;
const int columnGap = 4;

// the tags that are blocks of their own. Others, like <ul> and <div>, are only
// looked inside.
static const std::vector<std::string> blockNames = {
    "h1", "h2", "h3", "h4", "p", "li", "dt", "dd", "figcaption",
    "ol", "pre", "output", "section"};

std::vector<Element> findAll(const std::string &html,
                             const std::vector<std::string> &names) {
  std::vector<Element> elements;
  for (size_t start = html.find('<'); start != std::string::npos;
       start = html.find('<', start + 1)) {
    const std::string name =
        html.substr(start + 1, html.find_first_of(" >", start) - start - 1);
    if (!std::ranges::contains(names, name))
      continue;

    const size_t tagEnd = html.find('>', start);
    const size_t closing = html.find("</" + name + ">", tagEnd);
    if (closing == std::string::npos)
      throw std::runtime_error("<" + name + "> is missing its </" + name + ">");
    elements.push_back({name, html.substr(start, tagEnd + 1 - start),
                        html.substr(tagEnd + 1, closing - tagEnd - 1),
                        tagEnd + 1, closing});
    // jump past the content so tags inside it are not matched
    start = closing;
  }
  return elements;
}

Window windowAt(int row, int column, int height, int width) {
  if (height <= 0 || width <= 0)
    return nullptr;
  return Window(derwin(stdscr, height, width, row, column));
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

// the decoded lines between <output> and </output>, skipping the line break
// after <output> and the indent before </output>
static std::vector<std::string> outputLines(const std::string &html) {
  std::vector<std::string> lines;
  size_t start = html.find('\n') + 1;
  size_t end;
  while ((end = html.find('\n', start)) != std::string::npos) {
    lines.push_back(decodeEntities(html.substr(start, end - start)));
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
  return openingTag.substr(start, openingTag.find('"', start) - start);
}

// the text of the HTML without its tags
static std::string plainText(std::string html) {
  for (size_t tag; (tag = html.find('<')) != std::string::npos;)
    html.erase(tag, html.find('>', tag) - tag + 1);
  return decodeEntities(html);
}

// how many columns the HTML takes on screen. Each UTF-8 character has exactly
// one byte that does not start with 10.
static int textWidth(const std::string &html) {
  return static_cast<int>(
      std::ranges::count_if(plainText(html), [](unsigned char c) {
        return (c & 0xC0) != 0x80;
      }));
}

// Move to column on the row. If the row is past the bottom of the window, put
// ... on the last row instead to show there is more, and return false.
static bool startRow(WINDOW *window, int row, int column) {
  if (row < getmaxy(window))
    return wmove(window, row, column) == OK;
  wattrset(window, A_DIM);
  mvwaddstr(window, getmaxy(window) - 1, 0, "...");
  wclrtoeol(window);
  wattrset(window, A_NORMAL);
  return false;
}

// Draw the HTML from column on the row with its <b>, <i> and <u> tags applied,
// wrapping between words onto the rows below at the same column. Returns
// false if it ran out of rows.
static bool drawText(WINDOW *window, int &row, int column,
                     const std::string &html, attr_t style) {
  if (!startRow(window, row, column))
    return false;
  wattrset(window, style);
  bool space = false;
  for (size_t start = 0; start < html.size();) {
    if (html[start] == '<') {
      // a tag turns a style on or off; tags that are not styles change nothing
      const bool closing = html[start + 1] == '/';
      const size_t nameStart = start + 1 + closing;
      const attr_t attribute = attributeOf(html.substr(
          nameStart, html.find_first_of(" />", nameStart) - nameStart));
      closing ? wattroff(window, attribute) : wattron(window, attribute);
      start = html.find('>', start) + 1;
    } else if (std::isspace(static_cast<unsigned char>(html[start]))) {
      // a run of spaces and line breaks is one space, like in a browser, and
      // the last column stays empty so curses never wraps a row by itself
      if (!space && getcurx(window) > column &&
          getcurx(window) < getmaxx(window) - 1)
        waddch(window, ' ');
      space = true;
      ++start;
    } else {
      const size_t end = html.find_first_of(" \n<", start);
      const std::string word = html.substr(start, end - start);
      if (getcurx(window) > column &&
          getcurx(window) + textWidth(word) >= getmaxx(window) &&
          !startRow(window, ++row, column))
        return false;
      waddstr(window, decodeEntities(word).c_str());
      space = false;
      start = end;
    }
  }
  wattrset(window, A_NORMAL);
  ++row;
  return true;
}

// each line of code on a grey row, highlighted, ending with … if it is cut off
static bool drawCode(WINDOW *window, int &row, const Block &code) {
  const std::string language = languageOf(code.openingTag);
  // what a comment or string left open on one line means for the next
  std::string open;
  for (const std::string &line : code.lines) {
    if (!startRow(window, row, 0))
      return false;
    wattrset(window, COLOR_PAIR(codePair));
    whline(window, ' ', getmaxx(window));
    wmove(window, row, 1);
    const std::vector<attr_t> styles = highlight(line, language, open);
    for (size_t i = 0; i < line.size(); ++i) {
      if (getcurx(window) == getmaxx(window) - 2 && i + 1 < line.size()) {
        waddstr(window, "…");
        break;
      }
      waddch(window, static_cast<unsigned char>(line[i]) | styles[i]);
    }
    ++row;
  }
  wattrset(window, A_NORMAL);
  return true;
}

// each line the code printed, cut off at the edge
static bool drawOutput(WINDOW *window, int &row, const Block &output) {
  for (const std::string &line : output.lines) {
    if (!startRow(window, row, 0))
      return false;
    waddnstr(window, line.c_str(), getmaxx(window) - 1);
    ++row;
  }
  return true;
}

// a paragraph, bullet or other block of text, with its marker. Headings and
// the terms of a definition list are bold, and definitions are indented.
static bool drawBlock(WINDOW *window, int &row, const Block &block) {
  const bool bold = block.name.starts_with('h') || block.name == "dt";
  const attr_t style = (bold ? A_BOLD : A_NORMAL) | colorOf(block.openingTag);
  const int indent = block.name == "dd"      ? 4
                     : block.marker.empty() ? 0
                                            : textWidth(block.marker) + 1;
  wattrset(window, style);
  mvwaddstr(window, row, 0, block.marker.c_str());
  return drawText(window, row, indent, block.lines.front(), style);
}

static bool isListItem(const Block &block) {
  return block.name == "li" || block.name == "dt" || block.name == "dd";
}

// the blocks from the top of the window down, with a blank row between them
// except between the items of a list
static void drawBlocks(const Window &window, const std::vector<Block> &blocks) {
  int row = 0;
  for (size_t i = 0; i < blocks.size(); ++i) {
    const Block &block = blocks[i];
    if (i > 0 && !(isListItem(block) && isListItem(blocks[i - 1])))
      ++row;
    const bool fits = block.name == "pre"      ? drawCode(window.get(), row, block)
                      : block.name == "output" ? drawOutput(window.get(), row, block)
                                               : drawBlock(window.get(), row, block);
    if (!fits)
      return;
  }
}

Slide::Slide(const std::string &html, size_t sourceStart, bool twoColumns)
    : twoColumns(twoColumns) {
  addBlocks(html, sourceStart, 0, {"p"});
}

// Add the blocks in the HTML to the column, in the order they are written.
// The text between them, like a bullet's words before its code, becomes a
// block like text.
void Slide::addBlocks(const std::string &html, size_t sourceStart,
                      size_t column, Block text) {
  size_t textStart = 0;
  const auto addText = [&](size_t textEnd) {
    text.lines = {html.substr(textStart, textEnd - textStart)};
    if (plainText(text.lines.front()).find_first_not_of(" \n") ==
        std::string::npos)
      return;
    // the first heading is the title
    if (title.empty() && text.name.starts_with('h')) {
      title = text.lines.front();
      titleTag = text.openingTag;
    } else
      columns[column].push_back(text);
    // only the first text of a list item has its marker
    text.marker.clear();
  };

  for (const Element &element : findAll(html, blockNames)) {
    addText(element.contentStart - element.openingTag.size());
    addBlock(element, sourceStart, column);
    textStart = element.contentEnd + element.name.size() + 3; // 3 = "</>"
  }
  addText(html.size());
}

void Slide::addBlock(const Element &element, size_t sourceStart,
                     size_t column) {
  std::vector<Block> &blocks = columns[column];
  const size_t contentStart = sourceStart + element.contentStart;
  if (element.name == "pre") {
    const Element code = findAll(element.content, {"code"}).front();
    // the first code block is the one to edit
    if (!editable) {
      editable = BlockIndex{column, blocks.size()};
      codeSourceStart = contentStart + code.contentStart;
      codeSourceEnd = contentStart + code.contentEnd;
      language = languageOf(code.openingTag);
    }
    blocks.push_back({"pre", code.openingTag, splitCode(code.content)});
  } else if (element.name == "output") {
    isOutput = true;
    blocks.push_back({"output", "", outputLines(element.content)});
  } else if (element.name == "ol") {
    // numbered items, like 1. and 2.
    int number = 0;
    for (const Element &item : findAll(element.content, {"li"}))
      addBlocks(item.content, contentStart + item.contentStart, column,
                {"li", item.openingTag, {}, std::to_string(++number) + "."});
  } else if (element.name == "section")
    // two columns put the first section on the left and the rest on the right
    addBlocks(element.content, contentStart,
              twoColumns ? std::min(sections++, size_t{1}) : 0, {"p"});
  else
    addBlocks(element.content, contentStart, column,
              {element.name, element.openingTag, {},
               element.name == "li" ? "•" : ""});
}

void Slide::render() const {
  // the screen inside the margins, above the counter's row
  const Window screen = windowAt(0, margin, getmaxy(stdscr) - 1,
                                 getmaxx(stdscr) - 2 * margin);
  const int width = getmaxx(screen.get());
  const int height = getmaxy(screen.get());

  // a slide of only headings has them in the middle, one under the other
  if (columns[1].empty() &&
      std::ranges::all_of(columns[0], [](const Block &block) {
        return block.name.starts_with('h');
      })) {
    int row = (height - static_cast<int>(columns[0].size()) - 1) / 2;
    const auto center = [&](const std::string &html, const std::string &tag) {
      drawText(screen.get(), row, std::max((width - textWidth(html)) / 2, 0),
               html, A_BOLD | colorOf(tag));
    };
    center(title, titleTag);
    for (const Block &heading : columns[0])
      center(heading.lines.front(), heading.openingTag);
    return;
  }

  int row = 1;
  drawText(screen.get(), row, 0, title, A_BOLD | colorOf(titleTag));
  // the content starts a blank row under the title
  const int top = row + 1;
  if (twoColumns) {
    const int left = (width - columnGap) / 2;
    drawBlocks(windowAt(top, margin, height - top, left), columns[0]);
    drawBlocks(windowAt(top, margin + left + columnGap, height - top,
                        width - left - columnGap),
               columns[1]);
  } else
    drawBlocks(windowAt(top, margin, height - top, width), columns[0]);
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
