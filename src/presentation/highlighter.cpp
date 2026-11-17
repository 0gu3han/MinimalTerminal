module;
#include <algorithm>
#include <cctype>
#include <ncurses.h>
#include <string>
#include <vector>

module highlighter;
import styler;

static const std::vector<std::string> cppKeywords = {
    "auto",      "bool",     "break",    "case",     "catch",     "char",
    "class",     "const",    "constexpr", "continue", "default",  "delete",
    "do",        "double",   "else",     "enum",     "explicit",  "false",
    "float",     "for",      "if",       "inline",   "int",       "long",
    "namespace", "new",      "nullptr",  "operator", "private",   "protected",
    "public",    "return",   "short",    "signed",   "sizeof",    "static",
    "struct",    "switch",   "template", "this",     "throw",     "true",
    "try",       "typename", "unsigned", "using",    "virtual",   "void",
    "while"};

static const std::vector<std::string> pythonKeywords = {
    "and",     "as",     "assert", "async",    "await",  "break",
    "class",   "continue", "def",  "del",      "elif",   "else",
    "except",  "False",  "finally", "for",     "from",   "global",
    "if",      "import", "in",     "is",       "lambda", "None",
    "nonlocal", "not",   "or",     "pass",     "raise",  "return",
    "True",    "try",    "while",  "with",     "yield"};

// unsigned char so bytes of UTF-8 text don't break the <cctype> checks
static bool isDigit(unsigned char character) { return std::isdigit(character); }
static bool isWord(unsigned char character) {
  return std::isalnum(character) || character == '_';
}

std::vector<attr_t> highlight(const std::string &line,
                              const std::string &language, std::string &open) {
  std::vector<attr_t> styles(line.size(), COLOR_PAIR(codePair));
  const bool python = language == "python";
  if (!python && language != "cpp")
    return styles;
  const auto &keywords = python ? pythonKeywords : cppKeywords;

  // find where each piece of the line ends and what color it gets
  for (size_t start = 0; start < line.size();) {
    char character = line[start];
    size_t end = start + 1;
    short pair = codePair;

    if (!open.empty()) {
      // still inside a comment or string, up to where it closes
      size_t close = line.find(open, start);
      end = close == std::string::npos ? line.size() : close + open.size();
      pair = open == "*/" ? commentPair : stringPair;
      if (close != std::string::npos)
        open.clear();
    } else if (python ? character == '#' : line.compare(start, 2, "//") == 0) {
      // a comment runs to the end of the line
      end = line.size();
      pair = commentPair;
    } else if (!python && line.compare(start, 2, "/*") == 0) {
      // the next piece finds where the comment closes, maybe on a later line
      open = "*/";
      end = start + 2;
      pair = commentPair;
    } else if (python && (line.compare(start, 3, "\"\"\"") == 0 ||
                          line.compare(start, 3, "'''") == 0)) {
      // same for a string in triple quotes
      open = line.substr(start, 3);
      end = start + 3;
      pair = stringPair;
    } else if (!python && character == '#') {
      // so does #include and the like
      end = line.size();
      pair = preprocessorPair;
    } else if (character == '"' || character == '\'') {
      // up to the matching quote, skipping escaped characters like \"
      while (end < line.size() && line[end] != character)
        end += line[end] == '\\' ? 2 : 1;
      end = std::min(end + 1, line.size());
      pair = stringPair;
    } else if (isDigit(character)) {
      // takes in 3.14, 0x1F and 10u too
      while (end < line.size() && (isWord(line[end]) || line[end] == '.'))
        ++end;
      pair = numberPair;
    } else if (isWord(character)) {
      // a whole word, so the "int" in "print" is not a keyword
      while (end < line.size() && isWord(line[end]))
        ++end;
      if (std::ranges::contains(keywords, line.substr(start, end - start)))
        pair = keywordPair;
    }

    std::fill(styles.begin() + start, styles.begin() + end, COLOR_PAIR(pair));
    start = end;
  }
  return styles;
}
