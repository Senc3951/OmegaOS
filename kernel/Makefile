SRC_DIR := src
INC_DIR := include
OBJ_DIR := $(OBJ_DIR)/kernel

LINKER_FILE := linker.ld

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
SRC_ASM_FILES = $(call rwildcard,$(SRC_DIR),*.asm)
SRC_C_FILES = $(call rwildcard,$(SRC_DIR),*.c)
OBJ_ASM_FILES = $(patsubst $(SRC_DIR)/%.asm, $(OBJ_DIR)/%.o, $(SRC_ASM_FILES))
OBJ_C_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_C_FILES))

AFLAGS += -Wall
CFLAGS += -I $(INC_DIR) -I $(LIBC_DIR) -ffreestanding -nostdlib -mno-red-zone -fstack-protector-strong \
	-Wall -Wextra -fshort-wchar -Wno-implicit-fallthrough -Wno-int-to-pointer-cast \
	-Wno-address-of-packed-member -Wno-format -Wno-maybe-uninitialized
LFLAGS += -T $(LINKER_FILE) -static -Bsymbolic -z text -z noexecstack

.PHONY: build setup clean

build: $(OBJ_ASM_FILES) $(OBJ_C_FILES)
	@echo "\n========== Linking Kernel =========="
	$(LD) $(LFLAGS) -o $(OUTPUT_KERNEL) $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(@D)
	$(AS) $(AFLAGS) $^ -o $@

setup:

clean:
	rm -rf $(OBJ_DIR)/*