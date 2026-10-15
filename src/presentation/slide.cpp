#include "slide.hpp"

// remove spaces and tabs from both ends of the text
static std::string trim(const std::string& text)
{
    size_t first = text.find_first_not_of(" \t");
    if (first == std::string::npos) // empty or all whitespace
        return "";

    size_t last = text.find_last_not_of(" \t");
    return text.substr(first, last - first + 1);
}
//extract the heading and bullet points from the raw HTML of the slide's <article> element
Slide::Slide(std::string rawHTML)
{   //get the title of the slide
    size_t titleStart = rawHTML.find("<h1>");
    size_t titleEnd = rawHTML.find("</h1>");
    if (titleStart != std::string::npos && titleEnd != std::string::npos)
        heading = trim(rawHTML.substr(titleStart + 4, titleEnd - titleStart - 4)); // 4 = length of "<h1>"
//get the bullet points together
    size_t firstBullet = rawHTML.find("<ul>");
    size_t lastBullet = rawHTML.find("</ul>");
    if (firstBullet == std::string::npos || lastBullet == std::string::npos)
        return;
//get each individual bullet point and add it to the bullets vector
    size_t bulletStart = rawHTML.find("<li>", firstBullet);
    while (bulletStart != std::string::npos && bulletStart < lastBullet) {
        size_t bulletEnd = rawHTML.find("</li>", bulletStart + 4); // 4 = length of "<li>"
        if (bulletEnd == std::string::npos)
            break;

        bullets.push_back(trim(rawHTML.substr(bulletStart + 4, bulletEnd - bulletStart - 4)));
        bulletStart = rawHTML.find("<li>", bulletEnd + 5); // 5 = length of "</li>"
    }
}
