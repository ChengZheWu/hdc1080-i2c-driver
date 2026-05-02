#ifndef HDC1080_H
#define HDC1080_H

#include <stdint.h>

/*
 * HDC1080 - 溫濕度感測器
 * I2C 位址：0x40
 *
 * 暫存器表（參考 TI datasheet）：
 *   0x00 - Temperature    (read 2 bytes)
 *   0x01 - Humidity       (read 2 bytes)
 *   0x02 - Configuration  (read/write 2 bytes)
 *   0xFB - Serial ID[2:0]
 *   0xFC - Serial ID[1]
 *   0xFD - Serial ID[0]
 *   0xFE - Manufacturer ID (0x5449)
 *   0xFF - Device ID       (0x1050)
 */

#define HDC1080_I2C_ADDR    0x40

/* 暫存器位址 */
#define HDC1080_REG_TEMP    0x00
#define HDC1080_REG_HUM     0x01
#define HDC1080_REG_CONFIG  0x02
#define HDC1080_REG_MFR_ID  0xFE
#define HDC1080_REG_DEV_ID  0xFF

/* Config 暫存器 bits */
#define HDC1080_CONFIG_RESET  (1 << 15)
#define HDC1080_CONFIG_HEAT   (1 << 13) /* heater on */
#define HDC1080_CONFIG_MODE   (1 << 12) /* 1=同時量測 T+H */
#define HDC1080_CONFIG_TRES14 (0 << 10) /* 14-bit temp */
#define HDC1080_CONFIG_HRES14 (0 << 8)  /* 14-bit humid */

/* 換算公式（來自 datasheet）：
 *   Temperature(°C) = (raw / 65536) * 165 - 40
 *   Humidity(%)     = (raw / 65536) * 100
 */

/* 回傳 x100 避免浮點（bare-metal 常見作法） */
typedef struct {
    int32_t  temp_x100;   /* 溫度 × 100，例如 2550 = 25.50°C */
    uint32_t hum_x100;    /* 濕度 × 100，例如 6000 = 60.00% */
} hdc1080_data_t;

int hdc1080_init(void);
int hdc1080_read(hdc1080_data_t *out);

#endif /* HDC1080_H */
