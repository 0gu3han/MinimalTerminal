module;
#include <fstream>
#include <iostream>
#include <string>

module presentation;

Presentation::Presentation(std::ifstream& file)
{
    currSlide = 0;

    std::string html;
    std::string line;
    //turn the file into a single string
    while (std::getline(file, line))
        html += line + "\n";

    //find the beginning of the presentation, and the end of the presentation
    size_t presentationStart = html.find("<main>");
    size_t presentationEnd = html.find("</main>");
    if (presentationStart == std::string::npos || presentationEnd == std::string::npos)
        return;
    //after finding the start of the presentation, find each slide and add it to the slides vector
    size_t beginningOfSlide = html.find("<article>", presentationStart);
    while (beginningOfSlide != std::string::npos && beginningOfSlide < presentationEnd) {
        size_t endOfSlide = html.find("</article>", beginningOfSlide + 9); // 9 = length of "<article>"
        if (endOfSlide == std::string::npos)
            break;

    //extract the content between <article> and </article> and pass it to the Slide constructor, then add the resulting Slide instance to the slides vector
        slides.push_back(Slide(html.substr(beginningOfSlide + 9, endOfSlide - beginningOfSlide - 9)));
        beginningOfSlide = html.find("<article>", endOfSlide + 10); // 10 = length of "</article>"
    }
}
//show the slides one at a time and move between them based on the keys the user presses
void Presentation::start()
{
    if (slides.empty())
        return;
//draw the current slide, then wait for a key and Enter: n = next, p = previous, q = quit
    std::string input;
    while (true) {
        slides[currSlide].render();
        if (!std::getline(std::cin, input) || input == "q")
            break;

        if (input == "n" && currSlide < static_cast<int>(slides.size()) - 1)
            currSlide++;
        else if (input == "p" && currSlide > 0)
            currSlide--;
    }
//clear the last slide before exiting
    std::cout << "\033[2J\033[H";
}
