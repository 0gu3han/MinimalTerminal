module;
#include <ncurses.h>
#include <string>

export module styler;

/// The color pair for code blocks, after the 8 standard colors
export const short codePair = 9;

/// The color pairs for highlighted code, on the same grey as codePair
export const short keywordPair = 10;
export const short stringPair = 11;
export const short commentPair = 12;
export const short numberPair = 13;
export const short preprocessorPair = 14;

/// Call once after initscr()
export void startColors();

/// Turn &lt; &gt; &quot; &#39; &amp; back into < > " ' &
export std::string decodeEntities(std::string text);

/// Turn & < > into &amp; &lt; &gt; so text can go back into the HTML
export std::string encodeEntities(std::string text);

/// The curses color for a tag with a style like style="color: red"
export attr_t colorOf(const std::string &openingTag);

/// Draw text at the cursor with its <b>, <i> and <u> tags applied
export void drawStyled(const std::string &html, attr_t style);
