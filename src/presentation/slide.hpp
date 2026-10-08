#include <string>
#include <vector>

class Slide
{
public:
    /// @brief 
    /// @param rawHTML The contents of the slide's <article> element 
    Slide(std::string rawHTML);

    // Looks for <h1>...</h1>
    // Adds that content to its heading attribute
    // Looks for <ul>...</ul>
    // Within <ul>, looks for <li>...<li>
    // Adds content between <li> tags to its bullets vector

    /// @brief  Write the content of the slide to the terminal window
    void render();

private:
    std::string heading;
    std::vector<std::string> bullets;
};