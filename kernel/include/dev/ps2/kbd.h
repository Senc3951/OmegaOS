#pragma once

#include <arch/isr.h>

#define IRQ_KEYBOARD (IRQ0 + 1)

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
#define KEYBOARD_SCAN_CODE_1        1
#define KEYBOARD_SCAN_CODE_2        2
#define KEYBOARD_ENABLE_SCANNING    0xF4
#define KEYBOARD_RESEND             0xFE
#define KEYBOARD_ACK                0xFA

/// @brief Initialize the ps2 keyboard.
/// @return True if successfully initialized the keyboard, False, otherwise.
bool ps2_kbd_init();

/// @brief Read from the keyboard characters.
/// @param buffer Buffer to write the data to.
/// @param count Count of characters to read.
void ps2_kbd_read(void *buffer, size_t count);