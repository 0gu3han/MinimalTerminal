module;
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <ncurses.h>
#include <poll.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

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

struct TemporaryDirectory {
  std::filesystem::path path;

  ~TemporaryDirectory() {
    std::error_code error;
    std::filesystem::remove_all(path, error);
  }
};

// Write and run the code in an isolated directory, returning its output.
static std::string run(const std::string &code, const std::string &language) {
  TemporaryDirectory temporary;
  std::string directoryTemplate =
      (std::filesystem::temp_directory_path() / "dynodeck-XXXXXX").string();
  std::vector<char> directoryName(directoryTemplate.begin(),
                                  directoryTemplate.end());
  directoryName.push_back('\0');
  char *createdDirectory = mkdtemp(directoryName.data());
  if (!createdDirectory)
    return "Could not create temporary directory\n";
  temporary.path = createdDirectory;

  const std::string source = language == "python" ? "main.py" : "main.cpp";
  std::ofstream sourceFile(temporary.path / source);
  if (!sourceFile)
    return "Could not write source file\n";
  sourceFile << code;
  sourceFile.close();
  if (!sourceFile)
    return "Could not write source file\n";

  int outputPipe[2];
  if (pipe(outputPipe) == -1)
    return "Could not start code\n";

  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(5);
  const pid_t child = fork();
  if (child == -1) {
    close(outputPipe[0]);
    close(outputPipe[1]);
    return "Could not start code\n";
  }
  if (child == 0) {
    close(outputPipe[0]);
    if (setpgid(0, 0) == -1)
      _exit(127);
    if (dup2(outputPipe[1], STDOUT_FILENO) == -1 ||
        dup2(outputPipe[1], STDERR_FILENO) == -1)
      _exit(127);
    close(outputPipe[1]);
    const int input = open("/dev/null", O_RDONLY);
    if (input == -1 || dup2(input, STDIN_FILENO) == -1)
      _exit(127);
    close(input);
    if (chdir(temporary.path.c_str()) == -1)
      _exit(127);

    const char *command = language == "python"
                              ? "python3 -u main.py"
                              : "c++ -std=c++23 main.cpp -o main && ./main";
    execl("/bin/sh", "sh", "-c", command, static_cast<char *>(nullptr));
    _exit(127);
  }

  close(outputPipe[1]);
  setpgid(child, child);
  const int flags = fcntl(outputPipe[0], F_GETFL, 0);
  if (flags == -1 || fcntl(outputPipe[0], F_SETFL, flags | O_NONBLOCK) == -1) {
    kill(-child, SIGKILL);
    close(outputPipe[0]);
    while (waitpid(child, nullptr, 0) == -1 && errno == EINTR) {
    }
    return "Could not read code output\n";
  }

  constexpr size_t maxOutputBytes = 64 * 1024;
  constexpr size_t maxCaptureBytes = maxOutputBytes - 128;
  constexpr size_t maxOutputLines = 100;
  std::string output;
  size_t lineCount = 0;
  bool truncated = false;
  auto appendOutput = [&](const char *buffer, size_t length) {
    for (size_t i = 0; i < length; ++i) {
      if (output.size() >= maxCaptureBytes || lineCount >= maxOutputLines) {
        truncated = true;
        continue;
      }
      if (buffer[i] == '\0') {
        if (output.size() + 2 > maxCaptureBytes) {
          truncated = true;
          continue;
        }
        output += "\\0";
      } else {
        output += buffer[i];
        lineCount += buffer[i] == '\n';
      }
    }
  };

  bool timedOut = false;
  bool finishedReading = false;
  bool childExited = false;
  bool readFailed = false;
  int status = 0;
  while (!finishedReading || !childExited) {
    if (!childExited) {
      const pid_t result = waitpid(child, &status, WNOHANG);
      if (result == child)
        childExited = true;
      else if (result == -1 && errno != EINTR) {
        readFailed = true;
        break;
      }
    }

    const auto now = std::chrono::steady_clock::now();
    if (now >= deadline) {
      timedOut = true;
      kill(-child, SIGKILL);
      break;
    }

    const auto remaining =
        std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
    const int waitMilliseconds =
        std::min(static_cast<int>(remaining.count()), 50);
    pollfd descriptor{outputPipe[0],
                      static_cast<short>(finishedReading ? 0 : POLLIN), 0};
    int result = poll(finishedReading ? nullptr : &descriptor,
                      finishedReading ? 0 : 1, waitMilliseconds);
    if (result == -1 && errno == EINTR)
      continue;
    if (result == -1) {
      readFailed = true;
      kill(-child, SIGKILL);
      break;
    }
    if (result == 0)
      continue;

    char buffer[4096];
    while (true) {
      const ssize_t count = read(outputPipe[0], buffer, sizeof buffer);
      if (count > 0) {
        appendOutput(buffer, static_cast<size_t>(count));
      } else if (count == 0) {
        finishedReading = true;
        break;
      } else if (errno == EINTR) {
        continue;
      } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        finishedReading = true;
        break;
      } else {
        break;
      }
    }
  }

  if (timedOut || readFailed)
    kill(-child, SIGKILL);
  if (timedOut || readFailed) {
    char buffer[4096];
    while (true) {
      const ssize_t count = read(outputPipe[0], buffer, sizeof buffer);
      if (count > 0)
        appendOutput(buffer, static_cast<size_t>(count));
      else if (count == -1 && errno == EINTR)
        continue;
      else
        break;
    }
  }
  close(outputPipe[0]);

  if (!childExited) {
    while (waitpid(child, &status, 0) == -1 && errno == EINTR) {
    }
  }

  if (truncated) {
    if (!output.empty() && output.back() != '\n')
      output += '\n';
    output += "Output truncated\n";
  } else if (!output.empty() && output.back() != '\n') {
    output += '\n';
  }
  if (timedOut) {
    output += "Stopped after 5 seconds\n";
  } else if (readFailed) {
    output += "Could not read code output\n";
  } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
    output += "Exited with status " + std::to_string(WEXITSTATUS(status)) +
              "\n";
  } else if (WIFSIGNALED(status)) {
    output +=
        "Terminated by signal " + std::to_string(WTERMSIG(status)) + "\n";
  }
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