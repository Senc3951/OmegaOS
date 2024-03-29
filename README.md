# OmegaOS
x86_64 UEFI-based SMP operating system built from scratch.

## Goals
- [x] UEFI Bootloader
- [x] Interrupts
- [x] Physical memory manager
- [x] Virtual memory manager (Paging)
- [x] Heap
- [x] ATA
- [x] Virtual filesystem
- [x] Filesystem (ext2)
- [x] Process management
- [x] User space
- [x] Shell
- [x] APIC
- [x] SMP
- [ ] USB
- [ ] Network

## Building
> [!NOTE]
> I do all development on Ubuntu 22.04, therefore, other Linux environments should work but the project was never tested on Mac / Windows.

### Installation
Install `gcc`, `nasm`, `ld`, `makefile`, `mtools` and `qemu`.

### Setup
Enter the directory and run ```make setup && sudo ./build.sh```

### Running
run `make`.

### Debugging
First, make sure that debugging is enabled in config.mk, if it wasn't, enable it and clear the project by running ```make clean```.
Run ```make debug```.

On a seperate terminal, run the following commands
```bash
gdb
target remote localhost:1234
symbol-file output/kernel.elf
```

## Contribute
This project is open source, feel free to study and contribute.