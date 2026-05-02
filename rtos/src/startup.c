#include <stdint.h>

extern uint32_t _etext, _sdata, _edata, _sbss, _ebss, _stack_top;
extern int main(void);

/* FreeRTOS handlers（弱符號，FreeRTOS 會覆蓋） */
void SysTick_Handler(void) __attribute__((weak));
void PendSV_Handler(void)  __attribute__((weak));
void SVC_Handler(void)     __attribute__((weak));
void SysTick_Handler(void) { while(1); }
void PendSV_Handler(void)  { while(1); }
void SVC_Handler(void)     { while(1); }

void Default_Handler(void) { while(1); }

void Reset_Handler(void) {
    uint32_t *src = &_etext, *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    dst = &_sbss;
    while (dst < &_ebss) *dst++ = 0;
    main();
    while(1);
}

__attribute__((section(".vector_table")))
uint32_t vector_table[] = {
    (uint32_t)&_stack_top,
    (uint32_t)Reset_Handler,
    (uint32_t)Default_Handler,   /* NMI */
    (uint32_t)Default_Handler,   /* HardFault */
    (uint32_t)Default_Handler,   /* MemManage */
    (uint32_t)Default_Handler,   /* BusFault */
    (uint32_t)Default_Handler,   /* UsageFault */
    0, 0, 0, 0,                  /* Reserved */
    (uint32_t)SVC_Handler,
    (uint32_t)Default_Handler,   /* DebugMon */
    0,
    (uint32_t)PendSV_Handler,
    (uint32_t)SysTick_Handler,
};
