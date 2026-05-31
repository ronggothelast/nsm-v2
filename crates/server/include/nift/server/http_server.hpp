/// @file http_server.hpp
/// @brief Static dev server with hot-reload WebSocket-style polling.
///
/// Wraps `cpp-httplib` for serving the build output directory over HTTP/1.1.
/// HTTP/2 is deferred until libnghttp2 is plumbed (Phase 6).
///
/// Hot-reload model:
///   - Server exposes `GET /__nift/livereload` returning the current build
///     generation token (monotonic counter incremented each rebuild).
///   - Client-side JS polls every 500 ms; if the token changes, reload.
///   - This is simpler than raw WebSockets and works behind any proxy.

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include "nift/core/path.hpp"
#include "nift/core/types.hpp"

namespace nift::server {

struct ServerConfig {
  std::string host = "127.0.0.1";
  std::uint16_t port = 8080;
  ::nift::core::Path root;          ///< Directory to serve (build output)
  bool inject_livereload = true;    ///< Inject reload script into HTML
  std::string livereload_script_path = "/__nift/livereload.js";
  std::string livereload_token_path = "/__nift/livereload";
};

/// @brief Forward-declared impl — hides cpp-httplib include.
class HttpServerImpl;

/// @brief HTTP dev server.
class HttpServer {
 public:
  explicit HttpServer(ServerConfig config);
  ~HttpServer();

  HttpServer(const HttpServer&) = delete;
  HttpServer& operator=(const HttpServer&) = delete;

  /// Start listening. Non-blocking — spawns a background thread.
  /// Returns Error if bind/listen fails.
  ::nift::Expected<std::monostate, ::nift::Error> start();

  /// Stop the server. Idempotent.
  void stop();

  /// Whether the server is currently listening.
  bool is_running() const noexcept;

  /// Bump the live-reload generation token. Call after each rebuild.
  void notify_rebuild();

  /// Current generation token (for tests).
  std::uint64_t generation() const noexcept;

  /// Bound port (useful when port=0 → ephemeral).
  std::uint16_t bound_port() const noexcept;

 private:
  std::unique_ptr<HttpServerImpl> impl_;
};

/// @brief Inject a livereload script tag before `</body>` in HTML payloads.
/// Returns input unchanged if no `</body>` is present.
std::string inject_livereload_script(std::string_view html,
                                     std::string_view script_path);

}  // namespace nift::server
