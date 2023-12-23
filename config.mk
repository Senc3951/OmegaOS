DEBUG := true
OPTIMISATION := -O2

export OS_NAME := OmegaOS

export GNU_EFI_DIR := $(abspath gnu-efi)
export RESOURCES_DIR := $(abspath resources)
export OVMF_DIR := $(abspath OVMFbin)
export OBJ_DIR := $(abspath obj)
export OUTPUT_DIR := $(abspath output)

export KERNEL_DIR := kernel
export OUTPUT_KERNEL := $(OUTPUT_DIR)/kernel.elf
export OUTPUT_OS_FILE := $(OUTPUT_DIR)/$(OS_NAME).img
export FONT_FILE := $(RESOURCES_DIR)/zap-light16.psf
export BOOTEFI := $(GNU_EFI_DIR)/x86_64/bootloader/main.efi
export INITRD_FILE := $(OUTPUT_DIR)/rootfs.img
export LOG_FILE := serial.log

export AS := nasm
export CC := gcc
export LD := ld
export AFLAGS := -f elf64 $(OPTIMISATION)
export CFLAGS := -m64 $(OPTIMISATION)
export LFLAGS := -nostdlib $(OPTIMISATION)
export QEMU := qemu-system-x86_64
export QFLAGS := -machine q35 -d cpu_reset -m 2G -rtc base=localtime -net none -chardev stdio,id=char0,logfile=$(LOG_FILE),signal=off \
  -serial chardev:char0 -device piix3-ide,id=ide -drive id=disk,file=$(INITRD_FILE),format=raw,if=none -device ide-hd,drive=disk,bus=ide.0 \
  -drive if=pflash,format=raw,unit=0,file="$(OVMF_DIR)/OVMF_CODE-pure-efi.fd",readonly=on -drive if=pflash,format=raw,unit=1,file="$(OVMF_DIR)/OVMF_VARS-pure-efi.fd"

ifeq ($(DEBUG), true)
	AFLAGS += -g
	CFLAGS += -g -D DEBUG
	LFLAGS += -g
endif