#!/bin/bash
set -e

WORKSPACE=/workspace
LINUX_DIR=$WORKSPACE/linux-kernel
BUILDROOT_DIR=$LINUX_DIR/buildroot
BUILDROOT_VERSION="2024.02"
PATCH=$LINUX_DIR/scripts/0001-add-hdc1080-to-ast2600-i2c1.patch
OVERLAY=$LINUX_DIR/buildroot-overlay/rootfs
DRIVER_DIR=$LINUX_DIR/driver

echo "========================================"
echo "  HDC1080 Linux Kernel Environment Setup"
echo "========================================"

# Step 1: 下載 Buildroot
if [ ! -d "$BUILDROOT_DIR" ]; then
    echo "[1] Downloading Buildroot ${BUILDROOT_VERSION}..."
    cd $LINUX_DIR
    wget -q --show-progress \
        https://buildroot.org/downloads/buildroot-${BUILDROOT_VERSION}.tar.gz
    tar xzf buildroot-${BUILDROOT_VERSION}.tar.gz
    mv buildroot-${BUILDROOT_VERSION} buildroot
    rm buildroot-${BUILDROOT_VERSION}.tar.gz
    echo "    done"
else
    echo "[1] Buildroot already exists, skipping"
fi

cd $BUILDROOT_DIR

# Step 2: defconfig
echo "[2] Applying AST2600 defconfig..."
make aspeed_ast2600evb_defconfig
echo "    done"

# Step 3: 追加 kernel config（不需要進 menuconfig）
echo "[3] Enabling CONFIG_MODULES..."
cat >> $BUILDROOT_DIR/.config << 'KCONF'
CONFIG_MODULES=y
CONFIG_MODULE_UNLOAD=y
KCONF
make olddefconfig
echo "    done"

# Step 4: 設定 rootfs overlay（不需要進 menuconfig）
echo "[4] Setting rootfs overlay..."
if grep -q "BR2_ROOTFS_OVERLAY" $BUILDROOT_DIR/.config; then
    sed -i "s|BR2_ROOTFS_OVERLAY=.*|BR2_ROOTFS_OVERLAY=\"$OVERLAY\"|" $BUILDROOT_DIR/.config
else
    echo "BR2_ROOTFS_OVERLAY=\"$OVERLAY\"" >> $BUILDROOT_DIR/.config
fi
make olddefconfig
echo "    done"

# Step 5: 第一次完整編譯
echo "[5] Building Buildroot (1-2 hours on first run)..."
make -j$(nproc) 2>&1 | tee /tmp/buildroot.log
echo "    done"

# Step 6: 套用 DTS patch
echo "[6] Applying DTS patch..."
KDIR=$(ls -d $BUILDROOT_DIR/output/build/linux-* | head -1)
if git -C "$KDIR" apply --check "$PATCH" 2>/dev/null; then
    git -C "$KDIR" apply "$PATCH"
    echo "    patch applied"
else
    echo "    patch already applied, skipping"
fi

# Step 7: 重編 kernel
echo "[7] Rebuilding kernel with HDC1080 DTS..."
make linux-rebuild
make -j$(nproc)
echo "    done"

# Step 8: 編譯 hdc1080.ko
echo "[8] Building hdc1080 kernel module..."
CROSS=$(ls $BUILDROOT_DIR/output/host/bin/arm-buildroot-linux-gnueabihf-gcc | sed 's/gcc$//')
make -C "$KDIR" \
    M=$DRIVER_DIR \
    ARCH=arm \
    CROSS_COMPILE=$CROSS \
    modules

mkdir -p $OVERLAY/lib/modules
cp $DRIVER_DIR/hdc1080.ko $OVERLAY/lib/modules/
echo "    done"

# Step 9: 重新打包 rootfs（含 .ko）
echo "[9] Repacking rootfs..."
make -j$(nproc)
echo "    done"

echo ""
echo "========================================"
echo "  Setup complete!"
echo "  Run: bash $LINUX_DIR/scripts/run_qemu_ast2600.sh"
echo "========================================"
