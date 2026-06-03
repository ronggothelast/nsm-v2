// ─── nift/markdown/front_matter.cpp ──────────────────────────────────
// Front matter extraction: YAML-like and JSON formats.
//
// Full YAML front matter support:
//   - Simple key: value with type inference
//   - Indentation-based nesting (2-space)
//   - Multi-line strings: literal (|) and folded (>)
//   - Block sequences (- item)
//   - Inline flow mappings ({key: value})
//   - Inline flow sequences ([a, b, c])
//   - Comments (#)
//   - Quoted strings (single/double)

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
  if (start == std::string_view::npos)
    return {};
  auto end = s.find_last_not_of(" \t\r\n");
  return std::string(s.substr(start, end - start + 1));
}

// Measure leading whitespace.
int measure_indent(std::string_view line) {
  int indent = 0;
  for (char c : line) {
    if (c == ' ')
      ++indent;
    else if (c == '\t')
      indent += 2;
    else
      break;
  }
  return indent;
}

// Check if line is blank or comment.
bool is_blank_or_comment(std::string_view line) {
  auto t = line.find_first_not_of(" \t");
  return t == std::string_view::npos || line[t] == '#';
}

// Try to parse a value string as JSON-compatible type.
nlohmann::json parse_value(std::string_view val) {
  auto t = trim(val);
  if (t.empty())
    return nlohmann::json("");

  // Boolean (YAML 1.1 compat: true/false/yes/no/on/off).
  if (t == "true" || t == "True" || t == "TRUE" || t == "yes" || t == "Yes" ||
      t == "YES" || t == "on" || t == "On" || t == "ON")
    return nlohmann::json(true);
  if (t == "false" || t == "False" || t == "FALSE" || t == "no" || t == "No" ||
      t == "NO" || t == "off" || t == "Off" || t == "OFF")
    return nlohmann::json(false);
  if (t == "null" || t == "Null" || t == "NULL" || t == "~")
    return nlohmann::json(nullptr);

  // Integer (including negative and hex with base=0).
  // Intentional fallback: if stoll throws, value is not an integer.
  try {
    size_t pos = 0;
    long long i = std::stoll(std::string(t), &pos, 0);
    if (pos == t.size())
      return nlohmann::json(i);
  } catch (...) {
  }

  // Float (including scientific notation).
  // Intentional fallback: if stod throws, value is not a float.
  try {
    size_t pos = 0;
    double d = std::stod(std::string(t), &pos);
    if (pos == t.size())
      return nlohmann::json(d);
  } catch (...) {
  }

  // Inline flow sequence: [a, b, c]
  if (t.front() == '[' && t.back() == ']') {
    try {
      return nlohmann::json::parse(std::string(t));
    } catch (...) {
      nlohmann::json arr = nlohmann::json::array();
      std::string inner(t.substr(1, t.size() - 2));
      std::istringstream ss(inner);
      std::string item;
      while (std::getline(ss, item, ',')) {
        arr.push_back(parse_value(item));
      }
      return arr;
    }
  }

  // Inline flow mapping: {key: value}
  if (t.front() == '{' && t.back() == '}') {
    try {
      return nlohmann::json::parse(std::string(t));
    } catch (...) {
      nlohmann::json obj = nlohmann::json::object();
      std::string inner(t.substr(1, t.size() - 2));
      std::istringstream ss(inner);
      std::string pair;
      while (std::getline(ss, pair, ',')) {
        auto colon = pair.find(':');
        if (colon != std::string::npos) {
          std::string k = trim(pair.substr(0, colon));
          std::string v = trim(pair.substr(colon + 1));
          if (!k.empty())
            obj[k] = parse_value(v);
        }
      }
      return obj;
    }
  }

  // Double-quoted string with escape sequences.
  if (t.front() == '"' && t.back() == '"' && t.size() >= 2) {
    std::string result;
    for (size_t idx = 1; idx < t.size() - 1; ++idx) {
      if (t[idx] == '\\' && idx + 1 < t.size() - 1) {
        switch (t[idx + 1]) {
          case 'n':
            result += '\n';
            break;
          case 't':
            result += '\t';
            break;
          case 'r':
            result += '\r';
            break;
          case '\\':
            result += '\\';
            break;
          case '"':
            result += '"';
            break;
          default:
            result += t[idx];
            result += t[idx + 1];
            break;
        }
        ++idx;
      } else {
        result += t[idx];
      }
    }
    return nlohmann::json(result);
  }

  // Single-quoted string (no escapes except '' -> ').
  if (t.front() == '\'' && t.back() == '\'' && t.size() >= 2) {
    std::string result(t.substr(1, t.size() - 2));
    size_t pos = 0;
    while ((pos = result.find("''", pos)) != std::string::npos) {
      result.replace(pos, 2, "'");
      pos += 1;
    }
    return nlohmann::json(result);
  }

  // Plain string.
  return nlohmann::json(t);
}

// Forward declaration.
nlohmann::json parse_yaml_block(const std::vector<std::string>& lines, size_t& i,
                                int base_indent);

// Parse a multi-line scalar (literal | or folded >).
std::string parse_multiline_scalar(const std::vector<std::string>& lines, size_t& i,
                                   int block_indent, bool folded) {
  std::string result;
  bool first_line = true;

  while (i < lines.size()) {
    if (is_blank_or_comment(lines[i])) {
      if (first_line) {
        ++i;
        continue;
      }
      result += '\n';
      ++i;
      continue;
    }

    int indent = measure_indent(lines[i]);
    if (indent < block_indent)
      break;

    std::string content(lines[i].substr(block_indent));

    if (folded && !first_line && !result.empty() && result.back() != '\n') {
      result += ' ';
    }

    result += content;
    first_line = false;
    ++i;

    if (!folded)
      result += '\n';
  }

  if (folded && !result.empty() && result.back() == '\n')
    result.pop_back();

  return result;
}

// Parse a mapping entry within a block sequence (- key: value, possibly multi-line).
// Advances i past all consumed lines and returns the mapping object.
nlohmann::json parse_sequence_mapping_entry(const std::vector<std::string>& lines,
                                             size_t& i, int seq_indent, int indent,
                                             std::string_view item_content) {
  nlohmann::json item_obj = nlohmann::json::object();
  auto colon = item_content.find(':');
  std::string key = trim(item_content.substr(0, colon));
  std::string val_str = trim(item_content.substr(colon + 1));

  if (!val_str.empty()) {
    item_obj[key] = parse_value(val_str);
  } else {
    ++i;
    if (i < lines.size() && !is_blank_or_comment(lines[i])) {
      int next_indent = measure_indent(lines[i]);
      if (next_indent > indent + 2) {
        item_obj[key] = parse_yaml_block(lines, i, next_indent);
      } else {
        item_obj[key] = nlohmann::json("");
      }
    } else {
      item_obj[key] = nlohmann::json("");
    }
  }

  ++i;  // Move past the initial "- key: value" line.

  // Consume additional key: value lines at seq_indent + 2.
  int body_indent = seq_indent + 2;
  while (i < lines.size()) {
    if (is_blank_or_comment(lines[i])) {
      ++i;
      continue;
    }
    int ni = measure_indent(lines[i]);
    if (ni < body_indent)
      break;
    if (ni > body_indent) {
      ++i;
      continue;
    }

    auto ncs = lines[i].find_first_not_of(" \t");
    if (ncs == std::string::npos) {
      ++i;
      continue;
    }
    if (lines[i].substr(ncs, 2) == "- ")
      break;  // New seq item.

    auto ncolon = lines[i].find(':', ncs);
    if (ncolon == std::string::npos) {
      ++i;
      continue;
    }

    std::string nkey = trim(std::string_view(lines[i]).substr(ncs, ncolon - ncs));
    std::string nval = trim(std::string_view(lines[i]).substr(ncolon + 1));
    if (!nkey.empty()) {
      if (nval.empty() && i + 1 < lines.size()) {
        int nn = measure_indent(lines[i + 1]);
        if (nn > body_indent) {
          ++i;
          item_obj[nkey] = parse_yaml_block(lines, i, nn);
          continue;
        }
      }
      item_obj[nkey] = parse_value(nval);
    }
    ++i;
  }

  return item_obj;
}

// Parse a block sequence (- item).
nlohmann::json parse_block_sequence(const std::vector<std::string>& lines, size_t& i,
                                    int seq_indent) {
  nlohmann::json arr = nlohmann::json::array();

  while (i < lines.size()) {
    if (is_blank_or_comment(lines[i])) {
      ++i;
      continue;
    }

    int indent = measure_indent(lines[i]);
    if (indent < seq_indent)
      break;
    if (indent > seq_indent) {
      ++i;
      continue;
    }

    auto cs = lines[i].find_first_not_of(" \t");
    if (cs == std::string::npos || lines[i].substr(cs, 2) != "- ")
      break;

    std::string item_content = trim(lines[i].substr(cs + 2));

    // Mapping item: - key: value (may span multiple lines at seq_indent+2).
    if (!item_content.empty()) {
      auto colon = item_content.find(':');
      if (colon != std::string::npos && colon > 0) {
        arr.push_back(
            parse_sequence_mapping_entry(lines, i, seq_indent, indent, item_content));
        continue;
      }
    }

    // Simple scalar item.
    if (!item_content.empty()) {
      arr.push_back(parse_value(item_content));
    } else {
      ++i;
      if (i < lines.size() && !is_blank_or_comment(lines[i])) {
        int next_indent = measure_indent(lines[i]);
        if (next_indent > indent + 2) {
          arr.push_back(parse_yaml_block(lines, i, next_indent));
          continue;
        }
      }
      arr.push_back(nlohmann::json(""));
    }
    ++i;
  }

  return arr;
}

// Parse a YAML block (mapping or sequence).
nlohmann::json parse_yaml_block(const std::vector<std::string>& lines, size_t& i,
                                int base_indent) {
  // Detect sequence by peeking.
  for (size_t j = i; j < lines.size(); ++j) {
    if (is_blank_or_comment(lines[j]))
      continue;
    int indent = measure_indent(lines[j]);
    if (indent < base_indent)
      break;
    if (indent == base_indent) {
      auto cs = lines[j].find_first_not_of(" \t");
      if (cs != std::string::npos && lines[j].substr(cs, 2) == "- ") {
        return parse_block_sequence(lines, i, base_indent);
      }
      break;
    }
  }

  nlohmann::json result = nlohmann::json::object();

  while (i < lines.size()) {
    if (is_blank_or_comment(lines[i])) {
      ++i;
      continue;
    }

    int indent = measure_indent(lines[i]);
    if (indent < base_indent)
      break;
    if (indent > base_indent) {
      ++i;
      continue;
    }

    auto cs = lines[i].find_first_not_of(" \t");
    if (cs == std::string::npos) {
      ++i;
      continue;
    }

    if (lines[i].substr(cs, 2) == "- ") {
      return parse_block_sequence(lines, i, base_indent);
    }

    auto colon = lines[i].find(':', cs);
    if (colon == std::string::npos) {
      ++i;
      continue;
    }

    std::string key = trim(std::string_view(lines[i]).substr(cs, colon - cs));
    std::string val_str = trim(std::string_view(lines[i]).substr(colon + 1));

    if (key.empty()) {
      ++i;
      continue;
    }

    // Multi-line literal scalar: |
    if (val_str == "|" || val_str == "|+" || val_str == "|-") {
      ++i;
      bool keep = (val_str == "|+");
      bool strip = (val_str == "|-");
      int scalar_indent = base_indent + 2;
      if (i < lines.size() && !is_blank_or_comment(lines[i]))
        scalar_indent = measure_indent(lines[i]);
      std::string scalar = parse_multiline_scalar(lines, i, scalar_indent, false);
      if (strip) {
        while (!scalar.empty() && scalar.back() == '\n')
          scalar.pop_back();
      } else if (!keep && !scalar.empty() && scalar.back() != '\n')
        scalar += '\n';
      result[key] = nlohmann::json(scalar);
      continue;
    }

    // Multi-line folded scalar: >
    if (val_str == ">" || val_str == ">+" || val_str == ">-") {
      ++i;
      bool keep = (val_str == ">+");
      bool strip = (val_str == ">-");
      int scalar_indent = base_indent + 2;
      if (i < lines.size() && !is_blank_or_comment(lines[i]))
        scalar_indent = measure_indent(lines[i]);
      std::string scalar = parse_multiline_scalar(lines, i, scalar_indent, true);
      if (strip) {
        while (!scalar.empty() && scalar.back() == '\n')
          scalar.pop_back();
      } else if (!keep && !scalar.empty() && scalar.back() != '\n')
        scalar += '\n';
      result[key] = nlohmann::json(scalar);
      continue;
    }

    // Nested block (key with no value, next line indented).
    if (val_str.empty() && i + 1 < lines.size()) {
      int next_indent = measure_indent(lines[i + 1]);
      if (next_indent > base_indent) {
        ++i;
        result[key] = parse_yaml_block(lines, i, next_indent);
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
  if (!current.empty())
    lines.push_back(current);

  size_t i = 0;
  return parse_yaml_block(lines, i, 0);
}

ParsedDocument parse_front_matter(std::string_view source) {
  ParsedDocument result;

  if (source.empty()) {
    result.body = std::string(source);
    return result;
  }

  // YAML front matter: starts with ---
  if (source.substr(0, 3) == "---") {
    auto pos = source.find("\n---", 3);
    if (pos != std::string_view::npos) {
      std::string_view yaml_block = source.substr(3, pos - 3);
      result.front_matter = parse_yaml_simple(yaml_block);
      result.has_front_matter = true;

      size_t body_start = pos + 3;
      if (body_start < source.size() && source[body_start] == '\n')
        ++body_start;
      result.body = std::string(source.substr(body_start));
      return result;
    }
  }

  // JSON front matter: starts with {
  if (source[0] == '{') {
    int depth = 0;
    size_t end = 0;
    for (size_t idx = 0; idx < source.size(); ++idx) {
      if (source[idx] == '{') {
        ++depth;
      } else if (source[idx] == '}') {
        --depth;
        if (depth == 0) {
          end = idx;
          break;
        }
      }
    }
    if (end > 0) {
      try {
        result.front_matter = nlohmann::json::parse(source.substr(0, end + 1));
        result.has_front_matter = true;
        size_t body_start = end + 1;
        if (body_start < source.size() && source[body_start] == '\n')
          ++body_start;
        result.body = std::string(source.substr(body_start));
        return result;
      } catch (...) {
        // Intentional fallback: malformed JSON front matter; treat as plain body.
      }
    }
  }

  result.body = std::string(source);
  return result;
}

std::string strip_front_matter(std::string_view source) {
  return parse_front_matter(source).body;
}

}  // namespace nift::markdown
