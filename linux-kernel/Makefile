# Linux Kernel Module Makefile

KDIR  = /workspace/linux-kernel/buildroot/output/build/linux-5.17.7
CROSS = /workspace/linux-kernel/buildroot/output/host/bin/arm-buildroot-linux-gnueabihf-
MDIR  = /workspace/linux-kernel/driver
ARCH  = arm

.PHONY: module clean_module copy

module:
	make -C $(KDIR) M=$(MDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS) modules

clean_module:
	make -C $(KDIR) M=$(MDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS) clean

copy: module
	cp $(MDIR)/hdc1080.ko /tmp/hdc1080.ko
	@echo "hdc1080.ko ready at /tmp/hdc1080.ko"
	@echo "Transfer to QEMU via: see README for scp instructions"
