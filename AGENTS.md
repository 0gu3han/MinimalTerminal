Dynodeck is a presentation application written in C++.
The Dynodeck codebase adheres to modern C++ standards, including C++ modules and smart pointers.
Modern standard library feautures are strongly preferred.
The project's build system is managed by CMake, which uses Ninja. Clang is the recommended compiler.
The project is primarily designed for usage in iTerm2.
The Dynodeck executable targets an HTML file that uses a custom subset of semantic elements to guarantee accessibility and portability.
Dynodeck rejects malformed HTML. Input files are expected to be structured correctly, but errors may be thrown if they are not.
Dynodeck is meant to be lightweight and written with clean code that prioritizes architectural clarity over shortcuts or repetition.
Overhead should be limited, and no external libraries should be used without explicit approval.