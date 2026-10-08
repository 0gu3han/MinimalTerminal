#include <iostream>
#include <fstream>
#include <string>
#include <vector>


// A Node is one piece of the slide file.
// kind  → what it is: "Deck", "Slide", "H1", "P", "B", "Text"
// text  → only filled in for Text nodes
// children → what is nested inside this node

// The Style is how a Node should look on screen
//   color -> text color name: default, red, green, blue
//   bold/italic/underline -> on or off
//   align -> left, center, right
// only inherited properties flow downstream 
// from parent to child. Alignment belongs to the block itself

struct Style {
    std::string color     = "default";
    bool        bold      = false;
    bool        italic    = false;
    bool        underline = false;
    std::string align     = "left";
};

struct Node {
    std::string       kind;
    std::string       text;
    std::vector<Node> children;
    Style             style;      // filled in by style()
};


// Parse the source string and return a tree.
//
// How it works:
//   We scan the text from left to right.
//   We keep a "stack" — a list of nodes that are
//   currently open (we have seen <tag> but not </tag> yet).
//
//   When we see <tag>  → open a new node, push it on the stack.
//   When we see </tag> → the top node is finished; pop it and
//                        add it as a child of the node below it.
//   Plain text         → add a Text child to whatever is on top.

Node parse(const std::string& src) {
    std::vector<Node> stack;

    int i = 0;
    while (i < (int)src.size()) {

        if (src[i] == '<') {
            // find the closing '>'
            int end = src.find('>', i);
            if (end == (int)std::string::npos) break;

            std::string tag = src.substr(i + 1, end - i - 1);
            i = end + 1;

            // is this a closing tag like </h1> ?
            bool closing = !tag.empty() && tag[0] == '/';
            if (closing) tag = tag.substr(1);

            // lowercase the tag name so <H1> and <h1> both work
            for (char& c : tag) c = tolower(c);

            // trim spaces (handles <deck > with trailing space)
            while (!tag.empty() && tag.back()  == ' ') tag.pop_back();
            while (!tag.empty() && tag.front() == ' ') tag.erase(tag.begin());

            // skip tags we don't know about
            if (tag != "section" && tag != "slide" && tag != "h1" &&
                tag != "p"       && tag != "b"     && tag != "i")
                continue;

            if (!closing) {
                // open tag: put a new empty node on the stack
                Node n;
                if      (tag == "section") n.kind = "Section";
                else if (tag == "slide") n.kind = "Slide";
                else if (tag == "h1")    n.kind = "H1";
                else if (tag == "p")     n.kind = "P";
                else if (tag == "b")     n.kind = "B";
                else if (tag == "i")     n.kind = "I";
                stack.push_back(n);

            } else {
                // close tag
                if (stack.size() >= 2) {
                    // pop the top node and add it to its parent
                    Node finished = stack.back();
                    stack.pop_back();
                    stack.back().children.push_back(finished);
                } else {
                    // closed the root <deck> — we are done
                    break;
                }
            }

        } else {
            // plain text: collect characters until the next '<'
            int end = src.find('<', i);
            if (end == (int)std::string::npos) end = src.size();

            std::string text = src.substr(i, end - i);
            i = end;

            // skip text that is only spaces / newlines
            bool only_spaces = true;
            for (char c : text)
                if (c != ' ' && c != '\n' && c != '\r' && c != '\t')
                    { only_spaces = false; break; }

            if (!only_spaces && !stack.empty()) {
                Node n;
                n.kind = "Text";
                n.text = text;
                stack.back().children.push_back(n);
            }
        }
    }

    if (stack.empty()) return Node();      // file was empty / bad
    return stack[0];                       // stack[0] is the root Section
}

// ─────────────────────────────────────────────
// Styler: work out how each node should look.
//
// How it works:
//   Walk the tree from top-down. Each node starts with a copy of its
//   parent's style (so a <b> inside a red <p> is still red), then
//   its own kind adds on top of that (so <b> turns bold on).
//   Text nodes have no look of their own — they just take whatever
//   their parent has.
// ─────────────────────────────────────────────

void style(Node& node, const Style& parent) {
    // start from the parent's inheritable properties
    Style s;
    s.color     = parent.color;
    s.bold      = parent.bold;
    s.italic    = parent.italic;
    s.underline = parent.underline;

    // apply a node's own defaults
    if      (node.kind == "H1") { s.bold = true; s.align = "center"; }
    else if (node.kind == "B")  { s.bold = true; }
    else if (node.kind == "I")  { s.italic = true; }

    node.style = s;

    // pass the style down to the children
    for (Node& child : node.children)
        style(child, s);
}

// ─────────────────────────────────────────────
// Print the tree so we can see it in the terminal.
// prefix + connector builds the ├─ / └─ lines.
// ─────────────────────────────────────────────
void print_tree(Node node, std::string prefix, bool last) {
    std::string connector = last ? "└─ " : "├─ ";
    std::cout << prefix << connector << node.kind;
    if (node.kind == "Text")
        std::cout << " \"" << node.text << "\"";

    // show the style next to the node
    std::cout << "  [" << node.style.color;
    if (node.style.bold)      std::cout << " bold";
    if (node.style.italic)    std::cout << " italic";
    if (node.style.underline) std::cout << " underline";
    std::cout << " " << node.style.align << "]";
    std::cout << "\n";

    std::string next_prefix = prefix + (last ? "   " : "│  ");
    for (int j = 0; j < (int)node.children.size(); j++)
        print_tree(node.children[j], next_prefix, j + 1 == (int)node.children.size());
}

// ─────────────────────────────────────────────
// main: read the file, parse it, style it, print the tree
// ─────────────────────────────────────────────

auto main(int argc, char* argv[]) -> int {
    // For now, just expect execution from the project root
    const char* path = argc > 1 ? argv[1] : "sample/slides.html";

    std::ifstream file(path);
    if (!file) {
        std::cerr << "Cannot open: " << path << "\n";
        return 1;
    }

    // read the whole file into one string
    std::string src = "";
    std::string line;
    while (std::getline(file, line))
        src += line + "\n";

    Node root = parse(src);
    style(root, Style());

    // print root without a connector, then its children
    std::cout << root.kind << "\n";
    for (int j = 0; j < (int)root.children.size(); j++)
        print_tree(root.children[j], "", j + 1 == (int)root.children.size());

    return 0;
}