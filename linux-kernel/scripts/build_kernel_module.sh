#!/bin/bash
set -e

WORKSPACE=/workspace/linux-kernel
DRIVER_DIR=$WORKSPACE/driver
BUILDROOT=$WORKSPACE/buildroot
KERNEL_DIR=$BUILDROOT/output/build/linux-*

echo "=== Building HDC1080 kernel module ==="

# 確認 buildroot 已經建置過（需要 kernel headers）
if [ ! -d "$BUILDROOT/output/build" ]; then
    echo "ERROR: Buildroot output not found."
    echo "Please run 'make' in $BUILDROOT first."
    exit 1
fi

KDIR=$(ls -d $BUILDROOT/output/build/linux-* 2>/dev/null | head -1)
CROSS_COMPILE="$BUILDROOT/output/host/bin/arm-linux-gnueabihf-"
ARCH=arm

echo "Kernel dir: $KDIR"
echo "Cross compile: $CROSS_COMPILE"

# 建置 module
make -C "$KDIR" \
     M="$DRIVER_DIR" \
     ARCH=$ARCH \
     CROSS_COMPILE=$CROSS_COMPILE \
     modules

echo "=== Build complete ==="
ls -la $DRIVER_DIR/*.ko

echo ""
echo "=== To install into rootfs overlay ==="
echo "cp $DRIVER_DIR/hdc1080.ko $WORKSPACE/buildroot-overlay/rootfs/lib/modules/"
