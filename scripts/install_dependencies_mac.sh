#!/bin/bash

# Build and install third-party dependencies for multiplatform-vulkan-samples
# (macOS).
# Usage: ./install_dependencies_mac.sh [debug|release]
#
# Currently installs the minimal build toolchain via Homebrew, then builds
# spdlog from source. Other dependencies are added here as samples require them.
#
# Env:
#   SKIP_BREW=1   skip the Homebrew system package step (already installed)

set -e  # Exit on error

# Target OS for the install layout (this is the macOS build script)
TARGET_OS="mac"

# Get build type (default: release)
BUILD_TYPE=${1:-release}

# Convert to proper CMake build type
case "$BUILD_TYPE" in
    debug)
        CMAKE_BUILD_TYPE="Debug"
        ;;
    release)
        CMAKE_BUILD_TYPE="Release"
        ;;
    *)
        echo "Invalid build type: $BUILD_TYPE"
        echo "Usage: $0 [debug|release]"
        exit 1
        ;;
esac

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
THIRD_PARTY_DIR="$PROJECT_ROOT/third_party"
# Install per-OS: libs/{mac,ubuntu,ios}/<buildtype>
INSTALL_PREFIX="$PROJECT_ROOT/libs/$TARGET_OS/$BUILD_TYPE"
BUILD_DIR="$PROJECT_ROOT/build/deps-$TARGET_OS-$BUILD_TYPE"
PREFIX_PATH="$CMAKE_PREFIX_PATH"
if [ -n "$PREFIX_PATH" ]; then
    PREFIX_PATH="$PREFIX_PATH;$INSTALL_PREFIX"
else
    PREFIX_PATH="$INSTALL_PREFIX"
fi

echo "=================================================="
echo "Building third-party dependencies (macOS)"
echo "  Target OS:  $TARGET_OS"
echo "  Build Type: $CMAKE_BUILD_TYPE"
echo "  Install To: $INSTALL_PREFIX"
echo "  Build Dir:  $BUILD_DIR"
echo "=================================================="

# Install system packages: minimal build toolchain via Homebrew.
# (clang is provided by the Xcode Command Line Tools, not Homebrew.)
if [ "${SKIP_BREW:-0}" != "1" ]; then
    if ! command -v brew >/dev/null 2>&1; then
        echo "Error: Homebrew not found. Install it from https://brew.sh"
        echo "       or set SKIP_BREW=1 if the toolchain is already present."
        exit 1
    fi

    echo ""
    echo "Installing system packages via Homebrew..."
    echo "----------------------------------------"
    brew install cmake ninja git pkg-config
else
    echo ""
    echo "SKIP_BREW=1 set; skipping system package installation."
fi

# Create source, build and install directories
mkdir -p "$THIRD_PARTY_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$INSTALL_PREFIX"

# Number of parallel jobs
if command -v nproc >/dev/null 2>&1; then
    NPROC=$(nproc)
elif command -v sysctl >/dev/null 2>&1; then
    NPROC=$(sysctl -n hw.ncpu)
else
    NPROC=1
fi

# Function to download a dependency pinned to an exact ref (tag or commit SHA)
# Usage: fetch_source <name> <git-url> <ref> <dest-dir>
fetch_source() {
    local name=$1
    local url=$2
    local ref=$3
    local dest=$4

    if [ -d "$dest/.git" ]; then
        echo "[$name] source already present at $dest (skipping clone)"
        return
    fi

    echo ""
    echo "Fetching $name ($ref)..."
    echo "----------------------------------------"
    # Fetch the exact ref (works for both tags and commit SHAs on GitHub/GitLab),
    # then pull in submodules.
    git init "$dest"
    git -C "$dest" remote add origin "$url"
    git -C "$dest" fetch --depth 1 origin "$ref"
    git -C "$dest" checkout --detach FETCH_HEAD
    git -C "$dest" submodule update --init --recursive --depth 1
}

# Download all sources
echo ""
echo "Downloading third-party sources into $THIRD_PARTY_DIR ..."
fetch_source "spdlog"        "https://github.com/gabime/spdlog.git"         "33375433e096d59b1e4dd9d46cac9d58a5528ccb"   "$THIRD_PARTY_DIR/spdlog"

# Function to build and install a library
build_library() {
    local name=$1
    local source_dir=$2
    local cmake_args=$3

    echo ""
    echo "Building $name..."
    echo "----------------------------------------"

    local lib_build_dir="$BUILD_DIR/$name"
    mkdir -p "$lib_build_dir"
    cd "$lib_build_dir"

    cmake "$source_dir" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DCMAKE_PREFIX_PATH="$PREFIX_PATH" \
        $cmake_args

    cmake --build . -j$NPROC
    cmake --install .

    echo "$name installed successfully!"
}

# 1. spdlog
build_library "spdlog" "$THIRD_PARTY_DIR/spdlog" \
    "-DSPDLOG_BUILD_EXAMPLE=OFF \
     -DSPDLOG_USE_STD_FORMAT=ON \
     -DSPDLOG_BUILD_SHARED=ON \
     -DSPDLOG_BUILD_TESTS=OFF"

echo ""
echo "=================================================="
echo "All dependencies built and installed successfully!"
echo "Install location: $INSTALL_PREFIX"
echo "=================================================="
