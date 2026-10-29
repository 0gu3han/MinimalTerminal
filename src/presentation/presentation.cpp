module;
#include <fstream>
#include <ncurses.h>
#include <string>

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
    return;

  initscr();

  while (true) {
    printw("Slide %ld of %ld", std::distance(slides.begin(), slide) + 1, slides.size());

    // Render the current slide
    (*slide).render();

    refresh();

    // Wait for key press
    switch (getch()) {
    case KEY_RIGHT:
      slide = std::ranges::next(slide, 1, slides.end());
      break;
    case KEY_LEFT:
      slide = std::ranges::prev(slide, 1, slides.begin());
      break;
    case 'q':
      endwin();
      return;
    }
  }
}