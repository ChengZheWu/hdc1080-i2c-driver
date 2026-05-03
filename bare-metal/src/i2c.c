#include "i2c.h"
#include "uart.h"
#include <stdint.h>

/*
 * I2C Driver for MPS2-AN385
 *
 * 在真實驅動中，每一步都要等 controller 完成（polling STAT.DONE）
 * 或是用 interrupt。這裡用 polling，符合 bare-metal 常見作法。
 *
 * 因為 QEMU 的 CMSDK I2C 模型不完整，我們模擬 STAT 回傳，
 * 但整個流程（start → addr → data → stop）與真實硬體一致。
 */

/* 
 * 等待操作完成，避免因為 timeout 死鎖
 * QEMU 環境：STAT 暫存器不會更新，直接回傳 OK
 * 真實硬體需在此輪詢 I2C->STAT & I2C_STAT_DONE，並加 timeout
 */
static int i2c_wait_done(void) {
    return I2C_OK;
}

void i2c_init(void) {
    /* 啟用 I2C controller */
    I2C->CTRL = I2C_CTRL_EN | I2C_CTRL_ACKEN;
    uart_puts("[i2c] controller initialized\n");
}

/*
 * i2c_write - 送出 START + addr(W) + data bytes + STOP
 * addr: 7-bit slave address
 */
int i2c_write(uint8_t addr, const uint8_t *data, uint32_t len) {
    /* START condition */
    I2C->CTRL |= I2C_CTRL_START;

    /* 送出 7-bit addr + W bit(0) */
    I2C->DATA = (uint32_t)(addr << 1) & 0xFE;
    if (i2c_wait_done() != I2C_OK) return I2C_ERR;

    /* 送出資料 bytes */
    for (uint32_t i = 0; i < len; i++) {
        I2C->DATA = data[i];
        if (i2c_wait_done() != I2C_OK) return I2C_ERR;
    }

    /* STOP condition */
    I2C->CTRL |= I2C_CTRL_STOP;
    return I2C_OK;
}

/*
 * i2c_read - START + addr(R) + 讀取 data bytes + STOP
 */
int i2c_read(uint8_t addr, uint8_t *data, uint32_t len) {
    /* START condition */
    I2C->CTRL |= I2C_CTRL_START;

    /* 送出 7-bit addr + R bit(1) */
    I2C->DATA = (uint32_t)((addr << 1) | 0x01);
    if (i2c_wait_done() != I2C_OK) return I2C_ERR;

    /* 讀取資料 bytes */
    for (uint32_t i = 0; i < len; i++) {
        /* 最後一個 byte 送 NACK */
        if (i == len - 1) {
            I2C->CTRL &= ~I2C_CTRL_ACKEN;
        }
        if (i2c_wait_done() != I2C_OK) return I2C_ERR;
        data[i] = (uint8_t)(I2C->DATA & 0xFF);
    }

    /* STOP condition */
    I2C->CTRL |= I2C_CTRL_STOP;
    I2C->CTRL |= I2C_CTRL_ACKEN; /* 恢復 ACK */
    return I2C_OK;
}

/*
 * i2c_write_read - Write-then-Read（Repeated START）
 * 這是存取感測器暫存器的標準流程：
 *   1. START + addr(W) + reg_addr
 *   2. Repeated START + addr(R) + read data
 */
int i2c_write_read(uint8_t addr,
                   const uint8_t *wbuf, uint32_t wlen,
                   uint8_t *rbuf,       uint32_t rlen) {
    if (i2c_write(addr, wbuf, wlen) != I2C_OK) return I2C_ERR;
    if (i2c_read(addr, rbuf, rlen)  != I2C_OK) return I2C_ERR;
    return I2C_OK;
}
