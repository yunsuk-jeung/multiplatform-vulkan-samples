#!/usr/bin/env bash
#
# Download and unpack the LunarG Vulkan SDK locally (no root required).
#
# Usage:
#   scripts/setup-vulkan-sdk.sh                 # latest SDK, into ./third_party/vulkan-sdk
#   VULKAN_SDK_VERSION=1.4.313.0 scripts/setup-vulkan-sdk.sh
#   VULKAN_SDK_INSTALL_DIR=~/sdk scripts/setup-vulkan-sdk.sh
#
# After it finishes, activate the SDK in your current shell with:
#   source <install_dir>/<version>/setup-env.sh
#
set -euo pipefail

# --- config -----------------------------------------------------------------
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INSTALL_DIR="${VULKAN_SDK_INSTALL_DIR:-${REPO_ROOT}/third_party/vulkan-sdk}"
VERSION="${VULKAN_SDK_VERSION:-latest}"   # "latest" or a pinned version like 1.4.313.0

# --- helpers ----------------------------------------------------------------
log()  { printf '\033[1;34m[setup]\033[0m %s\n' "$*"; }
die()  { printf '\033[1;31m[error]\033[0m %s\n' "$*" >&2; exit 1; }

fetch() {
  # fetch <url> <output-file>
  if command -v curl >/dev/null 2>&1; then
    curl -fL --progress-bar -o "$2" "$1"
  elif command -v wget >/dev/null 2>&1; then
    wget -q --show-progress -O "$2" "$1"
  else
    die "neither curl nor wget is available"
  fi
}

# --- platform detection -----------------------------------------------------
OS="$(uname -s)"

case "$OS" in
  Darwin)
    cat <<'EOF'
[setup] macOS detected.

The Vulkan SDK for macOS ships as a .dmg installer (it bundles MoltenVK), not a
tarball, so this script cannot silently unpack it. Do one of:

  1. Download the installer from https://vulkan.lunarg.com/sdk/home#mac
  2. Or via Homebrew:  brew install --cask vulkan-sdk

Then set VULKAN_SDK (the installer/cask does this for you), e.g.:
  export VULKAN_SDK="$HOME/VulkanSDK/<version>/macOS"
EOF
    exit 0
    ;;
  Linux) : ;;
  *) die "unsupported OS: $OS (this script handles Linux tarballs; use the LunarG installer on Windows/macOS)" ;;
esac

ARCH="$(uname -m)"
[ "$ARCH" = "x86_64" ] || die "LunarG Linux SDK tarball only targets x86_64 (got: $ARCH)"

# --- download ---------------------------------------------------------------
mkdir -p "$INSTALL_DIR"
TMP_TARBALL="$(mktemp "${TMPDIR:-/tmp}/vulkan-sdk.XXXXXX.tar.xz")"
trap 'rm -f "$TMP_TARBALL"' EXIT

if [ "$VERSION" = "latest" ]; then
  URL="https://sdk.lunarg.com/sdk/download/latest/linux/vulkan-sdk.tar.xz"
else
  URL="https://sdk.lunarg.com/sdk/download/${VERSION}/linux/vulkansdk-linux-x86_64-${VERSION}.tar.xz"
fi

log "downloading Vulkan SDK ($VERSION) from:"
log "  $URL"
fetch "$URL" "$TMP_TARBALL"

# --- extract ----------------------------------------------------------------
log "extracting into $INSTALL_DIR"
tar -xf "$TMP_TARBALL" -C "$INSTALL_DIR"

# The tarball's top-level entry is the version directory (e.g. 1.4.313.0/).
SDK_DIR="$(find "$INSTALL_DIR" -maxdepth 1 -mindepth 1 -type d -name '1.*' | sort -V | tail -n1)"
[ -n "$SDK_DIR" ] || die "could not locate the unpacked SDK directory under $INSTALL_DIR"
[ -f "$SDK_DIR/setup-env.sh" ] || die "setup-env.sh not found in $SDK_DIR"

# --- report -----------------------------------------------------------------
log "Vulkan SDK ready at: $SDK_DIR"
cat <<EOF

Activate it in your current shell:
  source "$SDK_DIR/setup-env.sh"

That exports VULKAN_SDK / PATH / LD_LIBRARY_PATH so CMake's find_package(Vulkan)
and the shader compilers (glslangValidator, glslc, dxc) are picked up.

Then build:
  cmake -S "$REPO_ROOT" -B "$REPO_ROOT/build"
  cmake --build "$REPO_ROOT/build"

Note: if this machine has no GPU driver, install a software renderer to get a
usable device:  sudo apt install -y mesa-vulkan-drivers
EOF
