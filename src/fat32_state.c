#include "drivers/fat32Driver.h"

fat32_fs_t g_fat32_fs = {0};
u8 g_sector_buffer[FAT32_SECTOR_SIZE] = {0};
