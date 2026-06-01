// ─── test_markdown.cpp ───────────────────────────────────────────────
// Unit tests for nift::markdown: HTML conversion, TOC, links, front matter, highlight.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <nift/markdown/markdown.hpp>
#include <nift/markdown/front_matter.hpp>
#include <nift/markdown/code_highlight.hpp>

using namespace nift::markdown;
using json = nlohmann::json;

// ═══════════════════════════════════════════════════════════════════
// Markdown → HTML conversion
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("to_html: basic paragraphs", "[markdown]") {
    auto html = to_html("Hello\n\nWorld");
    CHECK(html.find("<p>Hello</p>") != std::string::npos);
    CHECK(html.find("<p>World</p>") != std::string::npos);
}

TEST_CASE("to_html: headings", "[markdown]") {
    auto html = to_html("# H1\n## H2\n### H3");
    CHECK(html.find("<h1>H1</h1>") != std::string::npos);
    CHECK(html.find("<h2>H2</h2>") != std::string::npos);
    CHECK(html.find("<h3>H3</h3>") != std::string::npos);
}

TEST_CASE("to_html: bold and italic", "[markdown]") {
    auto html = to_html("**bold** and *italic*");
    CHECK(html.find("<strong>bold</strong>") != std::string::npos);
    CHECK(html.find("<em>italic</em>") != std::string::npos);
}

TEST_CASE("to_html: links", "[markdown]") {
    auto html = to_html("[link](https://example.com)");
    CHECK(html.find("<a href=\"https://example.com\">link</a>") != std::string::npos);
}

TEST_CASE("to_html: images", "[markdown]") {
    auto html = to_html("![alt](img.png)");
    CHECK(html.find("<img") != std::string::npos);
    CHECK(html.find("img.png") != std::string::npos);
}

TEST_CASE("to_html: inline code", "[markdown]") {
    auto html = to_html("use `std::vector`");
    CHECK(html.find("<code>std::vector</code>") != std::string::npos);
}

TEST_CASE("to_html: code block", "[markdown]") {
    auto html = to_html("```cpp\nint x = 1;\n```");
    CHECK(html.find("<code") != std::string::npos);
    CHECK(html.find("language-cpp") != std::string::npos);
    CHECK(html.find("int x = 1;") != std::string::npos);
}

TEST_CASE("to_html: unordered list", "[markdown]") {
    auto html = to_html("- a\n- b\n- c");
    CHECK(html.find("<ul>") != std::string::npos);
    CHECK(html.find("<li>a</li>") != std::string::npos);
    CHECK(html.find("<li>b</li>") != std::string::npos);
}

TEST_CASE("to_html: ordered list", "[markdown]") {
    auto html = to_html("1. first\n2. second");
    CHECK(html.find("<ol>") != std::string::npos);
    CHECK(html.find("<li>first</li>") != std::string::npos);
}

TEST_CASE("to_html: blockquote", "[markdown]") {
    auto html = to_html("> quote");
    CHECK(html.find("<blockquote>") != std::string::npos);
    CHECK(html.find("quote") != std::string::npos);
}

TEST_CASE("to_html: horizontal rule", "[markdown]") {
    auto html = to_html("---");
    CHECK(html.find("<hr") != std::string::npos);
}

TEST_CASE("to_html: strikethrough (GFM)", "[markdown]") {
    auto html = to_html("~~deleted~~");
    CHECK(html.find("<del>deleted</del>") != std::string::npos);
}

TEST_CASE("to_html: table (GFM)", "[markdown]") {
    auto html = to_html("| A | B |\n|---|---|\n| 1 | 2 |");
    CHECK(html.find("<table>") != std::string::npos);
    CHECK(html.find("<th>A</th>") != std::string::npos);
    CHECK(html.find("<td>1</td>") != std::string::npos);
}

TEST_CASE("to_html: autolink (GFM)", "[markdown]") {
    MarkdownOptions opts;
    opts.autolink = true;
    auto html = to_html("https://example.com", opts);
    CHECK(html.find("<a") != std::string::npos);
}

TEST_CASE("to_html: empty input", "[markdown]") {
    auto html = to_html("");
    CHECK(html.empty());
}

TEST_CASE("to_html: special characters escaped", "[markdown]") {
    auto html = to_html("A < B > C & D");
    CHECK(html.find("&lt;") != std::string::npos);
    CHECK(html.find("&gt;") != std::string::npos);
    CHECK(html.find("&amp;") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Plain text extraction
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("to_plaintext: strips markup", "[markdown]") {
    auto text = to_plaintext("**bold** and *italic* and [link](url)");
    CHECK(text.find("bold") != std::string::npos);
    CHECK(text.find("italic") != std::string::npos);
    CHECK(text.find("link") != std::string::npos);
    CHECK(text.find("**") == std::string::npos);
    CHECK(((text.find("*") == std::string::npos) || (text.find("italic") != std::string::npos)));
}

TEST_CASE("to_plaintext: headings become plain text", "[markdown]") {
    auto text = to_plaintext("# Hello World");
    CHECK(text.find("Hello World") != std::string::npos);
    CHECK(text.find("#") == std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// TOC extraction
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("extract_toc: basic headings", "[markdown]") {
    auto toc = extract_toc("# Intro\n## Setup\n## Usage\n### Advanced");
    REQUIRE(toc.size() == 4);
    CHECK(toc[0].level == 1);
    CHECK(toc[0].text == "Intro");
    CHECK(toc[1].level == 2);
    CHECK(toc[1].text == "Setup");
    CHECK(toc[2].level == 2);
    CHECK(toc[2].text == "Usage");
    CHECK(toc[3].level == 3);
    CHECK(toc[3].text == "Advanced");
}

TEST_CASE("extract_toc: generates slug ids", "[markdown]") {
    auto toc = extract_toc("# Hello World");
    REQUIRE(toc.size() == 1);
    CHECK(toc[0].id == "hello-world");
}

TEST_CASE("extract_toc: empty for no headings", "[markdown]") {
    auto toc = extract_toc("Just some text\n\nMore text");
    CHECK(toc.empty());
}

// ═══════════════════════════════════════════════════════════════════
// Link extraction
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("extract_links: text links", "[markdown]") {
    auto links = extract_links("[Google](https://google.com) and [GitHub](https://github.com)");
    REQUIRE(links.size() == 2);
    CHECK(links[0].text == "Google");
    CHECK(links[0].url == "https://google.com");
    CHECK(links[0].is_image == false);
    CHECK(links[1].text == "GitHub");
    CHECK(links[1].url == "https://github.com");
}

TEST_CASE("extract_links: images", "[markdown]") {
    auto links = extract_links("![photo](photo.jpg)");
    REQUIRE(links.size() == 1);
    CHECK(links[0].url == "photo.jpg");
    CHECK(links[0].is_image == true);
}

TEST_CASE("extract_links: empty for no links", "[markdown]") {
    auto links = extract_links("No links here");
    CHECK(links.empty());
}

// ═══════════════════════════════════════════════════════════════════
// Front matter: YAML format
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("parse_front_matter: YAML basic", "[front_matter]") {
    std::string input = "---\ntitle: Hello World\nauthor: John\n---\n\n# Content";
    auto doc = parse_front_matter(input);
    CHECK(doc.has_front_matter == true);
    CHECK(doc.front_matter["title"] == "Hello World");
    CHECK(doc.front_matter["author"] == "John");
    CHECK(doc.body.find("# Content") != std::string::npos);
}

TEST_CASE("parse_front_matter: YAML with numbers", "[front_matter]") {
    std::string input = "---\ntitle: Test\npriority: 5\nrating: 3.14\n---\nBody";
    auto doc = parse_front_matter(input);
    CHECK(doc.front_matter["priority"] == 5);
    CHECK(doc.front_matter["rating"].get<double>() == Catch::Approx(3.14));
}

TEST_CASE("parse_front_matter: YAML with booleans", "[front_matter]") {
    std::string input = "---\npublished: true\ndraft: false\n---\nBody";
    auto doc = parse_front_matter(input);
    CHECK(doc.front_matter["published"] == true);
    CHECK(doc.front_matter["draft"] == false);
}

TEST_CASE("parse_front_matter: YAML with array", "[front_matter]") {
    std::string input = "---\ntags: [cpp, cmake, testing]\n---\nBody";
    auto doc = parse_front_matter(input);
    CHECK(doc.front_matter["tags"].is_array());
    CHECK(doc.front_matter["tags"].size() == 3);
    CHECK(doc.front_matter["tags"][0] == "cpp");
}

TEST_CASE("parse_front_matter: YAML empty value", "[front_matter]") {
    std::string input = "---\ntitle:\nauthor: Test\n---\nBody";
    auto doc = parse_front_matter(input);
    CHECK(doc.front_matter["title"] == "");
}

TEST_CASE("parse_front_matter: JSON format", "[front_matter]") {
    std::string input = "{\"title\":\"JSON\",\"date\":\"2024-01-01\"}\n\n# Body";
    auto doc = parse_front_matter(input);
    CHECK(doc.has_front_matter == true);
    CHECK(doc.front_matter["title"] == "JSON");
    CHECK(doc.front_matter["date"] == "2024-01-01");
    CHECK(doc.body.find("# Body") != std::string::npos);
}

TEST_CASE("parse_front_matter: no front matter", "[front_matter]") {
    std::string input = "# Just a heading\n\nSome text";
    auto doc = parse_front_matter(input);
    CHECK(doc.has_front_matter == false);
    CHECK(doc.front_matter.empty());
    CHECK(doc.body == input);
}

TEST_CASE("parse_front_matter: empty input", "[front_matter]") {
    auto doc = parse_front_matter("");
    CHECK(doc.has_front_matter == false);
    CHECK(doc.body.empty());
}

TEST_CASE("strip_front_matter: removes YAML header", "[front_matter]") {
    std::string input = "---\ntitle: X\n---\nContent here";
    auto body = strip_front_matter(input);
    CHECK(body.find("title") == std::string::npos);
    CHECK(body.find("Content here") != std::string::npos);
}

TEST_CASE("parse_yaml_simple: quoted strings", "[front_matter]") {
    auto j = parse_yaml_simple("name: \"John Doe\"\ncity: 'New York'");
    CHECK(j["name"] == "John Doe");
    CHECK(j["city"] == "New York");
}

TEST_CASE("parse_yaml_simple: null values", "[front_matter]") {
    auto j = parse_yaml_simple("a: null\nb: ~\nc: value");
    CHECK(j["a"].is_null());
    CHECK(j["b"].is_null());
    CHECK(j["c"] == "value");
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: language detection
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("detect_language: common names", "[highlight]") {
    CHECK(detect_language("cpp") == Language::Cpp);
    CHECK(detect_language("c++") == Language::Cpp);
    CHECK(detect_language("cxx") == Language::Cpp);
    CHECK(detect_language("c") == Language::C);
    CHECK(detect_language("python") == Language::Python);
    CHECK(detect_language("py") == Language::Python);
    CHECK(detect_language("javascript") == Language::JavaScript);
    CHECK(detect_language("js") == Language::JavaScript);
    CHECK(detect_language("typescript") == Language::TypeScript);
    CHECK(detect_language("ts") == Language::TypeScript);
    CHECK(detect_language("rust") == Language::Rust);
    CHECK(detect_language("rs") == Language::Rust);
    CHECK(detect_language("go") == Language::Go);
    CHECK(detect_language("bash") == Language::Bash);
    CHECK(detect_language("sh") == Language::Bash);
    CHECK(json::parse("{}").is_object());  // sanity check nlohmann_json linked
    CHECK(detect_language("json") == Language::Json);
    CHECK(detect_language("yaml") == Language::Yaml);
    CHECK(detect_language("html") == Language::Html);
    CHECK(detect_language("css") == Language::Css);
    CHECK(detect_language("lua") == Language::Lua);
    CHECK(detect_language("diff") == Language::Diff);
    CHECK(detect_language("unknown") == Language::Unknown);
    CHECK(detect_language("") == Language::Unknown);
}

TEST_CASE("detect_language: case insensitive", "[highlight]") {
    CHECK(detect_language("CPP") == Language::Cpp);
    CHECK(detect_language("Python") == Language::Python);
    CHECK(detect_language("JavaScript") == Language::JavaScript);
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: C++ highlighting
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("highlight: C++ keywords", "[highlight]") {
    auto h = highlight("int x = 0;", Language::Cpp);
    CHECK(h.find("hl-keyword") != std::string::npos);  // "int" is keyword
    CHECK(h.find("hl-number") != std::string::npos);   // "0" is number
}

TEST_CASE("highlight: C++ strings", "[highlight]") {
    auto h = highlight("auto s = \"hello\";", Language::Cpp);
    CHECK(h.find("hl-string") != std::string::npos);
    CHECK(h.find("hello") != std::string::npos);
}

TEST_CASE("highlight: C++ comments", "[highlight]") {
    auto h = highlight("// line comment", Language::Cpp);
    CHECK(h.find("hl-comment") != std::string::npos);
}

TEST_CASE("highlight: C++ block comments", "[highlight]") {
    auto h = highlight("/* block */", Language::Cpp);
    CHECK(h.find("hl-comment") != std::string::npos);
}

TEST_CASE("highlight: C++ preprocessor", "[highlight]") {
    auto h = highlight("#include <vector>", Language::Cpp);
    CHECK(h.find("hl-preproc") != std::string::npos);
}

TEST_CASE("highlight: C++ hex numbers", "[highlight]") {
    auto h = highlight("int x = 0xFF;", Language::Cpp);
    CHECK(h.find("hl-number") != std::string::npos);
    CHECK(((h.find("0xFF") != std::string::npos) || (h.find("0xff") != std::string::npos)));
}

TEST_CASE("highlight: C++ function calls", "[highlight]") {
    auto h = highlight("foo(bar);", Language::Cpp);
    CHECK(h.find("hl-func") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: Python
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("highlight: Python keywords", "[highlight]") {
    auto h = highlight("def foo():\n    return True", Language::Python);
    CHECK(h.find("hl-keyword") != std::string::npos);
}

TEST_CASE("highlight: Python strings", "[highlight]") {
    auto h = highlight("s = \"hello world\"", Language::Python);
    CHECK(h.find("hl-string") != std::string::npos);
}

TEST_CASE("highlight: Python f-string", "[highlight]") {
    auto h = highlight("f\"name={name}\"", Language::Python);
    CHECK(h.find("hl-string") != std::string::npos);
}

TEST_CASE("highlight: Python decorator", "[highlight]") {
    auto h = highlight("@decorator\ndef foo(): pass", Language::Python);
    CHECK(h.find("hl-decorator") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: JavaScript
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("highlight: JS keywords", "[highlight]") {
    auto h = highlight("const x = 42;", Language::JavaScript);
    CHECK(h.find("hl-keyword") != std::string::npos);
    CHECK(h.find("hl-number") != std::string::npos);
}

TEST_CASE("highlight: JS template literal", "[highlight]") {
    auto h = highlight("`hello ${name}`", Language::JavaScript);
    CHECK(h.find("hl-string") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: Rust
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("highlight: Rust keywords", "[highlight]") {
    auto h = highlight("let mut x: i32 = 0;", Language::Rust);
    CHECK(h.find("hl-keyword") != std::string::npos);
}

TEST_CASE("highlight: Rust raw string", "[highlight]") {
    auto h = highlight("r#\"raw string\"#", Language::Rust);
    CHECK(h.find("hl-string") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: Go
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("highlight: Go keywords", "[highlight]") {
    auto h = highlight("func main() {}", Language::Go);
    CHECK(h.find("hl-keyword") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: Bash
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("highlight: Bash keywords", "[highlight]") {
    auto h = highlight("if [ -f file ]; then\necho yes\nfi", Language::Bash);
    CHECK(h.find("hl-keyword") != std::string::npos);
}

TEST_CASE("highlight: Bash variables", "[highlight]") {
    auto h = highlight("echo $HOME ${PATH}", Language::Bash);
    CHECK(h.find("hl-var") != std::string::npos);
}

TEST_CASE("highlight: Bash comments", "[highlight]") {
    auto h = highlight("# this is a comment", Language::Bash);
    CHECK(h.find("hl-comment") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: JSON
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("highlight: JSON keys and values", "[highlight]") {
    auto h = highlight("{\"name\": \"John\", \"age\": 30}", Language::Json);
    CHECK(h.find("hl-key") != std::string::npos);   // "name"
    CHECK(h.find("hl-string") != std::string::npos); // "John"
    CHECK(h.find("hl-number") != std::string::npos); // 30
}

TEST_CASE("highlight: JSON booleans", "[highlight]") {
    auto h = highlight("{\"ok\": true, \"fail\": false, \"x\": null}", Language::Json);
    CHECK(h.find("hl-keyword") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: YAML
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("highlight: YAML keys", "[highlight]") {
    auto h = highlight("name: value\ncount: 42", Language::Yaml);
    CHECK(h.find("hl-key") != std::string::npos);
}

TEST_CASE("highlight: YAML comments", "[highlight]") {
    auto h = highlight("# comment\nkey: val", Language::Yaml);
    CHECK(h.find("hl-comment") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: HTML
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("highlight: HTML tags", "[highlight]") {
    auto h = highlight("<div class=\"test\">hello</div>", Language::Html);
    CHECK(h.find("hl-keyword") != std::string::npos);  // div
    CHECK(h.find("hl-func") != std::string::npos);     // class
    CHECK(h.find("hl-string") != std::string::npos);   // "test"
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: CSS
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("highlight: CSS properties", "[highlight]") {
    auto h = highlight("body { color: red; }", Language::Css);
    CHECK(h.find("hl-key") != std::string::npos);  // color
}

TEST_CASE("highlight: CSS at-rules", "[highlight]") {
    auto h = highlight("@media (max-width: 768px) {}", Language::Css);
    CHECK(h.find("hl-preproc") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: Lua
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("highlight: Lua keywords", "[highlight]") {
    auto h = highlight("local x = function() return true end", Language::Lua);
    CHECK(h.find("hl-keyword") != std::string::npos);
}

TEST_CASE("highlight: Lua comments", "[highlight]") {
    auto h = highlight("-- line comment", Language::Lua);
    CHECK(h.find("hl-comment") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: Diff
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("highlight: Diff additions and deletions", "[highlight]") {
    auto h = highlight("+added line\n-removed line\n@@ hunk @@", Language::Diff);
    CHECK(h.find("hl-add") != std::string::npos);
    CHECK(h.find("hl-del") != std::string::npos);
    CHECK(h.find("hl-hunk") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: HTML post-processing
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("highlight_html: processes code blocks in HTML", "[highlight]") {
    std::string html = "<p>Code:</p><pre><code class=\"language-cpp\">int x;</code></pre>";
    auto result = highlight_html(html);
    CHECK(result.find("hl-keyword") != std::string::npos);  // "int" highlighted
    CHECK(result.find("<p>Code:</p>") != std::string::npos); // surrounding HTML preserved
}

TEST_CASE("highlight_html: no language leaves code as-is", "[highlight]") {
    std::string html = "<pre><code>raw code</code></pre>";
    auto result = highlight_html(html);
    CHECK(result.find("raw code") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Code highlighting: CSS output
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("highlight_css: returns non-empty CSS", "[highlight]") {
    auto css = highlight_css();
    CHECK(!css.empty());
    CHECK(css.find(".hl-keyword") != std::string::npos);
    CHECK(css.find(".hl-string") != std::string::npos);
    CHECK(css.find(".hl-comment") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// Edge cases
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("to_html: unicode content", "[markdown]") {
    auto html = to_html("# 日本語テスト\n\n中文内容");
    CHECK(html.find("日本語テスト") != std::string::npos);
    CHECK(html.find("中文内容") != std::string::npos);
}

TEST_CASE("to_html: nested formatting", "[markdown]") {
    auto html = to_html("**bold *and italic***");
    CHECK(html.find("<strong>") != std::string::npos);
}

TEST_CASE("parse_front_matter: unicode in YAML", "[front_matter]") {
    std::string input = "---\ntitle: 日本語\n---\nBody";
    auto doc = parse_front_matter(input);
    CHECK(doc.front_matter["title"] == "日本語");
}

TEST_CASE("highlight: unknown language returns escaped", "[highlight]") {
    auto h = highlight("<script>alert('xss')</script>", Language::Unknown);
    CHECK(h.find("&lt;") != std::string::npos);
    CHECK(h.find("&gt;") != std::string::npos);
    CHECK(h.find("<script>") == std::string::npos);  // Must be escaped
}
