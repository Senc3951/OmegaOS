include config.mk

.PHONY: run debug build kernel initrd setup clean

run: $(OUTPUT_OS_FILE)
	@echo "\n========== Running $< =========="
	sudo $(QEMU) $(QFLAGS) -drive file=$<

debug: $(OUTPUT_OS_FILE)
	@echo "\n========== Debugging $< =========="
	sudo $(QEMU) -s -S $(QFLAGS) -drive file=$<

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

initrd: $(INITRD_FILE)
$(INITRD_FILE):
	dd if=/dev/zero of=$@ bs=1MiB count=64
	mkfs.ext2 -b 1024 $@
	mkdir -p tmp
	sudo mount -o loop $@ tmp
	cp .gitignore tmp
	mkdir tmp/empty
	mkdir tmp/notempty
	cp startup.nsh tmp/notempty
	sudo umount tmp
	rm -rf tmp

setup: initrd
	mkdir -p $(OBJ_DIR)
	mkdir -p $(OUTPUT_DIR)
	
	@$(MAKE) -C $(KERNEL_DIR) setup
	
clean:
	@$(MAKE) -C $(KERNEL_DIR) clean