module;
#include <string>
#include <vector>

export module slide;
import styler;

export class Slide
{
public:
    /// @brief 
    /// @param rawHTML The contents of the slide's <article> element 
    Slide(std::string rawHTML);

    /// @brief  Write the content of the slide to the terminal window
    void render();

private:
    std::string heading;
    Color headingColor = Color::Default;
    std::vector<std::string> bullets;
    std::vector<Color> bulletColors;
    std::vector<std::string> codeLines;
};