module;
#include <algorithm>
#include <clocale>
#include <cstdio>
#include <fstream>
#include <ncurses.h>
#include <string>
#include <vector>

module presentation;
import editor;
import layout;
import styler;

Presentation::Presentation(std::ifstream &file, const std::string &fileName)
    : fileName(fileName) {
  // a text file has no '\0', so this reads the whole file
  std::getline(file, document, '\0');
  load();
}

void Presentation::load() {
  const auto current =
      slides.empty() ? 0 : std::distance(slides.begin(), slide);
  slides.clear();

  // each article is one slide
  const auto presentations = findAll(document, {"main"});
  if (!presentations.empty())
    for (const Element &article :
         findAll(presentations.front().content, {"article"}))
      slides.emplace_back(article.content,
                          presentations.front().contentStart +
                              article.contentStart,
                          layoutFromTag(article.openingTag));

  slide = slides.begin() +
          std::min<std::ptrdiff_t>(
              current, std::max<std::ptrdiff_t>(slides.size(), 1) - 1);
}

void Presentation::start() {
  int key;

  if (slides.empty())
    return;

  // so … and other non-ASCII text show as themselves
  std::setlocale(LC_ALL, "");
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
      if (!is_hidden && slide->hasCode())
        editCode();
      break;
    }
    render();
  };

  endwin();
}

size_t Presentation::articleStart(std::vector<Slide>::iterator article) {
  size_t start = document.find("<main");
  for (auto other = slides.begin(); other <= article; ++other)
    start = document.find("<article", start + 1);
  return start;
}

void Presentation::save() {
  const size_t start = slide->codeStart();
  const size_t end = slide->codeEnd();
  if (start == std::string::npos || end == std::string::npos || start > end ||
      end > document.size())
    return;

  // keep </code> on its own line with its indent if it was written that way
  const std::string old = document.substr(start, end - start);
  const size_t lastBreak = old.rfind('\n');
  const bool closingOnOwnLine =
      lastBreak != std::string::npos &&
      old.find_first_not_of(" \t", lastBreak + 1) == std::string::npos;
  document.replace(start, end - start,
                   slide->codeHTML() +
                       (closingOnOwnLine ? old.substr(lastBreak) : ""));
  std::ofstream(fileName) << document;
  // everything after the code moved, so find where each slide is again
  load();
}

// Run the code and return what it printed. The code goes in a file in /tmp
// first, and each program gets 5 seconds of CPU time so an endless loop stops.
static std::string run(const std::string &code, const std::string &language) {
  const bool python = language == "python";
  std::ofstream(python ? "/tmp/dynodeck.py" : "/tmp/dynodeck.cpp") << code;
  const std::string program = python ? "python3 /tmp/dynodeck.py"
                                     : "c++ -std=c++23 /tmp/dynodeck.cpp -o "
                                       "/tmp/dynodeck && /tmp/dynodeck";
  // no input, so it never waits on the keyboard, and errors and messages from
  // the shell are read too instead of going over the slides
  FILE *pipe = popen(("exec 2>&1 < /dev/null; (ulimit -t 5; " + program +
                      ") || echo \"Exited with status $?\"")
                         .c_str(),
                     "r");
  std::string output;
  char buffer[256];
  // stop reading a program that prints forever
  while (output.size() < 10000 && fgets(buffer, sizeof buffer, pipe))
    output += buffer;
  pclose(pipe);
  return output;
}

static bool canRun(const std::string &language) {
  return language == "cpp" || language == "python";
}

void Presentation::runCode() {
  const std::string &language = slide->codeLanguage();
  if (!canRun(language))
    return;

  mvaddstr(getmaxy(stdscr) - 1, 0, "Running...");
  clrtoeol();
  refresh();
  const std::string html = "\n        <output>\n" +
                           encodeEntities(run(slide->codeText(), language)) +
                           "        </output>\n    ";

  // the output goes in its own article right after the code's article,
  // replacing the output from the last run if it is there
  if (slide + 1 != slides.end() && (slide + 1)->showsOutput()) {
    size_t start = document.find('>', articleStart(slide + 1)) + 1;
    document.replace(start, document.find("</article>", start) - start, html);
  } else {
    size_t codeEnd = document.find("</article>", articleStart(slide)) +
                     10; // 10 = length of "</article>"
    document.insert(codeEnd, "\n    <article>" + html + "</article>");
  }
  std::ofstream(fileName) << document;
  load();
}

// what the code printed, as an output block to draw
static Block outputBlock(const std::string &output) {
  Block block{BlockKind::Output, "<output>", {}};
  size_t start = 0;
  size_t end;
  while ((end = output.find('\n', start)) != std::string::npos) {
    block.lines.push_back(encodeEntities(output.substr(start, end - start)));
    start = end + 1;
  }
  if (start < output.size())
    block.lines.push_back(encodeEntities(output.substr(start)));
  return block;
}

// a header row across the screen, like " Code (cpp) "
static void drawBar(const Rect &area, const std::string &text) {
  if (area.height <= 0)
    return;
  attrset(A_REVERSE);
  mvhline(area.row, area.col, ' ', area.width);
  mvaddnstr(area.row, area.col, text.c_str(), area.width);
  attrset(A_NORMAL);
}

void Presentation::editCode() {
  const std::string &language = slide->codeLanguage();
  Editor editor(slide->codeLines(), language);
  std::string output = canRun(language)
                           ? "Press Ctrl-R to run the code\n"
                           : "Only cpp and python code can be run\n";
  const std::string help =
      "-- INSERT --   Ctrl-R: run   Esc: keep changes   Ctrl-X: undo changes";
  curs_set(1);

  while (true) {
    // the areas are worked out again each time, so a resize reflows them
    const SplitAreas areas = splitAreas(getmaxy(stdscr), getmaxx(stdscr));
    erase();
    drawBar(areas.editorHeader,
            language.empty() ? " Code" : " Code (" + language + ")");
    drawBar(areas.outputHeader, " Output");
    mvaddnstr(areas.status.row, areas.status.col, help.c_str(),
              areas.status.width);
    wnoutrefresh(stdscr);

    if (const Window outputWindow = windowFor(areas.output)) {
      drawBlocks(outputWindow.get(), {outputBlock(output)});
      wnoutrefresh(outputWindow.get());
    }
    // the editor goes last so the cursor ends up in it
    const Window editorWindow = windowFor(areas.editor);
    if (editorWindow) {
      keypad(editorWindow.get(), TRUE);
      editor.draw(editorWindow.get());
      wnoutrefresh(editorWindow.get());
    }
    doupdate();

    const int key = editorWindow ? wgetch(editorWindow.get()) : getch();
    switch (key) {
    case 27: // Esc
      slide->setCode(editor.lines());
      save();
      runCode();
      curs_set(0);
      return;
    case 24: // Ctrl-X
      curs_set(0);
      return;
    case 18: // Ctrl-R
      if (canRun(language)) {
        mvaddnstr(areas.status.row, areas.status.col, "Running...",
                  areas.status.width);
        clrtoeol();
        refresh();
        output = run(editor.text(), language);
        if (output.empty())
          output = "(no output)\n";
      }
      break;
    case KEY_RESIZE:
      break;
    default:
      editor.press(key);
    }
  }
}

void Presentation::render() {
  erase();

  // the slide counter in the bottom right corner
  const std::string counter =
      "Slide " + std::to_string(std::distance(slides.begin(), slide) + 1) +
      " of " + std::to_string(slides.size());
  mvaddstr(getmaxy(stdscr) - 1,
           std::max(getmaxx(stdscr) - static_cast<int>(counter.size()) - 1, 0),
           counter.c_str());
  // the screen goes first so the slide's windows are drawn over it
  wnoutrefresh(stdscr);

  if (!is_hidden)
    slide->render();

  doupdate();
}
