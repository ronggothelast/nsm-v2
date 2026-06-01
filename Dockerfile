# Multi-stage Dockerfile for nift v2.
#
# Build stage installs vcpkg deps and compiles a release binary.
# Runtime stage is a slim distroless-style image containing just the
# binary and required shared libraries.
#
# Usage:
#   docker build -t nift:latest .
#   docker run --rm -v $PWD:/site nift build /site

FROM ubuntu:22.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        git \
        zip \
        unzip \
        ninja-build \
        cmake \
        gcc-11 \
        g++-11 \
        pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

# The vcpkg submodule is included in the repo. Bootstrap if needed.
RUN if [ ! -x third_party/vcpkg/vcpkg ]; then \
        third_party/vcpkg/bootstrap-vcpkg.sh -disableMetrics; \
    fi

RUN cmake --preset release && cmake --build --preset release

# ─── Runtime image ──────────────────────────────────────────────────

FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /src/build/release/apps/nift/nift /usr/local/bin/nift

WORKDIR /site

ENTRYPOINT ["/usr/local/bin/nift"]
CMD ["help"]
