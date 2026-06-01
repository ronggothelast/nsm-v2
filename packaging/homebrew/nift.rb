# Homebrew formula for nift v2.
# Place at: $(brew --repository)/Library/Taps/yourtap/Formula/nift.rb

class Nift < Formula
  desc "Modern C++ static site generator (Nift v2)"
  homepage "https://github.com/ronggothelast/nsm-v2"
  url "https://github.com/ronggothelast/nsm-v2.git",
      branch: "main"
  version "2.0.0-dev"
  license "MIT"

  depends_on "cmake" => :build
  depends_on "ninja" => :build
  depends_on "pkg-config" => :build
  depends_on "git" => :build

  def install
    # vcpkg fetches/builds nested deps inline.
    system "git", "submodule", "update", "--init", "--recursive"
    system "cmake", "--preset", "release", *std_cmake_args
    system "cmake", "--build", "--preset", "release"
    bin.install "build/release/apps/nift/nift"
  end

  test do
    output = shell_output("#{bin}/nift version")
    assert_match "nift", output
  end
end
