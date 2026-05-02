#include <stdint.h>

/* 從 linker script 匯入的符號 */
extern uint32_t _etext;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _stack_top;

/* 主程式入口（由 main.c 定義） */
extern int main(void);

/* Reset Handler：硬體重置後第一個執行的函式 */
void Reset_Handler(void) {
    uint32_t *src, *dst;

    /* 1. 把 .data section 從 Flash 複製到 SRAM */
    src = &_etext;
    dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* 2. 清零 .bss section */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    /* 3. 跳入主程式 */
    main();

    /* 不應該到這裡，若回來就無限迴圈 */
    while (1);
}

/* 預設的例外處理（弱符號，可被覆蓋） */
void Default_Handler(void) {
    while (1);
}

/* 向量表：放在最開頭，順序固定 */
__attribute__((section(".vector_table")))
uint32_t vector_table[] = {
    (uint32_t)&_stack_top,        /* 初始 Stack Pointer */
    (uint32_t)Reset_Handler,      /* Reset */
    (uint32_t)Default_Handler,    /* NMI */
    (uint32_t)Default_Handler,    /* HardFault */
    (uint32_t)Default_Handler,    /* MemManage */
    (uint32_t)Default_Handler,    /* BusFault */
    (uint32_t)Default_Handler,    /* UsageFault */
};
