#ifndef FAT32_DRIVER_H
#define FAT32_DRIVER_H

#include "../package.h"

/*
 * FAT32 File System Driver for Spark Kernel
 * 
 * This driver provides basic FAT32 filesystem support including:
 * - Reading the boot sector and BPB (BIOS Parameter Block)
 * - Navigating directories
 * - Reading files
 * - Writing files (basic support)
 * 
 * All functions are static to avoid multiple definition errors when
 * included in multiple translation units.
 */

// ============================================================================
// FAT32 Constants
// ============================================================================

#define FAT32_SECTOR_SIZE       512
#define FAT32_MAX_FILENAME      255
#define FAT32_SHORT_NAME_LEN    11

// FAT32 Entry Special Values
#define FAT32_EOC_MIN           0x0FFFFFF8  // End of cluster chain minimum
#define FAT32_EOC               0x0FFFFFFF  // End of cluster chain
#define FAT32_FREE_CLUSTER      0x00000000  // Free cluster
#define FAT32_BAD_CLUSTER       0x0FFFFFF7  // Bad cluster

// Directory Entry Attributes
#define FAT32_ATTR_READ_ONLY    0x01
#define FAT32_ATTR_HIDDEN       0x02
#define FAT32_ATTR_SYSTEM       0x04
#define FAT32_ATTR_VOLUME_ID    0x08
#define FAT32_ATTR_DIRECTORY    0x10
#define FAT32_ATTR_ARCHIVE      0x20
#define FAT32_ATTR_LONG_NAME    0x0F  // Combined: RO|HID|SYS|VOL

// Directory Entry Special First Bytes
#define FAT32_DIR_ENTRY_FREE    0xE5
#define FAT32_DIR_ENTRY_END     0x00
#define FAT32_DIR_ENTRY_KANJI   0x05  // Actually 0xE5 in Kanji

// ============================================================================
// FAT32 Structures
// ============================================================================

// BIOS Parameter Block (BPB) - FAT32 Extended
typedef struct __attribute__((packed)) {
    u8  jmp_boot[3];           // Jump instruction to boot code
    u8  oem_name[8];           // OEM Name (e.g., "MSWIN4.1")
    u16 bytes_per_sector;      // Bytes per sector (usually 512)
    u8  sectors_per_cluster;   // Sectors per cluster
    u16 reserved_sectors;      // Reserved sector count
    u8  num_fats;              // Number of FATs (usually 2)
    u16 root_entry_count;      // Root entry count (0 for FAT32)
    u16 total_sectors_16;      // Total sectors (0 for FAT32)
    u8  media_type;            // Media type
    u16 fat_size_16;           // FAT size in sectors (0 for FAT32)
    u16 sectors_per_track;     // Sectors per track
    u16 num_heads;             // Number of heads
    u32 hidden_sectors;        // Hidden sectors
    u32 total_sectors_32;      // Total sectors (for FAT32)
    
    // FAT32 Extended BPB
    u32 fat_size_32;           // FAT size in sectors
    u16 ext_flags;             // Extended flags
    u16 fs_version;            // Filesystem version
    u32 root_cluster;          // First cluster of root directory
    u16 fs_info_sector;        // FSInfo sector number
    u16 backup_boot_sector;    // Backup boot sector location
    u8  reserved[12];          // Reserved
    u8  drive_number;          // Drive number
    u8  reserved1;             // Reserved
    u8  boot_signature;        // Extended boot signature (0x29)
    u32 volume_id;             // Volume serial number
    u8  volume_label[11];      // Volume label
    u8  fs_type[8];            // Filesystem type ("FAT32   ")
} fat32_bpb_t;

// FAT32 Directory Entry (32 bytes)
typedef struct __attribute__((packed)) {
    u8  name[11];              // Short filename (8.3 format)
    u8  attributes;            // File attributes
    u8  nt_reserved;           // Reserved for Windows NT
    u8  creation_time_tenths;  // Creation time (tenths of second)
    u16 creation_time;         // Creation time
    u16 creation_date;         // Creation date
    u16 last_access_date;      // Last access date
    u16 first_cluster_high;    // High 16 bits of first cluster
    u16 write_time;            // Last write time
    u16 write_date;            // Last write date
    u16 first_cluster_low;     // Low 16 bits of first cluster
    u32 file_size;             // File size in bytes
} fat32_dir_entry_t;

// Long Filename Entry (LFN)
typedef struct __attribute__((packed)) {
    u8  order;                 // Sequence number
    u16 name1[5];              // Characters 1-5 (UTF-16)
    u8  attributes;            // Always 0x0F
    u8  type;                  // Always 0x00
    u8  checksum;              // Checksum of short name
    u16 name2[6];              // Characters 6-11 (UTF-16)
    u16 first_cluster;         // Always 0x0000
    u16 name3[2];              // Characters 12-13 (UTF-16)
} fat32_lfn_entry_t;

// FSInfo Structure
typedef struct __attribute__((packed)) {
    u32 lead_signature;        // 0x41615252
    u8  reserved1[480];        // Reserved
    u32 struct_signature;      // 0x61417272
    u32 free_cluster_count;    // Free cluster count
    u32 next_free_cluster;     // Next free cluster hint
    u8  reserved2[12];         // Reserved
    u32 trail_signature;       // 0xAA550000
} fat32_fsinfo_t;

// FAT32 Filesystem State
typedef struct {
    u32 partition_start_lba;   // Starting LBA of partition
    u32 fat_start_lba;         // Starting LBA of FAT
    u32 data_start_lba;        // Starting LBA of data region
    u32 root_cluster;          // Root directory cluster
    u32 sectors_per_cluster;   // Sectors per cluster
    u32 bytes_per_cluster;     // Bytes per cluster
    u32 fat_size_sectors;      // FAT size in sectors
    u32 total_clusters;        // Total data clusters
    u8  num_fats;              // Number of FATs
    u8  initialized;           // Initialization flag
} fat32_fs_t;

// File Handle
typedef struct {
    u32 first_cluster;         // First cluster of file
    u32 current_cluster;       // Current cluster being read
    u32 file_size;             // Total file size
    u32 position;              // Current position in file
    u8  attributes;            // File attributes
    u8  is_open;               // File open flag
} fat32_file_t;

// Directory Iterator
typedef struct {
    u32 cluster;               // Current cluster
    u32 entry_index;           // Current entry index in cluster
    u32 sector_offset;         // Current sector offset in cluster
} fat32_dir_iter_t;

// ============================================================================
// Global FAT32 State
// ============================================================================

static fat32_fs_t g_fat32_fs;
static u8 g_sector_buffer[FAT32_SECTOR_SIZE];

// ============================================================================
// Software Division (ARM has no hardware divider)
// ============================================================================

static inline u32 fat32_div(u32 n, u32 d) {
    if (d == 0) return 0;
    u32 q = 0;
    u32 r = 0;
    for (int i = 31; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) {
            r -= d;
            q |= (1U << i);
        }
    }
    return q;
}

static inline u32 fat32_mod(u32 n, u32 d) {
    if (d == 0) return 0;
    u32 r = 0;
    for (int i = 31; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) {
            r -= d;
        }
    }
    return r;
}

// ============================================================================
// Platform-Specific Disk Implementation
// ============================================================================

/*
 * For VersatilePB/QEMU, we can use:
 * 1. PL181 SD Card Controller (MMCI)
 * 2. Memory-mapped disk image
 * 
 * This implementation uses a simple memory-mapped approach for QEMU
 * where a disk image can be loaded at a fixed address.
 */

// Memory-mapped disk base address (configurable)
#define DISK_BASE_ADDR      0x10000000  // Adjust based on your setup

// Simple memory-mapped disk read (for QEMU with -drive file=disk.img,if=pflash)
// Runtime-determined memory base for the disk image. Initialize to default.
static volatile u8 *fat32_mem_base = (volatile u8 *)DISK_BASE_ADDR;

static int fat32_disk_read_sectors(u32 lba, u32 count, void *buffer) {
    volatile u8 *disk = fat32_mem_base;
    u8 *buf = (u8 *)buffer;
    u32 offset = fat32_div(lba, 1) * FAT32_SECTOR_SIZE; /* lba * 512 */
    /* offset = lba * FAT32_SECTOR_SIZE; but keep arithmetic explicit */
    offset = lba * FAT32_SECTOR_SIZE;
    u32 size = count * FAT32_SECTOR_SIZE;
    
    for (u32 i = 0; i < size; i++) {
        buf[i] = disk[offset + i];
    }
    
    return 0;
}

static int fat32_disk_write_sectors(u32 lba, u32 count, const void *buffer) {
    volatile u8 *disk = fat32_mem_base;
    const u8 *buf = (const u8 *)buffer;
    u32 offset = lba * FAT32_SECTOR_SIZE;
    u32 size = count * FAT32_SECTOR_SIZE;
    
    for (u32 i = 0; i < size; i++) {
        disk[offset + i] = buf[i];
    }
    
    return 0;
}

// Probe a memory address to see if it contains a valid FAT32 boot sector.
static int fat32_probe_memory_base(u32 addr) {
    volatile u8 *mem = (volatile u8 *)addr;
    // Read first sector into temporary buffer
    for (u32 i = 0; i < FAT32_SECTOR_SIZE; i++) {
        g_sector_buffer[i] = mem[i];
    }
    // Check boot signature
    if (g_sector_buffer[510] == 0x55 && g_sector_buffer[511] == 0xAA) {
        // Basic sanity: ensure FAT32-specific fields exist: bytes per sector non-zero
        fat32_bpb_t *bpb = (fat32_bpb_t *)g_sector_buffer;
        if (bpb->bytes_per_sector == FAT32_SECTOR_SIZE || bpb->bytes_per_sector == 512) {
            return 1;
        }
    }
    return 0;
}

// ============================================================================
// Helper Functions
// ============================================================================

// Convert cluster number to LBA
static inline u32 fat32_cluster_to_lba(u32 cluster) {
    return g_fat32_fs.data_start_lba + 
           (cluster - 2) * g_fat32_fs.sectors_per_cluster;
}

// Read a FAT entry
static u32 fat32_read_fat_entry(u32 cluster) {
    u32 fat_offset = cluster * 4;
    u32 fat_sector = g_fat32_fs.fat_start_lba + fat32_div(fat_offset, FAT32_SECTOR_SIZE);
    u32 entry_offset = fat32_mod(fat_offset, FAT32_SECTOR_SIZE);
    
    if (fat32_disk_read_sectors(fat_sector, 1, g_sector_buffer) != 0) {
        return FAT32_EOC;  // Error, treat as end of chain
    }
    
    u32 entry = *(u32 *)&g_sector_buffer[entry_offset];
    return entry & 0x0FFFFFFF;  // Mask upper 4 bits
}

// Write a FAT entry
static int fat32_write_fat_entry(u32 cluster, u32 value) {
    u32 fat_offset = cluster * 4;
    u32 fat_sector = g_fat32_fs.fat_start_lba + fat32_div(fat_offset, FAT32_SECTOR_SIZE);
    u32 entry_offset = fat32_mod(fat_offset, FAT32_SECTOR_SIZE);
    
    // Read current sector
    if (fat32_disk_read_sectors(fat_sector, 1, g_sector_buffer) != 0) {
        return -1;
    }
    
    // Modify entry (preserve upper 4 bits)
    u32 *entry = (u32 *)&g_sector_buffer[entry_offset];
    *entry = (*entry & 0xF0000000) | (value & 0x0FFFFFFF);
    
    // Write back to all FATs
    for (u8 i = 0; i < g_fat32_fs.num_fats; i++) {
        u32 fat_lba = fat_sector + (i * g_fat32_fs.fat_size_sectors);
        if (fat32_disk_write_sectors(fat_lba, 1, g_sector_buffer) != 0) {
            return -1;
        }
    }
    
    return 0;
}

// Check if cluster is end of chain
static inline int fat32_is_eoc(u32 cluster) {
    return cluster >= FAT32_EOC_MIN;
}

// Get next cluster in chain
static u32 fat32_next_cluster(u32 cluster) {
    return fat32_read_fat_entry(cluster);
}

// Find a free cluster
static u32 fat32_find_free_cluster(void) {
    for (u32 cluster = 2; cluster < g_fat32_fs.total_clusters + 2; cluster++) {
        if (fat32_read_fat_entry(cluster) == FAT32_FREE_CLUSTER) {
            return cluster;
        }
    }
    return 0;  // No free clusters
}

// ============================================================================
// String Helpers
// ============================================================================

static int fat32_memcmp(const void *s1, const void *s2, u32 n) {
    const u8 *p1 = (const u8 *)s1;
    const u8 *p2 = (const u8 *)s2;
    for (u32 i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

static void fat32_memcpy(void *dest, const void *src, u32 n) {
    u8 *d = (u8 *)dest;
    const u8 *s = (const u8 *)src;
    for (u32 i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

static void fat32_memset(void *s, int c, u32 n) {
    u8 *p = (u8 *)s;
    for (u32 i = 0; i < n; i++) {
        p[i] = (u8)c;
    }
}

static u32 fat32_strlen(const char *s) {
    u32 len = 0;
    while (s[len]) len++;
    return len;
}

// Convert character to uppercase
static inline char fat32_toupper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

// Convert filename to 8.3 format
static int fat32_name_to_83(const char *name, u8 *name83) {
    fat32_memset(name83, ' ', 11);
    
    u32 len = fat32_strlen(name);
    if (len == 0 || len > 12) return -1;
    
    // Find the dot
    int dot_pos = -1;
    for (u32 i = 0; i < len; i++) {
        if (name[i] == '.') {
            dot_pos = i;
            break;
        }
    }
    
    // Copy name part (up to 8 chars)
    int name_len = (dot_pos >= 0) ? dot_pos : (int)len;
    if (name_len > 8) return -1;  // Name too long for 8.3
    
    for (int i = 0; i < name_len; i++) {
        name83[i] = fat32_toupper(name[i]);
    }
    
    // Copy extension (up to 3 chars)
    if (dot_pos >= 0 && dot_pos < (int)len - 1) {
        int ext_len = len - dot_pos - 1;
        if (ext_len > 3) return -1;  // Extension too long
        
        for (int i = 0; i < ext_len; i++) {
            name83[8 + i] = fat32_toupper(name[dot_pos + 1 + i]);
        }
    }
    
    return 0;
}

// Convert 8.3 format to readable name
static void fat32_83_to_name(const u8 *name83, char *name) {
    int pos = 0;
    
    // Copy name part (trimming spaces)
    for (int i = 0; i < 8 && name83[i] != ' '; i++) {
        name[pos++] = name83[i];
    }
    
    // Check if there's an extension
    if (name83[8] != ' ') {
        name[pos++] = '.';
        for (int i = 8; i < 11 && name83[i] != ' '; i++) {
            name[pos++] = name83[i];
        }
    }
    
    name[pos] = '\0';
}

// ============================================================================
// Core FAT32 Functions
// ============================================================================

// Initialize FAT32 filesystem
static int fat32_init(u32 partition_start_lba) {
    fat32_bpb_t *bpb = (fat32_bpb_t *)g_sector_buffer;
    
    // Try to locate the disk image in guest memory by probing likely addresses.
    u32 probe_addrs[] = {
        0x10000000u,
        0x0A000000u,
        0x00A00000u,
        0x00100000u,
        0x00000000u,
        0x20000000u,
        0x40000000u,
        0x08000000u,
        (u32)DISK_BASE_ADDR
    };
    int found = 0;
    for (u32 i = 0; i < sizeof(probe_addrs)/sizeof(probe_addrs[0]); i++) {
        u32 a = probe_addrs[i];
        if (fat32_probe_memory_base(a)) {
            fat32_mem_base = (volatile u8 *)a;
            writeOut("[FAT32] found image at base address: "); writeOutNum(a); writeOut("\n");
            found = 1;
            break;
        }
    }

    // If probe didn't find the image, fall back to default base and read
    if (!found) {
        fat32_mem_base = (volatile u8 *)DISK_BASE_ADDR;
        writeOut("[FAT32] probe failed — using default base\n");
    }

    // Read boot sector from the chosen memory base
    if (fat32_disk_read_sectors(partition_start_lba, 1, g_sector_buffer) != 0) {
        return -1;  // Disk read error
    }
    
    // Debug: dump first bytes and boot signature to help diagnose mapping
    writeOut("[FAT32] boot sector first bytes: ");
    for (int _i = 0; _i < 8; _i++) {
        writeOutNum((long)g_sector_buffer[_i]); writeOut(" ");
    }
    writeOut("\n");
    writeOut("[FAT32] boot sig bytes: ");
    writeOutNum((long)g_sector_buffer[510]); writeOut(","); writeOutNum((long)g_sector_buffer[511]); writeOut("\n");
    
    // Validate boot signature
    if (g_sector_buffer[510] != 0x55 || g_sector_buffer[511] != 0xAA) {
        return -2;  // Invalid boot signature
    }
    
    // Validate FAT32 (FAT size 16 should be 0, FAT size 32 should be non-zero)
    if (bpb->fat_size_16 != 0 || bpb->fat_size_32 == 0) {
        return -3;  // Not a FAT32 filesystem
    }
    
    // Store filesystem parameters
    g_fat32_fs.partition_start_lba = partition_start_lba;
    g_fat32_fs.sectors_per_cluster = bpb->sectors_per_cluster;
    g_fat32_fs.bytes_per_cluster = bpb->sectors_per_cluster * bpb->bytes_per_sector;
    g_fat32_fs.num_fats = bpb->num_fats;
    g_fat32_fs.fat_size_sectors = bpb->fat_size_32;
    g_fat32_fs.root_cluster = bpb->root_cluster;
    
    // Calculate LBA addresses
    g_fat32_fs.fat_start_lba = partition_start_lba + bpb->reserved_sectors;
    g_fat32_fs.data_start_lba = g_fat32_fs.fat_start_lba + 
                                 (bpb->num_fats * bpb->fat_size_32);
    
    // Calculate total clusters using software division
    u32 data_sectors = bpb->total_sectors_32 - 
                       (bpb->reserved_sectors + bpb->num_fats * bpb->fat_size_32);
    g_fat32_fs.total_clusters = fat32_div(data_sectors, bpb->sectors_per_cluster);
    
    g_fat32_fs.initialized = 1;
    
    return 0;  // Success
}

// Check if FAT32 is initialized
static int fat32_is_initialized(void) {
    return g_fat32_fs.initialized;
}

// Read a cluster into buffer
static int fat32_read_cluster(u32 cluster, void *buffer) {
    if (!g_fat32_fs.initialized) return -1;
    if (cluster < 2) return -1;
    
    u32 lba = fat32_cluster_to_lba(cluster);
    return fat32_disk_read_sectors(lba, g_fat32_fs.sectors_per_cluster, buffer);
}

// Write a cluster from buffer
static int fat32_write_cluster(u32 cluster, const void *buffer) {
    if (!g_fat32_fs.initialized) return -1;
    if (cluster < 2) return -1;
    
    u32 lba = fat32_cluster_to_lba(cluster);
    return fat32_disk_write_sectors(lba, g_fat32_fs.sectors_per_cluster, buffer);
}

// ============================================================================
// Directory Operations
// ============================================================================

// Initialize directory iterator
static void fat32_dir_open(fat32_dir_iter_t *iter, u32 cluster) {
    iter->cluster = cluster;
    iter->entry_index = 0;
    iter->sector_offset = 0;
}

// Open root directory
static void fat32_dir_open_root(fat32_dir_iter_t *iter) {
    fat32_dir_open(iter, g_fat32_fs.root_cluster);
}

// Read next directory entry
// Returns 0 if entry read, 1 if end of directory, -1 on error
static int fat32_dir_read(fat32_dir_iter_t *iter, fat32_dir_entry_t *entry) {
    if (!g_fat32_fs.initialized) return -1;
    
    u8 cluster_buffer[FAT32_SECTOR_SIZE];
    u32 entries_per_sector = fat32_div(FAT32_SECTOR_SIZE, sizeof(fat32_dir_entry_t));
    
    while (1) {
        // Check if we need to move to next cluster
        u32 entries_per_cluster = fat32_div(g_fat32_fs.bytes_per_cluster, sizeof(fat32_dir_entry_t));
        if (iter->entry_index >= entries_per_cluster) {
            u32 next = fat32_next_cluster(iter->cluster);
            if (fat32_is_eoc(next)) {
                return 1;  // End of directory
            }
            iter->cluster = next;
            iter->entry_index = 0;
            iter->sector_offset = 0;
        }
        
        // Calculate which sector within the cluster
        u32 sector_in_cluster = fat32_div(iter->entry_index * sizeof(fat32_dir_entry_t), FAT32_SECTOR_SIZE);
        u32 entry_in_sector = fat32_mod(iter->entry_index, entries_per_sector);
        
        // Read the sector
        u32 lba = fat32_cluster_to_lba(iter->cluster) + sector_in_cluster;
        if (fat32_disk_read_sectors(lba, 1, cluster_buffer) != 0) {
            return -1;
        }
        
        fat32_dir_entry_t *dir_entry = (fat32_dir_entry_t *)cluster_buffer + entry_in_sector;
        iter->entry_index++;
        
        // Check for end of directory
        if (dir_entry->name[0] == FAT32_DIR_ENTRY_END) {
            return 1;
        }
        
        // Skip deleted entries
        if (dir_entry->name[0] == FAT32_DIR_ENTRY_FREE) {
            continue;
        }
        
        // Skip LFN entries
        if ((dir_entry->attributes & FAT32_ATTR_LONG_NAME) == FAT32_ATTR_LONG_NAME) {
            continue;
        }
        
        // Skip volume label
        if (dir_entry->attributes & FAT32_ATTR_VOLUME_ID) {
            continue;
        }
        
        // Found a valid entry
        fat32_memcpy(entry, dir_entry, sizeof(fat32_dir_entry_t));
        return 0;
    }
}

// Find entry in directory by name
static int fat32_dir_find(u32 dir_cluster, const char *name, fat32_dir_entry_t *entry) {
    u8 name83[11];
    
    if (fat32_name_to_83(name, name83) != 0) {
        return -1;  // Invalid filename
    }
    
    fat32_dir_iter_t iter;
    fat32_dir_open(&iter, dir_cluster);
    
    while (fat32_dir_read(&iter, entry) == 0) {
        if (fat32_memcmp(entry->name, name83, 11) == 0) {
            return 0;  // Found
        }
    }
    
    return -1;  // Not found
}

// Get first cluster from directory entry
static inline u32 fat32_entry_cluster(const fat32_dir_entry_t *entry) {
    return ((u32)entry->first_cluster_high << 16) | entry->first_cluster_low;
}

// ============================================================================
// Path Resolution
// ============================================================================

// Resolve a path to a directory entry
// Path format: "/dir1/dir2/filename" or "dir1/dir2/filename"
static int fat32_resolve_path(const char *path, fat32_dir_entry_t *entry) {
    if (!g_fat32_fs.initialized) return -1;
    
    u32 current_cluster = g_fat32_fs.root_cluster;
    char component[13];  // 8.3 + dot + null
    int comp_pos = 0;
    
    // Skip leading slash
    if (*path == '/') path++;
    
    // Handle empty path (root directory)
    if (*path == '\0') {
        fat32_memset(entry, 0, sizeof(fat32_dir_entry_t));
        entry->attributes = FAT32_ATTR_DIRECTORY;
        entry->first_cluster_high = (g_fat32_fs.root_cluster >> 16) & 0xFFFF;
        entry->first_cluster_low = g_fat32_fs.root_cluster & 0xFFFF;
        return 0;
    }
    
    while (*path) {
        // Extract path component
        comp_pos = 0;
        while (*path && *path != '/' && comp_pos < 12) {
            component[comp_pos++] = *path++;
        }
        component[comp_pos] = '\0';
        
        // Skip slash
        if (*path == '/') path++;
        
        // Find component in current directory
        if (fat32_dir_find(current_cluster, component, entry) != 0) {
            return -1;  // Not found
        }
        
        // If not last component, must be a directory
        if (*path != '\0') {
            if (!(entry->attributes & FAT32_ATTR_DIRECTORY)) {
                return -1;  // Not a directory
            }
            current_cluster = fat32_entry_cluster(entry);
        }
    }
    
    return 0;  // Success
}

// ============================================================================
// File Operations
// ============================================================================

// Open a file
static int fat32_file_open(fat32_file_t *file, const char *path) {
    fat32_dir_entry_t entry;
    
    if (fat32_resolve_path(path, &entry) != 0) {
        return -1;  // File not found
    }
    
    if (entry.attributes & FAT32_ATTR_DIRECTORY) {
        return -2;  // Is a directory
    }
    
    file->first_cluster = fat32_entry_cluster(&entry);
    file->current_cluster = file->first_cluster;
    file->file_size = entry.file_size;
    file->position = 0;
    file->attributes = entry.attributes;
    file->is_open = 1;
    
    return 0;
}

// Close a file
static void fat32_file_close(fat32_file_t *file) {
    file->is_open = 0;
}

// Read from file
// Returns number of bytes read, or -1 on error
static int fat32_file_read(fat32_file_t *file, void *buffer, u32 size) {
    if (!file->is_open) return -1;
    
    u8 *buf = (u8 *)buffer;
    u32 bytes_read = 0;
    u8 cluster_buffer[4096];  // Max cluster size we support
    
    // Limit read to remaining file size
    if (file->position + size > file->file_size) {
        size = file->file_size - file->position;
    }
    
    while (bytes_read < size) {
        // Check for end of file
        if (file->current_cluster == 0 || fat32_is_eoc(file->current_cluster)) {
            break;
        }
        
        // Calculate position within current cluster
        u32 cluster_offset = fat32_mod(file->position, g_fat32_fs.bytes_per_cluster);
        u32 bytes_in_cluster = g_fat32_fs.bytes_per_cluster - cluster_offset;
        u32 bytes_to_read = (size - bytes_read < bytes_in_cluster) ? 
                            (size - bytes_read) : bytes_in_cluster;
        
        // Read cluster
        if (fat32_read_cluster(file->current_cluster, cluster_buffer) != 0) {
            return -1;
        }
        
        // Copy data
        fat32_memcpy(buf + bytes_read, cluster_buffer + cluster_offset, bytes_to_read);
        
        bytes_read += bytes_to_read;
        file->position += bytes_to_read;
        
        // Move to next cluster if needed
        if (fat32_mod(file->position, g_fat32_fs.bytes_per_cluster) == 0) {
            file->current_cluster = fat32_next_cluster(file->current_cluster);
        }
    }
    
    return bytes_read;
}

// Seek in file
static int fat32_file_seek(fat32_file_t *file, u32 position) {
    if (!file->is_open) return -1;
    if (position > file->file_size) return -1;
    
    // Reset to start
    file->current_cluster = file->first_cluster;
    file->position = 0;
    
    // Skip clusters to reach position
    u32 clusters_to_skip = fat32_div(position, g_fat32_fs.bytes_per_cluster);
    for (u32 i = 0; i < clusters_to_skip; i++) {
        u32 next = fat32_next_cluster(file->current_cluster);
        if (fat32_is_eoc(next)) {
            return -1;  // Unexpected end of cluster chain
        }
        file->current_cluster = next;
    }
    
    file->position = position;
    return 0;
}

// Get file size
static u32 fat32_file_size(fat32_file_t *file) {
    return file->is_open ? file->file_size : 0;
}

// ============================================================================
// High-Level Convenience Functions
// ============================================================================

// List directory contents (for shell/debugging)
static void fat32_list_dir(const char *path) {
    fat32_dir_entry_t entry;
    fat32_dir_iter_t iter;
    char name[13];
    u32 cluster;
    
    // Resolve path to get directory cluster
    if (path == (void*)0 || path[0] == '\0' || (path[0] == '/' && path[1] == '\0')) {
        cluster = g_fat32_fs.root_cluster;
    } else {
        if (fat32_resolve_path(path, &entry) != 0) {
            writeOut("Directory not found\n");
            return;
        }
        if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
            writeOut("Not a directory\n");
            return;
        }
        cluster = fat32_entry_cluster(&entry);
    }
    
    fat32_dir_open(&iter, cluster);
    
    while (fat32_dir_read(&iter, &entry) == 0) {
        fat32_83_to_name(entry.name, name);
        
        if (entry.attributes & FAT32_ATTR_DIRECTORY) {
            writeOut("[DIR]  ");
        } else {
            writeOut("       ");
        }
        
        writeOut(name);
        
        if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
            writeOut("  (");
            writeOutNum(entry.file_size);
            writeOut(" bytes)");
        }
        
        writeOut("\n");
    }
}

// Read entire file into buffer
// Returns bytes read or -1 on error
static int fat32_read_file(const char *path, void *buffer, u32 max_size) {
    fat32_file_t file;
    
    if (fat32_file_open(&file, path) != 0) {
        return -1;
    }
    
    u32 size = file.file_size;
    if (size > max_size) {
        size = max_size;
    }
    
    int result = fat32_file_read(&file, buffer, size);
    fat32_file_close(&file);
    
    return result;
}

// Check if path exists
static int fat32_exists(const char *path) {
    fat32_dir_entry_t entry;
    return fat32_resolve_path(path, &entry) == 0;
}

// Check if path is a directory
static int fat32_is_directory(const char *path) {
    fat32_dir_entry_t entry;
    if (fat32_resolve_path(path, &entry) != 0) {
        return 0;
    }
    return (entry.attributes & FAT32_ATTR_DIRECTORY) != 0;
}

// Get volume label
static void fat32_get_volume_label(char *label) {
    fat32_dir_entry_t entry;
    fat32_dir_iter_t iter;
    
    fat32_dir_open_root(&iter);
    
    while (fat32_dir_read(&iter, &entry) == 0) {
        // We already skip volume labels in dir_read, so read raw
    }
    
    // Default label if not found
    fat32_memcpy(label, "NO NAME    ", 11);
    label[11] = '\0';
}

#endif /* FAT32_DRIVER_H */
