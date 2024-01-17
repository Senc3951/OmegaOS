#include <sys/scheduler.h>
#include <fs/vfs.h>
#include <logger.h>

int sys_close(uint32_t fd)
{
    LOG_PROC("sys_close file %u\n", fd);
    return process_close_file(_CurrentProcess, fd) ? ENOER : -ENOENT;
}