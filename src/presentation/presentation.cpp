module;
#include <csignal>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ncurses.h>
#include <string>
#include <sys/wait.h>

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
      slides.emplace_back(article.content,
                          presentations.front().contentStart +
                              article.contentStart);

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
      if (!is_hidden && slide->hasCode() && slide->edit()) {
        save();
        runCode();
      }
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

  document.replace(start, end - start, slide->codeHTML());
  std::ofstream(fileName) << document;
}

// write the code to a file in the temp folder and run it, giving back the
// first lines it printed, errors included
static std::string run(const std::string &code, const std::string &language) {
  const auto folder = std::filesystem::temp_directory_path() / "dynodeck";
  std::filesystem::create_directories(folder);
  const std::string source = language == "python" ? "main.py" : "main.cpp";
  std::ofstream(folder / source) << code;

  // perl's alarm stops a program after 5 seconds, since macOS has no timeout
  // command
  const std::string limit = "perl -e 'alarm shift; exec @ARGV' 5 ";
  std::string command = language == "python"
                            ? limit + "python3 main.py"
                            : "c++ -std=c++23 main.cpp -o main && " + limit +
                                  "./main";
  // run in the folder so errors say main.cpp instead of the whole path, with
  // no input so a program that reads from cin doesn't wait forever. The outer
  // 2>/dev/null hides the shell's own "Alarm clock" message
  command = "(cd '" + folder.string() + "' && (" + command +
            ") < /dev/null 2>&1) 2>/dev/null";

  std::string output;
  int lineCount = 0;
  FILE *pipe = popen(command.c_str(), "r");
  char buffer[256];
  while (fgets(buffer, sizeof buffer, pipe))
    // keep reading after 100 lines so the program can finish
    if (lineCount < 100) {
      output += buffer;
      lineCount += output.back() == '\n';
    }
  if (!output.empty() && output.back() != '\n')
    output += "\n";
  // the shell gives 128 + 14 when SIGALRM stops the program
  int status = pclose(pipe);
  if (WEXITSTATUS(status) == 128 + SIGALRM)
    output += "Stopped after 5 seconds\n";
  return output;
}

void Presentation::runCode() {
  const std::string &language = slide->codeLanguage();
  if (language != "cpp" && language != "python")
    return;

  mvaddstr(getmaxy(stdscr) - 1, 0, "Running...");
  clrtoeol();
  refresh();
  const std::string html = "\n        <output>\n" +
                           encodeEntities(run(slide->codeText(), language)) +
                           "        </output>\n    ";

  // the output goes in its own article right after the code's article,
  // replacing the output from the last run if it is there
  size_t codeEnd = document.find("</article>", articleStart(slide)) +
                   10; // 10 = length of "</article>"
  if (slide + 1 != slides.end() && (slide + 1)->showsOutput()) {
    size_t start = document.find('>', articleStart(slide + 1)) + 1;
    document.replace(start, document.find("</article>", start) - start, html);
    *(slide + 1) = Slide(html);
  } else {
    document.insert(codeEnd, "\n    <article>" + html + "</article>");
    slide = slides.insert(slide + 1, Slide(html)) - 1;
  }
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