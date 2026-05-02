#!/bin/bash
set -e

WORKSPACE=/workspace
LINUX_DIR=$WORKSPACE/linux-kernel
BUILDROOT_DIR=$LINUX_DIR/buildroot
BUILDROOT_VERSION="2024.02"
PATCH=$LINUX_DIR/scripts/0001-add-hdc1080-to-ast2600-i2c1.patch
KDIR_PATTERN="$BUILDROOT_DIR/output/build/linux-*"
CROSS_PREFIX="arm-buildroot-linux-gnueabihf-"
CROSS="$BUILDROOT_DIR/output/host/bin/$CROSS_PREFIX"
OVERLAY=$LINUX_DIR/buildroot-overlay/rootfs

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

# Step 2: defconfig
echo "[2] Applying AST2600 defconfig..."
cd $BUILDROOT_DIR
make aspeed_ast2600evb_defconfig
echo "    done"

# Step 3: 開啟 CONFIG_MODULES
echo "[3] Enabling CONFIG_MODULES..."
./support/kconfig/merge_config.sh -m .config - << 'KCONFIG'
CONFIG_MODULES=y
CONFIG_MODULE_UNLOAD=y
KCONFIG
make olddefconfig
echo "    done"

# Step 4: 設定 rootfs overlay
echo "[4] Setting rootfs overlay..."
grep -q "BR2_ROOTFS_OVERLAY" .config && \
    sed -i "s|BR2_ROOTFS_OVERLAY=.*|BR2_ROOTFS_OVERLAY=\"$OVERLAY\"|" .config || \
    echo "BR2_ROOTFS_OVERLAY=\"$OVERLAY\"" >> .config
echo "    done"

# Step 5: 第一次完整編譯
echo "[5] Building Buildroot (1-2 hours first time)..."
make -j$(nproc) 2>&1 | tee /tmp/buildroot.log
echo "    done"

# Step 6: 套用 DTS patch
echo "[6] Applying DTS patch..."
KDIR=$(ls -d $KDIR_PATTERN | head -1)
if git -C "$KDIR" apply --check "$PATCH" 2>/dev/null; then
    git -C "$KDIR" apply "$PATCH"
    echo "    patch applied"
else
    echo "    patch already applied or conflict, skipping"
fi

# Step 7: 重編 kernel + rootfs
echo "[7] Rebuilding kernel with HDC1080 DTS..."
make linux-rebuild
make -j$(nproc)
echo "    done"

# Step 8: 編譯 hdc1080.ko
echo "[8] Building hdc1080 kernel module..."
make -C "$KDIR" \
    M=$LINUX_DIR/driver \
    ARCH=arm \
    CROSS_COMPILE=$CROSS \
    modules

mkdir -p $OVERLAY/lib/modules
cp $LINUX_DIR/driver/hdc1080.ko $OVERLAY/lib/modules/

# 重新打包 rootfs（含 .ko）
make -j$(nproc)
echo "    done"

echo ""
echo "========================================"
echo "  Setup complete!"
echo "  Run: bash $LINUX_DIR/scripts/run_qemu_ast2600.sh"
echo "========================================"
