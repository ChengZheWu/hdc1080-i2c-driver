#!/bin/bash
set -e

BUILDROOT=/workspace/linux-kernel/buildroot
OVERLAY=/workspace/linux-kernel/buildroot-overlay

echo "=== Configuring Buildroot for AST2600 ==="

cd $BUILDROOT

# 使用 AST2600 預設設定作為基礎
make aspeed_ast2600_defconfig

# 用 kconfig 工具修改設定（以下是業界最常用的嵌入式 Linux 選項）
# 使用 sed 修改 .config（自動化 CI 的做法）

# 目標架構：ARM
sed -i 's/^BR2_arm=.*/BR2_arm=y/' .config
# 使用者空間：busybox（最小系統）
grep -q BR2_PACKAGE_BUSYBOX .config || echo "BR2_PACKAGE_BUSYBOX=y" >> .config
# 啟用 overlayfs 支援
echo "BR2_PACKAGE_UTIL_LINUX=y" >> .config || true

# 更新 config（填入預設值）
make olddefconfig

echo "=== Buildroot configured ==="
echo "=== Run 'make' in $BUILDROOT to build ==="
