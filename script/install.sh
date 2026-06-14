#!/usr/bin/env bash
# =============================================================================
# NIKOS Install Script v1.0.0
# =============================================================================
# Installs NIKOS (static analyzer for C/C++) from source on Ubuntu 22.04/24.04.
#
# Usage:
#   bash install.sh [OPTIONS]
#
# Options:
#   --prefix PATH       Install prefix (default: $HOME/.local/nikos)
#   --with-apron        Also build and install APRON abstract domain library
#   --apron-prefix PATH APRON install prefix (default: $HOME/.local/apron)
#   --jobs N            Parallel build jobs (default: nproc)
#   --help              Show this message
#
# Examples:
#   bash install.sh
#   bash install.sh --prefix /opt/nikos
#   bash install.sh --with-apron
#   curl -sSf https://raw.githubusercontent.com/alternative-intelligence-cp/nikos/main/script/install.sh | bash
# =============================================================================
set -euo pipefail

# ---- defaults ---------------------------------------------------------------
PREFIX="${HOME}/.local/nikos"
WITH_APRON=0
APRON_PREFIX="${HOME}/.local/apron"
JOBS=$(nproc)
NIKOS_REPO="https://github.com/alternative-intelligence-cp/nikos.git"
APRON_REPO="https://github.com/antoinemine/apron.git"

# ---- colors -----------------------------------------------------------------
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
BOLD='\033[1m'; RESET='\033[0m'

info()    { echo -e "${GREEN}[nikos]${RESET} $*"; }
warn()    { echo -e "${YELLOW}[warn]${RESET}  $*"; }
error()   { echo -e "${RED}[error]${RESET} $*" >&2; exit 1; }
section() { echo -e "\n${BOLD}==> $*${RESET}"; }

# ---- arg parsing ------------------------------------------------------------
while [[ $# -gt 0 ]]; do
  case "$1" in
    --prefix)       PREFIX="$2"; shift 2 ;;
    --with-apron)   WITH_APRON=1; shift ;;
    --apron-prefix) APRON_PREFIX="$2"; shift 2 ;;
    --jobs)         JOBS="$2"; shift 2 ;;
    --help)
      grep '^#' "$0" | sed 's/^# \?//' | head -20
      exit 0 ;;
    *) error "Unknown option: $1. Use --help for usage." ;;
  esac
done

# ---- platform check ---------------------------------------------------------
section "Checking platform"
if [[ ! -f /etc/os-release ]]; then
  error "Cannot detect OS. Only Ubuntu 22.04 and 24.04 are supported."
fi
source /etc/os-release
if [[ "$ID" != "ubuntu" ]]; then
  error "Unsupported OS: $ID. Only Ubuntu is supported. (got: $PRETTY_NAME)"
fi
case "$VERSION_ID" in
  22.04|24.04) info "Detected: $PRETTY_NAME ✓" ;;
  *) warn "Unsupported Ubuntu version: $VERSION_ID. Proceeding anyway (may fail)." ;;
esac

# ---- dependencies -----------------------------------------------------------
section "Installing system dependencies"
sudo apt-get update -qq
sudo apt-get install -y --no-install-recommends \
  cmake build-essential git wget gnupg \
  libboost-all-dev libgmp-dev libsqlite3-dev \
  libtbb-dev libedit-dev zlib1g-dev \
  python3 python3-pip

# ---- LLVM 20 ----------------------------------------------------------------
section "Installing LLVM 20 + Clang 20"
if command -v llvm-config-20 &>/dev/null; then
  info "llvm-config-20 already found at $(which llvm-config-20)"
else
  info "Installing LLVM 20 via apt.llvm.org..."
  TMP_LLVM=$(mktemp -d)
  wget -q -O "$TMP_LLVM/llvm.sh" https://apt.llvm.org/llvm.sh
  chmod +x "$TMP_LLVM/llvm.sh"
  sudo bash "$TMP_LLVM/llvm.sh" 20
  rm -rf "$TMP_LLVM"
fi
LLVM_CONFIG=$(command -v llvm-config-20 || command -v llvm-config)
info "Using LLVM config: $LLVM_CONFIG"

# ---- APRON (optional) -------------------------------------------------------
if [[ $WITH_APRON -eq 1 ]]; then
  section "Building APRON (optional abstract domain library)"
  sudo apt-get install -y --no-install-recommends libmpfr-dev libppl-dev m4

  APRON_SRC=$(mktemp -d)
  info "Cloning APRON into $APRON_SRC..."
  git clone --depth=1 "$APRON_REPO" "$APRON_SRC"

  cp "$APRON_SRC/Makefile.config.model" "$APRON_SRC/Makefile.config"
  # Patch Makefile.config: set prefix, enable PPL, disable OCaml
  sed -i "s|^APRON_PREFIX.*|APRON_PREFIX = ${APRON_PREFIX}|" "$APRON_SRC/Makefile.config"
  sed -i "s|^#HAS_PPL.*|HAS_PPL = 1|" "$APRON_SRC/Makefile.config"
  sed -i "s|^HAS_PPL_PKG_CONFIG.*|HAS_PPL_PKG_CONFIG = 0|" "$APRON_SRC/Makefile.config"
  # Disable OCaml (C API only — OCaml not needed by NIKOS)
  sed -i "s|^HAS_OCAML *=.*|HAS_OCAML =|" "$APRON_SRC/Makefile.config"
  sed -i "s|^HAS_OCAMLOPT *=.*|HAS_OCAMLOPT =|" "$APRON_SRC/Makefile.config"
  sed -i "s|^HAS_OCAMLFIND *=.*|HAS_OCAMLFIND =|" "$APRON_SRC/Makefile.config"

  info "Building APRON (C targets only)..."
  make -C "$APRON_SRC" -j"$JOBS" 2>&1 | grep -v '^make\[' | tail -5 || true

  info "Installing APRON to $APRON_PREFIX..."
  mkdir -p "${APRON_PREFIX}/include" "${APRON_PREFIX}/lib"
  # Headers
  for dir in apron box octagons newpolka ppl products; do
    cp "$APRON_SRC/$dir"/*.h "${APRON_PREFIX}/include/" 2>/dev/null || true
  done
  cp "$APRON_SRC"/num/itv/*.h "${APRON_PREFIX}/include/" 2>/dev/null || true
  # Libs
  find "$APRON_SRC" -name 'lib*.a' -exec cp {} "${APRON_PREFIX}/lib/" \; 2>/dev/null || true
  info "APRON installed to $APRON_PREFIX ✓"
fi

# ---- clone or use existing repo ---------------------------------------------
section "Preparing NIKOS source"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [[ -f "$REPO_ROOT/CMakeLists.txt" ]]; then
  info "Using existing repo at $REPO_ROOT"
else
  info "Cloning NIKOS..."
  REPO_ROOT=$(mktemp -d)/nikos
  git clone --depth=1 "$NIKOS_REPO" "$REPO_ROOT"
fi

# ---- build ------------------------------------------------------------------
section "Building NIKOS"
BUILD_DIR="$REPO_ROOT/build"
mkdir -p "$BUILD_DIR"

CMAKE_ARGS=(
  -DCMAKE_INSTALL_PREFIX="$PREFIX"
  -DLLVM_CONFIG_EXECUTABLE="$LLVM_CONFIG"
  -DCMAKE_BUILD_TYPE=Release
)
if [[ $WITH_APRON -eq 1 ]]; then
  CMAKE_ARGS+=( -DAPRON_ROOT="$APRON_PREFIX" )
fi

info "Running cmake..."
cmake "${CMAKE_ARGS[@]}" -S "$REPO_ROOT" -B "$BUILD_DIR"

info "Building with $JOBS jobs..."
cmake --build "$BUILD_DIR" -j"$JOBS"

# ---- install ----------------------------------------------------------------
section "Installing to $PREFIX"
cmake --build "$BUILD_DIR" --target install
info "Installed ✓"

# ---- PATH setup -------------------------------------------------------------
section "Setup complete!"
echo ""
echo -e "  NIKOS installed to: ${BOLD}${PREFIX}${RESET}"
echo ""
echo -e "  Add to your shell profile to use NIKOS:"
echo -e "    ${BOLD}export PATH=\"${PREFIX}/bin:\$PATH\"${RESET}"
echo ""
echo -e "  Quick test:"
echo -e "    ${BOLD}ikos-analyzer --version${RESET}"
echo ""
if [[ $WITH_APRON -eq 1 ]]; then
  echo -e "  APRON domains enabled (apron-octagon, apron-polka-polyhedra, ...)."
  echo -e "  Note: APRON is NOT thread-safe. Do not use parallel jobs with APRON domains."
  echo ""
fi
