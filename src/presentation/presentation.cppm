module;
#include <fstream>
#include <string>
#include <vector>

export module presentation;
import slide;

export class Presentation
{
public:
    /// @param file The open presentation file
    Presentation(std::ifstream& file);

    /// @brief  Handle user interaction with the program; this is the main part of the program
    /// This method will call the render() method on the child slides, listen for keyboard input to adjust the slide index, etc.
    void start();

private:
    int currSlide;
    std::vector<Slide> slides;
};