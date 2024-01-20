#include <arch/pic.h>
#include <io/io.h>
#include <logger.h>

void pic_init(const uint8_t offset1, const uint8_t offset2, const bool autoEOI)
{
    // Initialize Control Word 1
    outb(PIC1_COMMAND, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    __IO_WAIT();
    outb(PIC2_COMMAND, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    __IO_WAIT();

    // Initialize Control Word 2
    outb(PIC1_DATA, offset1);
    __IO_WAIT();
    outb(PIC2_DATA, offset2);
    __IO_WAIT();

    // Initialize Control Word 3
    outb(PIC1_DATA, 4);    // Tell the Master that there is a Slave at IRQ2
    __IO_WAIT();
    outb(PIC2_DATA, 2);
    __IO_WAIT();

    uint8_t icw4 = PIC_ICW4_8086;
    if (autoEOI)
        icw4 |= PIC_ICW4_AUTO_EOI;

    // Initialize Control Word 4
    outb(PIC1_DATA, icw4);
    __IO_WAIT();
    outb(PIC2_DATA, icw4);
    __IO_WAIT();
    
    LOG("PIC mapped at 0x%x - 0x%x\n", offset1, offset2 + 8);
}

void pic_disable()
{
    outb(PIC1_DATA, 0xFF);
    __IO_WAIT();
    outb(PIC2_DATA, 0xFF);
    __IO_WAIT();
}

void pic_mask(uint8_t irq)
{
    uint8_t value;
    uint16_t port;

    if (irq < 8)
        port = PIC1_DATA;
    else
    {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) | (1 << irq);
    outb(port, value);
}

void pic_unmask(uint8_t irq)
{
    uint8_t value;
    uint16_t port;

    if (irq < 8)
        port = PIC1_DATA;
    else
    {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

void pic_sendEOI(const uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);
    
    outb(PIC1_COMMAND, PIC_EOI);
}