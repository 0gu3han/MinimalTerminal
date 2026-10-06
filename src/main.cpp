#include <print>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>


// A Node is one piece of the slide file.
// kind  → what it is: "Deck", "Slide", "H1", "P", "B", "Text"
// text  → only filled in for Text nodes
// children → what is nested inside this node

struct Node {
    std::string       kind;
    std::string       text;
    std::vector<Node> children;
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
// Print the tree so we can see it in the terminal.
// prefix + connector builds the ├─ / └─ lines.
// ─────────────────────────────────────────────
void print_tree(Node node, std::string prefix, bool last) {
    std::string connector = last ? "└─ " : "├─ ";
    std::cout << prefix << connector << node.kind;
    if (node.kind == "Text")
        std::cout << " \"" << node.text << "\"";
    std::cout << "\n";

    std::string next_prefix = prefix + (last ? "   " : "│  ");
    for (int j = 0; j < (int)node.children.size(); j++)
        print_tree(node.children[j], next_prefix, j + 1 == (int)node.children.size());
}

// ─────────────────────────────────────────────
// main: read the file, parse it, print the tree
// ─────────────────────────────────────────────

auto main(int argc, char* argv[]) -> int {
    const char* path = argc > 1 ? argv[1] : "slides.html";

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

    // print root without a connector, then its children
    std::cout << root.kind << "\n";
    for (int j = 0; j < (int)root.children.size(); j++)
        print_tree(root.children[j], "", j + 1 == (int)root.children.size());

    return 0;
}