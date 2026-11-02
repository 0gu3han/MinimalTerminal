
#pragma once

#include <cstddef>

class Navigation {
public:
  explicit Navigation(std::size_t totalSlides)
      : total(totalSlides), current(0) {}

  std::size_t currentSlide() const {
    return current;
  }

  bool next() {
    if (total == 0 || current >= total - 1)
      return false;

    ++current;
    return true;
  }

  bool previous() {
    if (current == 0)
      return false;

    --current;
    return true;
  }

  bool first() {
    if (current == 0)
      return false;

    current = 0;
    return true;
  }

  bool last() {
    if (total == 0 || current == total - 1)
      return false;

    current = total - 1;
    return true;
  }

private:
  std::size_t total;
  std::size_t current;
};
