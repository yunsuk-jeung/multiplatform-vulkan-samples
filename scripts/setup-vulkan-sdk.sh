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
  Darwin) : ;;
  Linux)  : ;;
  *) die "unsupported OS: $OS" ;;
esac

mkdir -p "$INSTALL_DIR"

if [ "$OS" = "Darwin" ]; then
  # --- macOS ----------------------------------------------------------------
  # The macOS SDK ships as a .zip containing a single Qt Installer Framework
  # installer app (it bundles MoltenVK). We download it and drive its headless
  # CLI to install locally into $INSTALL_DIR/<version> (no root, no /Applications),
  # so the layout matches Linux: $INSTALL_DIR/<version>/setup-env.sh.
  TMP_ZIP="$(mktemp "${TMPDIR:-/tmp}/vulkan-sdk.XXXXXX.zip")"
  TMP_EXTRACT="$(mktemp -d "${TMPDIR:-/tmp}/vulkan-sdk-extract.XXXXXX")"
  trap 'rm -rf "$TMP_ZIP" "$TMP_EXTRACT"' EXIT

  if [ "$VERSION" = "latest" ]; then
    URL="https://sdk.lunarg.com/sdk/download/latest/mac/vulkan-sdk.zip"
  else
    URL="https://sdk.lunarg.com/sdk/download/${VERSION}/mac/vulkansdk-macos-${VERSION}.zip"
  fi

  log "downloading Vulkan SDK ($VERSION) from:"
  log "  $URL"
  fetch "$URL" "$TMP_ZIP"

  log "unpacking installer"
  unzip -q "$TMP_ZIP" -d "$TMP_EXTRACT"

  # The zip contains one installer app: vulkansdk-macOS-<version>.app
  INSTALLER_APP="$(find "$TMP_EXTRACT" -maxdepth 1 -type d -name '*.app' | head -n1)"
  [ -n "$INSTALLER_APP" ] || die "could not find the installer .app inside the downloaded zip"
  INSTALLER_BIN="$INSTALLER_APP/Contents/MacOS/$(basename "${INSTALLER_APP%.app}")"
  [ -x "$INSTALLER_BIN" ] || die "installer binary not found at $INSTALLER_BIN"

  # Derive the version from the app name (e.g. vulkansdk-macOS-1.4.350.1.app).
  SDK_VERSION="$(basename "${INSTALLER_APP%.app}" | sed -E 's/^.*-([0-9][0-9.]*)$/\1/')"
  [ -n "$SDK_VERSION" ] || die "could not parse SDK version from $INSTALLER_APP"
  SDK_DIR="$INSTALL_DIR/$SDK_VERSION"

  log "installing into $SDK_DIR (headless, no root)"
  mkdir -p "$SDK_DIR"
  # --default-answer / --accept-licenses / --confirm-command make it non-interactive.
  "$INSTALLER_BIN" --root "$SDK_DIR" \
    --accept-licenses --default-answer --confirm-command install

  [ -f "$SDK_DIR/setup-env.sh" ] || die "setup-env.sh not found in $SDK_DIR after install"
else
  # --- Linux ----------------------------------------------------------------
  ARCH="$(uname -m)"
  [ "$ARCH" = "x86_64" ] || die "LunarG Linux SDK tarball only targets x86_64 (got: $ARCH)"

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

  log "extracting into $INSTALL_DIR"
  tar -xf "$TMP_TARBALL" -C "$INSTALL_DIR"

  # The tarball's top-level entry is the version directory (e.g. 1.4.313.0/).
  SDK_DIR="$(find "$INSTALL_DIR" -maxdepth 1 -mindepth 1 -type d -name '1.*' | sort -V | tail -n1)"
  [ -n "$SDK_DIR" ] || die "could not locate the unpacked SDK directory under $INSTALL_DIR"
  [ -f "$SDK_DIR/setup-env.sh" ] || die "setup-env.sh not found in $SDK_DIR"
fi

# --- report -----------------------------------------------------------------
log "Vulkan SDK ready at: $SDK_DIR"
cat <<EOF

Activate it in your current shell:
  source "$SDK_DIR/setup-env.sh"

That exports VULKAN_SDK / PATH and the loader/ICD paths so CMake's
find_package(Vulkan) and the shader compilers (glslangValidator, glslc, dxc)
are picked up.

Then build:
  cmake -S "$REPO_ROOT" -B "$REPO_ROOT/build"
  cmake --build "$REPO_ROOT/build"

Tip: this repo's .envrc auto-sources the highest installed SDK on cd (run
'direnv allow' once), so you usually don't need to source it by hand.
EOF

if [ "$OS" = "Linux" ]; then
  cat <<'EOF'
Note: if this machine has no GPU driver, install a software renderer to get a
usable device:  sudo apt install -y mesa-vulkan-drivers
EOF
else
  cat <<'EOF'
Note: on macOS the SDK bundles MoltenVK (Vulkan-over-Metal); the ICD is wired up
by setup-env.sh via VK_ICD_FILENAMES / VK_DRIVER_FILES.
EOF
fi
