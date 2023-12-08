#include <drivers/storage/ide.h>
#include <io/io.h>

static uint16_t g_bus;

static void ataSelect()
{
    outb(g_bus + ATA_REG_HDDEVSEL, 0xA0);
}

static void ataWaitReady()
{
    while (inb(g_bus + ATA_REG_STATUS) & ATA_SR_BSY) ;
}

static void ioWait()
{
    inb(g_bus + ATA_REG_ALTSTATUS);
    inb(g_bus + ATA_REG_ALTSTATUS);
    inb(g_bus + ATA_REG_ALTSTATUS);
    inb(g_bus + ATA_REG_ALTSTATUS);
}

static int ataWait(const int advanced)
{
    uint8_t status = 0;
    ioWait();

    while ((status = inb(g_bus + ATA_REG_STATUS)) & ATA_SR_BSY);
    if (advanced)
    {
        status = inb(g_bus + ATA_REG_STATUS);
        if (status   & ATA_SR_ERR)
            return 1;
        if (status   & ATA_SR_DF)
            return 1;
        if (!(status & ATA_SR_DRQ))
            return 1;
    }

    return 0;
}

void ide_init(const uint16_t bus)
{
    outb(bus + 1, 1);
    outb(bus + 0x306, 0);

    ataSelect();
    ioWait();

    outb(bus + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ioWait();
    ataWaitReady();
    
    ata_identify_t device;
    uint16_t *buf = (uint16_t *)&device;
    for (uint16_t i = 0; i < 256; i++)
        buf[i] = inw(bus);

    uint8_t *ptr = (uint8_t *)&device.model;
    for (uint8_t i = 0; i < 39; i += 2)
    {
        uint8_t tmp = ptr[i + 1];
        ptr[i + 1] = ptr[i];
        ptr[i] = tmp;
    }
    outb(bus + ATA_REG_CONTROL, 0x02);
}

bool ide_read(const uint32_t sector, void *buffer, const uint32_t count)
{
    if (!buffer)
        return false;
    
    uint8_t *buf = (uint8_t *)buffer;
    for (uint32_t i = 0; i < count; i++)
    {
        outb(g_bus + ATA_REG_CONTROL, 0x02);
        ataWaitReady();

        outb(g_bus + ATA_REG_HDDEVSEL,  0xe0 | 0 << 4 | 
                                    (sector & 0x0f000000) >> 24);
        outb(g_bus + ATA_REG_FEATURES, 0x00);
        outb(g_bus + ATA_REG_SECCOUNT0, 1);
        outb(g_bus + ATA_REG_LBA0, (sector & 0x000000ff) >>  0);
        outb(g_bus + ATA_REG_LBA1, (sector & 0x0000ff00) >>  8);
        outb(g_bus + ATA_REG_LBA2, (sector & 0x00ff0000) >> 16);
        outb(g_bus + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
        if (ataWait(1))
            return false;
        
        insm(g_bus, buf + i * ATA_SECTOR_SIZE, ATA_WSECTOR_SIZE);
        ataWait(0);
    }
    
    return true;
}

bool ide_write(const uint32_t sector, void *buffer, const uint32_t count)
{
    if (!buffer)
        return false;
    
    uint8_t *buf = (uint8_t *)buffer;
    for (uint32_t i = 0; i < count; i++)
    {
        outb(g_bus + ATA_REG_CONTROL, 0x02);
        ataWaitReady();
        
        outb(g_bus + ATA_REG_HDDEVSEL,  0xe0 | 0 << 4 | 
                                    (sector & 0x0f000000) >> 24);
        ataWait(0);
        outb(g_bus + ATA_REG_FEATURES, 0x00);
        outb(g_bus + ATA_REG_SECCOUNT0, 0x01);
        outb(g_bus + ATA_REG_LBA0, (sector & 0x000000ff) >>  0);
        outb(g_bus + ATA_REG_LBA1, (sector & 0x0000ff00) >>  8);
        outb(g_bus + ATA_REG_LBA2, (sector & 0x00ff0000) >> 16);
        outb(g_bus + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
        
        ataWait(0);
        outsm(g_bus, buf + i * ATA_SECTOR_SIZE, ATA_WSECTOR_SIZE);
        outb(g_bus + 0x07, ATA_CMD_CACHE_FLUSH);
        ataWait(0);
    }

    return true;
}