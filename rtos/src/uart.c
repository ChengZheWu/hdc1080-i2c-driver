#include "uart.h"
#include <stdarg.h>
#include <stdint.h>

void uart_init(void) {
    UART_BAUDDIV = 16;
    UART_CTRL    = 0x01;
}

void uart_putc(char c) {
    while (UART_STATE & UART_STATE_TX_FULL);
    UART_DATA = c;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

static void print_uint_padded(uint32_t n, int base, int width, char pad) {
    char buf[16];
    int  i = 0;
    if (n == 0) { buf[i++] = '0'; }
    const char *digits = "0123456789abcdef";
    while (n > 0) { buf[i++] = digits[n % base]; n /= base; }
    while (i < width) { buf[i++] = pad; }
    while (i > 0) uart_putc(buf[--i]);
}

void uart_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    while (*fmt) {
        if (*fmt != '%') {
            if (*fmt == '\n') uart_putc('\r');
            uart_putc(*fmt++);
            continue;
        }
        fmt++;
        char pad = ' ';
        if (*fmt == '0') { pad = '0'; fmt++; }
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') { width = width * 10 + (*fmt++ - '0'); }
        switch (*fmt) {
        case 'd': {
            int v = va_arg(ap, int);
            if (v < 0) { uart_putc('-'); v = -v; }
            print_uint_padded((uint32_t)v, 10, width, pad);
            break;
        }
        case 'u': print_uint_padded(va_arg(ap, uint32_t), 10, width, pad); break;
        case 'x': print_uint_padded(va_arg(ap, uint32_t), 16, width, pad); break;
        case 's': uart_puts(va_arg(ap, const char *)); break;
        case 'c': uart_putc((char)va_arg(ap, int)); break;
        case '%': uart_putc('%'); break;
        default:   uart_putc('%'); uart_putc(*fmt); break;
        }
        fmt++;
    }
    va_end(ap);
}
