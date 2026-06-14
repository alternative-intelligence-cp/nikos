# =============================================================================
# NIKOS — Multi-stage Docker build
# =============================================================================
# Stage 1: builder — full build environment
# Stage 2: runtime — minimal image with just the install tree
#
# Usage:
#   docker build -t nikos:1.0.0 .
#   docker run --rm -v $(pwd):/work nikos:1.0.0 /work/example.c
# =============================================================================

# ---------------------------------------------------------------------------
# Stage 1: Builder
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS builder

LABEL stage=builder
ENV DEBIAN_FRONTEND=noninteractive

# System dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git wget gnupg ca-certificates \
    libboost-all-dev libgmp-dev libsqlite3-dev \
    libtbb-dev libedit-dev zlib1g-dev \
    python3 python3-pip \
    && rm -rf /var/lib/apt/lists/*

# LLVM 20
RUN wget -qO /tmp/llvm.sh https://apt.llvm.org/llvm.sh \
    && chmod +x /tmp/llvm.sh \
    && /tmp/llvm.sh 20 \
    && rm /tmp/llvm.sh \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Copy source
WORKDIR /src
COPY . /src/

# Build NIKOS
RUN cmake \
    -DCMAKE_INSTALL_PREFIX=/opt/nikos \
    -DLLVM_CONFIG_EXECUTABLE=/usr/bin/llvm-config-20 \
    -DCMAKE_BUILD_TYPE=Release \
    -S /src -B /src/build \
    && cmake --build /src/build -j"$(nproc)" \
    && cmake --build /src/build --target install

# ---------------------------------------------------------------------------
# Stage 2: Runtime
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS runtime

LABEL org.opencontainers.image.title="NIKOS" \
      org.opencontainers.image.description="Static analyzer for C/C++ (LLVM 20, Abstract Interpretation)" \
      org.opencontainers.image.version="1.0.0" \
      org.opencontainers.image.url="https://github.com/alternative-intelligence-cp/nikos" \
      org.opencontainers.image.licenses="NOSA-1.3"

ENV DEBIAN_FRONTEND=noninteractive

# Runtime dependencies only
RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-filesystem1.83.0 libboost-graph1.83.0 \
    libboost-regex1.83.0 libboost-thread1.83.0 \
    libgmp10 libsqlite3-0 libtbb12 libedit2 zlib1g \
    python3 clang-20 \
    && rm -rf /var/lib/apt/lists/*

# Copy NIKOS install tree from builder
COPY --from=builder /opt/nikos /opt/nikos

# Add to PATH
ENV PATH="/opt/nikos/bin:${PATH}"

# Working directory for analysis
WORKDIR /work

# Default entrypoint — wraps ikos-analyzer for direct file analysis
# Usage: docker run --rm -v $(pwd):/work nikos:1.0.0 [ikos-analyzer args]
ENTRYPOINT ["ikos-analyzer"]
CMD ["--help"]
