#ifndef WRITE_DRIVER_H
#define WRITE_DRIVER_H

/*
 * FAT32 Write Driver for Spark Kernel
 *
 * This driver extends the FAT32 read driver with write capabilities:
 * - Creating empty files (touch)
 * - Writing data to files
 * - Creating directories
 * - Deleting files
 *
 * Requires: fat32Driver.h, pl181_sd.h
 */

#include "fat32Driver.h"

// ============================================================================
// SD Card Write Support
// ============================================================================

// Write sectors to SD card
// lba: Logical Block Address (sector number)
// count: Number of sectors to write
// buffer: Input buffer (must be at least count * 512 bytes)
// Returns 0 on success, -1 on error
static int sd_write_sectors(u32 lba, u32 count, const void *buffer) {
    if (!sd_initialized) {
        if (sd_init() != 0) {
            return -1;
        }
    }

    const u8 *buf = (const u8 *)buffer;

    // SD Commands for write
    #define SD_CMD_WRITE_SINGLE     24
    #define SD_CMD_WRITE_MULTIPLE   25

    for (u32 sector = 0; sector < count; sector++) {
        u32 addr = (lba + sector) * SD_SECTOR_SIZE;  // Byte address for standard SD

        // Clear status
        *MMCI_CLEAR = 0x7FF;

        // Set up data transfer (write direction: card receives)
        *MMCI_DATATIMER = 0xFFFFFF;
        *MMCI_DATALENGTH = SD_SECTOR_SIZE;
        // Direction bit = 0 for write (controller to card)
        *MMCI_DATACTRL = MMCI_DCTRL_ENABLE | MMCI_DCTRL_BLOCKSIZE(9);

        // CMD24: Write single block
        if (sd_send_cmd(SD_CMD_WRITE_SINGLE, addr, 1) != 0) {
            return -1;
        }

        // Write data to FIFO
        const u32 *buf32 = (const u32 *)(buf + sector * SD_SECTOR_SIZE);
        int words_written = 0;
        int timeout = 1000000;

        while (words_written < (SD_SECTOR_SIZE / 4) && timeout-- > 0) {
            u32 status = *MMCI_STATUS;

            if (status & (MMCI_STAT_DATACRCFAIL | MMCI_STAT_DATATIMEOUT | MMCI_STAT_TXUNDERRUN)) {
                return -1;  // Data error
            }

            // Check if FIFO has space (not full)
            if (!(status & MMCI_STAT_TXFIFOFULL)) {
                *MMCI_FIFO = buf32[words_written++];
            }
        }

        if (words_written < (SD_SECTOR_SIZE / 4)) {
            return -1;  // Incomplete write
        }

        // Wait for data end
        timeout = 100000;
        while (timeout-- > 0) {
            if (*MMCI_STATUS & MMCI_STAT_DATAEND) {
                break;
            }
        }

        // Clear status
        *MMCI_CLEAR = 0x7FF;

        // Small delay for card to finish programming
        for (volatile int i = 0; i < 10000; i++) {
            __asm__ volatile("nop");
        }
    }

    return 0;
}

// ============================================================================
// FAT32 Write Operations
// ============================================================================

// Allocate a new cluster and mark it as end-of-chain
// Returns the allocated cluster number, or 0 on failure
static u32 fat32_alloc_cluster(void) {
    u32 cluster = fat32_find_free_cluster();
    if (cluster == 0) {
        return 0;  // No free clusters
    }

    // Mark cluster as end-of-chain
    if (fat32_write_fat_entry(cluster, FAT32_EOC) != 0) {
        return 0;
    }

    return cluster;
}

// Free a cluster chain starting from the given cluster
static int fat32_free_chain(u32 start_cluster) {
    u32 cluster = start_cluster;

    while (cluster >= 2 && !fat32_is_eoc(cluster)) {
        u32 next = fat32_next_cluster(cluster);
        if (fat32_write_fat_entry(cluster, FAT32_FREE_CLUSTER) != 0) {
            return -1;
        }
        cluster = next;
    }

    return 0;
}

// Write a cluster to disk
static int fat32_write_cluster_data(u32 cluster, const void *buffer) {
    if (!g_fat32_fs.initialized) return -1;
    if (cluster < 2) return -1;

    u32 lba = fat32_cluster_to_lba(cluster);
    return sd_write_sectors(lba, g_fat32_fs.sectors_per_cluster, buffer);
}

// Get the parent directory cluster from a path
// Also extracts the filename component into 'filename' buffer
static u32 fat32_get_parent_dir(const char *path, char *filename, int filename_size) {
    if (!path || path[0] == '\0') return 0;

    // Skip leading slash
    if (*path == '/') path++;

    // Find the last component
    const char *last_slash = (void*)0;
    const char *p = path;
    while (*p) {
        if (*p == '/') last_slash = p;
        p++;
    }

    // Extract filename
    const char *fname_start = last_slash ? (last_slash + 1) : path;
    int i = 0;
    while (*fname_start && i < filename_size - 1) {
        filename[i++] = *fname_start++;
    }
    filename[i] = '\0';

    // If no directory component, parent is root
    if (!last_slash) {
        return g_fat32_fs.root_cluster;
    }

    // Resolve parent directory path
    // Build parent path (everything before last slash)
    char parent_path[256];
    int len = 0;
    parent_path[len++] = '/';
    p = path;
    while (p < last_slash && len < 255) {
        parent_path[len++] = *p++;
    }
    parent_path[len] = '\0';

    fat32_dir_entry_t entry;
    if (fat32_resolve_path(parent_path, &entry) != 0) {
        return 0;  // Parent directory not found
    }

    if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
        return 0;  // Parent is not a directory
    }

    return fat32_entry_cluster(&entry);
}

// Find a free directory entry slot in a directory
// Returns the sector LBA and entry offset within that sector
// Also returns the cluster containing the entry
static int fat32_find_free_dir_entry(u32 dir_cluster, u32 *out_sector_lba,
                                     u32 *out_entry_offset, u32 *out_cluster) {
    u8 sector_buffer[FAT32_SECTOR_SIZE];
    u32 entries_per_sector = FAT32_SECTOR_SIZE / sizeof(fat32_dir_entry_t);
    u32 current_cluster = dir_cluster;

    while (!fat32_is_eoc(current_cluster) && current_cluster >= 2) {
        u32 cluster_lba = fat32_cluster_to_lba(current_cluster);

        // Check each sector in the cluster
        for (u32 s = 0; s < g_fat32_fs.sectors_per_cluster; s++) {
            u32 sector_lba = cluster_lba + s;

            if (fat32_disk_read_sectors(sector_lba, 1, sector_buffer) != 0) {
                return -1;
            }

            fat32_dir_entry_t *entries = (fat32_dir_entry_t *)sector_buffer;

            for (u32 e = 0; e < entries_per_sector; e++) {
                // Check for free entry (0x00 = end of dir, 0xE5 = deleted)
                if (entries[e].name[0] == FAT32_DIR_ENTRY_END ||
                    entries[e].name[0] == FAT32_DIR_ENTRY_FREE) {
                    *out_sector_lba = sector_lba;
                    *out_entry_offset = e * sizeof(fat32_dir_entry_t);
                    *out_cluster = current_cluster;
                    return 0;
                }
            }
        }

        // Move to next cluster
        u32 next = fat32_next_cluster(current_cluster);
        if (fat32_is_eoc(next)) {
            // Need to allocate a new cluster for the directory
            u32 new_cluster = fat32_alloc_cluster();
            if (new_cluster == 0) {
                return -1;  // No space
            }

            // Link the new cluster
            if (fat32_write_fat_entry(current_cluster, new_cluster) != 0) {
                return -1;
            }

            // Zero out the new cluster
            u8 zero_buffer[FAT32_SECTOR_SIZE];
            fat32_memset(zero_buffer, 0, FAT32_SECTOR_SIZE);
            u32 new_lba = fat32_cluster_to_lba(new_cluster);
            for (u32 s = 0; s < g_fat32_fs.sectors_per_cluster; s++) {
                if (sd_write_sectors(new_lba + s, 1, zero_buffer) != 0) {
                    return -1;
                }
            }

            // Return first entry of new cluster
            *out_sector_lba = new_lba;
            *out_entry_offset = 0;
            *out_cluster = new_cluster;
            return 0;
        }
        current_cluster = next;
    }

    return -1;  // No free entry found
}

// Create a new file (empty)
// Returns 0 on success, -1 on error
static int fat32_create_file(const char *path) {
    if (!g_fat32_fs.initialized) {
        return -1;
    }

    // Check if file already exists
    if (fat32_exists(path)) {
        return -2;  // File already exists
    }

    // Get parent directory and filename
    char filename[13];
    u32 parent_cluster = fat32_get_parent_dir(path, filename, sizeof(filename));
    if (parent_cluster == 0) {
        return -3;  // Parent directory not found
    }

    // Convert filename to 8.3 format
    u8 name83[11];
    if (fat32_name_to_83(filename, name83) != 0) {
        return -4;  // Invalid filename
    }

    // Find a free directory entry
    u32 sector_lba, entry_offset, entry_cluster;
    if (fat32_find_free_dir_entry(parent_cluster, &sector_lba, &entry_offset, &entry_cluster) != 0) {
        return -5;  // No free directory entry
    }

    // Read the sector containing the entry
    u8 sector_buffer[FAT32_SECTOR_SIZE];
    if (fat32_disk_read_sectors(sector_lba, 1, sector_buffer) != 0) {
        return -6;
    }

    // Create the directory entry
    fat32_dir_entry_t *entry = (fat32_dir_entry_t *)(sector_buffer + entry_offset);
    fat32_memset(entry, 0, sizeof(fat32_dir_entry_t));

    // Set filename
    fat32_memcpy(entry->name, name83, 11);

    // Set attributes (archive bit for new files)
    entry->attributes = FAT32_ATTR_ARCHIVE;

    // Set timestamps (simplified - use fixed values)
    // Date format: bits 0-4 = day (1-31), bits 5-8 = month (1-12), bits 9-15 = year from 1980
    // Time format: bits 0-4 = seconds/2, bits 5-10 = minutes, bits 11-15 = hours
    u16 date = (45 << 9) | (12 << 5) | 9;   // Dec 9, 2025 (2025-1980=45)
    u16 time = (12 << 11) | (0 << 5) | 0;   // 12:00:00

    entry->creation_date = date;
    entry->creation_time = time;
    entry->write_date = date;
    entry->write_time = time;
    entry->last_access_date = date;

    // For an empty file, no cluster allocated yet (cluster = 0)
    entry->first_cluster_high = 0;
    entry->first_cluster_low = 0;
    entry->file_size = 0;

    // Write the sector back
    if (sd_write_sectors(sector_lba, 1, sector_buffer) != 0) {
        return -7;
    }

    return 0;
}

// Delete a file
// Returns 0 on success, -1 on error
static int fat32_delete_file(const char *path) {
    if (!g_fat32_fs.initialized) {
        return -1;
    }

    // Resolve the file path
    fat32_dir_entry_t entry;
    if (fat32_resolve_path(path, &entry) != 0) {
        return -2;  // File not found
    }

    // Cannot delete directories with this function
    if (entry.attributes & FAT32_ATTR_DIRECTORY) {
        return -3;
    }

    // Get parent directory
    char filename[13];
    u32 parent_cluster = fat32_get_parent_dir(path, filename, sizeof(filename));
    if (parent_cluster == 0) {
        return -4;
    }

    // Convert filename to 8.3
    u8 name83[11];
    if (fat32_name_to_83(filename, name83) != 0) {
        return -5;
    }

    // Find the entry in the parent directory
    u8 sector_buffer[FAT32_SECTOR_SIZE];
    u32 entries_per_sector = FAT32_SECTOR_SIZE / sizeof(fat32_dir_entry_t);
    u32 current_cluster = parent_cluster;

    while (!fat32_is_eoc(current_cluster) && current_cluster >= 2) {
        u32 cluster_lba = fat32_cluster_to_lba(current_cluster);

        for (u32 s = 0; s < g_fat32_fs.sectors_per_cluster; s++) {
            u32 sector_lba = cluster_lba + s;

            if (fat32_disk_read_sectors(sector_lba, 1, sector_buffer) != 0) {
                return -6;
            }

            fat32_dir_entry_t *entries = (fat32_dir_entry_t *)sector_buffer;

            for (u32 e = 0; e < entries_per_sector; e++) {
                if (entries[e].name[0] == FAT32_DIR_ENTRY_END) {
                    return -7;  // Reached end without finding file
                }

                if (entries[e].name[0] == FAT32_DIR_ENTRY_FREE) {
                    continue;  // Skip deleted entries
                }

                if (fat32_memcmp(entries[e].name, name83, 11) == 0) {
                    // Found the entry
                    u32 first_cluster = fat32_entry_cluster(&entries[e]);

                    // Free the cluster chain
                    if (first_cluster >= 2) {
                        fat32_free_chain(first_cluster);
                    }

                    // Mark entry as deleted
                    entries[e].name[0] = FAT32_DIR_ENTRY_FREE;

                    // Write sector back
                    if (sd_write_sectors(sector_lba, 1, sector_buffer) != 0) {
                        return -8;
                    }

                    return 0;
                }
            }
        }

        current_cluster = fat32_next_cluster(current_cluster);
    }

    return -9;  // Entry not found
}

// Write data to a file (overwrites existing content)
// Returns bytes written, or -1 on error
static int fat32_write_file(const char *path, const void *data, u32 size) {
    if (!g_fat32_fs.initialized) {
        return -1;
    }

    // For now, only support writing to new or existing empty files
    // Full implementation would handle partial writes, appending, etc.

    fat32_dir_entry_t entry;
    if (fat32_resolve_path(path, &entry) != 0) {
        // File doesn't exist, create it first
        if (fat32_create_file(path) != 0) {
            return -1;
        }
        if (fat32_resolve_path(path, &entry) != 0) {
            return -1;
        }
    }

    if (entry.attributes & FAT32_ATTR_DIRECTORY) {
        return -2;  // Cannot write to directory
    }

    // Get parent directory info to update the entry later
    char filename[13];
    u32 parent_cluster = fat32_get_parent_dir(path, filename, sizeof(filename));
    if (parent_cluster == 0) {
        return -3;
    }

    u8 name83[11];
    fat32_name_to_83(filename, name83);

    // Free existing cluster chain if file has content
    u32 old_cluster = fat32_entry_cluster(&entry);
    if (old_cluster >= 2) {
        fat32_free_chain(old_cluster);
    }

    // Allocate clusters for new data
    u32 bytes_per_cluster = g_fat32_fs.bytes_per_cluster;
    u32 clusters_needed = size > 0 ? ((size + bytes_per_cluster - 1) / bytes_per_cluster) : 0;

    u32 first_cluster = 0;
    u32 prev_cluster = 0;

    const u8 *src = (const u8 *)data;
    u32 bytes_remaining = size;

    for (u32 i = 0; i < clusters_needed; i++) {
        u32 cluster = fat32_alloc_cluster();
        if (cluster == 0) {
            // Out of space - free what we allocated
            if (first_cluster >= 2) {
                fat32_free_chain(first_cluster);
            }
            return -4;
        }

        if (first_cluster == 0) {
            first_cluster = cluster;
        }

        // Link to previous cluster
        if (prev_cluster >= 2) {
            fat32_write_fat_entry(prev_cluster, cluster);
        }
        prev_cluster = cluster;

        // Write data to cluster
        u8 cluster_buffer[4096];  // Max cluster size we support
        u32 bytes_to_write = bytes_remaining < bytes_per_cluster ? bytes_remaining : bytes_per_cluster;

        fat32_memset(cluster_buffer, 0, bytes_per_cluster);
        fat32_memcpy(cluster_buffer, src, bytes_to_write);

        if (fat32_write_cluster_data(cluster, cluster_buffer) != 0) {
            fat32_free_chain(first_cluster);
            return -5;
        }

        src += bytes_to_write;
        bytes_remaining -= bytes_to_write;
    }

    // Update directory entry with new cluster and size
    u8 sector_buffer[FAT32_SECTOR_SIZE];
    u32 entries_per_sector = FAT32_SECTOR_SIZE / sizeof(fat32_dir_entry_t);
    u32 current_cluster = parent_cluster;

    while (!fat32_is_eoc(current_cluster) && current_cluster >= 2) {
        u32 cluster_lba = fat32_cluster_to_lba(current_cluster);

        for (u32 s = 0; s < g_fat32_fs.sectors_per_cluster; s++) {
            u32 sector_lba = cluster_lba + s;

            if (fat32_disk_read_sectors(sector_lba, 1, sector_buffer) != 0) {
                return -6;
            }

            fat32_dir_entry_t *entries = (fat32_dir_entry_t *)sector_buffer;

            for (u32 e = 0; e < entries_per_sector; e++) {
                if (fat32_memcmp(entries[e].name, name83, 11) == 0) {
                    // Update entry
                    entries[e].first_cluster_high = (first_cluster >> 16) & 0xFFFF;
                    entries[e].first_cluster_low = first_cluster & 0xFFFF;
                    entries[e].file_size = size;

                    // Update modification time
                    u16 date = (45 << 9) | (12 << 5) | 9;
                    u16 time = (12 << 11) | (0 << 5) | 0;
                    entries[e].write_date = date;
                    entries[e].write_time = time;

                    // Write sector back
                    if (sd_write_sectors(sector_lba, 1, sector_buffer) != 0) {
                        return -7;
                    }

                    return size;
                }
            }
        }

        current_cluster = fat32_next_cluster(current_cluster);
    }

    return -8;  // Entry not found (shouldn't happen)
}

#endif /* WRITE_DRIVER_H */
