module;
#include <iostream>
#include <termios.h>

module terminal;

//the terminal settings from before raw mode was turned on, so they can be put back
termios oldSettings;

//turn off line buffering and echo, or put the old settings back
void rawMode(bool on)
{
    if (on) {
        tcgetattr(0, &oldSettings); // 0 = standard input
        termios rawSettings = oldSettings;
        rawSettings.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(0, TCSANOW, &rawSettings);
    }
    else
        tcsetattr(0, TCSANOW, &oldSettings);
}
//read one key press: q quits, and arrow keys send 3 characters: escape, [, then C for right or D for left
Key readKey()
{
    char key;
    if (!std::cin.get(key) || key == 'q')
        return Key::Quit;

    if (key == '\033' && std::cin.get(key) && key == '[' && std::cin.get(key)) {
        if (key == 'C')
            return Key::Right;
        if (key == 'D')
            return Key::Left;
    }
    return Key::Other;
}
