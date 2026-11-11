module;
#include <ncurses.h>
#include <string>
#include <vector>

export module highlighter;

/// The curses style of each character in a line of code, so keywords, strings,
/// comments and numbers stand out. Plain code color if the language is not
/// "cpp" or "python"
/// @param open What closes a /* comment or """ string left open by the lines
/// before; start with "" and pass the same string for each line in order
export std::vector<attr_t> highlight(const std::string &line,
                                     const std::string &language,
                                     std::string &open);
