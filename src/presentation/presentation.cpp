
module;
#include <stdexcept>
#include <fstream>
#include <ncurses.h>
#include <string>
#include "navigation.hpp"

module presentation;

Presentation::Presentation(std::ifstream &file) {
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

  slide = slides.begin();
}

void Presentation::start() {
  if (slides.empty())
  throw std::runtime_error("Presentation contains no slides");

  Navigation nav(slides.size());
  int key;

  initscr();
  keypad(stdscr, TRUE);
  curs_set(0);

  render();

  while ((key = getch()) != 'q' && key != ERR) {
    bool changed = false;

    switch (key) {
    case 'n':
    case ' ':
    case '\n':
    case '\r':
    case KEY_ENTER:
    case KEY_RIGHT:
      changed = nav.next();
      break;
    case 'p':
    case KEY_LEFT:
      changed = nav.previous();
      break;
    case 'g':
    case KEY_HOME:
      changed = nav.first();
      break;
    case 'G':
    case KEY_END:
      changed = nav.last();
      break;
    case KEY_UP:
      is_hidden = !is_hidden;
      changed = true;
      break;
    case KEY_RESIZE:
      changed = true;
      break;
    }

    if (changed)
      slide = slides.begin() + nav.currentSlide();

      render();

  }

  endwin();
}


void Presentation::render() {
  // Clear the screen
  erase();

  // Render the current slide
  if (!is_hidden)
    (*slide).render();

  // Print navigation instructions and the current slide number
  int rows = getmaxy(stdscr);
  int cols = getmaxx(stdscr);

  if (rows > 0 && cols > 0) {
    const char *hint = "n/p: navigate, q: quit";
    std::string position =
        std::to_string(std::distance(slides.begin(), slide) + 1) +
        "/" + std::to_string(slides.size());

   int positionWidth = static_cast<int>(position.size());

  if (cols >= positionWidth) {
    int hintWidth = cols - positionWidth - 1;

    if (hintWidth > 0)
      mvaddnstr(rows - 1, 0, hint, hintWidth);

    mvaddnstr(rows - 1, cols - positionWidth,
            position.c_str(), positionWidth);
    }
  }

  // Refresh the screen
  refresh();
}
