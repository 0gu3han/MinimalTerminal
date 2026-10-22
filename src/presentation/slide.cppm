module;
#include <string>
#include <vector>

export module slide;

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
    std::vector<std::string> bullets;
};