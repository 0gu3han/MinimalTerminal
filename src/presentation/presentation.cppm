module;
#include <fstream>
#include <string>
#include <vector>

export module presentation;
import slide;

export class Presentation {
public:
  /// @param file The open presentation file
  /// @param fileName Where to save edited code
  Presentation(std::ifstream &file, const std::string &fileName);

  /// @brief  Handle user interaction with the program; this is the main part of
  /// the program This method will call the render() method on the child slides,
  /// listen for keyboard input to adjust the slide index, etc.
  void start();

private:
  /// Whether or not the presentation is currently hidden
  bool is_hidden = false;
  std::vector<Slide>::iterator slide;
  std::vector<Slide> slides;
  /// The whole file, so edited code can be put back into it
  std::string document;
  std::string fileName;

  /// Make the slides from the document, keeping the same slide current. Call
  /// it after changing the document, since the slides remember where their
  /// code is in it.
  void load();
  void render();
  /// Where the slide's <article> starts in the document
  size_t articleStart(std::vector<Slide>::iterator article);
  /// Write the current slide's code into the file
  void save();
  /// Run the current slide's code and show what it printed on the next slide
  void runCode(const std::string &code);
  /// Edit the current slide's code with the code on top and what it printed
  /// below. Ctrl-R runs it, Esc keeps the changes and Ctrl-X throws them away.
  void editCode();
};