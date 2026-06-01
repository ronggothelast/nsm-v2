/// @file test_http_server.cpp

#include <httplib.h>

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

#include "nift/server/http_server.hpp"

using namespace nift::server;
namespace fs = std::filesystem;

namespace {

struct TempRoot {
  fs::path root;
  TempRoot() {
    root = fs::temp_directory_path() / ("nift_http_" + std::to_string(std::rand()));
    fs::create_directories(root);
  }
  ~TempRoot() {
    std::error_code ec;
    fs::remove_all(root, ec);
  }
  void write(const std::string& name, const std::string& body) {
    auto p = root / name;
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out.write(body.data(), static_cast<std::streamsize>(body.size()));
  }
};

ServerConfig make_cfg(const fs::path& root) {
  ServerConfig c;
  c.host = "127.0.0.1";
  c.port = 0;  // ephemeral
  c.root = ::nift::core::Path(root.string());
  return c;
}

}  // namespace

TEST_CASE("HttpServer: serves a static html", "[server][http]") {
  TempRoot t;
  t.write("index.html", "<!doctype html><body>hello</body>");

  HttpServer s(make_cfg(t.root));
  REQUIRE(s.start());
  REQUIRE(s.is_running());
  auto port = s.bound_port();
  REQUIRE(port != 0);

  httplib::Client cli("127.0.0.1", port);
  auto resp = cli.Get("/index.html");
  REQUIRE(resp);
  CHECK(resp->status == 200);
  CHECK(resp->body.find("hello") != std::string::npos);
  CHECK(resp->body.find("livereload.js") != std::string::npos);

  s.stop();
  CHECK_FALSE(s.is_running());
}

TEST_CASE("HttpServer: trailing slash -> index.html", "[server][http]") {
  TempRoot t;
  t.write("index.html", "<body>root</body>");

  HttpServer s(make_cfg(t.root));
  REQUIRE(s.start());
  httplib::Client cli("127.0.0.1", s.bound_port());
  auto resp = cli.Get("/");
  REQUIRE(resp);
  CHECK(resp->status == 200);
  CHECK(resp->body.find("root") != std::string::npos);
}

TEST_CASE("HttpServer: 404 for missing", "[server][http]") {
  TempRoot t;
  HttpServer s(make_cfg(t.root));
  REQUIRE(s.start());
  httplib::Client cli("127.0.0.1", s.bound_port());
  auto resp = cli.Get("/nope.html");
  REQUIRE(resp);
  CHECK(resp->status == 404);
}

TEST_CASE("HttpServer: livereload token bumps", "[server][http]") {
  TempRoot t;
  t.write("index.html", "<body>x</body>");
  HttpServer s(make_cfg(t.root));
  REQUIRE(s.start());

  httplib::Client cli("127.0.0.1", s.bound_port());
  auto r1 = cli.Get("/__nift/livereload");
  REQUIRE(r1);
  std::string t1 = r1->body;

  s.notify_rebuild();
  s.notify_rebuild();

  auto r2 = cli.Get("/__nift/livereload");
  REQUIRE(r2);
  CHECK(r2->body != t1);
}

TEST_CASE("HttpServer: serves CSS with correct MIME", "[server][http]") {
  TempRoot t;
  t.write("style.css", "body{color:red}");
  HttpServer s(make_cfg(t.root));
  REQUIRE(s.start());
  httplib::Client cli("127.0.0.1", s.bound_port());
  auto resp = cli.Get("/style.css");
  REQUIRE(resp);
  CHECK(resp->status == 200);
  CHECK(resp->get_header_value("Content-Type").find("text/css") != std::string::npos);
}

TEST_CASE("HttpServer: livereload script endpoint", "[server][http]") {
  TempRoot t;
  HttpServer s(make_cfg(t.root));
  REQUIRE(s.start());
  httplib::Client cli("127.0.0.1", s.bound_port());
  auto resp = cli.Get("/__nift/livereload.js");
  REQUIRE(resp);
  CHECK(resp->status == 200);
  CHECK(resp->body.find("setInterval") != std::string::npos);
}

TEST_CASE("inject_livereload_script: inserts before </body>", "[server][http]") {
  auto out = inject_livereload_script("<html><body>hi</body></html>", "/livereload.js");
  CHECK(out.find("livereload.js") != std::string::npos);
  // Inserted before </body>:
  auto a = out.find("livereload.js");
  auto b = out.find("</body>");
  REQUIRE(a != std::string::npos);
  REQUIRE(b != std::string::npos);
  CHECK(a < b);
}

TEST_CASE("inject_livereload_script: leaves non-html alone", "[server][http]") {
  auto out = inject_livereload_script("plain text", "/x");
  CHECK(out == "plain text");
}

TEST_CASE("HttpServer: stop is idempotent", "[server][http]") {
  TempRoot t;
  HttpServer s(make_cfg(t.root));
  REQUIRE(s.start());
  s.stop();
  s.stop();
  SUCCEED("no hang or crash");
}

// Security regression — Sentinel 2026-06-01.
// The dev server must refuse every request whose final path escapes the
// served root. We plant a "secret" sibling of the root and assert that no
// raw, percent-encoded, or mixed traversal payload can read it.
TEST_CASE("HttpServer: rejects path traversal", "[server][http][security]") {
  TempRoot tr;
  // Plant a secret OUTSIDE the served root but in the same temp dir.
  auto secret_path = tr.root.parent_path() /
                     ("sentinel_secret_" + std::to_string(std::rand()) + ".txt");
  {
    std::ofstream out(secret_path, std::ios::binary);
    out << "TOPSECRET";
  }
  tr.write("public.html", "<p>ok</p>");

  HttpServer s(make_cfg(tr.root));
  REQUIRE(s.start().has_value());

  httplib::Client cli("127.0.0.1", s.bound_port());
  cli.set_connection_timeout(std::chrono::seconds(2));

  const std::string secret_name = secret_path.filename().string();
  // Each payload must NOT yield a 200 with the secret body.
  const std::vector<std::string> payloads = {
      "/../" + secret_name,
      "/../../" + secret_name,
      "/foo/../../" + secret_name,
      "/%2e%2e/" + secret_name,    // encoded ..
      "/%2E%2E%2f" + secret_name,  // mixed-case + slash
      "/..%2f" + secret_name,      // partially encoded
      "/..\\" + secret_name,       // backslash variant
      "/foo/%2e%2e/%2e%2e/" + secret_name,
      "/./../" + secret_name,
  };

  for (const auto& p : payloads) {
    auto res = cli.Get(p.c_str());
    INFO("payload = " << p);
    if (res) {
      // If the server replied, it must not be 200 and must not contain the
      // secret. A 404/400 is the expected outcome.
      REQUIRE(res->status != 200);
      REQUIRE(res->body.find("TOPSECRET") == std::string::npos);
    }
    // No response at all is also acceptable (connection rejected).
  }

  // Sanity: the legitimate URL still works.
  auto ok = cli.Get("/public.html");
  REQUIRE(ok);
  REQUIRE(ok->status == 200);

  s.stop();
  std::error_code ec;
  std::filesystem::remove(secret_path, ec);
}
