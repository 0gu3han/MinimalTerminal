# Dynodeck
A lightweight presentation application that runs in the terminal. It targets iTerm2 on macOS.

## What it does
The presenter writes slides by hand in an HTML file and starts Dynodeck with that file. The first slide shows immediately,
and the presenter moves between slides with the arrow keys. If the file is invalid, Dynodeck exits with an error message
and shows nothing.

Slides can contain `<code>` blocks. The presenter can press `i` to edit the code during the presentation. The screen
splits, with the code on top and what it printed below, and `Ctrl-R` runs the code without leaving the editor. `Esc`
keeps the changes, saves them to the file and puts the output on the next slide. `Ctrl-X` throws the changes away. If
the code fails to compile or run, the error is shown instead of the output.

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

A slide picks its layout with `class` on the `<article>`:

- **Title and content** (no class): an `<h1>` with bullets and code blocks below it.
- **Two content** (`class="two-content"`): an `<h1>` with two `<section>`s side by side, each with its own bullets and
  code blocks.
- **Subsection** (`class="subsection"`): an `<h1>` in the middle of the screen with an optional `<h2>` under it.

```html
<article class="two-content">
    <h1>Before and after</h1>
    <section><ul><li>Left side</li></ul></section>
    <section><ul><li>Right side</li></ul></section>
</article>
```

Everything is fitted to the terminal and laid out again when it is resized. Long bullets wrap, code lines that are too
wide end with `…`, and `...` shows that a slide has more than fits.

An `<h1>` or `<li>` with `style="text-align: center"` is centered, the same CSS a browser uses. Dynodeck keeps a centered
bullet's marker next to its text, so put `style="list-style-position: inside"` on the `<ul>` for browsers to match.

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

Run the tests with `ctest --test-dir build`.

## How it works
Parser -> Styler -> Layout -> Renderer -> Presenter

- Parser: reads the HTML file and builds a tree of the supported tags.
- Styler: works out how each element should look, including styles inherited from parent elements.
- Layout: works out the area of the screen for each part of the slide and the rows inside it, with word wrapping and
  cutting off. Each area is drawn in its own curses window. The layout logic does not use curses, so it is unit tested.
- Renderer: draws the slide to the terminal with escape codes, in raw mode so key presses are read one at a time.
- Presenter: tracks the current slide and handles navigation, edit mode, search, and exit.
