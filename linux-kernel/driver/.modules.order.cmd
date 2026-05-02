cmd_/workspace/linux-kernel/driver/modules.order := {   echo /workspace/linux-kernel/driver/hdc1080.ko; :; } | awk '!x[$$0]++' - > /workspace/linux-kernel/driver/modules.order
