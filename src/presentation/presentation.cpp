module;
#include <fstream>
#include <iostream>
#include <string>

module presentation;

import terminal;

Presentation::Presentation(std::ifstream &file) {
  currSlide = 0;

  std::string document;
  std::string line;
  // turn the file into a single string
  while (std::getline(file, line))
    document += line + "\n";

  // find the beginning of the presentation, and the end of the presentation
  size_t presentationStart = document.find("<main>");
  size_t presentationEnd = document.find("</main>");
  if (presentationStart == std::string::npos ||
      presentationEnd == std::string::npos)
    return;
  // after finding the start of the presentation, find each slide and add it to
  // the slides vector
  size_t beginningOfSlide = document.find("<article>", presentationStart);
  while (beginningOfSlide != std::string::npos &&
         beginningOfSlide < presentationEnd) {
    size_t endOfSlide = document.find(
        "</article>", beginningOfSlide + 9); // 9 = length of "<article>"
    if (endOfSlide == std::string::npos)
      break;

    // extract the content between <article> and </article> and pass it to the
    // Slide constructor, then add the resulting Slide instance to the slides
    // vector
    slides.push_back(Slide(document.substr(beginningOfSlide + 9,
                                           endOfSlide - beginningOfSlide - 9)));
    beginningOfSlide = document.find(
        "<article>", endOfSlide + 10); // 10 = length of "</article>"
  }
}
// show the slides one at a time and move between them based on the keys the
// user presses
void Presentation::start() {
  if (slides.empty())
    return;

  rawMode(true);
  // draw the current slide, then wait for a key: right arrow = next, left arrow
  // = previous, q = quit
  while (true) {
    slides[currSlide].render();
    Key key = readKey();
    if (key == Key::Quit)
      break;

    if (key == Key::Right && currSlide < static_cast<int>(slides.size()) - 1)
      currSlide++;
    else if (key == Key::Left && currSlide > 0)
      currSlide--;
  }
  // clear the last slide and put the terminal back to normal before exiting
  std::cout << "\033[2J\033[H";
  rawMode(false);
}
