#!/bin/sh -e

HOSTNAME="$1"
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
OVERLAY_DIR="$SCRIPT_DIR/lanetos-overlay"
TMP_DIR="$(mktemp -d)"

cleanup() {
    rm -rf -- "$TMP_DIR"
}
trap cleanup EXIT

mkdir -p "$TMP_DIR/etc/apk" "$TMP_DIR/etc/init.d" "$TMP_DIR/etc/runlevels/boot"
printf '%s\n' "$HOSTNAME" > "$TMP_DIR/etc/hostname"
cat > "$TMP_DIR/etc/apk/world" <<'EOF'
alpine-base
linux-lts
alpine-conf
lanet-shell
EOF
cp "$OVERLAY_DIR/etc/init.d/lanet-firstboot" "$TMP_DIR/etc/init.d/lanet-firstboot"
chmod 0755 "$TMP_DIR/etc/init.d/lanet-firstboot"
ln -s /etc/init.d/lanet-firstboot "$TMP_DIR/etc/runlevels/boot/lanet-firstboot"

chown -R root:root "$TMP_DIR"
tar -C "$TMP_DIR" -czf "$HOSTNAME.apkovl.tar.gz" .
