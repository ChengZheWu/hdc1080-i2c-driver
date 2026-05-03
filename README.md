# HDC1080 I2C Driver — Embedded Systems Portfolio Project

三個層次的嵌入式 I2C 驅動實做，全部跑在 QEMU 模擬環境，使用模擬感測器數據。
開發環境透過 VSCode DevContainer 隔離，不污染本機。

---

## 專案架構

```
hdc1080-i2c-driver/
├── .devcontainer/
│   ├── devcontainer.json           # VSCode DevContainer 設定
│   ├── Dockerfile                  # 開發環境映像（Ubuntu 22.04）
│   └── post-create.sh              # 容器建立後的初始化腳本
├── bare-metal/
│   ├── include/
│   │   ├── uart.h                  # UART driver header
│   │   ├── i2c.h                   # CMSDK APB I2C driver header
│   │   └── hdc1080.h               # HDC1080 sensor header
│   ├── src/
│   │   ├── startup.c               # 向量表、Reset_Handler、.data/.bss 初始化
│   │   ├── uart.c                  # UART driver（自製 printf，支援 %d %u %x %02u）
│   │   ├── i2c.c                   # I2C driver（write / read / write-then-read）
│   │   ├── hdc1080.c               # HDC1080 sensor driver（模擬數據）
│   │   └── main.c                  # 主程式（輪詢迴圈）
│   ├── scripts/
│   │   └── mps2-an385.ld           # Linker script（Flash / SRAM memory map）
│   └── Makefile
├── rtos/
│   ├── include/
│   │   ├── FreeRTOSConfig.h        # FreeRTOS kernel 設定
│   │   ├── uart.h
│   │   ├── i2c.h
│   │   └── hdc1080.h
│   ├── src/
│   │   ├── startup.c               # 向量表（含 SysTick / PendSV / SVC handlers）
│   │   ├── uart.c
│   │   ├── i2c.c
│   │   ├── hdc1080.c
│   │   └── main.c                  # sensor_task + display_task（Queue + Mutex）
│   ├── freertos/
│   │   ├── kernel/                 # FreeRTOS kernel source（git clone）
│   │   └── portable/               # ARM_CM3 port + heap_4.c
│   └── Makefile
└── linux-kernel/
    ├── driver/
    │   ├── hdc1080.c               # Linux IIO kernel module
    │   └── Makefile                # Kbuild Makefile（obj-m）
    ├── buildroot-overlay/
    │   └── rootfs/
    │       └── usr/bin/
    │           └── test_hdc1080.sh # 在 QEMU Linux 上執行的測試腳本
    ├── scripts/
    │   ├── setup.sh                # 一鍵建置環境（全自動）
    │   ├── run_qemu_ast2600.sh     # 啟動 QEMU AST2600
    │   ├── 0001-add-hdc1080-to-ast2600-i2c1.patch  # DTS patch
    │   └── build_kernel_module.sh  # 單獨編譯 hdc1080.ko
    └── Makefile                    # make module / make clean_module
```

---

## 系統需求

- **OS**：Ubuntu 20.04 / 22.04（或任何支援 Docker 的 Linux 系統）
- **軟體**：
  - [Docker](https://docs.docker.com/get-docker/)
  - [VSCode](https://code.visualstudio.com/)
  - VSCode 擴充套件：[Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

---

## 快速開始

### Step 1：Clone 專案並開啟 DevContainer

```bash
git clone <your-repo-url> hdc1080-i2c-driver
cd hdc1080-i2c-driver
code .
```

VSCode 開啟後：

```
Ctrl+Shift+P → Dev Containers: Reopen in Container
```

等待 Docker image 建立完成（第一次約 5 分鐘）。**之後所有指令都在 DevContainer 內的 terminal 執行。**

---

## 執行方式

### 1. Bare-metal（MPS2-AN385 / Cortex-M3）

無作業系統，直接操作硬體暫存器。自製向量表、startup、UART printf、I2C driver，不依賴任何 libc。

```bash
cd /workspace/bare-metal
make
make run
```

**預期輸出**：

```
========================================
  HDC1080 I2C Driver - Bare-metal Demo
  Target: MPS2-AN385 (Cortex-M3)
  Mode: Simulation (QEMU)
========================================

[i2c] controller initialized
[hdc1080] init...
[hdc1080] (simulation mode: skipping ID check)
[hdc1080] init done

[main] starting sensor polling loop...

[1] Temp: 25.00 C  Humidity: 60.00 %
[2] Temp: 25.00 C  Humidity: 60.00 %
[3] Temp: 25.00 C  Humidity: 60.00 %
```

離開 QEMU：`Ctrl+A` 然後 `X`

---

### 2. FreeRTOS（MPS2-AN385 / Cortex-M3）

FreeRTOS 多任務架構。`sensor_task` 每 2 秒讀取感測器寫入 Queue，`display_task` 從 Queue 取出顯示。I2C bus 用 Mutex 保護。

```bash
cd /workspace/rtos

# 第一次需要下載 FreeRTOS kernel
git clone --depth 1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git freertos/kernel
mkdir -p freertos/portable
cp -r freertos/kernel/portable/GCC/ARM_CM3 freertos/portable/
cp freertos/kernel/portable/MemMang/heap_4.c freertos/portable/

make
make run
```

**預期輸出**：

```
========================================
  HDC1080 I2C Driver - FreeRTOS Demo
  Target: MPS2-AN385 (Cortex-M3)
  Mode: Simulation (QEMU)
========================================

[i2c] controller initialized
[hdc1080] init done
[main] starting scheduler...

[sensor_task] started
[display_task] started
[1] Temp: 25.00 C  Hum: 60.00 %  (free heap: 28432 bytes)
[2] Temp: 25.00 C  Hum: 60.00 %  (free heap: 28432 bytes)
```

離開 QEMU：`Ctrl+A` 然後 `X`

---

### 3. Linux Kernel Module（AST2600 / Cortex-A7 + Buildroot）

Linux IIO kernel module，透過 Device Tree 匹配，`insmod` 後自動 probe。
使用 Buildroot 建置完整 Linux 系統映像，跑在 QEMU AST2600 上。

> **注意**：第一次需要執行 Buildroot 完整編譯，約 1-2 小時。

#### 3-1. 一鍵建置環境（只需做一次）

`setup.sh` 全自動完成以下步驟，不需要任何互動：
- 下載 Buildroot 2024.02
- 套用 AST2600 defconfig
- 透過 config fragment 開啟 `CONFIG_MODULES`（不需要進 menuconfig）
- 設定 rootfs overlay
- 編譯整個系統（約 1-2 小時）
- 套用 HDC1080 DTS patch
- 編譯 hdc1080.ko 並打包進 rootfs

```bash
bash /workspace/linux-kernel/scripts/setup.sh
```

#### 3-2. 啟動 QEMU

```bash
bash /workspace/linux-kernel/scripts/run_qemu_ast2600.sh
```

登入（帳號 `root`，無密碼）後執行：

```bash
insmod /lib/modules/hdc1080.ko
sh /usr/bin/test_hdc1080.sh
```

**預期輸出**：

```
=== HDC1080 Driver Test ===
[1] Loading kernel module...
    done
[2] Module info:
hdc1080                16384  -
[3] IIO devices:
iio:device0
    Found: /sys/bus/iio/devices/iio:device0
[4] Device name: hdc1080
[5] Continuous reading (5 samples):
    Formula: Temp(C) = (raw + offset) x scale / 1000
             Humi(%) = raw x scale / 1000

    Sample 1: Temp: 27.032 C  Humidity: 59.999 %
    Sample 2: Temp: 27.032 C  Humidity: 59.999 %
    Sample 3: Temp: 27.032 C  Humidity: 59.999 %
    Sample 4: Temp: 27.032 C  Humidity: 59.999 %
    Sample 5: Temp: 27.032 C  Humidity: 59.999 %
=== Test complete ===
```

也可以直接讀 IIO sysfs 原始值：

```bash
cat /sys/bus/iio/devices/iio:device0/in_temp_raw      # 26624
cat /sys/bus/iio/devices/iio:device0/in_temp_scale    # 2.517700
cat /sys/bus/iio/devices/iio:device0/in_temp_offset   # -15887
cat /sys/bus/iio/devices/iio:device0/in_humidityrelative_raw    # 39321
cat /sys/bus/iio/devices/iio:device0/in_humidityrelative_scale  # 1.525879
```

離開 QEMU：`Ctrl+A` 然後 `X`

---

## 技術要點

### Bare-metal

| 項目 | 說明 |
|------|------|
| 向量表 | 放在 `.vector_table` section，Linker script 確保在 Flash 最開頭（0x00000000） |
| Startup | `Reset_Handler` 複製 `.data` 到 SRAM、清零 `.bss`、跳入 `main` |
| I2C | CMSDK APB I2C（0x40022000），write-then-read 實作 Repeated START |
| 無 libc | `-nostdlib`，自製 `uart_printf` 支援寬度修飾符（`%02u` 等） |
| 數值換算 | 整數 ×100 避免浮點（`temp_x100 = 2500` 表示 25.00°C） |

### FreeRTOS

| 項目 | 說明 |
|------|------|
| 任務架構 | Producer-consumer：`sensor_task` → Queue → `display_task` |
| 精確週期 | `vTaskDelayUntil` 不因任務執行時間飄移（vs `vTaskDelay` 的相對延遲） |
| 共享資源 | Mutex 保護 I2C bus，支援未來多任務擴充 |
| 除錯 | `configCHECK_FOR_STACK_OVERFLOW=2`，runtime stack 溢出即觸發 hook |
| Toolchain | `-nostartfiles --specs=nano.specs`，用 newlib-nano 提供 `memset`/`memcpy` |

### Linux Kernel

| 項目 | 說明 |
|------|------|
| 驅動框架 | Linux IIO（Industrial I/O）子系統，工業感測器標準介面 |
| 裝置匹配 | Device Tree `compatible = "ti,hdc1080"` → kernel 自動呼叫 `probe` |
| 資源管理 | `devm_*` API，probe 失敗時自動釋放所有資源，無記憶體洩漏 |
| sysfs 介面 | `in_temp_raw` + `scale` + `offset`，user-space 套公式得物理量 |
| 模擬模式 | DTS 加入 `ti,simulated` property，probe 時跳過真實 I2C 讀寫 |
| Out-of-tree | 標準 Kbuild `obj-m` 格式，獨立於 kernel source 編譯 |
| DTS 管理 | 以 patch 管理對 upstream kernel DTS 的修改（業界標準做法） |
| 自動化建置 | `setup.sh` 透過 config fragment 設定 kernel，不需要互動式 menuconfig |

---

## HDC1080 感測器換算公式

來自 TI HDC1080 datasheet：

```
Temperature(°C) = (raw / 65536) × 165 - 40
Humidity(%)     = (raw / 65536) × 100
```

Linux IIO 的 scale/offset 表示（`in_temp_input = (raw + offset) × scale / 1000`）：

```
scale  = 165000 / 65536 ≈ 2.5177   (milli-°C per LSB)
offset = -40000 / scale ≈ -15887
```

驗算（模擬原始值）：

```
Temp : (26624 + (-15887)) × 2.517700 / 1000 ≈ 27.03°C
Humi : 39321 × 1.525879 / 1000              ≈ 60.00%
```

---

## 硬體平台說明

| 子專案 | QEMU Machine | SoC | 架構 |
|--------|-------------|-----|------|
| Bare-metal | `mps2-an385` | ARM MPS2 | Cortex-M3 |
| FreeRTOS | `mps2-an385` | ARM MPS2 | Cortex-M3 |
| Linux | `ast2600-evb` | ASPEED AST2600 | Cortex-A7 (dual core) |

**AST2600** 廣泛用於伺服器 BMC（Baseboard Management Controller），具備 16 個 I2C controller，是業界做 BMC 感測器驅動的常見平台（Dell、HPE、Meta 等均有使用）。

---

## 未來可擴充方向

- **Phase 5**：實作 QEMU HDC1080 模擬裝置（`hw/i2c/hdc1080.c`），讓真實 I2C 讀寫也能通，移除對 `ti,simulated` 的依賴
- 加入 interrupt 驅動模式（取代 polling）
- 加入 IIO triggered buffer，支援連續高速取樣
- 移植到真實硬體（Raspberry Pi i2c-1 + 實體 HDC1080 模組）
