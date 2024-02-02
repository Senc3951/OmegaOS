#include <dev/ps2/kbd.h>
#include <dev/ps2/translator.h>
#include <arch/isr.h>
#include <io/io.h>
#include <assert.h>
#include <libc/string.h>
#include <logger.h>

#define BUFFER_SIZE     PAGE_SIZE
#define MAX_TRIES       3

#define KBD_LSHIFT      0x01
#define KBD_RSHIFT      0x02
#define KBD_CAPS_LOCK   0x04
#define KBD_NUM_LOCK    0x08

#define WRITE_DATA(data) ({ \
    while (inb(KEYBOARD_STATUS_PORT) & 0x2); \
    outb(KEYBOARD_DATA_PORT, data); \
})
#define WRITE_COMMAND(data) ({ \
    outb(KEYBOARD_COMMAND_PORT, data); \
    __IO_WAIT(); \
})    
#define READ_DATA(data) ({\
    while ((inb(KEYBOARD_STATUS_PORT) & 0x1) == 0); \
    inb(KEYBOARD_DATA_PORT); \
})
#define WRITE_KEYBOARD(data) ({\
    uint8_t res = 0, tries = 0; \
    do {\
        WRITE_DATA(data); \
        tries++; \
    } while (tries < MAX_TRIES && (res = READ_DATA()) == KEYBOARD_RESEND); \
    \
    res; \
})

static void interruptHandler(InterruptStack_t *stack)
{
    UNUSED(stack);
    ps2_kbd_getc();
}

int ps2_kbd_getc()
{
    static uint32_t shift;
    static uint8_t *charcode[4] = { normalmap, shiftmap, ctlmap, ctlmap };
    uint32_t st, data, c;
    
    st = inb(KBSTATP);
    if ((st & KBS_DIB) == 0)
        return -1;
    
    data = inb(KBDATAP);
    if (data == 0xE0)
    {
        shift |= E0ESC;
        return 0;
    }
    else if (data & 0x80)
    {
        // Key released
        data = (shift & E0ESC ? data : data & 0x7F);
        shift &= ~(shiftcode[data] | E0ESC);
        return 0;
    }
    else if (shift & E0ESC)
    {
        // Last character was an E0 escape; or with 0x80
        data |= 0x80;
        shift &= ~E0ESC;
    }

    shift |= shiftcode[data];
    shift ^= togglecode[data];
    c = charcode[shift & (CTL | SHIFT)][data];
    
    if (shift & CAPSLOCK)
    {
        if ('a' <= c && c <= 'z')
            c += 'A' - 'a';
        else if ('A' <= c && c <= 'Z')
            c += 'a' - 'A';
    }
    
    return c;
}

bool ps2_kbd_init()
{    
    // Disable device
    WRITE_COMMAND(CONTROLLER_DISABLE_1);
    WRITE_COMMAND(CONTROLLER_DISABLE_2);

    // Flush the buffer
    while (inb(KEYBOARD_COMMAND_PORT) & 1)
        inb(KEYBOARD_DATA_PORT);

    // Configure the device
    WRITE_COMMAND(CONTROLLER_READ_CFG);
    uint8_t config = inb(KEYBOARD_DATA_PORT);
    config |= (CFG_CLK1 | CFG_INT1);
    config &= ~(CFG_CLK2 | CFG_INT2 | CFG_TRANS);

    WRITE_COMMAND(CONTROLLER_WRITE_CFG);
    WRITE_DATA(config);

    // Test the controller
    WRITE_COMMAND(CONTROLLER_TEST);
    if (READ_DATA() != CONTROLLER_TEST_RESULT)
    {
        LOG("Controller tester failed.\n");
        return false;
    }

    // Test the interface
    WRITE_COMMAND(CONTROLLER_INTERFACE_TEST);
    if (READ_DATA() != 0)
    {
        LOG("Controller interface tester failed.\n");
        return false;
    }

    // Enable device
    WRITE_COMMAND(CONTROLLER_ENABLE_1);
    WRITE_COMMAND(CONTROLLER_ENABLE_2);

    // Reset device
    if (WRITE_KEYBOARD(DEVICE_RESET) != DEVICE_SUCCESS)
    {
        LOG("Unable to reset the Controller.\n");
        return false;
    }

    if (WRITE_KEYBOARD(KEYBOARD_SET_SCAN_CODE) != KEYBOARD_ACK)
    {
        LOG("Unable to set the keyboard's scan code.\n");
        return false;
    }
    if (WRITE_KEYBOARD(KEYBOARD_SCAN_CODE_1) != KEYBOARD_ACK)
    {
        LOG("Unable to set the keyboard's scan code.\n");
        return false;
    }
    
    // Enable scanning
    if (WRITE_KEYBOARD(KEYBOARD_ENABLE_SCANNING) != KEYBOARD_ACK)
    {
        LOG("Unable to start the keyboard.\n");
        return false;
    }
    
    assert(isr_registerHandler(PS2_KBD_ISR, interruptHandler));
    LOG("PS2 keyboard initialized\n");

    return true;
}