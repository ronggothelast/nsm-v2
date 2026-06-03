/// @file http_server.cpp
/// @brief HttpServer implementation via cpp-httplib.

#include "nift/server/http_server.hpp"

#include <httplib.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <system_error>
#include <thread>
#include <utility>

#include "nift/core/filesystem.hpp"

namespace nift::server {

class HttpServerImpl {
 public:
  ServerConfig config;
  httplib::Server srv;
  std::thread thread;
  std::atomic<bool> running{false};
  std::atomic<std::uint64_t> generation{0};
  std::atomic<std::uint16_t> bound_port{0};

  // SSE client tracking.
  std::mutex sse_mu;
  std::vector<httplib::DataSink*> sse_clients;

  std::string livereload_js() const {
    std::string out =
        "(function(){var useSSE=false;function trySSE(){try{"
        "var es=new EventSource('" +
        config.livereload_token_path +
        "');"
        "es.onmessage=function(){window.location.reload();};"
        "es.onerror=function(){es.close();tryPoll();};"
        "useSSE=true;}catch(e){tryPoll();}}"
        "function tryPoll(){var last=null;function poll(){fetch('" +
        config.livereload_token_path +
        "/poll',{cache:'no-store'})"
        ".then(function(r){return r.text();}).then(function(t){"
        "if(last===null){last=t;return;}if(t!==last)window.location.reload();})"
        ".catch(function(){});}setInterval(poll,500);poll();}"
        "trySSE();})();";
    return out;
  }

  void push_reload_event() {
    std::lock_guard<std::mutex> lk(sse_mu);
    std::string data = "data: reload\n\n";
    std::vector<httplib::DataSink*> dead;
    for (auto* sink : sse_clients) {
      if (!sink->write(data.data(), data.size())) {
        dead.push_back(sink);
      }
    }
    // Remove dead clients.
    for (auto* d : dead) {
      sse_clients.erase(std::remove(sse_clients.begin(), sse_clients.end(), d),
                        sse_clients.end());
    }
  }
};

namespace {

/// Validate and sanitize URL path. Returns false if path is invalid or unsafe.
/// On success, url_path is percent-decoded and verified to contain no traversal.
bool validate_path(std::string& url_path) {
  if (url_path.empty() || url_path[0] != '/')
    return false;

  // Strip query string.
  auto q = url_path.find('?');
  if (q != std::string::npos)
    url_path.resize(q);

  // (1) NUL byte → reject.
  if (url_path.find('\0') != std::string::npos)
    return false;

  // (2) Percent-decode in place. We use a tiny inline decoder to avoid
  //     pulling in extra deps. Invalid escapes leave the byte literal.
  auto hex = [](char c) -> int {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'a' && c <= 'f')
      return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
      return 10 + (c - 'A');
    return -1;
  };
  std::string decoded;
  decoded.reserve(url_path.size());
  for (std::size_t i = 0; i < url_path.size(); ++i) {
    if (url_path[i] == '%' && i + 2 < url_path.size()) {
      int hi = hex(url_path[i + 1]);
      int lo = hex(url_path[i + 2]);
      if (hi >= 0 && lo >= 0) {
        char c = static_cast<char>((hi << 4) | lo);
        if (c == '\0')
          return false;  // NUL via encoding
        decoded.push_back(c);
        i += 2;
        continue;
      }
    }
    decoded.push_back(url_path[i]);
  }
  url_path = std::move(decoded);

  // (3) Reject any `..` segment, treating both `/` and `\` as separators
  //     so Windows-style traversal cannot slip through.
  {
    std::string seg;
    auto is_sep = [](char c) { return c == '/' || c == '\\'; };
    for (std::size_t i = 0; i <= url_path.size(); ++i) {
      char c = (i == url_path.size()) ? '/' : url_path[i];
      if (is_sep(c)) {
        if (seg == "..")
          return false;
        seg.clear();
      } else {
        seg.push_back(c);
      }
    }
  }

  return true;
}

/// Resolve MIME type from URL path extension.
std::string resolve_mime(const std::string& url_path) {
  auto pos = url_path.rfind('.');
  std::string ext = (pos == std::string::npos) ? "" : url_path.substr(pos);
  if (ext == ".html" || ext == ".htm")
    return "text/html; charset=utf-8";
  else if (ext == ".css")
    return "text/css; charset=utf-8";
  else if (ext == ".js")
    return "application/javascript; charset=utf-8";
  else if (ext == ".json")
    return "application/json; charset=utf-8";
  else if (ext == ".png")
    return "image/png";
  else if (ext == ".jpg" || ext == ".jpeg")
    return "image/jpeg";
  else if (ext == ".svg")
    return "image/svg+xml";
  else if (ext == ".ico")
    return "image/x-icon";
  else if (ext == ".webp")
    return "image/webp";
  else if (ext == ".woff2")
    return "font/woff2";
  else if (ext == ".txt")
    return "text/plain; charset=utf-8";
  else
    return "application/octet-stream";
}

/// Resolve filesystem path, verify containment, and read file.
/// Returns true on success with out_body populated.
bool read_file_response(const ::nift::core::Path& root, const std::string& url_path,
                        std::string& out_body) {
  std::string fs_path = root.str() + url_path;
  ::nift::core::Path candidate(fs_path);

  if (!::nift::core::file_exists(candidate)) {
    // Try .html append (clean URLs)
    auto with_html = ::nift::core::Path(fs_path + ".html");
    if (::nift::core::file_exists(with_html)) {
      candidate = with_html;
    } else {
      return false;
    }
  }

  // (4) Canonicalize and confirm containment. weakly_canonical resolves
  //     symlinks and `.`/`..` against an existing prefix; we paired it
  //     with the explicit `..` rejection above so unresolved relatives
  //     (rare on a static tree) still fail closed.
  std::error_code ec;
  auto canon_root = std::filesystem::weakly_canonical(root.native(), ec);
  if (ec)
    return false;
  auto canon_candidate = std::filesystem::weakly_canonical(candidate.native(), ec);
  if (ec)
    return false;

  // Lexical containment check using path iterators (no string-prefix
  // pitfalls like `/srv/site` vs `/srv/site-evil`).
  auto rit = canon_root.begin();
  auto cit = canon_candidate.begin();
  for (; rit != canon_root.end(); ++rit, ++cit) {
    if (cit == canon_candidate.end() || *rit != *cit)
      return false;
  }

  auto content = ::nift::core::read_file(candidate);
  if (!content)
    return false;
  out_body = std::move(*content);
  return true;
}

/// Try to read a file relative to the root, with a few index fallbacks.
///
/// SECURITY: this resolver is the dev-server's only authority boundary. It
/// must reject every request whose final filesystem path escapes the served
/// root, otherwise an attacker (any browser tab on the same host, or any LAN
/// peer when the server is bound to 0.0.0.0) can read arbitrary files such
/// as `~/.ssh/id_rsa`, `/etc/passwd`, or project source outside `output/`.
///
/// Defenses applied:
///   1. Reject NUL bytes outright (path-truncation tricks).
///   2. Decode percent-encoded sequences before any `..` checks so that
///      `%2e%2e%2f`, `%2E%2E%5C`, and similar tricks cannot bypass us.
///   3. Reject any decoded segment equal to ".." — we never rely on
///      `weakly_canonical` alone because the candidate file may not exist
///      yet for `.html`-append fallback.
///   4. After resolving, canonicalize both root and candidate with
///      `weakly_canonical` and verify the candidate path is lexically
///      contained within the root. Symlinks therefore cannot escape either.
bool resolve_static(const ::nift::core::Path& root, std::string url_path,
                    std::string& out_body, std::string& out_mime) {
  if (!validate_path(url_path))
    return false;

  // Trailing slash → index.html
  if (url_path.back() == '/')
    url_path += "index.html";

  if (!read_file_response(root, url_path, out_body))
    return false;

  out_mime = resolve_mime(url_path);
  return true;
}

}  // namespace

HttpServer::HttpServer(ServerConfig config)
    : impl_(std::make_unique<HttpServerImpl>()) {
  impl_->config = std::move(config);

  // GET /__nift/livereload — SSE endpoint (text/event-stream).
  // Uses chunked transfer encoding with a content provider that blocks
  // until the client disconnects.
  impl_->srv.Get(impl_->config.livereload_token_path, [this](const httplib::Request&,
                                                             httplib::Response& res) {
    res.set_chunked_content_provider(
        "text/event-stream", [this](size_t, httplib::DataSink& sink) -> bool {
          // Send initial connection event.
          std::string init = "data: connected\n\n";
          sink.write(init.data(), init.size());

          // Register for push events.
          {
            std::lock_guard<std::mutex> lk(impl_->sse_mu);
            impl_->sse_clients.push_back(&sink);
          }

          // Block until client disconnects or server stops.
          constexpr auto kSSEKeepAliveInterval = std::chrono::milliseconds(100);
          while (impl_->running.load() && sink.is_writable()) {
            std::this_thread::sleep_for(kSSEKeepAliveInterval);
          }
          return false;  // End stream.
        });
  });

  // GET /__nift/livereload/poll — polling fallback (returns token).
  impl_->srv.Get(impl_->config.livereload_token_path + "/poll",
                 [this](const httplib::Request&, httplib::Response& res) {
                   res.set_content(std::to_string(impl_->generation.load()),
                                   "text/plain");
                 });

  // /__nift/livereload.js → injected script.
  impl_->srv.Get(impl_->config.livereload_script_path,
                 [this](const httplib::Request&, httplib::Response& res) {
                   res.set_content(impl_->livereload_js(), "application/javascript");
                 });

  // Catch-all static file handler.
  impl_->srv.Get(".*", [this](const httplib::Request& req, httplib::Response& res) {
    std::string body, mime;
    if (!resolve_static(impl_->config.root, req.path, body, mime)) {
      res.status = 404;
      res.set_content("Not Found", "text/plain");
      return;
    }

    if (impl_->config.inject_livereload &&
        mime.find("text/html") != std::string::npos) {
      body = inject_livereload_script(body, impl_->config.livereload_script_path);
    }
    res.set_content(body, mime.c_str());
  });
}

HttpServer::~HttpServer() {
  stop();
}

::nift::Expected<std::monostate, ::nift::Error> HttpServer::start() {
  if (impl_->running.load())
    return std::monostate{};

  // Bind explicitly so we can capture the chosen port (port=0 → ephemeral).
  int actual_port;
  if (impl_->config.port == 0) {
    actual_port = impl_->srv.bind_to_any_port(impl_->config.host);
  } else {
    actual_port = impl_->srv.bind_to_port(impl_->config.host, impl_->config.port)
                      ? static_cast<int>(impl_->config.port)
                      : -1;
  }
  if (actual_port < 0) {
    return ::nift::unexpected<::nift::Error>(::nift::Error::io_error);
  }
  impl_->bound_port.store(static_cast<std::uint16_t>(actual_port));

  impl_->running.store(true);
  impl_->thread = std::thread([this]() { impl_->srv.listen_after_bind(); });

  // Wait briefly for the server to start accepting.
  constexpr int kStartupMaxAttempts = 100;
  constexpr auto kStartupPollInterval = std::chrono::milliseconds(2);
  for (int i = 0; i < kStartupMaxAttempts; ++i) {
    if (impl_->srv.is_running())
      break;
    std::this_thread::sleep_for(kStartupPollInterval);
  }
  return std::monostate{};
}

void HttpServer::stop() {
  if (!impl_->running.exchange(false))
    return;
  impl_->srv.stop();
  if (impl_->thread.joinable())
    impl_->thread.join();
}

bool HttpServer::is_running() const noexcept {
  return impl_->running.load() && impl_->srv.is_running();
}

void HttpServer::notify_rebuild() {
  impl_->generation.fetch_add(1, std::memory_order_acq_rel);
  // Push SSE reload event to all connected clients.
  impl_->push_reload_event();
}

std::uint64_t HttpServer::generation() const noexcept {
  return impl_->generation.load();
}

std::uint16_t HttpServer::bound_port() const noexcept {
  return impl_->bound_port.load();
}

std::string inject_livereload_script(std::string_view html,
                                     std::string_view script_path) {
  std::string s(html);
  std::string tag = "<script src=\"" + std::string(script_path) + "\"></script>";
  // Find last </body> (case-insensitive small check).
  auto find_ci = [](const std::string& haystack, const std::string& needle) {
    auto n = haystack.size();
    auto m = needle.size();
    if (m > n)
      return std::string::npos;
    for (std::size_t i = 0; i + m <= n; ++i) {
      bool ok = true;
      for (std::size_t j = 0; j < m; ++j) {
        char a = haystack[i + j];
        char b = needle[j];
        if (a >= 'A' && a <= 'Z')
          a = a + ('a' - 'A');
        if (b >= 'A' && b <= 'Z')
          b = b + ('a' - 'A');
        if (a != b) {
          ok = false;
          break;
        }
      }
      if (ok)
        return i;
    }
    return std::string::npos;
  };
  auto pos = find_ci(s, "</body>");
  if (pos == std::string::npos)
    return s;
  s.insert(pos, tag);
  return s;
}

}  // namespace nift::server
