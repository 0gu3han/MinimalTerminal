module;
#include <iostream>
#include <string>
#include <vector>

export module testing;

// module linkage rather than static, so the exported template can use them
int failures = 0;

std::string show(const std::string &text) { return "\"" + text + "\""; }
std::string show(const char *text) { return show(std::string(text)); }
std::string show(bool value) { return value ? "true" : "false"; }
std::string show(int number) { return std::to_string(number); }
std::string show(size_t number) { return std::to_string(number); }

std::string show(const std::vector<std::string> &lines) {
  std::string text = "{";
  for (const std::string &line : lines)
    text += " " + show(line);
  return text + " }";
}

/// Report a failure if actual is not expected
export template <typename T>
void expect(const T &actual, const T &expected, const std::string &what) {
  if (actual == expected)
    return;
  std::cerr << "FAIL " << what << "\n  expected " << show(expected)
            << "\n  got      " << show(actual) << "\n";
  ++failures;
}

/// Print the result and give the exit code for main
export int finish(const std::string &name) {
  if (failures == 0)
    std::cout << "All " << name << " tests passed\n";
  else
    std::cerr << failures << " " << name << " checks failed\n";
  return failures == 0 ? 0 : 1;
}
