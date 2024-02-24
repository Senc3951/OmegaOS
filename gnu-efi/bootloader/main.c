#include <efi.h>
#include <elf.h>
#include <efilib.h>

#define PAGE_SIZE			0x1000
#define PSF1_MAGIC0			0x36
#define PSF1_MAGIC1			0x04
#define KERNEL_ELF_FILE 	L"kernel.elf"
#define PSF_FONT_FILE		L"zap-light16.psf"

typedef struct PSF1_HEADER
{
	UINT8 magic[2];
	UINT8 mode;
	UINT8 charsize;
} PSF1Header_t;

typedef struct PSF1_FONT
{
	PSF1Header_t* header;
	void* glyphBuffer;
} PSF1Font_t;

typedef struct FRAMEBUFFER
{
	VOID *baseAddress;
	UINTN bufferSize;
	UINT32 width;
	UINT32 height;
	UINT32 pixelsPerScanLine;
	UINT32 bytesPerPixel;
} Framebuffer_t;

typedef struct BOOT_INFO
{
	Framebuffer_t *fb;
	PSF1Font_t *font;
	EFI_MEMORY_DESCRIPTOR *mmap;
	UINTN mmapSize;
	UINTN mmapDescriptorSize;
	VOID *rsdp;
} BootInfo_t;

static EFI_HANDLE g_imageHandle;
static EFI_SYSTEM_TABLE* g_systemTable;
static Framebuffer_t g_fb;

UINTN strcmp(CHAR8* a, CHAR8* b, UINTN length)
{
	for (UINTN i = 0; i < length; i++)
	{
		if (a[i] != b[i])
			return 0;
	}

	return 1;
}

int memcmp(const void *a, const void *b, const size_t n)
{
	const uint8_t *aPtr = a, *bPtr = b;
	for (size_t i = 0; i < n; i++)
	{
		if (aPtr[i] < bPtr[i])
			return -1;
		if (aPtr[i] > bPtr[i])
			return 1;
	}
	
	return 0;
}

int validateElfHeader(Elf64_Ehdr *hdr)
{
	return memcmp(&hdr->e_ident[EI_MAG0], ELFMAG, SELFMAG) == 0 &&
		hdr->e_ident[EI_CLASS] == ELFCLASS64 &&
		hdr->e_ident[EI_DATA] == ELFDATA2LSB &&
		hdr->e_type == ET_EXEC &&
		hdr->e_machine == EM_X86_64 &&
		hdr->e_version == EV_CURRENT;
}

Framebuffer_t *initializeGOP()
{
	EFI_GUID efiGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
	
	if (EFI_ERROR(uefi_call_wrapper(BS->LocateProtocol, 3, &efiGuid, NULL, (VOID **)&gop)))
		return NULL;

	g_fb.baseAddress = (VOID *)gop->Mode->FrameBufferBase;
	g_fb.bufferSize = gop->Mode->FrameBufferSize;
	g_fb.width = gop->Mode->Info->HorizontalResolution;
	g_fb.height = gop->Mode->Info->VerticalResolution;
	g_fb.pixelsPerScanLine = gop->Mode->Info->PixelsPerScanLine;
	g_fb.bytesPerPixel = sizeof(uint32_t);
	
	// Verify 32 bit pixel format
	if (gop->Mode->Info->PixelFormat != PixelRedGreenBlueReserved8BitPerColor && gop->Mode->Info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor)
		return NULL;
	
	Print(L"Framebuffer address: %x. width: %u, height: %u, pixelsperline: %u\n\r", gop->Mode->FrameBufferBase, gop->Mode->Info->HorizontalResolution, gop->Mode->Info->VerticalResolution, gop->Mode->Info->PixelsPerScanLine);
	return &g_fb;
}

EFI_FILE* loadFile(EFI_FILE* dir, CHAR16* path)
{
	EFI_FILE* loadedFile;

	// Load protocols
	EFI_LOADED_IMAGE_PROTOCOL* loadedImage;
	g_systemTable->BootServices->HandleProtocol(g_imageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&loadedImage);
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
	g_systemTable->BootServices->HandleProtocol(loadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&fs);
	
	if (!dir)
		fs->OpenVolume(fs, &dir);

	EFI_STATUS status = dir->Open(dir, &loadedFile, path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
	return status != EFI_SUCCESS ? NULL : loadedFile;
}

PSF1Font_t* loadFont(EFI_FILE* dir, CHAR16* path)
{
	EFI_FILE *file = loadFile(dir, path);
	if (!file)
		return NULL;

	// Read header
	PSF1Header_t* fontHeader;
	g_systemTable->BootServices->AllocatePool(EfiLoaderData, sizeof(PSF1Header_t), (void**)&fontHeader);
	UINTN size = sizeof(PSF1Header_t);
	file->Read(file, &size, fontHeader);

	// Validate header
	if (fontHeader->magic[0] != PSF1_MAGIC0 || fontHeader->magic[1] != PSF1_MAGIC1)
		return NULL;

	UINTN glyphBufferSize = fontHeader->charsize * 256;
	if (fontHeader->mode == 1)
		glyphBufferSize *= 2;
	
	// Read rest
	void* glyphBuffer;
	file->SetPosition(file, sizeof(PSF1Header_t));
	g_systemTable->BootServices->AllocatePool(EfiLoaderData, glyphBufferSize, (void**)&glyphBuffer);
	file->Read(file, &glyphBufferSize, glyphBuffer);

	PSF1Font_t* finishedFont;
	g_systemTable->BootServices->AllocatePool(EfiLoaderData, sizeof(PSF1Font_t), (void**)&finishedFont);
	finishedFont->header = fontHeader;
	finishedFont->glyphBuffer = glyphBuffer;
	
	return finishedFont;
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	InitializeLib(ImageHandle, SystemTable);
	g_imageHandle = ImageHandle; g_systemTable = SystemTable;	

	// Load kernel
	EFI_FILE *loadedFile = loadFile(NULL, KERNEL_ELF_FILE);
	if (!loadedFile)
	{
		Print(L"Couldn't find %ls\n\r", KERNEL_ELF_FILE);
		return EFI_NOT_FOUND;
	}
	
	// Load header
	Elf64_Ehdr header;
	UINTN fileInfoSize;
	EFI_FILE_INFO* fileInfo;
	loadedFile->GetInfo(loadedFile, &gEfiFileInfoGuid, &fileInfoSize, NULL);

	// Allocate memory with the size of fileInfoSize
	g_systemTable->BootServices->AllocatePool(EfiLoaderData, fileInfoSize, (VOID **)&fileInfo);
	loadedFile->GetInfo(loadedFile, &gEfiFileInfoGuid, &fileInfoSize, (VOID *)&fileInfo);
	
	UINTN headerSize = sizeof(header);
	loadedFile->Read(loadedFile, &headerSize, &header);

	// Validate header
	if (!validateElfHeader(&header))
	{
		Print(L"%ls is corrupted\n\r", KERNEL_ELF_FILE);
		return EFI_INCOMPATIBLE_VERSION;
	}
	
	// Load file
	Elf64_Phdr* headers;
	loadedFile->SetPosition(loadedFile, header.e_phoff);
	UINTN fileSize = header.e_phnum * header.e_phentsize;
	g_systemTable->BootServices->AllocatePool(EfiLoaderData, fileSize, (VOID **)&headers);
	loadedFile->Read(loadedFile, &fileSize, (VOID *)headers);
	
	for (Elf64_Phdr* phdr = headers;
		(char *)phdr < (char *)headers + header.e_phnum * header.e_phentsize;
		phdr = (Elf64_Phdr *)((char *)phdr + header.e_phentsize))
	{
		switch (phdr->p_type)
		{
			case PT_LOAD:
				int pages = (phdr->p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;
				Elf64_Addr segment = phdr->p_paddr;
				g_systemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, pages, &segment);
				
				loadedFile->SetPosition(loadedFile, phdr->p_offset);
				UINTN segmentSize = phdr->p_filesz;
				loadedFile->Read(loadedFile, &segmentSize, (VOID *)segment);
				
				break;
		}
	}
	
	Framebuffer_t *fb = initializeGOP();
	if (!fb)
	{
		Print(L"Failed loading Framebuffer\n\r");
		return EFI_LOAD_ERROR;
	}

	PSF1Font_t *font = loadFont(NULL, PSF_FONT_FILE);
	if (!font)
	{
		Print(L"Failed loading %ls\n\r", PSF_FONT_FILE);
		return EFI_LOAD_ERROR;
	}
	
	EFI_MEMORY_DESCRIPTOR *mmap = NULL;
	UINT32 descriptorVersion;
	UINTN mapSize, mapKey, descriptorSize;
	g_systemTable->BootServices->GetMemoryMap(&mapSize, mmap, &mapKey, &descriptorSize, &descriptorVersion);
	g_systemTable->BootServices->AllocatePool(EfiLoaderData, mapSize, (VOID **)&mmap);
	g_systemTable->BootServices->GetMemoryMap(&mapSize, mmap, &mapKey, &descriptorSize, &descriptorVersion);

	EFI_CONFIGURATION_TABLE *configTable = g_systemTable->ConfigurationTable;
	void *rsdp = NULL;
	EFI_GUID ACPI2TableGuid = ACPI_20_TABLE_GUID;
	for (UINTN index = 0; index < g_systemTable->NumberOfTableEntries; index++)
	{
		if (CompareGuid(&configTable[index].VendorGuid, &ACPI2TableGuid))
		{
			if (strcmp((CHAR8 *)"RSD PTR ", (CHAR8 *)configTable->VendorTable, 8))
				rsdp = configTable->VendorTable;
		}
		
		configTable++;
	}
	ASSERT(rsdp);

	BootInfo_t bootInfo = { .fb = fb, .font = font, .mmap = mmap, .mmapSize = mapSize, .mmapDescriptorSize = descriptorSize, .rsdp = rsdp };
	Print(L"Starting OS\n\r");
	g_systemTable->BootServices->ExitBootServices(g_imageHandle, mapKey);
		
	int (*kernelStart)(BootInfo_t*) = ((__attribute((sysv_abi)) int (*)(BootInfo_t*)) header.e_entry);
	kernelStart(&bootInfo);
	
	return EFI_SUCCESS;
}