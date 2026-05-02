#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "uart.h"
#include "i2c.h"
#include "hdc1080.h"

/*
 * RTOS 架構設計（業界常見的感測器讀取模式）：
 *
 *   [sensor_task]  定時讀取感測器 → 寫入 Queue
 *       ↓ (xQueueSend)
 *   [display_task] 從 Queue 收資料 → 印出
 *
 * 優點：
 *   - 讀取與顯示解耦，各自的時序獨立
 *   - Queue 提供 back-pressure，避免資料丟失
 *   - I2C bus 用 Mutex 保護，支援多任務共享
 */

/* Task 優先級 */
#define SENSOR_TASK_PRIORITY   (tskIDLE_PRIORITY + 2)
#define DISPLAY_TASK_PRIORITY  (tskIDLE_PRIORITY + 1)

/* Stack size（words） */
#define STACK_SIZE  256

/* Queue：容納 10 筆感測器資料 */
#define QUEUE_LEN   10
static QueueHandle_t    g_sensor_queue;

/* I2C Mutex：保護共享 bus */
static SemaphoreHandle_t g_i2c_mutex;

/* ─────────────────────────────────────────
 * sensor_task：每 2 秒讀一次 HDC1080
 * ───────────────────────────────────────── */
static void sensor_task(void *param) {
    (void)param;
    hdc1080_data_t data;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    uart_puts("[sensor_task] started\n");

    while (1) {
        /* 取得 I2C mutex */
        if (xSemaphoreTake(g_i2c_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            int ret = hdc1080_read(&data);
            xSemaphoreGive(g_i2c_mutex);

            if (ret == 0) {
                /* 送到 queue（非阻塞：若滿了就丟棄最舊的） */
                if (xQueueSend(g_sensor_queue, &data,
                               pdMS_TO_TICKS(10)) != pdTRUE) {
                    uart_puts("[sensor_task] queue full, dropped\n");
                }
            } else {
                uart_puts("[sensor_task] read error\n");
            }
        } else {
            uart_puts("[sensor_task] i2c mutex timeout\n");
        }

        /* 精確週期：vTaskDelayUntil 不會因為執行時間而飄移 */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
    }
}

/* ─────────────────────────────────────────
 * display_task：從 queue 收資料並印出
 * ───────────────────────────────────────── */
static void display_task(void *param) {
    (void)param;
    hdc1080_data_t data;
    uint32_t count = 0;

    uart_puts("[display_task] started\n");

    while (1) {
        /* 阻塞等待 queue 有資料（最多等 5 秒） */
        if (xQueueReceive(g_sensor_queue, &data,
                          pdMS_TO_TICKS(5000)) == pdTRUE) {
            int32_t  t_int  = data.temp_x100 / 100;
            uint32_t t_frac = (uint32_t)(data.temp_x100 < 0
                              ? -data.temp_x100 : data.temp_x100) % 100;
            uint32_t h_int  = data.hum_x100 / 100;
            uint32_t h_frac = data.hum_x100 % 100;

            uart_printf("[%u] Temp: %d.%02u C  Hum: %u.%02u %%"
                        "  (free heap: %u bytes)\n",
                        ++count, t_int, t_frac, h_int, h_frac,
                        (uint32_t)xPortGetFreeHeapSize());
        } else {
            uart_puts("[display_task] no data (timeout)\n");
        }
    }
}

/* ─────────────────────────────────────────
 * FreeRTOS hook 函式
 * ───────────────────────────────────────── */
void vApplicationMallocFailedHook(void) {
    uart_puts("[FATAL] malloc failed!\n");
    while(1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    uart_printf("[FATAL] stack overflow in task: %s\n", pcTaskName);
    while(1);
}

void vApplicationIdleHook(void) {
    /* 可在這裡進入低功耗模式 */
}

/* ─────────────────────────────────────────
 * main
 * ───────────────────────────────────────── */
int main(void) {
    uart_init();
    uart_puts("\n");
    uart_puts("========================================\n");
    uart_puts("  HDC1080 I2C Driver - FreeRTOS Demo    \n");
    uart_puts("  Target: MPS2-AN385 (Cortex-M3)        \n");
    uart_puts("  Mode: Simulation (QEMU)                \n");
    uart_puts("========================================\n\n");

    i2c_init();

    if (hdc1080_init() != 0) {
        uart_puts("[main] sensor init failed!\n");
        while(1);
    }

    /* 建立 Queue 和 Mutex */
    g_sensor_queue = xQueueCreate(QUEUE_LEN, sizeof(hdc1080_data_t));
    g_i2c_mutex    = xSemaphoreCreateMutex();

    if (!g_sensor_queue || !g_i2c_mutex) {
        uart_puts("[main] failed to create RTOS objects\n");
        while(1);
    }

    /* 建立 Tasks */
    xTaskCreate(sensor_task,  "sensor",  STACK_SIZE,
                NULL, SENSOR_TASK_PRIORITY,  NULL);
    xTaskCreate(display_task, "display", STACK_SIZE,
                NULL, DISPLAY_TASK_PRIORITY, NULL);

    uart_puts("[main] starting scheduler...\n\n");

    /* 啟動排程器（不應 return） */
    vTaskStartScheduler();

    uart_puts("[main] scheduler returned! (should not happen)\n");
    while(1);
    return 0;
}
