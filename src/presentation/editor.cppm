module;
#include <ncurses.h>
#include <string>
#include <vector>

export module editor;

/// Edits a copy of a slide's code, so throwing the edits away is just not
/// keeping the copy
export class Editor {
public:
  /// @param lines The code, one string per line
  /// @param language "cpp", "python" or anything else for no highlighting
  Editor(std::vector<std::string> lines, std::string language);

  /// Apply one key: arrows, Home, End, Backspace, Delete, Enter, Tab or a
  /// printable character. Other keys do nothing.
  void press(int key);

  /// Draw the code with line numbers, scrolled so the cursor shows, and put
  /// the cursor in place
  void draw(WINDOW *window);

  const std::vector<std::string> &lines() const { return code; }

  /// The code as plain text, to run it
  std::string text() const;

  size_t row() const { return cursorRow; }
  size_t column() const { return cursorColumn; }

private:
  std::vector<std::string> code;
  std::string language;
  size_t cursorRow = 0;
  size_t cursorColumn = 0;
  /// The first line and column that show in the window
  size_t topRow = 0;
  size_t leftColumn = 0;
};
