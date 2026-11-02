#include "src/presentation/navigation.hpp"
#include <cassert>
#include <iostream>

int main() {
    Navigation nav(3);

    // Start at the first slide
    assert(nav.currentSlide() == 0);

    // Cannot go backward from the first slide
    assert(!nav.previous());
    assert(nav.currentSlide() == 0);

    // Move to the last slide
    assert(nav.next());
    assert(nav.next());
    assert(nav.currentSlide() == 2);

    // Cannot go beyond the last slide
    assert(!nav.next());
    assert(nav.currentSlide() == 2);

    // Jump to the first slide
    assert(nav.first());
    assert(nav.currentSlide() == 0);

    // Jump to the last slide
    assert(nav.last());
    assert(nav.currentSlide() == 2);

    // Empty presentations must not allow navigation
    Navigation empty(0);
    assert(!empty.next());
    assert(!empty.previous());
    assert(!empty.last());

    std::cout << "All navigation tests passed!\n";
}
