// ─── nift/markdown/code_highlight.cpp ────────────────────────────────
// Code syntax highlighting: regex-free, fast, class-based.
// Language-specific tokenizers for C++, Python, JS, Rust, Go, Bash, JSON, etc.

#include "nift/markdown/code_highlight.hpp"

#include <algorithm>
#include <sstream>

namespace nift::markdown {

Language detect_language(std::string_view info) {
  // Normalize: lowercase, take first word.
  std::string lower;
  for (char c : info) {
    if (c == ' ' || c == '\t')
      break;
    lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }

  if (lower == "cpp" || lower == "c++" || lower == "cxx" || lower == "cc")
    return Language::Cpp;
  if (lower == "c")
    return Language::C;
  if (lower == "python" || lower == "py")
    return Language::Python;
  if (lower == "javascript" || lower == "js")
    return Language::JavaScript;
  if (lower == "typescript" || lower == "ts")
    return Language::TypeScript;
  if (lower == "rust" || lower == "rs")
    return Language::Rust;
  if (lower == "go")
    return Language::Go;
  if (lower == "bash" || lower == "sh" || lower == "shell" || lower == "zsh")
    return Language::Bash;
  if (lower == "json")
    return Language::Json;
  if (lower == "yaml" || lower == "yml")
    return Language::Yaml;
  if (lower == "html" || lower == "htm")
    return Language::Html;
  if (lower == "css")
    return Language::Css;
  if (lower == "lua")
    return Language::Lua;
  if (lower == "diff" || lower == "patch")
    return Language::Diff;
  return Language::Unknown;
}

namespace {

// HTML-escape a character.
void escape_char(char c, std::string& out) {
  switch (c) {
    case '&':
      out += "&amp;";
      break;
    case '<':
      out += "&lt;";
      break;
    case '>':
      out += "&gt;";
      break;
    case '"':
      out += "&quot;";
      break;
    default:
      out += c;
      break;
  }
}

// HTML-escape a string.
std::string escape(std::string_view s) {
  std::string out;
  out.reserve(s.size() + 16);
  for (char c : s)
    escape_char(c, out);
  return out;
}

// Wrap text in a span with class.
std::string span(const char* cls, std::string_view text) {
  return std::string("<span class=\"") + cls + "\">" + escape(text) + "</span>";
}

// Check if character is alphanumeric or underscore.
bool is_alnum(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
         c == '_';
}

// Check if character is a digit.
bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

// C/C++ keywords.
bool is_cpp_keyword(std::string_view word) {
  static const char* keywords[] = {
      "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool",
      "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class",
      "compl", "concept", "const", "consteval", "constexpr", "constinit", "const_cast",
      "continue", "co_await", "co_return", "co_yield", "decltype", "default", "delete",
      "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern",
      "false", "float", "for", "friend", "goto", "if", "inline", "int", "long",
      "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator",
      "or", "or_eq", "private", "protected", "public", "register", "reinterpret_cast",
      "requires", "return", "short", "signed", "sizeof", "static", "static_assert",
      "static_cast", "struct", "switch", "template", "this", "thread_local", "throw",
      "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using",
      "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq",
      // Common types/macros
      "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t",
      "uint64_t", "size_t", "ptrdiff_t", "nullptr_t", "string", "string_view", "vector",
      "map", "set", "pair", "optional", "variant", "unique_ptr", "shared_ptr", "array",
      "span", "std", "fmt", "nift", nullptr};
  for (const char** kw = keywords; *kw; ++kw) {
    if (word == *kw)
      return true;
  }
  return false;
}

// Python keywords.
bool is_python_keyword(std::string_view word) {
  static const char* keywords[] = {
      "and",      "as",       "assert", "async", "await",  "break",  "class",
      "continue", "def",      "del",    "elif",  "else",   "except", "finally",
      "for",      "from",     "global", "if",    "import", "in",     "is",
      "lambda",   "nonlocal", "not",    "or",    "pass",   "raise",  "return",
      "try",      "while",    "with",   "yield", "True",   "False",  "None",
      "self",     "cls",      nullptr};
  for (const char** kw = keywords; *kw; ++kw) {
    if (word == *kw)
      return true;
  }
  return false;
}

// JavaScript/TypeScript keywords.
bool is_js_keyword(std::string_view word) {
  static const char* keywords[] = {
      "abstract",   "arguments", "async",     "await",      "boolean",   "break",
      "byte",       "case",      "catch",     "char",       "class",     "const",
      "continue",   "debugger",  "default",   "delete",     "do",        "double",
      "else",       "enum",      "export",    "extends",    "false",     "final",
      "finally",    "float",     "for",       "function",   "goto",      "if",
      "implements", "import",    "in",        "instanceof", "int",       "interface",
      "let",        "long",      "native",    "new",        "null",      "of",
      "package",    "private",   "protected", "public",     "return",    "short",
      "static",     "super",     "switch",    "this",       "throw",     "true",
      "try",        "typeof",    "undefined", "var",        "void",      "volatile",
      "while",      "with",      "yield",     "type",       "namespace", "module",
      "declare",    "readonly",  "any",       "never",      "unknown",   "string",
      "number",     "boolean",   nullptr};
  for (const char** kw = keywords; *kw; ++kw) {
    if (word == *kw)
      return true;
  }
  return false;
}

// Rust keywords.
bool is_rust_keyword(std::string_view word) {
  static const char* keywords[] = {
      "as",      "async",  "await",    "break",    "const",  "continue", "crate",
      "dyn",     "else",   "enum",     "extern",   "false",  "fn",       "for",
      "if",      "impl",   "in",       "let",      "loop",   "match",    "mod",
      "move",    "mut",    "pub",      "ref",      "return", "self",     "Self",
      "static",  "struct", "super",    "trait",    "true",   "type",     "unsafe",
      "use",     "where",  "while",    "abstract", "become", "box",      "do",
      "final",   "macro",  "override", "priv",     "try",    "typeof",   "unsized",
      "virtual", "yield",  "bool",     "char",     "f32",    "f64",      "i8",
      "i16",     "i32",    "i64",      "i128",     "isize",  "str",      "u8",
      "u16",     "u32",    "u64",      "u128",     "usize",  "Option",   "Result",
      "Some",    "None",   "Ok",       "Err",      "Vec",    "String",   "Box",
      "Rc",      "Arc",    "Cell",     "RefCell",  "Mutex",  nullptr};
  for (const char** kw = keywords; *kw; ++kw) {
    if (word == *kw)
      return true;
  }
  return false;
}

// Go keywords.
bool is_go_keyword(std::string_view word) {
  static const char* keywords[] = {
      "break",   "case",      "chan",        "const",     "continue", "default",
      "defer",   "else",      "fallthrough", "for",       "func",     "go",
      "goto",    "if",        "import",      "interface", "map",      "package",
      "range",   "return",    "select",      "struct",    "switch",   "type",
      "var",     "true",      "false",       "nil",       "iota",     "bool",
      "byte",    "complex64", "complex128",  "error",     "float32",  "float64",
      "int",     "int8",      "int16",       "int32",     "int64",    "rune",
      "string",  "uint",      "uint8",       "uint16",    "uint32",   "uint64",
      "uintptr", "append",    "cap",         "close",     "complex",  "copy",
      "delete",  "imag",      "len",         "make",      "new",      "panic",
      "print",   "println",   "real",        "recover",   nullptr};
  for (const char** kw = keywords; *kw; ++kw) {
    if (word == *kw)
      return true;
  }
  return false;
}

// Check if word is a keyword for the given language.
bool is_keyword(std::string_view word, Language lang) {
  switch (lang) {
    case Language::Cpp:
    case Language::C:
      return is_cpp_keyword(word);
    case Language::Python:
      return is_python_keyword(word);
    case Language::JavaScript:
    case Language::TypeScript:
      return is_js_keyword(word);
    case Language::Rust:
      return is_rust_keyword(word);
    case Language::Go:
      return is_go_keyword(word);
    default:
      return false;
  }
}

// Tokenize and highlight code for C-family languages (C, C++, JS, TS, Rust, Go).
std::string highlight_c_family(std::string_view code, Language lang) {
  std::string out;
  out.reserve(code.size() * 2);
  size_t i = 0;
  size_t len = code.size();

  while (i < len) {
    char c = code[i];

    // Single-line comment.
    if (c == '/' && i + 1 < len && code[i + 1] == '/') {
      size_t start = i;
      while (i < len && code[i] != '\n')
        ++i;
      out += span("hl-comment", code.substr(start, i - start));
      continue;
    }

    // Multi-line comment.
    if (c == '/' && i + 1 < len && code[i + 1] == '*') {
      size_t start = i;
      i += 2;
      while (i + 1 < len && !(code[i] == '*' && code[i + 1] == '/'))
        ++i;
      if (i + 1 < len)
        i += 2;
      out += span("hl-comment", code.substr(start, i - start));
      continue;
    }

    // Rust line comment.
    if (lang == Language::Rust && c == '/' && i + 1 < len && code[i + 1] == '/') {
      size_t start = i;
      while (i < len && code[i] != '\n')
        ++i;
      out += span("hl-comment", code.substr(start, i - start));
      continue;
    }

    // String (double-quoted).
    if (c == '"') {
      size_t start = i;
      ++i;
      while (i < len && code[i] != '"') {
        if (code[i] == '\\' && i + 1 < len)
          ++i;
        ++i;
      }
      if (i < len)
        ++i;  // Skip closing quote.
      out += span("hl-string", code.substr(start, i - start));
      continue;
    }

    // String (single-quoted) — char literal in C/C++.
    if (c == '\'' && (lang == Language::Cpp || lang == Language::C)) {
      size_t start = i;
      ++i;
      while (i < len && code[i] != '\'') {
        if (code[i] == '\\' && i + 1 < len)
          ++i;
        ++i;
      }
      if (i < len)
        ++i;
      out += span("hl-string", code.substr(start, i - start));
      continue;
    }

    // Python/Rust string (single-quoted = string in Rust, char in Python).
    if (c == '\'' && lang == Language::Python) {
      // Check for triple-quote.
      if (i + 2 < len && code[i + 1] == '\'' && code[i + 2] == '\'') {
        size_t start = i;
        i += 3;
        while (i + 2 < len &&
               !(code[i] == '\'' && code[i + 1] == '\'' && code[i + 2] == '\''))
          ++i;
        if (i + 2 < len)
          i += 3;
        out += span("hl-string", code.substr(start, i - start));
        continue;
      }
      size_t start = i;
      ++i;
      while (i < len && code[i] != '\'') {
        if (code[i] == '\\' && i + 1 < len)
          ++i;
        ++i;
      }
      if (i < len)
        ++i;
      out += span("hl-string", code.substr(start, i - start));
      continue;
    }

    // Python/Rust triple double-quote.
    if (c == '"' && i + 2 < len && code[i + 1] == '"' && code[i + 2] == '"') {
      size_t start = i;
      i += 3;
      while (i + 2 < len &&
             !(code[i] == '"' && code[i + 1] == '"' && code[i + 2] == '"'))
        ++i;
      if (i + 2 < len)
        i += 3;
      out += span("hl-string", code.substr(start, i - start));
      continue;
    }

    // Python f-string.
    if (lang == Language::Python && c == 'f' && i + 1 < len && code[i + 1] == '"') {
      size_t start = i;
      i += 2;
      while (i < len && code[i] != '"') {
        if (code[i] == '\\' && i + 1 < len)
          ++i;
        ++i;
      }
      if (i < len)
        ++i;
      out += span("hl-string", code.substr(start, i - start));
      continue;
    }

    // Backtick string (JS/TS template literal).
    if (c == '`' && (lang == Language::JavaScript || lang == Language::TypeScript)) {
      size_t start = i;
      ++i;
      while (i < len && code[i] != '`') {
        if (code[i] == '\\' && i + 1 < len)
          ++i;
        ++i;
      }
      if (i < len)
        ++i;
      out += span("hl-string", code.substr(start, i - start));
      continue;
    }

    // Rust raw string.
    if (lang == Language::Rust && c == 'r' && i + 1 < len &&
        (code[i + 1] == '#' || code[i + 1] == '"')) {
      size_t start = i;
      ++i;
      int hashes = 0;
      while (i < len && code[i] == '#') {
        ++hashes;
        ++i;
      }
      if (i < len && code[i] == '"') {
        ++i;
        while (i < len) {
          if (code[i] == '"') {
            int end_hashes = 0;
            size_t j = i + 1;
            while (j < len && code[j] == '#' && end_hashes < hashes) {
              ++end_hashes;
              ++j;
            }
            if (end_hashes == hashes) {
              i = j;
              break;
            }
          }
          ++i;
        }
        out += span("hl-string", code.substr(start, i - start));
        continue;
      }
      i = start + 1;  // Not a raw string, reset.
    }

    // Numbers.
    if (is_digit(c) || (c == '.' && i + 1 < len && is_digit(code[i + 1]))) {
      size_t start = i;
      // Hex.
      if (c == '0' && i + 1 < len && (code[i + 1] == 'x' || code[i + 1] == 'X')) {
        i += 2;
        while (i < len && (is_digit(code[i]) || (code[i] >= 'a' && code[i] <= 'f') ||
                           (code[i] >= 'A' && code[i] <= 'F')))
          ++i;
      }
      // Binary.
      else if (c == '0' && i + 1 < len && (code[i + 1] == 'b' || code[i + 1] == 'B')) {
        i += 2;
        while (i < len && (code[i] == '0' || code[i] == '1'))
          ++i;
      }
      // Decimal/float.
      else {
        while (i < len && (is_digit(code[i]) || code[i] == '.'))
          ++i;
        // Scientific notation.
        if (i < len && (code[i] == 'e' || code[i] == 'E')) {
          ++i;
          if (i < len && (code[i] == '+' || code[i] == '-'))
            ++i;
          while (i < len && is_digit(code[i]))
            ++i;
        }
      }
      // Suffix (f, u, l, ll, etc.).
      while (i < len && (code[i] == 'f' || code[i] == 'u' || code[i] == 'l' ||
                         code[i] == 'L' || code[i] == 'U' || code[i] == 'F' ||
                         code[i] == 'z' || code[i] == 'i' || code[i] == '_'))
        ++i;
      out += span("hl-number", code.substr(start, i - start));
      continue;
    }

    // Preprocessor directives (#include, #define, etc.).
    if (c == '#' && (lang == Language::Cpp || lang == Language::C)) {
      size_t start = i;
      while (i < len && code[i] != '\n')
        ++i;
      out += span("hl-preproc", code.substr(start, i - start));
      continue;
    }

    // Decorators (Python @decorator).
    if (c == '@' && lang == Language::Python) {
      size_t start = i;
      ++i;
      while (i < len && is_alnum(code[i]))
        ++i;
      out += span("hl-decorator", code.substr(start, i - start));
      continue;
    }

    // Attributes (Rust #[...]).
    if (c == '#' && lang == Language::Rust && i + 1 < len && code[i + 1] == '[') {
      size_t start = i;
      i += 2;
      while (i < len && code[i] != ']')
        ++i;
      if (i < len)
        ++i;
      out += span("hl-preproc", code.substr(start, i - start));
      continue;
    }

    // Identifiers / keywords.
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
      size_t start = i;
      while (i < len && is_alnum(code[i]))
        ++i;
      std::string_view word = code.substr(start, i - start);
      if (is_keyword(word, lang)) {
        out += span("hl-keyword", word);
      } else if (i < len && code[i] == '(') {
        out += span("hl-func", word);
      } else {
        out += escape(word);
      }
      continue;
    }

    // Operators.
    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '=' ||
        c == '!' || c == '<' || c == '>' || c == '&' || c == '|' || c == '^' ||
        c == '~' || c == '?' || c == ':') {
      out += span("hl-op", std::string_view(&c, 1));
      ++i;
      continue;
    }

    // Everything else (whitespace, punctuation).
    escape_char(c, out);
    ++i;
  }

  return out;
}

// Highlight Bash/shell code.
std::string highlight_bash(std::string_view code) {
  std::string out;
  out.reserve(code.size() * 2);
  size_t i = 0;
  size_t len = code.size();

  while (i < len) {
    char c = code[i];

    // Comment.
    if (c == '#') {
      size_t start = i;
      while (i < len && code[i] != '\n')
        ++i;
      out += span("hl-comment", code.substr(start, i - start));
      continue;
    }

    // String (double-quoted).
    if (c == '"') {
      size_t start = i;
      ++i;
      while (i < len && code[i] != '"') {
        if (code[i] == '\\' && i + 1 < len)
          ++i;
        ++i;
      }
      if (i < len)
        ++i;
      out += span("hl-string", code.substr(start, i - start));
      continue;
    }

    // String (single-quoted).
    if (c == '\'') {
      size_t start = i;
      ++i;
      while (i < len && code[i] != '\'')
        ++i;
      if (i < len)
        ++i;
      out += span("hl-string", code.substr(start, i - start));
      continue;
    }

    // Variables ($VAR, ${VAR}).
    if (c == '$') {
      size_t start = i;
      ++i;
      if (i < len && code[i] == '{') {
        ++i;
        while (i < len && code[i] != '}')
          ++i;
        if (i < len)
          ++i;
      } else {
        while (i < len && (is_alnum(code[i]) || code[i] == '?'))
          ++i;
      }
      out += span("hl-var", code.substr(start, i - start));
      continue;
    }

    // Numbers.
    if (is_digit(c)) {
      size_t start = i;
      while (i < len && is_digit(code[i]))
        ++i;
      out += span("hl-number", code.substr(start, i - start));
      continue;
    }

    // Keywords/commands.
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
      size_t start = i;
      while (i < len && is_alnum(code[i]))
        ++i;
      std::string_view word = code.substr(start, i - start);
      // Common bash keywords.
      if (word == "if" || word == "then" || word == "else" || word == "elif" ||
          word == "fi" || word == "for" || word == "while" || word == "do" ||
          word == "done" || word == "case" || word == "esac" || word == "in" ||
          word == "function" || word == "return" || word == "local" ||
          word == "export" || word == "source" || word == "exit" || word == "set" ||
          word == "unset" || word == "shift" || word == "declare" ||
          word == "readonly" || word == "trap" || word == "true" || word == "false" ||
          word == "echo" || word == "cd" || word == "ls" || word == "cat" ||
          word == "grep" || word == "sed" || word == "awk" || word == "mkdir" ||
          word == "rm" || word == "cp" || word == "mv" || word == "chmod" ||
          word == "chown" || word == "sudo" || word == "apt" || word == "apt-get" ||
          word == "pip" || word == "npm" || word == "cargo" || word == "git" ||
          word == "make" || word == "cmake" || word == "curl" || word == "wget" ||
          word == "nift") {
        out += span("hl-keyword", word);
      } else {
        out += escape(word);
      }
      continue;
    }

    // Operators.
    if (c == '|' || c == '&' || c == ';' || c == '>' || c == '<' || c == '(' ||
        c == ')' || c == '{' || c == '}') {
      out += span("hl-op", std::string_view(&c, 1));
      ++i;
      continue;
    }

    escape_char(c, out);
    ++i;
  }

  return out;
}

// Highlight JSON.
std::string highlight_json(std::string_view code) {
  std::string out;
  out.reserve(code.size() * 2);
  size_t i = 0;
  size_t len = code.size();

  while (i < len) {
    char c = code[i];

    // String (keys and values).
    if (c == '"') {
      size_t start = i;
      ++i;
      while (i < len && code[i] != '"') {
        if (code[i] == '\\' && i + 1 < len)
          ++i;
        ++i;
      }
      if (i < len)
        ++i;
      // Check if followed by : (key) or not (value).
      size_t j = i;
      while (j < len && (code[j] == ' ' || code[j] == '\t'))
        ++j;
      if (j < len && code[j] == ':') {
        out += span("hl-key", code.substr(start, i - start));
      } else {
        out += span("hl-string", code.substr(start, i - start));
      }
      continue;
    }

    // Numbers.
    if (is_digit(c) || c == '-') {
      size_t start = i;
      if (c == '-')
        ++i;
      while (i < len && (is_digit(code[i]) || code[i] == '.' || code[i] == 'e' ||
                         code[i] == 'E' || code[i] == '+' || code[i] == '-'))
        ++i;
      out += span("hl-number", code.substr(start, i - start));
      continue;
    }

    // Booleans / null.
    if (code.substr(i, 4) == "true") {
      out += span("hl-keyword", "true");
      i += 4;
      continue;
    }
    if (code.substr(i, 5) == "false") {
      out += span("hl-keyword", "false");
      i += 5;
      continue;
    }
    if (code.substr(i, 4) == "null") {
      out += span("hl-keyword", "null");
      i += 4;
      continue;
    }

    // Structural characters.
    if (c == '{' || c == '}' || c == '[' || c == ']' || c == ':') {
      out += span("hl-op", std::string_view(&c, 1));
      ++i;
      continue;
    }

    escape_char(c, out);
    ++i;
  }

  return out;
}

// Highlight YAML.
std::string highlight_yaml(std::string_view code) {
  std::string out;
  out.reserve(code.size() * 2);
  size_t i = 0;
  size_t len = code.size();

  while (i < len) {
    char c = code[i];

    // Comment.
    if (c == '#') {
      size_t start = i;
      while (i < len && code[i] != '\n')
        ++i;
      out += span("hl-comment", code.substr(start, i - start));
      continue;
    }

    // String (quoted).
    if (c == '"' || c == '\'') {
      char quote = c;
      size_t start = i;
      ++i;
      while (i < len && code[i] != quote) {
        if (code[i] == '\\' && i + 1 < len)
          ++i;
        ++i;
      }
      if (i < len)
        ++i;
      out += span("hl-string", code.substr(start, i - start));
      continue;
    }

    // Key: value.
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
      size_t start = i;
      while ((i < len && is_alnum(code[i])) || code[i] == '-')
        ++i;
      std::string_view word = code.substr(start, i - start);
      // Check if followed by :
      size_t j = i;
      while (j < len && code[j] == ' ')
        ++j;
      if (j < len && code[j] == ':') {
        out += span("hl-key", word);
      } else {
        out += escape(word);
      }
      continue;
    }

    // Booleans / null.
    if (code.substr(i, 4) == "true" || code.substr(i, 5) == "false" ||
        code.substr(i, 4) == "null") {
      bool is_false = code.substr(i, 5) == "false";
      size_t w = is_false ? 5 : 4;
      out += span("hl-keyword", code.substr(i, w));
      i += w;
      continue;
    }

    // Numbers.
    if (is_digit(c) || c == '-') {
      size_t start = i;
      if (c == '-')
        ++i;
      while (i < len && (is_digit(code[i]) || code[i] == '.'))
        ++i;
      out += span("hl-number", code.substr(start, i - start));
      continue;
    }

    // Dashes (list items).
    if (c == '-' && i + 1 < len && code[i + 1] == ' ') {
      out += span("hl-op", "-");
      ++i;
      continue;
    }

    escape_char(c, out);
    ++i;
  }

  return out;
}

// Highlight HTML (basic tag highlighting).
std::string highlight_html_code(std::string_view code) {
  std::string out;
  out.reserve(code.size() * 2);
  size_t i = 0;
  size_t len = code.size();

  while (i < len) {
    char c = code[i];

    // HTML comment.
    if (code.substr(i, 4) == "<!--") {
      size_t start = i;
      i += 4;
      size_t end = code.find("-->", i);
      if (end != std::string_view::npos)
        i = end + 3;
      else
        i = len;
      out += span("hl-comment", code.substr(start, i - start));
      continue;
    }

    // Tags.
    if (c == '<') {
      size_t start = i;
      ++i;
      bool is_closing = (i < len && code[i] == '/');
      if (is_closing)
        ++i;
      // Tag name.
      size_t name_start = i;
      while (i < len && is_alnum(code[i]))
        ++i;
      std::string_view tag_name = code.substr(name_start, i - name_start);

      out += escape(code.substr(start, 1));  // <
      if (is_closing)
        out += escape("/");
      out += span("hl-keyword", tag_name);

      // Attributes.
      while (i < len && code[i] != '>' &&
             !(code[i] == '/' && i + 1 < len && code[i + 1] == '>')) {
        if (code[i] == ' ' || code[i] == '\t' || code[i] == '\n') {
          escape_char(code[i], out);
          ++i;
          continue;
        }
        // Attribute name.
        size_t attr_start = i;
        while ((i < len && is_alnum(code[i])) || code[i] == '-' || code[i] == ':')
          ++i;
        if (i > attr_start) {
          out += span("hl-func", code.substr(attr_start, i - attr_start));
        }
        // = sign.
        if (i < len && code[i] == '=') {
          out += span("hl-op", "=");
          ++i;
          // Attribute value.
          if (i < len && (code[i] == '"' || code[i] == '\'')) {
            char q = code[i];
            size_t val_start = i;
            ++i;
            while (i < len && code[i] != q)
              ++i;
            if (i < len)
              ++i;
            out += span("hl-string", code.substr(val_start, i - val_start));
          }
        }
      }

      // Close tag.
      if (i < len && code[i] == '/' && i + 1 < len && code[i + 1] == '>') {
        out += span("hl-op", "/>");
        i += 2;
      } else if (i < len && code[i] == '>') {
        out += span("hl-op", ">");
        ++i;
      }
      continue;
    }

    escape_char(c, out);
    ++i;
  }

  return out;
}

// Highlight CSS.
std::string highlight_css_code(std::string_view code) {
  std::string out;
  out.reserve(code.size() * 2);
  size_t i = 0;
  size_t len = code.size();

  while (i < len) {
    char c = code[i];

    // Comment.
    if (c == '/' && i + 1 < len && code[i + 1] == '*') {
      size_t start = i;
      i += 2;
      while (i + 1 < len && !(code[i] == '*' && code[i + 1] == '/'))
        ++i;
      if (i + 1 < len)
        i += 2;
      out += span("hl-comment", code.substr(start, i - start));
      continue;
    }

    // String.
    if (c == '"' || c == '\'') {
      char q = c;
      size_t start = i;
      ++i;
      while (i < len && code[i] != q) {
        if (code[i] == '\\' && i + 1 < len)
          ++i;
        ++i;
      }
      if (i < len)
        ++i;
      out += span("hl-string", code.substr(start, i - start));
      continue;
    }

    // Property: value.
    if ((c >= 'a' && c <= 'z') || c == '-') {
      size_t start = i;
      while (i < len && (is_alnum(code[i]) || code[i] == '-'))
        ++i;
      std::string_view word = code.substr(start, i - start);
      // Check if followed by :.
      size_t j = i;
      while (j < len && code[j] == ' ')
        ++j;
      if (j < len && code[j] == ':') {
        out += span("hl-key", word);
      } else {
        out += escape(word);
      }
      continue;
    }

    // Selectors (.class, #id, tag).
    if (c == '.' || c == '#') {
      size_t start = i;
      ++i;
      while (i < len && (is_alnum(code[i]) || code[i] == '-'))
        ++i;
      out += span("hl-func", code.substr(start, i - start));
      continue;
    }

    // Numbers (with units).
    if (is_digit(c) || c == '.') {
      size_t start = i;
      while (i < len && (is_digit(code[i]) || code[i] == '.'))
        ++i;
      // Units.
      while (i < len &&
             (code[i] == 'p' || code[i] == 'x' || code[i] == 'e' || code[i] == 'm' ||
              code[i] == 'r' || code[i] == 'v' || code[i] == 'w' || code[i] == 'h' ||
              code[i] == '%' || code[i] == 'd' || code[i] == 'c' || code[i] == 'i' ||
              code[i] == 'f' || code[i] == 's' || code[i] == 'l' || code[i] == 'b' ||
              code[i] == 'k'))
        ++i;
      out += span("hl-number", code.substr(start, i - start));
      continue;
    }

    // At-rules (@media, @keyframes, etc.).
    if (c == '@') {
      size_t start = i;
      ++i;
      while (i < len && is_alnum(code[i]))
        ++i;
      out += span("hl-preproc", code.substr(start, i - start));
      continue;
    }

    // Colors (#hex).
    if (c == '#' && i + 1 < len && is_alnum(code[i + 1])) {
      size_t start = i;
      ++i;
      while (i < len && is_alnum(code[i]))
        ++i;
      out += span("hl-number", code.substr(start, i - start));
      continue;
    }

    escape_char(c, out);
    ++i;
  }

  return out;
}

// Highlight Lua code.
std::string highlight_lua_code(std::string_view code) {
  std::string out;
  out.reserve(code.size() * 2);
  size_t i = 0;
  size_t len = code.size();

  while (i < len) {
    char c = code[i];

    // Comment (-- or --[[ ]]).
    if (c == '-' && i + 1 < len && code[i + 1] == '-') {
      size_t start = i;
      i += 2;
      // Long comment?
      if (i < len && code[i] == '[') {
        size_t j = i + 1;
        int level = 0;
        while (j < len && code[j] == '=') {
          ++level;
          ++j;
        }
        if (j < len && code[j] == '[') {
          i = j + 1;
          // Find matching ]=...]
          std::string closer = "]";
          for (int k = 0; k < level; ++k)
            closer += '=';
          closer += ']';
          size_t end = code.find(closer, i);
          if (end != std::string_view::npos)
            i = end + closer.size();
          else
            i = len;
          out += span("hl-comment", code.substr(start, i - start));
          continue;
        }
      }
      while (i < len && code[i] != '\n')
        ++i;
      out += span("hl-comment", code.substr(start, i - start));
      continue;
    }

    // String (double-quoted).
    if (c == '"') {
      size_t start = i;
      ++i;
      while (i < len && code[i] != '"') {
        if (code[i] == '\\' && i + 1 < len)
          ++i;
        ++i;
      }
      if (i < len)
        ++i;
      out += span("hl-string", code.substr(start, i - start));
      continue;
    }

    // String (single-quoted).
    if (c == '\'') {
      size_t start = i;
      ++i;
      while (i < len && code[i] != '\'') {
        if (code[i] == '\\' && i + 1 < len)
          ++i;
        ++i;
      }
      if (i < len)
        ++i;
      out += span("hl-string", code.substr(start, i - start));
      continue;
    }

    // Long string [=[ ]=].
    if (c == '[' && i + 1 < len && (code[i + 1] == '[' || code[i + 1] == '=')) {
      size_t j = i + 1;
      int level = 0;
      while (j < len && code[j] == '=') {
        ++level;
        ++j;
      }
      if (j < len && code[j] == '[') {
        size_t start = i;
        i = j + 1;
        std::string closer = "]";
        for (int k = 0; k < level; ++k)
          closer += '=';
        closer += ']';
        size_t end = code.find(closer, i);
        if (end != std::string_view::npos)
          i = end + closer.size();
        else
          i = len;
        out += span("hl-string", code.substr(start, i - start));
        continue;
      }
    }

    // Numbers.
    if (is_digit(c)) {
      size_t start = i;
      if (c == '0' && i + 1 < len && (code[i + 1] == 'x' || code[i + 1] == 'X')) {
        i += 2;
        while (i < len && (is_digit(code[i]) || (code[i] >= 'a' && code[i] <= 'f') ||
                           (code[i] >= 'A' && code[i] <= 'F')))
          ++i;
      } else {
        while (i < len && (is_digit(code[i]) || code[i] == '.'))
          ++i;
        if (i < len && (code[i] == 'e' || code[i] == 'E')) {
          ++i;
          if (i < len && (code[i] == '+' || code[i] == '-'))
            ++i;
          while (i < len && is_digit(code[i]))
            ++i;
        }
      }
      out += span("hl-number", code.substr(start, i - start));
      continue;
    }

    // Keywords.
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
      size_t start = i;
      while (i < len && is_alnum(code[i]))
        ++i;
      std::string_view word = code.substr(start, i - start);
      if (word == "and" || word == "break" || word == "do" || word == "else" ||
          word == "elseif" || word == "end" || word == "false" || word == "for" ||
          word == "function" || word == "goto" || word == "if" || word == "in" ||
          word == "local" || word == "nil" || word == "not" || word == "or" ||
          word == "repeat" || word == "return" || word == "then" || word == "true" ||
          word == "until" || word == "while") {
        out += span("hl-keyword", word);
      } else if (i < len && code[i] == '(') {
        out += span("hl-func", word);
      } else {
        out += escape(word);
      }
      continue;
    }

    escape_char(c, out);
    ++i;
  }

  return out;
}

// Highlight diff.
std::string highlight_diff_code(std::string_view code) {
  std::string out;
  out.reserve(code.size() * 2);
  std::istringstream ss{std::string(code)};
  std::string line;
  bool first = true;

  while (std::getline(ss, line)) {
    if (!first)
      out += '\n';
    first = false;

    if (!line.empty() && line[0] == '+') {
      out += span("hl-add", line);
    } else if (!line.empty() && line[0] == '-') {
      out += span("hl-del", line);
    } else if (!line.empty() && line[0] == '@') {
      out += span("hl-hunk", line);
    } else {
      out += escape(line);
    }
  }

  return out;
}

}  // namespace

std::string highlight(std::string_view code, Language lang) {
  switch (lang) {
    case Language::Cpp:
    case Language::C:
    case Language::JavaScript:
    case Language::TypeScript:
    case Language::Rust:
    case Language::Go:
      return highlight_c_family(code, lang);
    case Language::Python:
      return highlight_c_family(code, lang);
    case Language::Bash:
      return highlight_bash(code);
    case Language::Json:
      return highlight_json(code);
    case Language::Yaml:
      return highlight_yaml(code);
    case Language::Html:
      return highlight_html_code(code);
    case Language::Css:
      return highlight_css_code(code);
    case Language::Lua:
      return highlight_lua_code(code);
    case Language::Diff:
      return highlight_diff_code(code);
    case Language::Unknown:
    default:
      return escape(code);
  }
}

std::string highlight_html(std::string_view html) {
  std::string result;
  result.reserve(html.size());

  size_t pos = 0;
  size_t len = html.size();

  while (pos < len) {
    // Find <code class="language-XXX">
    auto code_start = html.find("<code", pos);
    if (code_start == std::string_view::npos) {
      result += html.substr(pos);
      break;
    }

    // Copy everything before this code tag.
    result += html.substr(pos, code_start - pos);

    // Find the end of the opening tag.
    auto tag_end = html.find('>', code_start);
    if (tag_end == std::string_view::npos) {
      result += html.substr(code_start);
      break;
    }

    std::string_view open_tag = html.substr(code_start, tag_end - code_start + 1);

    // Check for language- class.
    std::string lang_str;
    auto lang_pos = open_tag.find("language-");
    if (lang_pos != std::string_view::npos) {
      size_t lang_start = lang_pos + 9;  // Skip "language-"
      size_t lang_end = lang_start;
      while (lang_end < open_tag.size() && open_tag[lang_end] != '"' &&
             open_tag[lang_end] != '\'' && open_tag[lang_end] != ' ' &&
             open_tag[lang_end] != '>') {
        ++lang_end;
      }
      lang_str = std::string(open_tag.substr(lang_start, lang_end - lang_start));
    }

    // Find </code>.
    auto code_end = html.find("</code>", tag_end);
    if (code_end == std::string_view::npos) {
      result += html.substr(code_start);
      break;
    }

    // Extract code content (between > and </code>).
    std::string_view code_content = html.substr(tag_end + 1, code_end - tag_end - 1);

    // Highlight if language detected.
    if (!lang_str.empty()) {
      Language lang = detect_language(lang_str);
      result += open_tag;
      result += highlight(code_content, lang);
    } else {
      result += open_tag;
      result += code_content;
    }

    pos = code_end;
  }

  return result;
}

std::string highlight_css() {
  return R"(
/* ─── Nift Code Highlight Theme (Dark OLED) ─── */
code { background: rgba(255,255,255,0.04); border-radius: 6px; padding: 0.2em 0.4em; font-size: 0.875em; font-family: 'SF Mono', 'Fira Code', 'JetBrains Mono', monospace; }
pre > code { display: block; padding: 1rem 1.25rem; overflow-x: auto; background: rgba(0,0,0,0.5); border: 1px solid rgba(255,255,255,0.06); font-size: 0.8125em; line-height: 1.7; }
.hl-keyword { color: #ff7b72; font-weight: 600; }
.hl-string { color: #a5d6ff; }
.hl-comment { color: #8b949e; font-style: italic; }
.hl-number { color: #79c0ff; }
.hl-func { color: #d2a8ff; }
.hl-key { color: #ff7b72; }
.hl-var { color: #ffa657; }
.hl-op { color: #ff7b72; }
.hl-preproc { color: #ffa657; font-weight: 600; }
.hl-decorator { color: #ffa657; }
.hl-add { color: #3fb950; background: rgba(63,185,80,0.1); display: block; }
.hl-del { color: #f85149; background: rgba(248,81,73,0.1); display: block; }
.hl-hunk { color: #79c0ff; background: rgba(121,192,255,0.1); display: block; }
)";
}

}  // namespace nift::markdown
