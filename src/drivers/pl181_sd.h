#ifndef PL181_SD_H
#define PL181_SD_H

/*
 * PL181 SD/MMC Controller Driver for VersatilePB
 * 
 * This driver provides block-level read access to SD cards
 * attached via QEMU's -drive file=disk.img,if=sd option.
 */

#include "../package.h"

// PL181 MMCI base address on VersatilePB
#define MMCI_BASE           0x10005000

// PL181 Register offsets
#define MMCI_POWER          ((volatile u32 *)(MMCI_BASE + 0x00))
#define MMCI_CLOCK          ((volatile u32 *)(MMCI_BASE + 0x04))
#define MMCI_ARGUMENT       ((volatile u32 *)(MMCI_BASE + 0x08))
#define MMCI_COMMAND        ((volatile u32 *)(MMCI_BASE + 0x0C))
#define MMCI_RESPCMD        ((volatile u32 *)(MMCI_BASE + 0x10))
#define MMCI_RESPONSE0      ((volatile u32 *)(MMCI_BASE + 0x14))
#define MMCI_RESPONSE1      ((volatile u32 *)(MMCI_BASE + 0x18))
#define MMCI_RESPONSE2      ((volatile u32 *)(MMCI_BASE + 0x1C))
#define MMCI_RESPONSE3      ((volatile u32 *)(MMCI_BASE + 0x20))
#define MMCI_DATATIMER      ((volatile u32 *)(MMCI_BASE + 0x24))
#define MMCI_DATALENGTH     ((volatile u32 *)(MMCI_BASE + 0x28))
#define MMCI_DATACTRL       ((volatile u32 *)(MMCI_BASE + 0x2C))
#define MMCI_DATACNT        ((volatile u32 *)(MMCI_BASE + 0x30))
#define MMCI_STATUS         ((volatile u32 *)(MMCI_BASE + 0x34))
#define MMCI_CLEAR          ((volatile u32 *)(MMCI_BASE + 0x38))
#define MMCI_MASK0          ((volatile u32 *)(MMCI_BASE + 0x3C))
#define MMCI_MASK1          ((volatile u32 *)(MMCI_BASE + 0x40))
#define MMCI_FIFOCNT        ((volatile u32 *)(MMCI_BASE + 0x48))
#define MMCI_FIFO           ((volatile u32 *)(MMCI_BASE + 0x80))

// Status bits
#define MMCI_STAT_CMDCRCFAIL    (1 << 0)
#define MMCI_STAT_DATACRCFAIL   (1 << 1)
#define MMCI_STAT_CMDTIMEOUT    (1 << 2)
#define MMCI_STAT_DATATIMEOUT   (1 << 3)
#define MMCI_STAT_TXUNDERRUN    (1 << 4)
#define MMCI_STAT_RXOVERRUN     (1 << 5)
#define MMCI_STAT_CMDRESPEND    (1 << 6)
#define MMCI_STAT_CMDSENT       (1 << 7)
#define MMCI_STAT_DATAEND       (1 << 8)
#define MMCI_STAT_DATABLOCKEND  (1 << 10)
#define MMCI_STAT_CMDACTIVE     (1 << 11)
#define MMCI_STAT_TXACTIVE      (1 << 12)
#define MMCI_STAT_RXACTIVE      (1 << 13)
#define MMCI_STAT_TXFIFOHALF    (1 << 14)
#define MMCI_STAT_RXFIFOHALF    (1 << 15)
#define MMCI_STAT_TXFIFOFULL    (1 << 16)
#define MMCI_STAT_RXFIFOFULL    (1 << 17)
#define MMCI_STAT_TXFIFOEMPTY   (1 << 18)
#define MMCI_STAT_RXFIFOEMPTY   (1 << 19)
#define MMCI_STAT_TXDATAAVAIL   (1 << 20)
#define MMCI_STAT_RXDATAAVAIL   (1 << 21)

// Command register bits
#define MMCI_CMD_RESPONSE       (1 << 6)
#define MMCI_CMD_LONGRESP       (1 << 7)
#define MMCI_CMD_INTERRUPT      (1 << 8)
#define MMCI_CMD_PENDING        (1 << 9)
#define MMCI_CMD_ENABLE         (1 << 10)

// Data control bits
#define MMCI_DCTRL_ENABLE       (1 << 0)
#define MMCI_DCTRL_DIRECTION    (1 << 1)  // 1 = read (card to controller)
#define MMCI_DCTRL_MODE         (1 << 2)  // 0 = block, 1 = stream
#define MMCI_DCTRL_DMAENABLE    (1 << 3)
#define MMCI_DCTRL_BLOCKSIZE(n) (((n) & 0xF) << 4)  // 2^n bytes

// Power control
#define MMCI_POWER_OFF          0x00
#define MMCI_POWER_UP           0x02
#define MMCI_POWER_ON           0x03

// SD Commands
#define SD_CMD_GO_IDLE          0
#define SD_CMD_SEND_OP_COND     1
#define SD_CMD_ALL_SEND_CID     2
#define SD_CMD_SEND_REL_ADDR    3
#define SD_CMD_SELECT_CARD      7
#define SD_CMD_SEND_IF_COND     8
#define SD_CMD_SEND_CSD         9
#define SD_CMD_STOP_TRANSMISSION 12
#define SD_CMD_SET_BLOCKLEN     16
#define SD_CMD_READ_SINGLE      17
#define SD_CMD_READ_MULTIPLE    18
#define SD_CMD_APP_CMD          55
#define SD_ACMD_SD_SEND_OP_COND 41

// Sector size
#define SD_SECTOR_SIZE          512

// Global state
static int sd_initialized = 0;
static u32 sd_rca = 0;  // Relative Card Address

// Delay loop
static void sd_delay(int count) {
    for (volatile int i = 0; i < count; i++) {
        __asm__ volatile("nop");
    }
}

// Send command and wait for response
static int sd_send_cmd(u32 cmd, u32 arg, int response) {
    // Clear status flags
    *MMCI_CLEAR = 0x7FF;
    
    // Set argument
    *MMCI_ARGUMENT = arg;
    
    // Build command register value
    u32 cmd_reg = (cmd & 0x3F) | MMCI_CMD_ENABLE;
    if (response) {
        cmd_reg |= MMCI_CMD_RESPONSE;
    }
    
    // Send command
    *MMCI_COMMAND = cmd_reg;
    
    // Wait for command to complete
    u32 status;
    int timeout = 100000;
    while (timeout-- > 0) {
        status = *MMCI_STATUS;
        if (status & (MMCI_STAT_CMDRESPEND | MMCI_STAT_CMDSENT | 
                      MMCI_STAT_CMDTIMEOUT | MMCI_STAT_CMDCRCFAIL)) {
            break;
        }
    }
    
    if (timeout <= 0 || (status & MMCI_STAT_CMDTIMEOUT)) {
        return -1;  // Timeout
    }
    
    return 0;
}

// Initialize SD card
static int sd_init(void) {
    if (sd_initialized) return 0;
    
    // Power on the controller
    *MMCI_POWER = MMCI_POWER_UP;
    sd_delay(10000);
    *MMCI_POWER = MMCI_POWER_ON;
    sd_delay(10000);
    
    // Set clock (slow for init)
    *MMCI_CLOCK = 0x1FF;  // Enable clock, slow divider
    sd_delay(10000);
    
    // CMD0: Go idle
    sd_send_cmd(SD_CMD_GO_IDLE, 0, 0);
    sd_delay(10000);
    
    // CMD8: Send interface condition (for SD 2.0+)
    sd_send_cmd(SD_CMD_SEND_IF_COND, 0x1AA, 1);
    sd_delay(1000);
    
    // ACMD41: Send operating condition (with HCS bit for SDHC)
    int retries = 100;
    while (retries-- > 0) {
        sd_send_cmd(SD_CMD_APP_CMD, 0, 1);
        sd_send_cmd(SD_ACMD_SD_SEND_OP_COND, 0x40300000, 1);
        
        u32 resp = *MMCI_RESPONSE0;
        if (resp & 0x80000000) {
            // Card is ready
            break;
        }
        sd_delay(10000);
    }
    
    if (retries <= 0) {
        return -1;  // Card init failed
    }
    
    // CMD2: Get CID
    sd_send_cmd(SD_CMD_ALL_SEND_CID, 0, 1);
    sd_delay(1000);
    
    // CMD3: Get RCA
    sd_send_cmd(SD_CMD_SEND_REL_ADDR, 0, 1);
    sd_rca = (*MMCI_RESPONSE0 >> 16) & 0xFFFF;
    sd_delay(1000);
    
    // CMD7: Select card
    sd_send_cmd(SD_CMD_SELECT_CARD, sd_rca << 16, 1);
    sd_delay(1000);
    
    // CMD16: Set block length to 512
    sd_send_cmd(SD_CMD_SET_BLOCKLEN, SD_SECTOR_SIZE, 1);
    sd_delay(1000);
    
    // Speed up clock now that card is initialized
    *MMCI_CLOCK = 0x100;  // Faster clock
    
    sd_initialized = 1;
    return 0;
}

// Check if SD is initialized
static int sd_is_initialized(void) {
    return sd_initialized;
}

// Read sectors from SD card
// lba: Logical Block Address (sector number)
// count: Number of sectors to read
// buffer: Output buffer (must be at least count * 512 bytes)
// Returns 0 on success, -1 on error
static int sd_read_sectors(u32 lba, u32 count, void *buffer) {
    if (!sd_initialized) {
        if (sd_init() != 0) {
            return -1;
        }
    }
    
    u8 *buf = (u8 *)buffer;
    
    for (u32 sector = 0; sector < count; sector++) {
        u32 addr = (lba + sector) * SD_SECTOR_SIZE;  // Byte address for standard SD
        
        // Clear status
        *MMCI_CLEAR = 0x7FF;
        
        // Set up data transfer
        *MMCI_DATATIMER = 0xFFFFFF;
        *MMCI_DATALENGTH = SD_SECTOR_SIZE;
        *MMCI_DATACTRL = MMCI_DCTRL_ENABLE | MMCI_DCTRL_DIRECTION | 
                         MMCI_DCTRL_BLOCKSIZE(9);  // 2^9 = 512 bytes
        
        // CMD17: Read single block
        if (sd_send_cmd(SD_CMD_READ_SINGLE, addr, 1) != 0) {
            return -1;
        }
        
        // Read data from FIFO
        u32 *buf32 = (u32 *)(buf + sector * SD_SECTOR_SIZE);
        int words_read = 0;
        int timeout = 1000000;
        
        while (words_read < (SD_SECTOR_SIZE / 4) && timeout-- > 0) {
            u32 status = *MMCI_STATUS;
            
            if (status & (MMCI_STAT_DATACRCFAIL | MMCI_STAT_DATATIMEOUT | MMCI_STAT_RXOVERRUN)) {
                return -1;  // Data error
            }
            
            if (status & MMCI_STAT_RXDATAAVAIL) {
                buf32[words_read++] = *MMCI_FIFO;
            }
        }
        
        if (words_read < (SD_SECTOR_SIZE / 4)) {
            return -1;  // Incomplete read
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
    }
    
    return 0;
}

#endif
