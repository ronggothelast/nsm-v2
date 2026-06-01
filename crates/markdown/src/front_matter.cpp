// ─── nift/markdown/front_matter.cpp ──────────────────────────────────
// Front matter extraction: YAML-like and JSON formats.

#include "nift/markdown/front_matter.hpp"

#include <fmt/format.h>
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace nift::markdown {

namespace {

// Trim whitespace from both ends.
std::string trim(std::string_view s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return {};
    auto end = s.find_last_not_of(" \t\r\n");
    return std::string(s.substr(start, end - start + 1));
}

// Check if a line is the YAML delimiter `---`.

// Try to parse a value string as JSON-compatible type.
nlohmann::json parse_value(std::string_view val) {
    auto t = trim(val);
    if (t.empty()) return nlohmann::json("");

    // Boolean.
    if (t == "true")  return nlohmann::json(true);
    if (t == "false") return nlohmann::json(false);
    if (t == "null" || t == "~") return nlohmann::json(nullptr);

    // Integer.
    try {
        size_t pos = 0;
        long long i = std::stoll(std::string(t), &pos);
        if (pos == t.size()) return nlohmann::json(i);
    } catch (...) {}

    // Float.
    try {
        size_t pos = 0;
        double d = std::stod(std::string(t), &pos);
        if (pos == t.size()) return nlohmann::json(d);
    } catch (...) {}

    // Array: [a, b, c]
    if (t.front() == '[' && t.back() == ']') {
        nlohmann::json arr = nlohmann::json::array();
        std::string inner(t.substr(1, t.size() - 2));
        std::istringstream ss(inner);
        std::string item;
        while (std::getline(ss, item, ',')) {
            arr.push_back(parse_value(item));
        }
        return arr;
    }

    // Quoted string.
    if ((t.front() == '"' && t.back() == '"') ||
        (t.front() == '\'' && t.back() == '\'')) {
        return nlohmann::json(t.substr(1, t.size() - 2));
    }

    // Plain string.
    return nlohmann::json(t);
}

// Parse YAML-like key: value pairs into JSON.
// Handles simple indentation-based nesting (2-space indent).
nlohmann::json parse_yaml_lines(const std::vector<std::string>& lines,
                                 size_t start, int base_indent) {
    nlohmann::json result = nlohmann::json::object();
    size_t i = start;

    while (i < lines.size()) {
        const auto& line = lines[i];
        if (line.empty() || line[0] == '#') {  // Skip empty/comment lines.
            ++i;
            continue;
        }

        // Measure indent.
        int indent = 0;
        for (char c : line) {
            if (c == ' ') ++indent;
            else if (c == '\t') indent += 2;
            else break;
        }

        if (indent < base_indent) break;  // Dedented — return to caller.
        if (indent > base_indent) { ++i; continue; }  // Skip unexpected indent.

        // Find key: value separator.
        auto colon = line.find(':');
        if (colon == std::string::npos) { ++i; continue; }

        std::string key = trim(line.substr(0, colon));
        std::string val_str = trim(line.substr(colon + 1));

        if (key.empty()) { ++i; continue; }

        // Check if next lines are nested (indented more).
        if (val_str.empty() && i + 1 < lines.size()) {
            // Look ahead for nested content.
            int next_indent = 0;
            for (char c : lines[i + 1]) {
                if (c == ' ') ++next_indent;
                else if (c == '\t') next_indent += 2;
                else break;
            }
            if (next_indent > base_indent) {
                // Parse nested block.
                result[key] = parse_yaml_lines(lines, i + 1, next_indent);
                // Skip nested lines.
                ++i;
                while (i < lines.size()) {
                    int ni = 0;
                    for (char c : lines[i]) {
                        if (c == ' ') ++ni;
                        else break;
                    }
                    if (ni <= base_indent || lines[i].empty()) break;
                    ++i;
                }
                continue;
            }
        }

        result[key] = parse_value(val_str);
        ++i;
    }

    return result;
}

}  // namespace

nlohmann::json parse_yaml_simple(std::string_view yaml) {
    // Split into lines.
    std::vector<std::string> lines;
    std::string current;
    for (char c : yaml) {
        if (c == '\n') {
            lines.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty()) lines.push_back(current);

    return parse_yaml_lines(lines, 0, 0);
}

ParsedDocument parse_front_matter(std::string_view source) {
    ParsedDocument result;

    if (source.empty()) {
        result.body = std::string(source);
        return result;
    }

    // Check for YAML front matter: starts with `---`.
    if (source.substr(0, 3) == "---") {
        // Find closing `---`.
        auto pos = source.find("\n---", 3);
        if (pos != std::string_view::npos) {
            std::string_view yaml_block = source.substr(3, pos - 3);
            result.front_matter = parse_yaml_simple(yaml_block);
            result.has_front_matter = true;

            // Body starts after closing `---` + optional newline.
            size_t body_start = pos + 3;
            if (body_start < source.size() && source[body_start] == '\n') {
                ++body_start;
            }
            result.body = std::string(source.substr(body_start));
            return result;
        }
    }

    // Check for JSON front matter: starts with `{`.
    if (source[0] == '{') {
        // Find matching `}`.
        int depth = 0;
        size_t end = 0;
        for (size_t i = 0; i < source.size(); ++i) {
            if (source[i] == '{') ++depth;
            else if (source[i] == '}') {
                --depth;
                if (depth == 0) { end = i; break; }
            }
        }

        if (end > 0) {
            try {
                result.front_matter = nlohmann::json::parse(
                    source.substr(0, end + 1));
                result.has_front_matter = true;

                size_t body_start = end + 1;
                if (body_start < source.size() && source[body_start] == '\n') {
                    ++body_start;
                }
                result.body = std::string(source.substr(body_start));
                return result;
            } catch (...) {
                // Not valid JSON — treat as regular markdown.
            }
        }
    }

    // No front matter found.
    result.body = std::string(source);
    return result;
}

std::string strip_front_matter(std::string_view source) {
    return parse_front_matter(source).body;
}

}  // namespace nift::markdown
