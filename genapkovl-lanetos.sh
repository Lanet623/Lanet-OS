#!/bin/sh -e

HOSTNAME="$1"
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
OVERLAY_DIR="$SCRIPT_DIR/lanetos-overlay"
TMP_DIR="$(mktemp -d)"

cleanup() {
    rm -rf -- "$TMP_DIR"
}
trap cleanup EXIT

rc_add() {
    mkdir -p "$TMP_DIR/etc/runlevels/$2"
    ln -sf "/etc/init.d/$1" "$TMP_DIR/etc/runlevels/$2/$1"
}

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

rc_add devfs sysinit
rc_add dmesg sysinit
rc_add mdev sysinit
rc_add hwdrivers sysinit
rc_add modloop sysinit
rc_add hwclock boot
rc_add modules boot
rc_add sysctl boot
rc_add hostname boot
rc_add bootmisc boot
rc_add syslog boot
rc_add mount-ro shutdown
rc_add killprocs shutdown
rc_add savecache shutdown

chown -R root:root "$TMP_DIR"
chmod 0755 "$TMP_DIR"
tar -C "$TMP_DIR" -czf "$HOSTNAME.apkovl.tar.gz" .
