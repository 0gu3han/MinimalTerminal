#include "slide.hpp"
#include <string>
#include <vector>

class Presentation
{
public:
    /// @brief Extracts each contained <article> element into a Slide class, then adds those to itself.
    /// @param rawHTML The raw content of the <main> element of the presentation file
    Presentation(std::string rawHTML);

    // Look for content between <article> and </article>
    // Take that content, pass it to Slide() constructor
    // Add the resulting class instance to its slides vector

    /// @brief  Handle user interaction with the program; this is the main part of the program
    /// This method will call the render() method on the child slides, listen for keyboard input to adjust the slide index, etc.
    void start();

private:
    int currSlide;
    std::vector<Slide> slides;
};