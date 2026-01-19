#!/bin/bash
# Rebuild PE (Physics Engine) and install updated library/headers
# Run this script from the project root after modifying PE source files

set -e  # Exit on error

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

echo "======================================"
echo "Rebuilding PE (Physics Engine)"
echo "======================================"

# 1. Rebuild PE library
echo ""
echo "[1/4] Building PE library..."
cd third-party/build/src/pe-build/Release
ninja

# 2. Install library
echo ""
echo "[2/4] Installing PE library..."
cd ../..  # Now in third-party/build/src
cp pe-build/Release/lib/libpe.a ../lib/libpe.a
echo "  ✓ Copied libpe.a to third-party/build/lib/"

# 3. Install headers
echo ""
echo "[3/4] Installing PE headers..."
cmake -E copy_directory pe/pe ../include/pe
echo "  ✓ Copied headers to third-party/build/include/pe/"

# 4. Rebuild examples
echo ""
echo "[4/4] Rebuilding examples..."
cd "$PROJECT_ROOT/build/release"

# Remove binaries to force relink
rm -f examples/00-physics-basic/00-physics-basic
rm -f sdl-bgfx-imgui-starter

ninja

echo ""
echo "======================================"
echo "✓ PE rebuild complete!"
echo "======================================"
echo ""
echo "Library:  third-party/build/lib/libpe.a"
echo "Headers:  third-party/build/include/pe/"
echo "Examples: build/release/examples/"
echo ""
