#include <dev/ps2/kbd.h>
#include <dev/ps2/translator.h>
#include <arch/isr.h>
#include <io/io.h>
#include <assert.h>
#include <logger.h>

#define MAX_TRIES                   3
#define KEYBOARD_DATA_PORT          0x60
#define KEYBOARD_STATUS_PORT        0x64
#define KEYBOARD_COMMAND_PORT       0x64

#define CONTROLLER_ENABLE_1         0xAE
#define CONTROLLER_ENABLE_2         0xA8
#define CONTROLLER_DISABLE_1        0xAD
#define CONTROLLER_DISABLE_2        0xA7
#define CONTROLLER_TEST             0xAA
#define CONTROLLER_TEST_RESULT      0x55
#define CONTROLLER_INTERFACE_TEST   0xAB

#define CONTROLLER_READ_CFG         0x20
#define CONTROLLER_WRITE_CFG        0x60
#define CFG_INT1                    0x1
#define CFG_INT2                    0x2
#define CFG_CLK1                    0x10
#define CFG_CLK2                    0x20
#define CFG_TRANS                   0x40

#define DEVICE_RESET                0xFF
#define DEVICE_SUCCESS              0xFA
#define KEYBOARD_SET_SCAN_CODE      0xF0
#define KEYBOARD_SCAN_CODE_2        2
#define KEYBOARD_ENABLE_SCANNING    0xF4
#define KEYBOARD_RESEND             0xFE
#define KEYBOARD_ACK                0xFA

#define WRITE_DATA(data) ({                     \
    while (inb(KEYBOARD_STATUS_PORT) & 0x2);    \
    outb(KEYBOARD_DATA_PORT, data);             \
})
#define WRITE_COMMAND(data) ({          \
    outb(KEYBOARD_COMMAND_PORT, data);  \
    __IO_WAIT();                        \
})
#define READ_DATA(data) ({                          \
    while ((inb(KEYBOARD_STATUS_PORT) & 0x1) == 0); \
    inb(KEYBOARD_DATA_PORT);                        \
})
#define WRITE_KEYBOARD(data) ({                                             \
    uint8_t res = 0, tries = 0;                                             \
    do {                                                                    \
        WRITE_DATA(data);                                                   \
        tries++;                                                            \
    } while (tries < MAX_TRIES && (res = READ_DATA()) == KEYBOARD_RESEND);  \
    \
    res;                                                                    \
})

static bool g_caps, g_released, g_shift;
static uint8_t g_lastChar = NO_KEY, g_currentChar = NO_KEY;

static void interruptHandler(InterruptStack_t *stack)
{
    UNUSED(stack);
    
    uint8_t tmp, data = inb(KEYBOARD_DATA_PORT);
    switch (data)
    {
        case 0:
        case INVALID_KEY:
            LOG("Key detection error or internal buffer overrun\n");
            break;
        case RELEASED_KEY:
            g_released = true;
            break;
        case CAPS_KEY:
            if (!g_released)
                g_caps = !g_caps;
            
            break;
        case RIGHT_SHIFT:
        case LEFT_SHIFT:
            g_shift = !g_shift;
            break;
        default:
            if (g_released)
                g_released = false;
            else
            {
                tmp = translate(data, g_caps, g_shift);
                if (tmp != NO_KEY)
                    g_currentChar = tmp;
            }

            break;
    }

    if (g_lastChar == RELEASED_KEY)
        g_released = false;

    g_lastChar = data;
}

static bool initController()
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
        LOG("Controller tester failed\n");
        return false;
    }

    // Test the interface
    WRITE_COMMAND(CONTROLLER_INTERFACE_TEST);
    if (READ_DATA() != 0)
    {
        LOG("Controller interface tester failed\n");
        return false;
    }

    // Enable device
    WRITE_COMMAND(CONTROLLER_ENABLE_1);
    WRITE_COMMAND(CONTROLLER_ENABLE_2);

    // Reset device
    if (WRITE_KEYBOARD(DEVICE_RESET) != DEVICE_SUCCESS)
    {
        LOG("Unable to reset the Controller\n");
        return false;
    }

    return true;
}

static bool initKeyboard()
{
    // Set the scan code.
    if (WRITE_KEYBOARD(KEYBOARD_SET_SCAN_CODE) != KEYBOARD_ACK)
    {
        LOG("Unable to set the keyboard's scan code\n");
        return false;
    }
    if (WRITE_KEYBOARD(KEYBOARD_SCAN_CODE_2) != KEYBOARD_ACK)
    {
        LOG("Unable to set the keyboard's scan code\n");
        return false;
    }
            
    // Enable scanning
    if (WRITE_KEYBOARD(KEYBOARD_ENABLE_SCANNING) != KEYBOARD_ACK)
    {
        LOG("Unable to start the keyboard\n");
        return false;
    }
    
    return true;
}

void ps2_kbd_init()
{
    assert(isr_registerHandler(IRQ_KEYBOARD, interruptHandler));
    assert(initController() && initKeyboard());
    
    LOG("PS2 keyboard initialized\n");
    g_caps = g_released = g_shift = false;
}

char ps2_kbd_getc()
{
    while (g_currentChar == NO_KEY)
        __HALT();

    char c = g_currentChar;
    g_currentChar = NO_KEY;
    
    return c;
}