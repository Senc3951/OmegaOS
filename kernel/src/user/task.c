#include <user/task.h>

extern void x64_enter_userspace(const uint16_t ds, const uint16_t cs, void *addr);

void jump_to_user(void *addr)
{
    x64_enter_userspace(GDT_USER_DS, GDT_USER_CS, addr);
}