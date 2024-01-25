#include <dev/ps2/kbd.h>
#include <dev/ps2/translator.h>
#include <arch/isr.h>
#include <io/io.h>
#include <assert.h>
#include <libc/string.h>
#include <logger.h>

#define BUFFER_SIZE PAGE_SIZE
#define MAX_TRIES 3

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

static bool g_shift = false, g_caps = false;

static size_t g_bufferSize = 0;
static __ALIGNED__(PAGE_SIZE) char g_buffer[BUFFER_SIZE];

static void interruptHandler(InterruptStack_t *stack)
{
    UNUSED(stack);
    
    uint8_t data = inb(KEYBOARD_DATA_PORT);
    switch (data)
    {
        case 0:
        case INVALID_KEY:
            break;
        case LEFT_SHIFT:
        case RIGHT_SHIFT:
            g_shift = true;
            break;
        case LEFT_SHIFT_REL:
        case RIGHT_SHFIT_REL:
            g_shift = false;
            break;
        case CAPS_LOCK:
            g_caps = !g_caps;
            break;
        default:
            char c = translate(data, g_shift, g_caps);
            if (c != NO_KEY && g_bufferSize < BUFFER_SIZE)
                g_buffer[g_bufferSize++] = c;
                                    
            break;
    }
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
    LOG("Initialized the ps2 keyboard\n");
    
    return true;
}

bool ps2_kbd_read(void *buffer, size_t count)
{
    if (count > g_bufferSize)
        return false;
    
    __CLI();
    memcpy(buffer, g_buffer, count);
    g_bufferSize -= count;
    
    memcpy(g_buffer, g_buffer + count, g_bufferSize);
    LOG("");
    __STI();
    
    return true;
}