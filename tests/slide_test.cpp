#include <stdexcept>
#include <string>
#include <vector>

import layout;
import slide;
import testing;

using Lines = std::vector<std::string>;

// the part of the file the slide would replace when saving its code
static std::string savedPart(const std::string &file, const Slide &slide) {
  return file.substr(slide.codeStart(), slide.codeEnd() - slide.codeStart());
}

static void testTitleAndContent() {
  const std::string article = R"(
    <h1>Code</h1>
    <ul><li>A point</li></ul>
    <pre><code class="language-cpp">
int x = 1 &lt; 2;
    </code></pre>
    <pre><code>second block</code></pre>
)";
  const std::string file = "<main><article>" + article + "</article></main>";
  const Slide slide(article, file.find(article));
  expect(slide.hasCode(), true, "finds the code");
  expect(slide.codeLanguage(), std::string("cpp"), "reads the language");
  expect(slide.codeLines(), Lines{"int x = 1 < 2;"},
         "the first code block is the one to edit, decoded");
  expect(savedPart(file, slide), std::string("\nint x = 1 &lt; 2;\n    "),
         "saves over the first block");
  expect(slide.showsOutput(), false, "not an output slide");
}

static void testTwoContent() {
  const std::string article = R"(
    <h1>Side by side</h1>
    <section><ul><li>Left</li></ul></section>
    <section>
      <pre><code class="language-python">print("right")</code></pre>
    </section>
)";
  const std::string file =
      "<main><article class=\"two-content\">" + article + "</article></main>";
  const Slide slide(article, file.find(article), LayoutType::TwoContent);
  expect(slide.codeLanguage(), std::string("python"), "code in a section");
  expect(slide.codeLines(), Lines{"print(\"right\")"}, "its lines");
  expect(savedPart(file, slide), std::string("print(\"right\")"),
         "code in a section saves to the right place in the file");
}

static void testSetCode() {
  Slide slide("<pre><code>old</code></pre>");
  slide.setCode({"new", "a < b"});
  expect(slide.codeLines(), Lines{"new", "a < b"}, "edited lines are kept");
  expect(slide.codeText(), std::string("new\na < b\n"), "text to run");
  expect(slide.codeHTML(), std::string("\nnew\na &lt; b"),
         "HTML to save is encoded");
}

static void testOutputAndNoCode() {
  const Slide output("\n        <output>\nhello\n        </output>\n    ");
  expect(output.showsOutput(), true, "output slide");
  expect(output.hasCode(), false, "output is not code to edit");

  const Slide plain("<h1>Just a title</h1>", 0, LayoutType::Subsection);
  expect(plain.hasCode(), false, "no code");
  expect(plain.codeLines().empty(), true, "no code lines");
}

// whether reading the HTML throws, rather than hanging or reading past the end
static bool rejects(const std::string &html) {
  try {
    findAll(html, {"h1"});
  } catch (const std::runtime_error &) {
    return true;
  }
  return false;
}

static void testMalformed() {
  expect(rejects("<h1>never closed"), true, "missing closing tag");
  expect(rejects("text <h1"), true, "missing >");
  expect(rejects("<h1>closed</h1>"), false, "well formed");
}

auto main() -> int {
  testTitleAndContent();
  testTwoContent();
  testSetCode();
  testOutputAndNoCode();
  testMalformed();
  return finish("slide");
}
