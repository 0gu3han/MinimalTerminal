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
  const int width = screenWidth - 2 * MARGIN;
  // the content starts below the title and the gap, and stops above the
  // counter on the last row
  const int contentRow = TITLE_ROW + 1 + GAP;
  const int contentHeight = screenHeight - 1 - contentRow;

  Areas areas{};
  if (layout == LayoutType::Subsection) {
    // the title in the middle of the screen, the subtitle right under it
    const int middle = screenHeight / 2 - 1;
    areas.title = {middle, MARGIN, 1, width};
    areas.left = {middle + 1, MARGIN, 1, width};
    return areas;
  }

  areas.title = {TITLE_ROW, MARGIN, 1, width};
  if (layout == LayoutType::TwoContent) {
    // the right side gets any leftover column so nothing is lost on odd widths
    const int leftWidth = (width - COLUMN_GAP) / 2;
    areas.left = {contentRow, MARGIN, contentHeight, leftWidth};
    areas.right = {contentRow, MARGIN + leftWidth + COLUMN_GAP, contentHeight,
                   width - leftWidth - COLUMN_GAP};
  } else
    areas.left = {contentRow, MARGIN, contentHeight, width};
  return areas;
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

  // the bytes after the first byte of a UTF-8 character all start with 10
  size_t size = 1;
  while (pos + size < html.size() &&
         (static_cast<unsigned char>(html[pos + size]) & 0xC0) == 0x80)
    ++size;
  return {size, 1};
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
