# MinimalTerminal
A terminal / command line presentation application

## How It Could Work
We write slides by hand in an HTML file. The app supports only a small set of tags, so the parser stays manageable.
Supported tags include `deck`, `slide`, `h1`, `p`, `ul`, `li`, `b`, `footer`, etc. Anything else can either notify or be ignored.

### Themes (`theme.css`)
Start with a built-in Light and Dark theme. We can create more elaborate ones depending on how much time is left.

> Terminals can't change font family or font size, but they can support:
> - foreground and background color
> - bold, italic, underline, dim
> - text alignment, padding, and margins (measured in character cells)
> Large titles can be drawn with ASCII-art lettering instead.

---

**Parser → Styler → Layout → Renderer → Presenter**

### 1. Parser
Reads through the slides file and picks out the relevant tags, which each become a node. The output is a tree, not a flat list:
a `<p>` can contain a `<b>`, and a `<ul>` contains `<li>`s.

### 2. Styler
Takes each node and works out how it should look. For example, a `<b>` inside a red `<p>` is still red.

### 3. Layout
Takes the styled nodes and decides where each one goes on screen. Using the terminal's current size, it gives each node a position and
size, handles word wrapping and alignment, and runs again when the terminal window is resized.

### 4. Renderer
Turns the nodes into a grid of cells, then writes that grid to the terminal as ANSI escape codes,the special character sequences that set colors and 
move the cursor. The renderer also switches the terminal into raw mode (via `termios`) so key presses can be read one at a time.

### 5. Presenter / Input Loop
Waits for key presses and moves through the deck. The presenter tracks the current slide, and going back is supported.
If slides use incremental reveals like bullet points, the presenter tracks the pair (slide, step) instead of a single slide number.

## Things to Think About
### PDF Export
Maybe have a way to save the deck as a PDF so it can be shared as a handout.

### PowerPoint Converter
Converts an existing `.pptx` file into a `slides.html` that follows the same schema. The file can then be presented or edited like any hand-written deck.
