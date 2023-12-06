include config.mk

.PHONY: run debug build kernel ovmf setup clean

run: $(OUTPUT_OS_FILE)
	@echo "\n========== Running $< =========="
	$(QEMU) $(QFLAGS) -drive file=$<

debug: $(OUTPUT_OS_FILE)
	@echo "\n========== Debugging $< =========="
	$(QEMU) -s -S $(QFLAGS) -drive file=$<

build: $(OUTPUT_OS_FILE)
$(OUTPUT_OS_FILE): kernel
	@echo "\n========== Building $@ =========="
	dd if=/dev/zero of=$@ bs=512 count=93750
	mformat -i $@ ::
	mmd -i $@ ::/EFI
	mmd -i $@ ::/EFI/BOOT
	mcopy -i $@ $(BOOTEFI) ::/EFI/BOOT
	mcopy -i $@ startup.nsh ::
	mcopy -i $@ $(OUTPUT_KERNEL) ::
	mcopy -i $@ $(FONT_FILE) ::

kernel:
	@echo "========== Building Kernel =========="
	@$(MAKE) -C $(KERNEL_DIR) build

ovmf:
	mkdir -p ovmf
	cd ovmf && curl -Lo OVMF.fd https://retrage.github.io/edk2-nightly/bin/RELEASEX64_OVMF.fd

setup: ovmf
	mkdir -p $(OBJ_DIR)
	mkdir -p $(OUTPUT_DIR)
	
	@$(MAKE) -C $(KERNEL_DIR) setup
	
clean:
	@$(MAKE) -C $(KERNEL_DIR) clean