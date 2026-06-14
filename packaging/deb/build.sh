#!/usr/bin/env bash
# =============================================================================
# Build script for nikos Debian package
# =============================================================================
# Usage: bash packaging/deb/build.sh [--prefix /opt/nikos]
#
# Output: packaging/nikos_1.0.0_amd64.deb
# =============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VERSION="1.0.0"
ARCH="amd64"
PKG_NAME="nikos_${VERSION}_${ARCH}"
STAGING="${REPO_ROOT}/packaging/deb/staging"
OUTPUT="${REPO_ROOT}/packaging/${PKG_NAME}.deb"
INSTALL_PREFIX="/usr"

RED='\033[0;31m'; GREEN='\033[0;32m'; BOLD='\033[1m'; RESET='\033[0m'
info()  { echo -e "${GREEN}[deb]${RESET} $*"; }
error() { echo -e "${RED}[error]${RESET} $*" >&2; exit 1; }

# Check deps
command -v dpkg-deb &>/dev/null || error "dpkg-deb not found. Install dpkg: sudo apt-get install dpkg"
command -v cmake &>/dev/null    || error "cmake not found"

info "Building NIKOS for Debian package..."
BUILD_DIR="${REPO_ROOT}/build-deb"
mkdir -p "$BUILD_DIR"

LLVM_CONFIG=$(command -v llvm-config-20 2>/dev/null || command -v llvm-config)
info "LLVM config: $LLVM_CONFIG"

cmake \
  -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
  -DLLVM_CONFIG_EXECUTABLE="$LLVM_CONFIG" \
  -DCMAKE_BUILD_TYPE=Release \
  -S "$REPO_ROOT" -B "$BUILD_DIR"

cmake --build "$BUILD_DIR" -j"$(nproc)"

# Install into staging directory
info "Installing to staging: $STAGING"
rm -rf "$STAGING"
mkdir -p "$STAGING"
DESTDIR="$STAGING" cmake --build "$BUILD_DIR" --target install

# Copy DEBIAN control files into staging
cp -r "${REPO_ROOT}/packaging/deb/DEBIAN" "${STAGING}/DEBIAN"

# Fix installed-size (in KB)
SIZE_KB=$(du -sk "${STAGING}" | awk '{print $1}')
sed -i "s/^Installed-Size:.*/Installed-Size: ${SIZE_KB}/" "${STAGING}/DEBIAN/control"

# Build .deb
info "Building ${OUTPUT}..."
dpkg-deb --build --root-owner-group "$STAGING" "$OUTPUT"

info "${BOLD}Done!${RESET} Package: $OUTPUT"
info "Install with: sudo dpkg -i $OUTPUT"
info "Verify: ikos-analyzer --help | head -3"
