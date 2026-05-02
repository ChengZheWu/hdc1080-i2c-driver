#!/bin/sh
# 在 QEMU AST2600 Linux 上執行此腳本

echo "=== HDC1080 Driver Test ==="

# 載入 module
echo "[1] Loading kernel module..."
modprobe hdc1080 || insmod /lib/modules/hdc1080.ko
echo "    done"

# 確認 module 載入
echo "[2] Module info:"
lsmod | grep hdc1080

# 尋找 IIO 裝置
echo "[3] IIO devices:"
ls /sys/bus/iio/devices/

IIO_DEV=$(ls /sys/bus/iio/devices/ | grep iio:device | head -1)
if [ -z "$IIO_DEV" ]; then
    echo "ERROR: No IIO device found"
    exit 1
fi

IIO_PATH=/sys/bus/iio/devices/$IIO_DEV
echo "    Found: $IIO_PATH"

# 讀取感測器名稱
echo "[4] Device name: $(cat $IIO_PATH/name)"

# 讀取原始溫度值
echo "[5] Reading temperature..."
TEMP_RAW=$(cat $IIO_PATH/in_temp_raw)
TEMP_SCALE=$(cat $IIO_PATH/in_temp_scale)
TEMP_OFFSET=$(cat $IIO_PATH/in_temp_offset)
echo "    Raw: $TEMP_RAW"
echo "    Scale: $TEMP_SCALE"
echo "    Offset: $TEMP_OFFSET"
echo "    Formula: (raw + offset) * scale / 1000 = temperature in °C"

# 讀取原始濕度值
echo "[6] Reading humidity..."
HUM_RAW=$(cat $IIO_PATH/in_humidityrelative_raw)
HUM_SCALE=$(cat $IIO_PATH/in_humidityrelative_scale)
echo "    Raw: $HUM_RAW"
echo "    Scale: $HUM_SCALE"

# 連續讀取 5 次
echo ""
echo "[7] Continuous reading (5 samples):"
for i in $(seq 1 5); do
    TR=$(cat $IIO_PATH/in_temp_raw)
    HR=$(cat $IIO_PATH/in_humidityrelative_raw)
    echo "    Sample $i: temp_raw=$TR  hum_raw=$HR"
    sleep 1
done

echo ""
echo "=== Test complete ==="
