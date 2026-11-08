module;
#include <fstream>
#include <ncurses.h>
#include <string>

module presentation;
import styler;

Presentation::Presentation(std::ifstream &file, const std::string &fileName)
    : fileName(fileName) {
  // a text file has no '\0', so this reads the whole file
  std::getline(file, document, '\0');

  // each article is one slide
  const auto presentations = findAll(document, {"main"});
  if (!presentations.empty())
    for (const Element &article :
         findAll(presentations.front().content, {"article"}))
      slides.emplace_back(article.content);

  slide = slides.begin();
}

void Presentation::start() {
  int key;

  if (slides.empty())
    return;

  initscr();
  startColors();
  // read keys as they are pressed, without printing them
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  // so Esc doesn't wait a whole second to see if it starts an arrow key
  set_escdelay(25);
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
    case 'i':
      if (!is_hidden && slide->hasCode() && slide->edit())
        save();
      break;
    }
    render();
  };

  endwin();
}

void Presentation::save() {
  // find the slide's article, then the first code element in it, the same one
  // the slide was made from
  size_t start = document.find("<main");
  for (auto article = slides.begin(); article <= slide; ++article)
    start = document.find("<article", start + 1);
  start = document.find('>', document.find("<code", start)) + 1;

  // replace up to the last line break, keeping the indent before </code>
  size_t end = document.rfind('\n', document.find("</code>", start));
  document.replace(start, end - start, slide->codeHTML());
  std::ofstream(fileName) << document;
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