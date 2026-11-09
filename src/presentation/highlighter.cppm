module;
#include <ncurses.h>
#include <string>
#include <vector>

export module highlighter;

/// The curses style of each character in a line of code, so keywords, strings,
/// comments and numbers stand out. Plain code color if the language is not
/// "cpp" or "python"
export std::vector<attr_t> highlight(const std::string &line,
                                     const std::string &language);
