#!/bin/bash
set -e

echo "=== Post-create setup ==="

# 驗證工具鏈
echo "[1] 檢查 arm-none-eabi-gcc..."
arm-none-eabi-gcc --version

echo "[2] 檢查 arm-linux-gnueabihf-gcc..."
arm-linux-gnueabihf-gcc --version

echo "[3] 檢查 QEMU..."
qemu-system-arm --version

echo "[4] 確認目錄結構..."
ls /workspace

echo "=== 環境就緒 ==="
