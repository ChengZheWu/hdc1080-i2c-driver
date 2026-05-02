#include "hdc1080.h"
#include "i2c.h"
#include "uart.h"
#include <stdint.h>

/*
 * HDC1080 Driver - Bare-metal 版本
 *
 * 模擬數據說明：
 * 真實 HDC1080 需要等待量測完成（~6.5ms），
 * 在 QEMU 環境中我們用固定的原始值模擬，
 * 換算公式與真實硬體完全相同。
 *
 * 業界實務：
 * - init 時驗證 Device ID，確保是正確的感測器
 * - 量測前送出暫存器位址，量測完再讀回
 * - 結果用整數 ×100 表示，避免 bare-metal 引入 libm
 */

/* 模擬的原始感測器數值 */
#define SIM_TEMP_RAW  0x6800U  /* ≈ 25.00°C */
#define SIM_HUM_RAW   0x9999U  /* ≈ 60.00% */

int hdc1080_init(void) {
    uint8_t reg = HDC1080_REG_DEV_ID;
    uint8_t id[2];

    uart_puts("[hdc1080] init...\n");

    /*
     * 讀 Device ID：先寫暫存器位址，再讀 2 bytes
     * 真實硬體：I2C write 0x40 0xFF → I2C read 0x40 → 得到 0x1050
     * QEMU 模擬：我們直接回傳假的 ID
     */
    if (i2c_write_read(HDC1080_I2C_ADDR, &reg, 1, id, 2) != I2C_OK) {
        uart_puts("[hdc1080] i2c error\n");
        return -1;
    }

    /* QEMU 不會真的回傳 0x1050，跳過 ID 驗證，印出提示 */
    uart_printf("[hdc1080] device ID read (simulated): 0x%x%x\n", id[0], id[1]);
    uart_puts("[hdc1080] (simulation mode: skipping ID check)\n");

    /* 寫入 config：14-bit 解析度，分開量測模式 */
    uint8_t cfg_cmd[3];
    cfg_cmd[0] = HDC1080_REG_CONFIG;
    cfg_cmd[1] = 0x00; /* MSB */
    cfg_cmd[2] = 0x00; /* LSB - 預設值 */
    i2c_write(HDC1080_I2C_ADDR, cfg_cmd, 3);

    uart_puts("[hdc1080] init done\n");
    return 0;
}

int hdc1080_read(hdc1080_data_t *out) {
    uint8_t reg;
    uint8_t raw[2];

    /*
     * Step 1: 讀溫度
     * 送 0x00 (temp reg)，等待 6.5ms（QEMU 跳過），再讀 2 bytes
     */
    reg = HDC1080_REG_TEMP;
    if (i2c_write_read(HDC1080_I2C_ADDR, &reg, 1, raw, 2) != I2C_OK) {
        return -1;
    }

    /*
     * 在 QEMU 中 I2C 不會真的返回感測器數值，
     * 所以我們注入模擬原始值
     */
    uint16_t temp_raw = ((uint16_t)SIM_TEMP_RAW);
    uint16_t hum_raw  = ((uint16_t)SIM_HUM_RAW);

    /*
     * 換算公式（來自 TI HDC1080 datasheet）：
     *   T(°C) = (raw / 2^16) * 165 - 40
     *
     * 避免浮點：
     *   T * 100 = (raw * 16500 / 65536) - 4000
     *   使用 32-bit 乘法避免溢位
     */
    out->temp_x100 = (int32_t)((uint32_t)temp_raw * 16500U / 65536U) - 4000;

    /*
     * H(%) = (raw / 2^16) * 100
     * H * 100 = raw * 10000 / 65536
     */
    out->hum_x100 = (uint32_t)hum_raw * 10000U / 65536U;

    /* Step 2: 讀濕度（流程同上） */
    reg = HDC1080_REG_HUM;
    i2c_write_read(HDC1080_I2C_ADDR, &reg, 1, raw, 2);

    return 0;
}
