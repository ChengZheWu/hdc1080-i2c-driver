#ifndef I2C_H
#define I2C_H

#include <stdint.h>

/*
 * MPS2-AN385 CMSDK APB I2C
 * 基底位址：0x40022000
 *
 * 這個 I2C controller 是簡化版：
 *   - 用 bit-bang 方式透過 SDA/SCL GPIO 控制
 *   - 本專案用模擬數據，controller 的存取流程符合真實協定
 */
#define I2C_BASE    0x40022000UL

/* I2C 暫存器（CMSDK APB I2C） */
typedef struct {
    volatile uint32_t CTRL;   /* 0x00: Control */
    volatile uint32_t STAT;   /* 0x04: Status */
    volatile uint32_t DATA;   /* 0x08: Data */
    volatile uint32_t ADDR;   /* 0x0C: Address */
} I2C_TypeDef;

#define I2C  ((I2C_TypeDef *)I2C_BASE)

/* CTRL bits */
#define I2C_CTRL_START  (1 << 0)
#define I2C_CTRL_STOP   (1 << 1)
#define I2C_CTRL_ACKEN  (1 << 2)
#define I2C_CTRL_EN     (1 << 7)

/* STAT bits */
#define I2C_STAT_DONE   (1 << 0)
#define I2C_STAT_ACK    (1 << 1)
#define I2C_STAT_BUSY   (1 << 2)

/* 回傳值 */
#define I2C_OK    0
#define I2C_ERR  -1

void    i2c_init(void);
int     i2c_write(uint8_t addr, const uint8_t *data, uint32_t len);
int     i2c_read(uint8_t addr, uint8_t *data, uint32_t len);
int     i2c_write_read(uint8_t addr,
                       const uint8_t *wbuf, uint32_t wlen,
                       uint8_t *rbuf,       uint32_t rlen);

#endif /* I2C_H */
