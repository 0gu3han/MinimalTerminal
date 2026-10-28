module;

export module terminal;

/// @brief The keys Dynodeck reacts to
export enum class Key { Left, Right, Quit, Other };

/// @brief Turn raw mode on, so each key press is read right away without being
/// shown, or off to put the terminal back to normal
export void rawMode(bool on);

/// @brief Wait for a key press and return which key it was
export Key readKey();
