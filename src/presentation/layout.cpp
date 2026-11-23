module;
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

module layout;

const int MARGIN = 2;      // empty columns on the left and right
const int TITLE_ROW = 1;
const int GAP = 1;         // blank rows between the title and the content
const int COLUMN_GAP = 4;  // columns between the two areas in two content

LayoutType layoutFromTag(const std::string &openingTag) {
  if (openingTag.find("two-content") != std::string::npos)
    return LayoutType::TwoContent;
  if (openingTag.find("subsection") != std::string::npos)
    return LayoutType::Subsection;
  return LayoutType::TitleAndContent;
}

Areas areasFor(LayoutType layout, int screenHeight, int screenWidth) {
  const int width = std::max(screenWidth - 2 * MARGIN, 0);
  // the content starts below the title and the gap, and stops above the
  // counter on the last row
  const int contentRow = TITLE_ROW + 1 + GAP;
  const int contentHeight = std::max(screenHeight - 1 - contentRow, 0);

  Areas areas{};
  if (layout == LayoutType::Subsection) {
    // the title in the middle of the screen, the subtitle right under it
    const int middle = std::max(screenHeight / 2 - 1, 0);
    areas.title = {middle, MARGIN, 1, width};
    areas.left = {middle + 1, MARGIN, 1, width};
    return areas;
  }

  areas.title = {TITLE_ROW, MARGIN, 1, width};
  if (layout == LayoutType::TwoContent) {
    // the right side gets any leftover column so nothing is lost on odd widths
    const int leftWidth = std::max((width - COLUMN_GAP) / 2, 0);
    const int rightWidth = std::max(width - leftWidth - COLUMN_GAP, 0);
    areas.left = {contentRow, MARGIN, contentHeight, leftWidth};
    areas.right = {contentRow, MARGIN + leftWidth + COLUMN_GAP, contentHeight,
                   rightWidth};
  } else
    areas.left = {contentRow, MARGIN, contentHeight, width};
  return areas;
}

SplitAreas splitAreas(int screenHeight, int screenWidth) {
  const int width = std::max(screenWidth, 0);
  // the rows left once the two headers and the key help have theirs, with
  // two fifths of them for the output
  const int rows = std::max(screenHeight - 3, 0);
  const int outputHeight = rows * 2 / 5;
  const int editorHeight = rows - outputHeight;

  SplitAreas areas{};
  areas.editorHeader = {0, 0, 1, width};
  areas.editor = {1, 0, editorHeight, width};
  areas.outputHeader = {1 + editorHeight, 0, 1, width};
  areas.output = {2 + editorHeight, 0, outputHeight, width};
  areas.status = {std::max(screenHeight - 1, 0), 0, 1, width};
  return areas;
}

// the bytes of the UTF-8 character starting at pos; the bytes after the first
// byte of a character all start with 10
static size_t characterSize(const std::string &text, size_t pos) {
  size_t size = 1;
  while (pos + size < text.size() &&
         (static_cast<unsigned char>(text[pos + size]) & 0xC0) == 0x80)
    ++size;
  return size;
}

// how many bytes and columns one piece of the text takes: a whole tag, a whole
// entity or one UTF-8 character
struct Piece {
  size_t size;
  int width;
};

static Piece pieceAt(const std::string &html, size_t pos) {
  if (html[pos] == '<') {
    size_t end = html.find('>', pos);
    return {end == std::string::npos ? html.size() - pos : end - pos + 1, 0};
  }

  if (html[pos] == '&') {
    size_t end = html.find(';', pos);
    // &amp; &lt; &#39; and so on, but not a lone & in the text
    if (end != std::string::npos && end - pos <= 8 &&
        std::all_of(html.begin() + pos + 1, html.begin() + end, [](char c) {
          return std::isalnum(static_cast<unsigned char>(c)) || c == '#';
        }))
      return {end - pos + 1, 1};
  }

  return {characterSize(html, pos), 1};
}

static bool isSpace(char c) { return c == ' ' || c == '\n' || c == '\t'; }

// "<b>" and "</b>" both give "b"
static std::string tagName(const std::string &tag) {
  size_t start = tag[1] == '/' ? 2 : 1;
  size_t end = tag.find_first_of(" >", start);
  return tag.substr(start, end - start);
}

int displayWidth(const std::string &html) {
  int width = 0;
  for (size_t pos = 0; pos < html.size();) {
    Piece piece = pieceAt(html, pos);
    width += piece.width;
    pos += piece.size;
  }
  return width;
}

std::vector<std::string> wrap(const std::string &html, int width) {
  width = std::max(width, 1);
  std::vector<std::string> lines;
  std::vector<std::string> openTags;
  std::string line;
  int lineWidth = 0;

  auto breakLine = [&] {
    for (auto tag = openTags.rbegin(); tag != openTags.rend(); ++tag)
      line += "</" + tagName(*tag) + ">";
    lines.push_back(line);
    line.clear();
    for (const std::string &tag : openTags)
      line += tag;
    lineWidth = 0;
  };

  auto addWord = [&](const std::string &word) {
    int wordWidth = displayWidth(word);
    if (wordWidth > 0 && lineWidth > 0) {
      if (lineWidth + 1 + wordWidth > width)
        breakLine();
      else {
        line += ' ';
        ++lineWidth;
      }
    }

    for (size_t pos = 0; pos < word.size();) {
      Piece piece = pieceAt(word, pos);
      std::string text = word.substr(pos, piece.size);
      if (piece.width == 0) {
        // keep track of which styles are on
        if (text[1] == '/') {
          auto open = std::ranges::find_last_if(openTags, [&](auto &tag) {
            return tagName(tag) == tagName(text);
          });
          if (!open.empty())
            openTags.erase(open.begin());
        } else
          openTags.push_back(text);
      } else if (lineWidth + piece.width > width)
        // only a word wider than the whole line gets here
        breakLine();
      line += text;
      lineWidth += piece.width;
      pos += piece.size;
    }
  };

  // spaces inside a tag like <span style="..."> do not split words
  std::string word;
  for (size_t pos = 0; pos < html.size();) {
    Piece piece = pieceAt(html, pos);
    if (piece.width == 1 && isSpace(html[pos])) {
      addWord(word);
      word.clear();
    } else
      word += html.substr(pos, piece.size);
    pos += piece.size;
  }
  addWord(word);

  lines.push_back(line);
  return lines;
}

std::string truncate(const std::string &html, int width) {
  if (displayWidth(html) <= width)
    return html;
  if (width <= 0)
    return "";

  // leave one column for the …
  std::string result;
  int used = 0;
  for (size_t pos = 0; pos < html.size();) {
    Piece piece = pieceAt(html, pos);
    if (used + piece.width > width - 1)
      break;
    result += html.substr(pos, piece.size);
    used += piece.width;
    pos += piece.size;
  }
  return result + "…";
}

// replace each tab with spaces up to the next stop, with pieceOf saying how
// many bytes and columns the rest of the text takes
template <typename PieceOf>
static std::string expandTabsWith(const std::string &text, PieceOf pieceOf) {
  std::string result;
  int column = 0;
  for (size_t pos = 0; pos < text.size();) {
    const Piece piece = pieceOf(text, pos);
    if (text[pos] == '\t') {
      const int spaces = 8 - column % 8;
      result.append(spaces, ' ');
      column += spaces;
    } else {
      result.append(text, pos, piece.size);
      column += piece.width;
    }
    pos += piece.size;
  }
  return result;
}

std::string expandTabs(const std::string &html) {
  return expandTabsWith(html, pieceAt);
}

std::string expandCodeTabs(const std::string &text) {
  // in code < and & are characters like any other
  return expandTabsWith(text, [](const std::string &code, size_t pos) {
    return Piece{characterSize(code, pos), 1};
  });
}

Fit fitText(const std::string &text, int width) {
  if (width <= 0)
    return {0, false};

  int columns = 0;
  // the bytes in all but the last column, which is where the … goes
  size_t allButLast = 0;
  for (size_t pos = 0; pos < text.size(); pos += characterSize(text, pos)) {
    if (columns == width)
      return {allButLast, true};
    if (++columns == width - 1)
      allButLast = pos + characterSize(text, pos);
  }
  return {text.size(), false};
}

std::vector<Row> layOut(const std::vector<Block> &blocks, int width,
                        int height) {
  std::vector<Row> rows;
  for (size_t block = 0; block < blocks.size(); ++block) {
    const Block &current = blocks[block];
    // bullets stack right under each other, and a blank row sets everything
    // else apart
    if (block > 0 && (current.kind != BlockKind::Bullet ||
                      blocks[block - 1].kind != BlockKind::Bullet))
      rows.push_back({.kind = RowKind::Blank});

    for (size_t line = 0; line < current.lines.size(); ++line) {
      const std::string &text = current.lines[line];
      switch (current.kind) {
      case BlockKind::Bullet: {
        const auto pieces = wrap(text, width - bulletIndent);
        for (size_t piece = 0; piece < pieces.size(); ++piece) {
          const bool first = piece == 0;
          int column = bulletIndent;
          if (current.centered) {
            // the first row is centered with its marker, the rest on their own
            const int marker = first ? bulletIndent : 0;
            const int rowWidth = marker + displayWidth(pieces[piece]);
            column = std::max((width - rowWidth) / 2, 0) + marker;
          }
          rows.push_back({.kind = RowKind::Bullet,
                          .block = block,
                          .line = line,
                          .column = column,
                          .text = pieces[piece],
                          .first = first});
        }
        break;
      }
      case BlockKind::Code: {
        const std::string expanded = expandCodeTabs(text);
        const Fit fit = fitText(expanded, width - 2 * codePadding);
        rows.push_back({.kind = RowKind::Code,
                        .block = block,
                        .line = line,
                        .column = codePadding,
                        .text = expanded.substr(0, fit.bytes),
                        .cut = fit.cut});
        break;
      }
      case BlockKind::Output:
        rows.push_back({.kind = RowKind::Output,
                        .block = block,
                        .line = line,
                        .text = truncate(expandTabs(text), width)});
        break;
      }
    }
  }

  if (height <= 0)
    return {};
  // show what fits, and say there is more on the last row
  if (rows.size() > static_cast<size_t>(height)) {
    rows.resize(height - 1);
    rows.push_back({.kind = RowKind::Overflow, .text = "..."});
  }
  return rows;
}
