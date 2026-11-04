module;
#include <fstream>
#include <ncurses.h>
#include <string>

module presentation;
import styler;

Presentation::Presentation(std::ifstream &file) {
  // a text file has no '\0', so this reads the whole file
  std::string document;
  std::getline(file, document, '\0');

  // each article is one slide
  for (const Element &article : findAll(document, {"article"}))
    slides.emplace_back(article.content);

  slide = slides.begin();
}

void Presentation::start() {
  int key;

  if (slides.empty())
    return;

  initscr();
  startColors();
  keypad(stdscr, TRUE);
  curs_set(0);

  render();

  while ((key = getch()) != 'q') {
    switch (key) {
    case KEY_RIGHT:
      slide = std::ranges::next(slide, 1, slides.end() - 1);
      break;
    case KEY_LEFT:
      slide = std::ranges::prev(slide, 1, slides.begin());
      break;
    case KEY_UP:
      is_hidden = !is_hidden;
      break;
    }
    render();
  };

  endwin();
}

void Presentation::render() {
  // Clear the screen
  erase();

  // Render the current slide
  if (!is_hidden)
    (*slide).render();

  // Print this neat little debug info
  mvprintw(getmaxy(stdscr) - 1, getmaxx(stdscr) - 20, "Slide %ld of %ld",
           std::distance(slides.begin(), slide) + 1, slides.size());

  // Refresh the screen
  refresh();
}