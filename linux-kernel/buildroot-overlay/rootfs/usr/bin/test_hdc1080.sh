#!/bin/sh
# 在 QEMU AST2600 Linux 上執行此腳本

echo "=== HDC1080 Driver Test ==="

echo "[1] Loading kernel module..."
modprobe hdc1080 || insmod /lib/modules/hdc1080.ko
echo "    done"

echo "[2] Module info:"
lsmod | grep hdc1080

echo "[3] IIO devices:"
ls /sys/bus/iio/devices/

IIO_DEV=$(ls /sys/bus/iio/devices/ | grep iio:device | head -1)
if [ -z "$IIO_DEV" ]; then
    echo "ERROR: No IIO device found"
    exit 1
fi

IIO_PATH=/sys/bus/iio/devices/$IIO_DEV
echo "    Found: $IIO_PATH"
echo "[4] Device name: $(cat $IIO_PATH/name)"
echo ""

echo "[5] Continuous reading (5 samples):"
echo "    Formula: Temp(C)  = (raw + offset) x scale / 1000"
echo "             Humi(%)  = raw x scale / 1000"
echo ""

i=1
while [ $i -le 5 ]; do
    T_RAW=$(cat $IIO_PATH/in_temp_raw)
    T_OFF=$(cat $IIO_PATH/in_temp_offset)
    H_RAW=$(cat $IIO_PATH/in_humidityrelative_raw)

    # scale=2.5177 -> 乘以 25177 再除 10000
    T_MDEG=$(( (T_RAW + T_OFF) * 25177 / 10000 ))
    T_INT=$(( T_MDEG / 1000 ))
    T_FRAC=$(( T_MDEG % 1000 ))
    if [ $T_FRAC -lt 0 ]; then T_FRAC=$(( -T_FRAC )); fi

    # scale=1.525879 -> 乘以 1525879 再除 1000000
    H_MDEG=$(( H_RAW * 1525879 / 1000000 ))
    H_INT=$(( H_MDEG / 1000 ))
    H_FRAC=$(( H_MDEG % 1000 ))

    printf "    Sample %d: Temp: %d.%03d C  Humidity: %d.%03d %%\n" \
        $i $T_INT $T_FRAC $H_INT $H_FRAC

    i=$(( i + 1 ))
    sleep 1
done

echo ""
echo "=== Test complete ==="
