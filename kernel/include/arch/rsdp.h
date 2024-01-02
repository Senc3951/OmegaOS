#pragma once

#include <common.h>

typedef struct RSDP
{
    char signature[8];
    uint8_t checksum;
    char _OEMID[6];
    uint8_t revision;
    uint32_t rsdtAddress;
} __PACKED__ RSDP_t;

typedef struct XSDP
{
    RSDP_t rsdp;
    uint32_t length;
    uint64_t xsdtAddress;
    uint8_t extendedChecksum;
    uint8_t reserved[3];
} __PACKED__ XSDP_t;

typedef struct RSDT_HEADER
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char _OEMID[6];
    char _OEMTableID[8];
    uint32_t _OEMRevision;
    uint32_t creatorID;
    uint32_t creatorRevision;
} __PACKED__ RSDTHeader_t;

typedef struct RSDT
{
    RSDTHeader_t header;
    uint32_t tables[]; 
} __PACKED__ RSDT_t;

typedef struct XSDT
{
    RSDTHeader_t header;
    uint64_t tables[]; 
} __PACKED__ XSDT_t;

#define RSDT_NAME_APIC  "APIC"
#define RSDT_NAME_DSDT  "DSDT"
#define RSDT_NAME_ACPI  "FACP"

/// @brief Initialize and validate the rsdp.
/// @param ptr Address of the rsdp.
void rsdp_init(void *ptr);

/// @brief Find a table in the rsdt.
/// @param name Name of the table.
/// @return Table address, NULL if failed to find one.
void *rsdt_findTable(const char *name);