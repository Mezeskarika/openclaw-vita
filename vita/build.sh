#!/bin/sh
# Builds build-vita/openclaw.vpk inside the VitaSDK Docker image, so no local VitaSDK install is needed.
#   vita/build.sh            configure + build
#   vita/build.sh clean      remove build-vita/ first
set -e

# Pinned so builds are reproducible; bump deliberately.
IMAGE="vitasdk/vitasdk@sha256:8227da1b596d981f4513cd9b36bda1e43857f6dae5bcf4312c1cd40167f7d9bb"

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build-vita"

if [ "$1" = "clean" ]; then
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"

docker run --rm \
    -u "$(id -u):$(id -g)" \
    -v "$ROOT":/src \
    -w /src/build-vita \
    "$IMAGE" \
    sh -c 'cmake -S /src -B . -DVita=ON && make -j"$(nproc)"'

VPK="$BUILD_DIR/openclaw.vpk"
echo "Contents of $VPK:"
unzip -l "$VPK"

# The VPK must at least carry these to install and start
for f in eboot.bin sce_sys/param.sfo sce_sys/icon0.png sce_sys/livearea/contents/template.xml \
         config.xml SAVES.XML ASSETS.ZIP clacon.ttf console02.tga fonts/Cinzel-Bold.ttf; do
    unzip -l "$VPK" "$f" > /dev/null || { echo "MISSING in VPK: $f"; exit 1; }
done
echo "VPK OK: $VPK"
