#!/bin/sh
set -eu

PROJECT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
OUTPUT_DIR="$PROJECT_DIR/output"
TMPDIR="${TMPDIR:-/var/tmp}"
export TMPDIR
WORK_DIR="$(mktemp -d "$TMPDIR/lanetos-mkimage.XXXXXX")"
APORTS_DIR="$WORK_DIR/aports"
ALPINE_VERSION="3.24"
ISO_NAME="lanetos-alpine-${ALPINE_VERSION}-x86_64.iso"

cleanup() {
    status=$?
    if [ "$status" -eq 0 ]; then
        rm -rf -- "$WORK_DIR"
    else
        echo "BUILD FAILED: сохранена рабочая папка: $WORK_DIR" >&2
        echo "=== DISK SPACE ===" >&2
        df -h "$WORK_DIR" >&2
        echo "=== WORKDIR SIZE ===" >&2
        du -sh "$WORK_DIR" 2>/dev/null >&2 || true
    fi
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM

if [ "$(id -u)" -ne 0 ]; then
    echo "Run this builder as root: doas sh ./build-lanetos.sh" >&2
    exit 1
fi

if ! command -v apk >/dev/null 2>&1 || [ ! -f /etc/alpine-release ]; then
    echo "This builder must run on Alpine Linux, not Ubuntu, Debian, or native Windows." >&2
    echo "Use an Alpine Linux VM and follow QUICKSTART.md." >&2
    exit 1
fi

if [ ! -f "$PROJECT_DIR/mkimg.lanetos.sh" ]; then
    echo "Missing custom profile: $PROJECT_DIR/mkimg.lanetos.sh" >&2
    exit 1
fi

apk add --no-cache alpine-sdk alpine-conf syslinux xorriso squashfs-tools grub mtools git fakeroot

git clone --depth 1 --branch "${ALPINE_VERSION}-stable" https://gitlab.alpinelinux.org/alpine/aports.git "$APORTS_DIR"
chmod 755 "$WORK_DIR" "$APORTS_DIR"
cp "$PROJECT_DIR/mkimg.lanetos.sh" "$APORTS_DIR/scripts/mkimg.lanetos.sh"
cp "$PROJECT_DIR/genapkovl-lanetos.sh" "$APORTS_DIR/scripts/genapkovl-lanetos.sh"
chmod +x "$APORTS_DIR/scripts/genapkovl-lanetos.sh"

BUILD_USER="lanetbuild"
if ! id "$BUILD_USER" >/dev/null 2>&1; then
    adduser -D -G abuild "$BUILD_USER"
fi
BUILD_HOME="/home/$BUILD_USER"
PACKAGE_DIR="$APORTS_DIR/testing/lanet-shell"
mkdir -p "$PACKAGE_DIR"
cp "$PROJECT_DIR/packaging/lanet-shell/APKBUILD" "$PACKAGE_DIR/APKBUILD"
cp -R "$PROJECT_DIR/lanet-shell" "$PACKAGE_DIR/lanet-shell"
chown -R "$BUILD_USER:abuild" "$PACKAGE_DIR"

PRIVATE_KEY="$(find "$BUILD_HOME/.abuild" -maxdepth 1 -name '*.rsa' -print -quit)"

if [ -z "$PRIVATE_KEY" ]; then
    su - "$BUILD_USER" -c 'abuild-keygen -a -n'
    PRIVATE_KEY="$(find "$BUILD_HOME/.abuild" -maxdepth 1 -name '*.rsa' -print -quit)"
fi

PUBLIC_KEY="${PRIVATE_KEY}.pub"

if [ ! -f "$PRIVATE_KEY" ] || [ ! -f "$PUBLIC_KEY" ]; then
    echo "Could not find a valid Alpine package signing key." >&2
    exit 1
fi

install -m 0644 -o root -g root "$PUBLIC_KEY" /etc/apk/keys/

su - "$BUILD_USER" -c "cd '$PACKAGE_DIR' && PACKAGER_PRIVKEY='$PRIVATE_KEY' abuild -r && PACKAGER_PRIVKEY='$PRIVATE_KEY' abuild index"

LOCAL_REPO_DIR="$BUILD_HOME/packages/testing"
LOCAL_REPO_INDEX="$LOCAL_REPO_DIR/x86_64/APKINDEX.tar.gz"

if [ ! -f "$LOCAL_REPO_INDEX" ]; then
    echo "Local Lanet package repository was not created." >&2
    exit 1
fi

# Remove network-extras: it conflicts with ifupdown-ng through vlan.
sed -i 's/kbd-bkeymaps network-extras openntpd openssl openssh/kbd-bkeymaps openntpd openssl openssh/' "$APORTS_DIR/scripts/mkimg.base.sh"

if grep -q 'network-extras' "$APORTS_DIR/scripts/mkimg.base.sh"; then
    echo "Failed to remove network-extras from Alpine base profile." >&2
    exit 1
fi

OVERLAY_DIR="$APORTS_DIR/scripts/lanetos-overlay"
mkdir -p "$OVERLAY_DIR/etc/init.d"
cp "$PROJECT_DIR/lanet-shell/data/lanet-firstboot" "$OVERLAY_DIR/etc/init.d/lanet-firstboot"

mkdir -p "$OUTPUT_DIR"
cd "$APORTS_DIR"
chown -R "$BUILD_USER:abuild" "$WORK_DIR"

su - "$BUILD_USER" -c "
    export TMPDIR='$TMPDIR' &&
    cd '$APORTS_DIR' &&
    PACKAGER_PRIVKEY='$PRIVATE_KEY' sh scripts/mkimage.sh \
        --tag '$ALPINE_VERSION' \
        --outdir '$WORK_DIR/out' \
        --workdir '$WORK_DIR/cache' \
        --arch x86_64 \
        --profile lanetos \
        --repository 'file://$LOCAL_REPO_DIR' \
        --repository 'https://dl-cdn.alpinelinux.org/alpine/v${ALPINE_VERSION}/main' \
        --repository 'https://dl-cdn.alpinelinux.org/alpine/v${ALPINE_VERSION}/community'
"

BUILT_ISO=""
for candidate in "$WORK_DIR"/out/lanetos-*.iso; do
    if [ -f "$candidate" ] && [ -s "$candidate" ]; then
        BUILT_ISO="$candidate"
        break
    fi
done
if [ -z "$BUILT_ISO" ]; then
    echo "mkimage did not produce the expected Lanet OS ISO." >&2
    exit 1
fi

xorriso -indev "$BUILT_ISO" -report_el_torito plain >/dev/null
cp "$BUILT_ISO" "$OUTPUT_DIR/$ISO_NAME"
printf '\nBuild complete: %s\n' "$OUTPUT_DIR/$ISO_NAME"
du -h "$OUTPUT_DIR/$ISO_NAME"
printf 'ISO boot records verified; test in a VM before using physical hardware.\n'
