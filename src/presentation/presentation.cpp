module;
#include <fstream>
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
