DEBUG := true
OPTIMISATION := -O2

export OS_NAME := OmegaOS

export GNU_EFI_DIR := $(abspath gnu-efi)
export RESOURCES_DIR := $(abspath resources)
export OVMF_DIR := $(abspath ovmf)
export OBJ_DIR := $(abspath obj)
export OUTPUT_DIR := $(abspath output)

export KERNEL_DIR := kernel
export OUTPUT_KERNEL := $(OUTPUT_DIR)/kernel.elf
export OUTPUT_OS_FILE := $(OUTPUT_DIR)/$(OS_NAME).img
export FONT_FILE := $(RESOURCES_DIR)/zap-light16.psf
export BOOTEFI := $(GNU_EFI_DIR)/x86_64/bootloader/main.efi
export OVMF_FILE := $(OVMF_DIR)/OVMF.fd

export AS := nasm
export CC := gcc
export LD := ld
export AFLAGS := -f elf64 $(OPTIMISATION)
export CFLAGS := -m64 $(OPTIMISATION)
export LFLAGS := -nostdlib $(OPTIMISATION)
export QEMU := qemu-system-x86_64
export QFLAGS := -M q35 -m 2G -rtc base=localtime -net none -bios $(OVMF_FILE)

ifeq ($(DEBUG), true)
	AFLAGS += -g
	CFLAGS += -g -D DEBUG
	LFLAGS += -g
endif