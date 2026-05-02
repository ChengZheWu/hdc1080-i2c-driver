#include "uart.h"
#include "i2c.h"
#include "hdc1080.h"
#include <stdint.h>

static void delay(volatile uint32_t count) { while (count--); }

int main(void) {
    uart_init();
    uart_puts("\n========================================\n");
    uart_puts("  HDC1080 I2C Driver - Bare-metal Demo  \n");
    uart_puts("  Target: MPS2-AN385 (Cortex-M3)        \n");
    uart_puts("  Mode: Simulation (QEMU)                \n");
    uart_puts("========================================\n\n");

    i2c_init();

    if (hdc1080_init() != 0) {
        uart_puts("[main] sensor init failed!\n");
        while (1);
    }

    uart_puts("\n[main] starting sensor polling loop...\n\n");

    uint32_t sample = 0;
    while (1) {
        hdc1080_data_t data;
        if (hdc1080_read(&data) == 0) {
            int32_t  t_abs  = data.temp_x100 < 0 ? -data.temp_x100 : data.temp_x100;
            int32_t  t_int  = t_abs / 100;
            uint32_t t_frac = (uint32_t)(t_abs % 100);
            uint32_t h_int  = data.hum_x100 / 100;
            uint32_t h_frac = data.hum_x100 % 100;
            if (data.temp_x100 < 0) uart_putc('-');
            uart_printf("[%u] Temp: %d.%02u C  Humidity: %u.%02u %%\n",
                        ++sample, t_int, t_frac, h_int, h_frac);
        } else {
            uart_puts("[main] read error\n");
        }
        delay(2000000);
    }
    return 0;
}
