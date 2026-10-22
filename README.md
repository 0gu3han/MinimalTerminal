# Dynodeck
A lightweight presentation application that runs in the terminal. It targets iTerm2 on macOS.

## What it does
The presenter writes slides by hand in an HTML file and starts Dynodeck with that file. The first slide shows immediately,
and the presenter moves between slides with the arrow keys. If the file is invalid, Dynodeck exits with an error message
and shows nothing.

Slides can contain `<code>` blocks. The presenter can press a key to edit the code during the presentation, then go to
the next slide to see the output. If the code fails to compile or run, the error is shown instead of the output. Edits
can also be thrown away.

The presenter can search for a word or phrase. Dynodeck lists the slides that match and jumps to the one the presenter
picks. If nothing matches, it says so.

When the presentation is over, the presenter can exit and optionally save it as a PDF, including any code edits and
their output.

## Presentation format
The `<main>` element holds the presentation and each `<article>` is one slide. See `sample/slides.html`.

```html
<main>
    <article>
        <h1>Hello</h1>
        <ul>
            <li>First point</li>
        </ul>
    </article>
</main>
```

## Building
Dynodeck is written in C++23 and uses C++ modules (`.cppm` files) instead of header files. Each `.cppm` file declares
`export module <name>;`, its implementation goes in the matching `.cpp` file, and other files use `import <name>;`.

The build setup is still to be determined. For now, CMake needs the Ninja generator to compile modules. Ninja is only
used for compiling and is not part of Dynodeck itself.

```sh
cmake -B build -G Ninja
cmake --build build
./build/dynodeck sample/slides.html
```

## How it works
Parser -> Styler -> Layout -> Renderer -> Presenter

- Parser: reads the HTML file and builds a tree of the supported tags.
- Styler: works out how each element should look, including styles inherited from parent elements.
- Layout: places each element based on the terminal size, with word wrapping and alignment.
- Renderer: draws the slide to the terminal with escape codes, in raw mode so key presses are read one at a time.
- Presenter: tracks the current slide and handles navigation, edit mode, search, and exit.
