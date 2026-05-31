/// @file http_server.cpp
/// @brief HttpServer implementation via cpp-httplib.

#include "nift/server/http_server.hpp"

#include <httplib.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>

#include "nift/core/filesystem.hpp"

namespace nift::server {

constexpr const char* kLivereloadJs = R"JS(
(function() {
  var lastToken = null;
  function poll() {
    fetch('__TOKEN_PATH__', { cache: 'no-store' })
      .then(function(r) { return r.text(); })
      .then(function(t) {
        if (lastToken === null) { lastToken = t; return; }
        if (t !== lastToken) { window.location.reload(); }
      })
      .catch(function() {});
  }
  setInterval(poll, 500);
  poll();
})();
)JS";

class HttpServerImpl {
 public:
  ServerConfig config;
  httplib::Server srv;
  std::thread thread;
  std::atomic<bool> running{false};
  std::atomic<std::uint64_t> generation{0};
  std::atomic<std::uint16_t> bound_port{0};

  std::string livereload_js() const {
    std::string js(kLivereloadJs);
    auto pos = js.find("__TOKEN_PATH__");
    if (pos != std::string::npos) {
      js.replace(pos, std::string("__TOKEN_PATH__").size(),
                 "'" + config.livereload_token_path + "'");
      // Re-quote: pattern was '__TOKEN_PATH__' originally with quotes,
      // we replace the token only (between the quotes).
    }
    // Simpler approach: rebuild without placeholder gymnastics.
    std::string out =
        "(function(){var lastToken=null;function poll(){fetch('" +
        config.livereload_token_path +
        "',{cache:'no-store'}).then(function(r){return r.text();}).then("
        "function(t){if(lastToken===null){lastToken=t;return;}"
        "if(t!==lastToken){window.location.reload();}}).catch(function(){});}"
        "setInterval(poll,500);poll();})();";
    return out;
  }
};

namespace {

/// Try to read a file relative to the root, with a few index fallbacks.
bool resolve_static(const ::nift::core::Path& root, std::string url_path,
                    std::string& out_body, std::string& out_mime) {
  if (url_path.empty() || url_path[0] != '/') return false;

  // Strip query string.
  auto q = url_path.find('?');
  if (q != std::string::npos) url_path.resize(q);

  // Trailing slash → index.html
  if (url_path.back() == '/') url_path += "index.html";

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

  auto content = ::nift::core::read_file(candidate);
  if (!content) return false;
  out_body = std::move(*content);

  // Tiny MIME map.
  auto pos = url_path.rfind('.');
  std::string ext = (pos == std::string::npos) ? "" : url_path.substr(pos);
  if (ext == ".html" || ext == ".htm") out_mime = "text/html; charset=utf-8";
  else if (ext == ".css") out_mime = "text/css; charset=utf-8";
  else if (ext == ".js") out_mime = "application/javascript; charset=utf-8";
  else if (ext == ".json") out_mime = "application/json; charset=utf-8";
  else if (ext == ".png") out_mime = "image/png";
  else if (ext == ".jpg" || ext == ".jpeg") out_mime = "image/jpeg";
  else if (ext == ".svg") out_mime = "image/svg+xml";
  else if (ext == ".ico") out_mime = "image/x-icon";
  else if (ext == ".webp") out_mime = "image/webp";
  else if (ext == ".woff2") out_mime = "font/woff2";
  else if (ext == ".txt") out_mime = "text/plain; charset=utf-8";
  else out_mime = "application/octet-stream";
  return true;
}

}  // namespace

HttpServer::HttpServer(ServerConfig config)
    : impl_(std::make_unique<HttpServerImpl>()) {
  impl_->config = std::move(config);

  // /__nift/livereload → current generation token (plain text).
  impl_->srv.Get(impl_->config.livereload_token_path,
                 [this](const httplib::Request&, httplib::Response& res) {
                   res.set_content(
                       std::to_string(impl_->generation.load()),
                       "text/plain");
                 });

  // /__nift/livereload.js → injected script.
  impl_->srv.Get(impl_->config.livereload_script_path,
                 [this](const httplib::Request&, httplib::Response& res) {
                   res.set_content(impl_->livereload_js(),
                                   "application/javascript");
                 });

  // Catch-all static file handler.
  impl_->srv.Get(".*", [this](const httplib::Request& req,
                              httplib::Response& res) {
    std::string body, mime;
    if (!resolve_static(impl_->config.root, req.path, body, mime)) {
      res.status = 404;
      res.set_content("Not Found", "text/plain");
      return;
    }

    if (impl_->config.inject_livereload &&
        mime.find("text/html") != std::string::npos) {
      body = inject_livereload_script(body,
                                      impl_->config.livereload_script_path);
    }
    res.set_content(body, mime.c_str());
  });
}

HttpServer::~HttpServer() { stop(); }

::nift::Expected<std::monostate, ::nift::Error> HttpServer::start() {
  if (impl_->running.load()) return std::monostate{};

  // Bind explicitly so we can capture the chosen port (port=0 → ephemeral).
  int actual_port = impl_->srv.bind_to_any_port(impl_->config.host);
  if (actual_port < 0) {
    return ::nift::unexpected<::nift::Error>(::nift::Error::io_error);
  }
  impl_->bound_port.store(static_cast<std::uint16_t>(actual_port));

  impl_->running.store(true);
  impl_->thread = std::thread([this]() {
    impl_->srv.listen_after_bind();
  });

  // Wait briefly for the server to start accepting.
  for (int i = 0; i < 100; ++i) {
    if (impl_->srv.is_running()) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  return std::monostate{};
}

void HttpServer::stop() {
  if (!impl_->running.exchange(false)) return;
  impl_->srv.stop();
  if (impl_->thread.joinable()) impl_->thread.join();
}

bool HttpServer::is_running() const noexcept {
  return impl_->running.load() && impl_->srv.is_running();
}

void HttpServer::notify_rebuild() {
  impl_->generation.fetch_add(1, std::memory_order_acq_rel);
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
  std::string tag =
      "<script src=\"" + std::string(script_path) + "\"></script>";
  // Find last </body> (case-insensitive small check).
  auto find_ci = [](const std::string& haystack, const std::string& needle) {
    auto n = haystack.size();
    auto m = needle.size();
    if (m > n) return std::string::npos;
    for (std::size_t i = 0; i + m <= n; ++i) {
      bool ok = true;
      for (std::size_t j = 0; j < m; ++j) {
        char a = haystack[i + j];
        char b = needle[j];
        if (a >= 'A' && a <= 'Z') a = a + ('a' - 'A');
        if (b >= 'A' && b <= 'Z') b = b + ('a' - 'A');
        if (a != b) {
          ok = false;
          break;
        }
      }
      if (ok) return i;
    }
    return std::string::npos;
  };
  auto pos = find_ci(s, "</body>");
  if (pos == std::string::npos) return s;
  s.insert(pos, tag);
  return s;
}

}  // namespace nift::server
