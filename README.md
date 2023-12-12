# OmegaOS
x86_64 UEFI-based operating system built from scratch.

## Goals
- [x] UEFI Bootloader
- [x] Interrupts
- [x] Physical memory manager
- [x] Virtual memory manager (Paging)
- [x] Heap
- [x] ATA
- [x] Virtual filesystem
- [x] Filesystem (ext2)
- [ ] Process management
- [ ] User space

## Building
> [!NOTE]
> I do all development on Ubuntu 22.04, therefore, other Linux environments should work but the project was never tested on Mac / Windows.

### Installation
Install `gcc`, `nasm`, `ld`, `makefile`, `mtools` and `qemu`.

### Setup
Once installed, enter the directory and run the following commands:
```bash
cd gnu-efi
make && make bootloader
cd ..
sudo make setup     # Building the initrd requires sudo privileges
```

### Running
To run, simply run `make`.

### Debugging
First, make sure that debugging is enabled in config.mk, then run make debug.

On a seperate terminal, run the following commands
```bash
gdb
target remote localhost:1234
symbol-file output/kernel.elf
layout src
b _entry
c
```

## Contribute
This project is open source, feel free to study and contribute.