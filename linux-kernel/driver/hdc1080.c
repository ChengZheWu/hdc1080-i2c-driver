// SPDX-License-Identifier: GPL-2.0-only
/*
 * HDC1080 - Temperature and Humidity Sensor I2C Driver
 *
 * Copyright (C) 2024 - Embedded Systems Demo
 *
 * 業界實務說明：
 * Linux I2C 驅動使用 IIO (Industrial I/O) 子系統暴露感測器數據。
 * 驅動結構：
 *   - probe/remove：裝置生命週期管理
 *   - IIO channel 定義：描述每個量測值
 *   - read_raw：實際讀取感測器
 *   - of_match_table：Device Tree 匹配
 *
 * 使用方式：
 *   cat /sys/bus/iio/devices/iio:device0/in_temp_input
 *   cat /sys/bus/iio/devices/iio:device0/in_humidityrelative_input
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/delay.h>
#include <linux/regmap.h>

/* HDC1080 registers */
#define HDC1080_REG_TEMP    0x00
#define HDC1080_REG_HUM     0x01
#define HDC1080_REG_CONFIG  0x02
#define HDC1080_REG_MFR_ID  0xFE
#define HDC1080_REG_DEV_ID  0xFF

#define HDC1080_I2C_ADDR    0x40
#define HDC1080_DEV_ID      0x1050
#define HDC1080_MFR_ID      0x5449  /* "TI" */

/* 量測等待時間（ms） */
#define HDC1080_CONV_TIME_MS  7

/*
 * 驅動私有資料結構
 * 每個感測器實例有一份，存在 iio_dev 的 priv 區域
 */
struct hdc1080_data {
    struct i2c_client *client;
    struct mutex       lock;     /* 保護 I2C 操作 */
    bool               simulated; /* QEMU 模擬模式 */
};

/*
 * IIO channel 定義
 * 這告訴 IIO 子系統這個裝置提供哪些量測值
 */
static const struct iio_chan_spec hdc1080_channels[] = {
    {
        /* 溫度通道 */
        .type  = IIO_TEMP,
        .info_mask_separate =
            BIT(IIO_CHAN_INFO_RAW)    |  /* 原始 ADC 值 */
            BIT(IIO_CHAN_INFO_SCALE)  |  /* 換算比例 */
            BIT(IIO_CHAN_INFO_OFFSET),   /* 偏移量 */
        .indexed = 0,
        .channel = 0,
    },
    {
        /* 相對濕度通道 */
        .type  = IIO_HUMIDITYRELATIVE,
        .info_mask_separate =
            BIT(IIO_CHAN_INFO_RAW)   |
            BIT(IIO_CHAN_INFO_SCALE),
        .indexed = 0,
        .channel = 0,
    },
};

/*
 * hdc1080_read_reg16 - 讀取 16-bit 感測器暫存器
 *
 * HDC1080 的讀取協定：
 *   1. Write: slave_addr + reg_addr（觸發量測）
 *   2. 等待量測完成（約 6.5ms）
 *   3. Read: slave_addr → 2 bytes MSB first
 */
static int hdc1080_read_reg16(struct hdc1080_data *data,
                               u8 reg, u16 *val)
{
    struct i2c_client *client = data->client;
    u8 buf[2];
    int ret;

    /* 模擬模式：直接回傳固定值 */
    if (data->simulated) {
        if (reg == HDC1080_REG_TEMP) {
            *val = 0x6800; /* ~25°C */
        } else if (reg == HDC1080_REG_HUM) {
            *val = 0x9999; /* ~60% */
        } else if (reg == HDC1080_REG_MFR_ID) {
            *val = HDC1080_MFR_ID;
        } else if (reg == HDC1080_REG_DEV_ID) {
            *val = HDC1080_DEV_ID;
        } else {
            *val = 0;
        }
        return 0;
    }

    /* 真實硬體：Step 1 - 寫入暫存器位址（觸發量測） */
    ret = i2c_smbus_write_byte(client, reg);
    if (ret < 0) {
        dev_err(&client->dev, "failed to write reg 0x%02x: %d\n",
                reg, ret);
        return ret;
    }

    /* Step 2 - 等待量測完成 */
    if (reg == HDC1080_REG_TEMP || reg == HDC1080_REG_HUM)
        msleep(HDC1080_CONV_TIME_MS);

    /* Step 3 - 讀取 2 bytes */
    ret = i2c_master_recv(client, buf, 2);
    if (ret != 2) {
        dev_err(&client->dev, "failed to read: %d\n", ret);
        return ret < 0 ? ret : -EIO;
    }

    *val = (u16)(buf[0] << 8) | buf[1];
    return 0;
}

/*
 * hdc1080_read_raw - IIO 子系統呼叫此函式讀取數據
 *
 * val/val2 的意義由 mask 決定：
 *   IIO_CHAN_INFO_RAW:   val = ADC 原始值
 *   IIO_CHAN_INFO_SCALE: val.val2 組成浮點 = val + val2/10^6
 *   IIO_CHAN_INFO_OFFSET: val = 偏移量
 *
 * 業界實務：scale 和 offset 讓 user-space 不需要知道感測器細節，
 * 只要 raw * scale + offset 就能得到物理量。
 */
static int hdc1080_read_raw(struct iio_dev *indio_dev,
                             struct iio_chan_spec const *chan,
                             int *val, int *val2, long mask)
{
    struct hdc1080_data *data = iio_priv(indio_dev);
    u16 raw;
    int ret;

    switch (mask) {
    case IIO_CHAN_INFO_RAW:
        mutex_lock(&data->lock);
        if (chan->type == IIO_TEMP)
            ret = hdc1080_read_reg16(data, HDC1080_REG_TEMP, &raw);
        else
            ret = hdc1080_read_reg16(data, HDC1080_REG_HUM, &raw);
        mutex_unlock(&data->lock);

        if (ret)
            return ret;
        *val = raw;
        return IIO_VAL_INT;

    case IIO_CHAN_INFO_SCALE:
        if (chan->type == IIO_TEMP) {
            /*
             * T(m°C) = raw * 165000 / 65536 - 40000
             * scale = 165000/65536 ≈ 2.5177...
             * IIO 慣例：溫度用 m°C，所以 scale=2, val2=517700
             */
            *val  = 2;
            *val2 = 517700;
        } else {
            /*
             * H(m%) = raw * 100000 / 65536
             * scale = 100000/65536 ≈ 1.52587...
             */
            *val  = 1;
            *val2 = 525879;
        }
        return IIO_VAL_INT_PLUS_MICRO;

    case IIO_CHAN_INFO_OFFSET:
        if (chan->type == IIO_TEMP) {
            /*
             * offset = -40°C = -40000 m°C
             * 但 IIO 的 offset 是在 scale 之前套用，
             * 所以 offset = -40000 / scale = -40000 / 2.5177 ≈ -15887
             */
            *val  = -15887;
            *val2 = 0;
            return IIO_VAL_INT;
        }
        return -EINVAL;

    default:
        return -EINVAL;
    }
}

/* IIO ops 結構：連接 callback */
static const struct iio_info hdc1080_info = {
    .read_raw = hdc1080_read_raw,
};

/*
 * hdc1080_probe - 驅動 probe 函式
 *
 * 當 I2C bus 上出現匹配的裝置時，kernel 呼叫此函式。
 * 業界實務：probe 應該：
 *   1. 分配 iio_dev
 *   2. 初始化硬體
 *   3. 驗證裝置 ID
 *   4. 設定 IIO 描述
 *   5. 向 IIO 子系統註冊
 */
static int hdc1080_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct iio_dev      *indio_dev;
    struct hdc1080_data *data;
    u16 dev_id;
    int ret;

    /* 分配 iio_dev + 私有資料空間 */
    indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
    if (!indio_dev)
        return -ENOMEM;

    data = iio_priv(indio_dev);
    data->client = client;
    mutex_init(&data->lock);

    /* 檢查是否在 QEMU 模擬環境 */
    data->simulated = of_property_read_bool(client->dev.of_node,
                                             "ti,simulated");
    if (data->simulated)
        dev_info(&client->dev, "running in simulation mode\n");

    /* 驗證裝置 ID */
    ret = hdc1080_read_reg16(data, HDC1080_REG_DEV_ID, &dev_id);
    if (ret) {
        dev_err(&client->dev, "failed to read device ID\n");
        return ret;
    }

    if (!data->simulated && dev_id != HDC1080_DEV_ID) {
        dev_err(&client->dev,
                "unexpected device ID: 0x%04x (expected 0x%04x)\n",
                dev_id, HDC1080_DEV_ID);
        return -ENODEV;
    }

    dev_info(&client->dev, "device ID: 0x%04x\n", dev_id);

    /* 設定 IIO 裝置描述 */
    indio_dev->name     = "hdc1080";
    indio_dev->channels = hdc1080_channels;
    indio_dev->num_channels = ARRAY_SIZE(hdc1080_channels);
    indio_dev->info     = &hdc1080_info;
    indio_dev->modes    = INDIO_DIRECT_MODE;

    /* 向 IIO 子系統註冊裝置 */
    ret = devm_iio_device_register(&client->dev, indio_dev);
    if (ret) {
        dev_err(&client->dev, "failed to register IIO device: %d\n", ret);
        return ret;
    }

    dev_info(&client->dev, "HDC1080 initialized successfully\n");
    return 0;
}

/* Device Tree 匹配表 */
static const struct of_device_id hdc1080_of_match[] = {
    { .compatible = "ti,hdc1080" },
    { }
};
MODULE_DEVICE_TABLE(of, hdc1080_of_match);

/* I2C 裝置 ID 表（非 DT 系統用） */
static const struct i2c_device_id hdc1080_id[] = {
    { "hdc1080", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, hdc1080_id);

/* I2C driver 結構 */
static struct i2c_driver hdc1080_driver = {
    .driver = {
        .name   = "hdc1080",
        .of_match_table = hdc1080_of_match,
    },
    .probe    = hdc1080_probe,
    .id_table = hdc1080_id,
};

module_i2c_driver(hdc1080_driver);

MODULE_AUTHOR("Embedded Systems Demo");
MODULE_DESCRIPTION("HDC1080 Temperature and Humidity Sensor Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");
