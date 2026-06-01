// ─── nift/markdown/markdown.cpp ──────────────────────────────────────
// cmark-gfm wrapper: Markdown → HTML, plain text, TOC, link extraction.

#include "nift/markdown/markdown.hpp"

#include <cmark-gfm-core-extensions.h>
#include <fmt/format.h>
#include <stdexcept>
#include <cstring>

namespace nift::markdown {

namespace {

// RAII wrapper for cmark_node*.
struct CmarkNodeDeleter {
    void operator()(cmark_node* node) const noexcept {
        if (node) cmark_node_free(node);
    }
};

// Build cmark options bitmask from MarkdownOptions.
int build_options(const MarkdownOptions& opts) noexcept {
    int flags = CMARK_OPT_DEFAULT;
    if (opts.hardbreaks)  flags |= CMARK_OPT_HARDBREAKS;
    if (opts.unsafe)      flags |= CMARK_OPT_UNSAFE;
    if (opts.smart)       flags |= CMARK_OPT_SMART;
    if (opts.strikethrough) {
        // Strikethrough requires GFM extensions — registered globally in
        // to_html() before parsing.
    }
    // tagfilter: handled by extension, no option flag
    return flags;
}

// Recursively walk the AST and collect plain text.
void collect_text(cmark_node* node, std::string& out) {
    if (!node) return;

    auto type = cmark_node_get_type(node);

    if (type == CMARK_NODE_TEXT || type == CMARK_NODE_CODE) {
        const char* text = cmark_node_get_literal(node);
        if (text) out += text;
    } else if (type == CMARK_NODE_SOFTBREAK) {
        out += '\n';
    } else if (type == CMARK_NODE_LINEBREAK) {
        out += '\n';
    } else if (type == CMARK_NODE_LINEBREAK) {
        out += '\n';
    }

    // Recurse children.
    for (cmark_node* child = cmark_node_first_child(node);
         child;
         child = cmark_node_next(child)) {
        collect_text(child, out);
    }
}

// Recursively walk AST and collect headings for TOC.
void collect_headings(cmark_node* node, std::vector<TocEntry>& toc) {
    if (!node) return;

    if (cmark_node_get_type(node) == CMARK_NODE_HEADING) {
        int level = cmark_node_get_heading_level(node);
        std::string text;
        collect_text(node, text);

        // Generate slug id from text.
        std::string id;
        id.reserve(text.size());
        for (char c : text) {
            if (c == ' ' || c == '_') id += '-';
            else if (c >= 'A' && c <= 'Z') id += static_cast<char>(c + 32);
            else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')
                id += c;
            // Skip other characters.
        }

        toc.push_back({level, std::move(text), std::move(id)});
    }

    for (cmark_node* child = cmark_node_first_child(node);
         child;
         child = cmark_node_next(child)) {
        collect_headings(child, toc);
    }
}

// Recursively walk AST and collect links.
void collect_links(cmark_node* node, std::vector<LinkInfo>& links) {
    if (!node) return;

    auto type = cmark_node_get_type(node);
    if (type == CMARK_NODE_LINK || type == CMARK_NODE_IMAGE) {
        const char* url = cmark_node_get_url(node);
        std::string text;
        collect_text(node, text);
        links.push_back({
            std::move(text),
            url ? std::string(url) : std::string{},
            type == CMARK_NODE_IMAGE
        });
    }

    for (cmark_node* child = cmark_node_first_child(node);
         child;
         child = cmark_node_next(child)) {
        collect_links(child, links);
    }
}

}  // namespace

std::string to_html(std::string_view markdown, const MarkdownOptions& opts) {
    // Register GFM extensions (idempotent — cmark handles duplicates).
    cmark_gfm_core_extensions_ensure_registered();

    int options = build_options(opts);

    cmark_parser* parser = cmark_parser_new(options);
    if (!parser) {
        throw std::runtime_error("cmark: failed to create parser");
    }

    // Attach GFM extensions.
    cmark_syntax_extension* ext_table = cmark_find_syntax_extension("table");
    cmark_syntax_extension* ext_autolink = cmark_find_syntax_extension("autolink");
    cmark_syntax_extension* ext_strikethrough = cmark_find_syntax_extension("strikethrough");
    cmark_syntax_extension* ext_tagfilter = cmark_find_syntax_extension("tagfilter");

    if (opts.table && ext_table)
        cmark_parser_attach_syntax_extension(parser, ext_table);
    if (opts.autolink && ext_autolink)
        cmark_parser_attach_syntax_extension(parser, ext_autolink);
    if (opts.strikethrough && ext_strikethrough)
        cmark_parser_attach_syntax_extension(parser, ext_strikethrough);
    if (opts.tagfilter && ext_tagfilter)
        cmark_parser_attach_syntax_extension(parser, ext_tagfilter);

    // Feed input.
    cmark_parser_feed(parser, markdown.data(), markdown.size());
    cmark_node* doc = cmark_parser_finish(parser);

    if (!doc) {
        cmark_parser_free(parser);
        throw std::runtime_error("cmark: failed to parse markdown");
    }

    // Render to HTML — must happen BEFORE parser_free (ext list is owned by parser).
    char* html = cmark_render_html(doc, options,
                                    cmark_parser_get_syntax_extensions(parser));
    cmark_parser_free(parser);

    std::string result;
    if (html) {
        result = html;
        free(html);
    }
    cmark_node_free(doc);

    return result;
}

std::string to_plaintext(std::string_view markdown) {
    cmark_gfm_core_extensions_ensure_registered();

    cmark_parser* parser = cmark_parser_new(CMARK_OPT_DEFAULT);
    if (!parser) {
        throw std::runtime_error("cmark: failed to create parser");
    }

    cmark_parser_feed(parser, markdown.data(), markdown.size());
    cmark_node* doc = cmark_parser_finish(parser);
    cmark_parser_free(parser);

    if (!doc) {
        throw std::runtime_error("cmark: failed to parse markdown");
    }

    std::string text;
    collect_text(doc, text);
    cmark_node_free(doc);
    return text;
}

std::vector<TocEntry> extract_toc(std::string_view markdown) {
    cmark_gfm_core_extensions_ensure_registered();

    cmark_parser* parser = cmark_parser_new(CMARK_OPT_DEFAULT);
    if (!parser) {
        throw std::runtime_error("cmark: failed to create parser");
    }

    cmark_parser_feed(parser, markdown.data(), markdown.size());
    cmark_node* doc = cmark_parser_finish(parser);
    cmark_parser_free(parser);

    if (!doc) {
        throw std::runtime_error("cmark: failed to parse markdown");
    }

    std::vector<TocEntry> toc;
    collect_headings(doc, toc);
    cmark_node_free(doc);
    return toc;
}

std::vector<LinkInfo> extract_links(std::string_view markdown) {
    cmark_gfm_core_extensions_ensure_registered();

    cmark_parser* parser = cmark_parser_new(CMARK_OPT_DEFAULT);
    if (!parser) {
        throw std::runtime_error("cmark: failed to create parser");
    }

    // Need autolink extension to detect bare URLs.
    cmark_syntax_extension* ext_autolink = cmark_find_syntax_extension("autolink");
    if (ext_autolink) {
        cmark_parser_attach_syntax_extension(parser, ext_autolink);
    }

    cmark_parser_feed(parser, markdown.data(), markdown.size());
    cmark_node* doc = cmark_parser_finish(parser);
    cmark_parser_free(parser);

    if (!doc) {
        throw std::runtime_error("cmark: failed to parse markdown");
    }

    std::vector<LinkInfo> links;
    collect_links(doc, links);
    cmark_node_free(doc);
    return links;
}

}  // namespace nift::markdown
