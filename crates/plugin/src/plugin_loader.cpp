/// @file plugin_loader.cpp

#include "nift/plugin/plugin_loader.hpp"

#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#define NIFT_DL_OPEN(p) (void*)LoadLibraryA((p))
#define NIFT_DL_SYM(h, n) (void*)GetProcAddress((HMODULE)(h), (n))
#define NIFT_DL_CLOSE(h) FreeLibrary((HMODULE)(h))
#else
#include <dlfcn.h>
#define NIFT_DL_OPEN(p) dlopen((p), RTLD_NOW | RTLD_LOCAL)
#define NIFT_DL_SYM(h, n) dlsym((h), (n))
#define NIFT_DL_CLOSE(h) dlclose((h))
#endif

namespace nift::plugin {

LoadedPlugin::LoadedPlugin() = default;

LoadedPlugin::~LoadedPlugin() {
  if (handle_)
    NIFT_DL_CLOSE(handle_);
}

LoadedPlugin::LoadedPlugin(LoadedPlugin&& other) noexcept
    : handle_(other.handle_), vtable_(other.vtable_) {
  other.handle_ = nullptr;
  other.vtable_ = nullptr;
}

LoadedPlugin& LoadedPlugin::operator=(LoadedPlugin&& other) noexcept {
  if (this != &other) {
    if (handle_)
      NIFT_DL_CLOSE(handle_);
    handle_ = other.handle_;
    vtable_ = other.vtable_;
    other.handle_ = nullptr;
    other.vtable_ = nullptr;
  }
  return *this;
}

std::string_view LoadedPlugin::name() const noexcept {
  if (!vtable_ || !vtable_->info)
    return {};
  return vtable_->info->name ? vtable_->info->name : "";
}

std::string_view LoadedPlugin::version() const noexcept {
  if (!vtable_ || !vtable_->info)
    return {};
  return vtable_->info->version ? vtable_->info->version : "";
}

std::vector<std::string> LoadedPlugin::directives() const {
  std::vector<std::string> out;
  if (!vtable_ || !vtable_->directives)
    return out;
  for (std::size_t i = 0; i < vtable_->directives->count; ++i) {
    const char* n = vtable_->directives->names[i];
    if (n)
      out.emplace_back(n);
  }
  return out;
}

::nift::Expected<std::string, ::nift::Error> LoadedPlugin::render(
    std::string_view directive_name, std::string_view args) const {
  if (!vtable_ || !vtable_->render) {
    return ::nift::unexpected<::nift::Error>(::nift::Error::invalid_argument);
  }
  std::string name(directive_name);
  std::string arg(args);
  char* err = nullptr;
  char* result = vtable_->render(name.c_str(), arg.c_str(), &err);
  if (!result) {
    if (err && vtable_->free_result)
      vtable_->free_result(err);
    return ::nift::unexpected<::nift::Error>(::nift::Error::unknown);
  }
  std::string out(result);
  if (vtable_->free_result)
    vtable_->free_result(result);
  return out;
}

// ─── PluginRegistry ──────────────────────────────────────────────────

::nift::Expected<std::monostate, ::nift::Error> PluginRegistry::load(
    const ::nift::core::Path& dll_path) {
  void* handle = NIFT_DL_OPEN(dll_path.str().c_str());
  if (!handle) {
    return ::nift::unexpected<::nift::Error>(::nift::Error::not_found);
  }

  auto init =
      reinterpret_cast<NiftPluginInitFn>(NIFT_DL_SYM(handle, "nift_plugin_init"));
  if (!init) {
    NIFT_DL_CLOSE(handle);
    return ::nift::unexpected<::nift::Error>(::nift::Error::invalid_argument);
  }

  const NiftPluginVtable* vt = init();
  if (!vt || vt->abi_version != NIFT_PLUGIN_ABI_VERSION || !vt->info ||
      !vt->directives || !vt->render) {
    NIFT_DL_CLOSE(handle);
    return ::nift::unexpected<::nift::Error>(::nift::Error::invalid_argument);
  }

  auto plug = std::unique_ptr<LoadedPlugin>(new LoadedPlugin());
  plug->handle_ = handle;
  plug->vtable_ = vt;

  // Register directives.
  for (std::size_t i = 0; i < vt->directives->count; ++i) {
    const char* n = vt->directives->names[i];
    if (n)
      by_directive_[std::string(n)] = plug.get();
  }
  plugins_.push_back(std::move(plug));
  return std::monostate{};
}

bool PluginRegistry::has_directive(std::string_view name) const {
  return by_directive_.find(std::string(name)) != by_directive_.end();
}

::nift::Expected<std::string, ::nift::Error> PluginRegistry::render(
    std::string_view directive_name, std::string_view args) const {
  auto it = by_directive_.find(std::string(directive_name));
  if (it == by_directive_.end()) {
    return ::nift::unexpected<::nift::Error>(::nift::Error::not_found);
  }
  return it->second->render(directive_name, args);
}

std::vector<std::string> PluginRegistry::names() const {
  std::vector<std::string> out;
  out.reserve(plugins_.size());
  for (const auto& p : plugins_)
    out.emplace_back(p->name());
  return out;
}

}  // namespace nift::plugin
