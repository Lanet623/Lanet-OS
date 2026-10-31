#!/bin/sh
set -eu

PROJECT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

if [ ! -f /etc/alpine-release ]; then
    echo "This build script must run inside Alpine Linux." >&2
    exit 1
fi

if [ "$(id -u)" -ne 0 ]; then
    echo "Run with doas: doas sh ./build-alpine.sh" >&2
    exit 1
fi

apk add --no-cache alpine-sdk cmake gtk+3.0-dev gtk+3.0 sway thunar xfce4-terminal

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --parallel "$(nproc)"
cmake --install "$BUILD_DIR"

printf '\nLanet Shell installed. Start it inside a Sway session with:\n  lanet-shell\n'
