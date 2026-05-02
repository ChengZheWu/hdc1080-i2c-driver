#ifndef UART_H
#define UART_H

#include <stdint.h>

/* MPS2-AN385 UART0 基底位址 */
#define UART0_BASE  0x40004000UL

/* UART 暫存器偏移 */
#define UART_DATA   (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART_STATE  (*(volatile uint32_t *)(UART0_BASE + 0x04))
#define UART_CTRL   (*(volatile uint32_t *)(UART0_BASE + 0x08))
#define UART_BAUDDIV (*(volatile uint32_t *)(UART0_BASE + 0x10))

/* UART_STATE bits */
#define UART_STATE_TX_FULL  (1 << 0)

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_printf(const char *fmt, ...);

#endif /* UART_H */
