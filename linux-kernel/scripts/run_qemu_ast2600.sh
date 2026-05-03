#!/bin/bash
set -e

WORKSPACE=/workspace/linux-kernel
BUILDROOT=$WORKSPACE/buildroot
OUTPUT=$BUILDROOT/output

echo "=== QEMU AST2600 Launch Script ==="

# 檢查 image 是否存在
KERNEL=$OUTPUT/images/zImage
DTBS=$(ls $OUTPUT/images/aspeed-ast2600-*.dtb 2>/dev/null | head -1)
ROOTFS=$OUTPUT/images/rootfs.cpio

if [ ! -f "$KERNEL" ]; then
    echo "ERROR: Kernel image not found: $KERNEL"
    echo "Run: cd $BUILDROOT && make"
    exit 1
fi

echo "Kernel: $KERNEL"
echo "DTB: $DTBS"
echo "Rootfs: $ROOTFS"
echo ""
echo "=== Starting QEMU (AST2600) ==="
echo "=== Login: root (no password) ==="
echo "=== Press Ctrl+A then X to exit ==="
echo ""

qemu-system-arm \
    -machine ast2600-evb \
    -kernel "$KERNEL" \
    ${DTBS:+-dtb "$DTBS"} \
    -initrd "$ROOTFS" \
    -append "console=ttyS4,115200 root=/dev/ram rw" \
    -nographic \
    -no-reboot \
    -m 512M
