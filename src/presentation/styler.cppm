module;
#include <ncurses.h>
#include <string>

export module styler;

/// The color pair for code blocks, after the 8 standard colors
export const short codePair = 9;

/// Call once after initscr()
export void startColors();

/// Turn &lt; &gt; &quot; &#39; &amp; back into < > " ' &
export std::string decodeEntities(std::string text);

/// The curses color for a tag with a style like style="color: red"
export attr_t colorOf(const std::string &openingTag);
