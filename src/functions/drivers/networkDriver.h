#ifndef NETWORK_DRIVER_H
#define NETWORK_DRIVER_H

#include "../../package.h"

// SMC91C111 Ethernet Controller for VersatilePB
// Base address for the network controller
#define ETH_BASE        0x10010000

// SMC91C111 Register offsets
#define ETH_TCR         ((volatile unsigned short*)(ETH_BASE + 0x00))  // Transmit Control
#define ETH_EPH_STATUS  ((volatile unsigned short*)(ETH_BASE + 0x02))  // EPH Status
#define ETH_RCR         ((volatile unsigned short*)(ETH_BASE + 0x04))  // Receive Control
#define ETH_COUNTER     ((volatile unsigned short*)(ETH_BASE + 0x06))  // Counter
#define ETH_MIR         ((volatile unsigned short*)(ETH_BASE + 0x08))  // Memory Info
#define ETH_RPCR        ((volatile unsigned short*)(ETH_BASE + 0x0A))  // Receive/PHY Control
#define ETH_BANK_SEL    ((volatile unsigned short*)(ETH_BASE + 0x0E))  // Bank Select

// Bank 1 registers
#define ETH_CONFIG      ((volatile unsigned short*)(ETH_BASE + 0x00))  // Configuration
#define ETH_BASE_ADDR   ((volatile unsigned short*)(ETH_BASE + 0x02))  // Base Address
#define ETH_IA0_1       ((volatile unsigned short*)(ETH_BASE + 0x04))  // Individual Address 0-1
#define ETH_IA2_3       ((volatile unsigned short*)(ETH_BASE + 0x06))  // Individual Address 2-3
#define ETH_IA4_5       ((volatile unsigned short*)(ETH_BASE + 0x08))  // Individual Address 4-5
#define ETH_GP_REG      ((volatile unsigned short*)(ETH_BASE + 0x0A))  // General Purpose
#define ETH_CONTROL     ((volatile unsigned short*)(ETH_BASE + 0x0C))  // Control

// Bank 2 registers
#define ETH_MMU_CMD     ((volatile unsigned short*)(ETH_BASE + 0x00))  // MMU Command
#define ETH_PNR         ((volatile unsigned short*)(ETH_BASE + 0x02))  // Packet Number
#define ETH_FIFO        ((volatile unsigned short*)(ETH_BASE + 0x04))  // FIFO Ports
#define ETH_POINTER     ((volatile unsigned short*)(ETH_BASE + 0x06))  // Pointer
#define ETH_DATA        ((volatile unsigned short*)(ETH_BASE + 0x08))  // Data
#define ETH_INTERRUPT   ((volatile unsigned short*)(ETH_BASE + 0x0C))  // Interrupt Status

// Control bits
#define TCR_ENABLE      0x0001  // Enable transmitter
#define RCR_ENABLE      0x0100  // Enable receiver
#define RCR_STRIP_CRC   0x0200

// MAC address (02:xx:xx:xx:xx:xx is locally administered)
static unsigned char mac_address[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

// IP address (default: 10.0.2.15 - QEMU user networking default)
static unsigned char ip_address[4] = {10, 0, 2, 15};
static unsigned char subnet_mask[4] = {255, 255, 255, 0};
static unsigned char gateway[4] = {10, 0, 2, 2};

// Set IP address
static void net_set_ip(unsigned char a, unsigned char b, unsigned char c, unsigned char d) {
    ip_address[0] = a;
    ip_address[1] = b;
    ip_address[2] = c;
    ip_address[3] = d;
}

// Get IP address as string
static void net_get_ip_string(char *buf) {
    int pos = 0;
    for (int i = 0; i < 4; i++) {
        unsigned char val = ip_address[i];
        if (val >= 100) {
            buf[pos++] = '0' + (val / 100);
            val %= 100;
            buf[pos++] = '0' + (val / 10);
            buf[pos++] = '0' + (val % 10);
        } else if (val >= 10) {
            buf[pos++] = '0' + (val / 10);
            buf[pos++] = '0' + (val % 10);
        } else {
            buf[pos++] = '0' + val;
        }
        if (i < 3) buf[pos++] = '.';
    }
    buf[pos] = '\0';
}

// Select bank
static inline void eth_select_bank(int bank) {
    *ETH_BANK_SEL = (unsigned short)(bank & 0x03);
}

// Initialize the network controller
static int net_init(void) {
    // Select bank 0
    eth_select_bank(0);
    
    // Reset transmit and receive
    *ETH_RCR = 0x8000;  // Soft reset
    *ETH_RCR = 0x0000;  // Clear reset
    
    // Small delay
    for (volatile int i = 0; i < 10000; i++);
    
    // Select bank 1 to set MAC address
    eth_select_bank(1);
    *ETH_IA0_1 = mac_address[0] | (mac_address[1] << 8);
    *ETH_IA2_3 = mac_address[2] | (mac_address[3] << 8);
    *ETH_IA4_5 = mac_address[4] | (mac_address[5] << 8);
    
    // Enable auto-release of transmitted packets
    *ETH_CONTROL = 0x0800;
    
    // Select bank 0, enable TX and RX
    eth_select_bank(0);
    *ETH_TCR = TCR_ENABLE;
    *ETH_RCR = RCR_ENABLE | RCR_STRIP_CRC;
    
    return 0;  // Success
}

// Check if network is available
static int net_link_up(void) {
    eth_select_bank(0);
    unsigned short status = *ETH_EPH_STATUS;
    return (status & 0x4000) ? 1 : 0;  // Link OK bit
}

// Get MAC address as string
static void net_get_mac_string(char *buf) {
    const char hex[] = "0123456789ABCDEF";
    int pos = 0;
    for (int i = 0; i < 6; i++) {
        buf[pos++] = hex[(mac_address[i] >> 4) & 0x0F];
        buf[pos++] = hex[mac_address[i] & 0x0F];
        if (i < 5) buf[pos++] = ':';
    }
    buf[pos] = '\0';
}

struct eth_header {
    unsigned char dst[6];
    unsigned char src[6];
    unsigned short ethertype;
};

static int net_receive(unsigned char *buffer, int max_len) {
    eth_select_bank(2);

    // Check if there is a packet
    unsigned short status = *ETH_FIFO;
    if (status & 0x8000) {
        // This means FIFO is empty
        return 0;
    }

    // Select packet
    unsigned short packet_number = *ETH_PNR;

    // Read packet length
    *ETH_POINTER = 0x4000; // auto increment, read
    unsigned short length = *ETH_DATA;

    // Read packet contents
    for (int i = 0; i < length && i < max_len; i += 2) {
        unsigned short word = *ETH_DATA;
        buffer[i] = word & 0xFF;
        buffer[i+1] = (word >> 8) & 0xFF;
    }

    // Release packet
    *ETH_MMU_CMD = 0x0080; // release

    return length;
}

// Send a raw ethernet packet
// dst: destination MAC address (6 bytes)
// ethertype: protocol type (e.g., 0x0800 for IPv4, 0x0806 for ARP)
// data: payload data
// len: length of payload
static int net_send(unsigned char *dst, unsigned short ethertype, unsigned char *data, int len) {
    eth_select_bank(2);
    
    // Allocate memory for TX
    *ETH_MMU_CMD = 0x0020;  // Allocate TX memory
    
    // Wait for allocation (with timeout)
    int timeout = 10000;
    while (timeout-- > 0) {
        eth_select_bank(2);
        if (*ETH_INTERRUPT & 0x0008) {  // ALLOC_INT
            break;
        }
    }
    if (timeout <= 0) {
        return -1;  // Allocation failed
    }
    
    // Get allocated packet number
    eth_select_bank(2);
    unsigned char pkt_num = (*ETH_PNR >> 8) & 0x3F;
    
    // Set packet number for writing
    *ETH_PNR = pkt_num;
    
    // Set pointer to start of packet data area, auto-increment, write
    *ETH_POINTER = 0x4000;  // Auto-increment + write
    
    // Write status word (will be filled by hardware)
    *ETH_DATA = 0x0000;
    
    // Write packet length (header + data + control byte)
    int total_len = 14 + len;  // 14 = ethernet header
    *ETH_DATA = (unsigned short)(total_len + 6);  // +6 for status/length/control
    
    // Write destination MAC
    *ETH_DATA = dst[0] | (dst[1] << 8);
    *ETH_DATA = dst[2] | (dst[3] << 8);
    *ETH_DATA = dst[4] | (dst[5] << 8);
    
    // Write source MAC (our MAC)
    *ETH_DATA = mac_address[0] | (mac_address[1] << 8);
    *ETH_DATA = mac_address[2] | (mac_address[3] << 8);
    *ETH_DATA = mac_address[4] | (mac_address[5] << 8);
    
    // Write ethertype (big endian on wire)
    *ETH_DATA = ((ethertype >> 8) & 0xFF) | ((ethertype & 0xFF) << 8);
    
    // Write payload data
    for (int i = 0; i < len; i += 2) {
        unsigned short word = data[i];
        if (i + 1 < len) {
            word |= (data[i + 1] << 8);
        }
        *ETH_DATA = word;
    }
    
    // Write control byte (odd/even indicator)
    if (total_len & 1) {
        *ETH_DATA = 0x2000;  // Odd length, control byte
    } else {
        *ETH_DATA = 0x0000;
    }
    
    // Enqueue packet for transmission
    *ETH_MMU_CMD = 0x00C0;  // Enqueue TX
    
    return len;
}

// Send a broadcast packet (to all devices on LAN)
static int net_send_broadcast(unsigned short ethertype, unsigned char *data, int len) {
    unsigned char broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    return net_send(broadcast, ethertype, data, len);
}

// Check if there's a packet waiting to be received
static int net_has_packet(void) {
    eth_select_bank(2);
    unsigned short fifo = *ETH_FIFO;
    return !(fifo & 0x8000);  // Bit 15 = RX FIFO empty
}

// Print network info
static void net_info(void) {
    char mac_str[18];
    char ip_str[16];
    net_get_mac_string(mac_str);
    net_get_ip_string(ip_str);
    
    writeOut("Network Driver: SMC91C111\n");
    writeOut("MAC Address:  ");
    writeOut(mac_str);
    writeOut("\n");
    writeOut("IP Address:   ");
    writeOut(ip_str);
    writeOut("\n");
    writeOut("Link Status:  ");
    writeOut(net_link_up() ? "UP" : "DOWN");
    writeOut("\n");
}

// ============== ARP Protocol ==============

#define ETH_TYPE_ARP    0x0806
#define ETH_TYPE_IP     0x0800

#define ARP_REQUEST     1
#define ARP_REPLY       2

// ARP packet structure
struct arp_packet {
    unsigned short hw_type;      // Hardware type (1 = Ethernet)
    unsigned short proto_type;   // Protocol type (0x0800 = IPv4)
    unsigned char hw_len;        // Hardware address length (6)
    unsigned char proto_len;     // Protocol address length (4)
    unsigned short opcode;       // Operation (1=request, 2=reply)
    unsigned char sender_mac[6];
    unsigned char sender_ip[4];
    unsigned char target_mac[6];
    unsigned char target_ip[4];
};

// Swap bytes (network byte order)
static unsigned short htons(unsigned short x) {
    return ((x & 0xFF) << 8) | ((x >> 8) & 0xFF);
}

// Compare IP addresses
static int ip_match(unsigned char *a, unsigned char *b) {
    return (a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3]);
}

// Copy bytes
static void mem_copy(unsigned char *dst, unsigned char *src, int len) {
    for (int i = 0; i < len; i++) dst[i] = src[i];
}

// Handle incoming ARP packet
static void handle_arp(unsigned char *packet, int len) {
    if (len < 14 + 28) return;  // Ethernet header + ARP packet
    
    struct arp_packet *arp = (struct arp_packet *)(packet + 14);
    
    // Check if it's an ARP request for our IP
    if (htons(arp->opcode) == ARP_REQUEST && ip_match(arp->target_ip, ip_address)) {
        // Build ARP reply
        unsigned char reply[42];  // 14 (eth) + 28 (arp)
        
        // Ethernet header - send to requester
        mem_copy(reply, arp->sender_mac, 6);       // Destination MAC
        mem_copy(reply + 6, mac_address, 6);       // Source MAC (us)
        reply[12] = 0x08; reply[13] = 0x06;        // EtherType = ARP
        
        // ARP reply
        struct arp_packet *reply_arp = (struct arp_packet *)(reply + 14);
        reply_arp->hw_type = htons(1);             // Ethernet
        reply_arp->proto_type = htons(0x0800);     // IPv4
        reply_arp->hw_len = 6;
        reply_arp->proto_len = 4;
        reply_arp->opcode = htons(ARP_REPLY);
        mem_copy(reply_arp->sender_mac, mac_address, 6);
        mem_copy(reply_arp->sender_ip, ip_address, 4);
        mem_copy(reply_arp->target_mac, arp->sender_mac, 6);
        mem_copy(reply_arp->target_ip, arp->sender_ip, 4);
        
        // Send reply
        net_send(arp->sender_mac, ETH_TYPE_ARP, reply + 14, 28);
    }
}

// ============== ICMP Protocol (Ping) ==============

#define ICMP_ECHO_REQUEST   8
#define ICMP_ECHO_REPLY     0

// IP header structure
struct ip_header {
    unsigned char version_ihl;    // Version (4 bits) + IHL (4 bits)
    unsigned char tos;            // Type of service
    unsigned short total_len;     // Total length
    unsigned short id;            // Identification
    unsigned short flags_frag;    // Flags + Fragment offset
    unsigned char ttl;            // Time to live
    unsigned char protocol;       // Protocol (1 = ICMP)
    unsigned short checksum;      // Header checksum
    unsigned char src_ip[4];      // Source IP
    unsigned char dst_ip[4];      // Destination IP
};

// ICMP header structure
struct icmp_header {
    unsigned char type;
    unsigned char code;
    unsigned short checksum;
    unsigned short id;
    unsigned short seq;
};

// Calculate checksum
static unsigned short checksum(unsigned char *data, int len) {
    unsigned long sum = 0;
    
    while (len > 1) {
        sum += (data[0] << 8) | data[1];
        data += 2;
        len -= 2;
    }
    if (len == 1) {
        sum += data[0] << 8;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return (unsigned short)(~sum);
}

// Handle incoming ICMP packet (ping)
static void handle_icmp(unsigned char *packet, int len) {
    struct ip_header *ip = (struct ip_header *)(packet + 14);
    int ip_hdr_len = (ip->version_ihl & 0x0F) * 4;
    struct icmp_header *icmp = (struct icmp_header *)(packet + 14 + ip_hdr_len);
    
    // Check if it's an echo request (ping) for us
    if (icmp->type == ICMP_ECHO_REQUEST && ip_match(ip->dst_ip, ip_address)) {
        int icmp_len = htons(ip->total_len) - ip_hdr_len;
        
        // Build reply packet
        unsigned char reply[1500];
        int reply_len = 14 + 20 + icmp_len;  // Eth + IP + ICMP
        
        // Ethernet header
        mem_copy(reply, packet + 6, 6);        // Dst = original src MAC
        mem_copy(reply + 6, mac_address, 6);   // Src = our MAC
        reply[12] = 0x08; reply[13] = 0x00;    // EtherType = IPv4
        
        // IP header
        struct ip_header *reply_ip = (struct ip_header *)(reply + 14);
        reply_ip->version_ihl = 0x45;          // IPv4, 20 byte header
        reply_ip->tos = 0;
        reply_ip->total_len = htons(20 + icmp_len);
        reply_ip->id = ip->id;
        reply_ip->flags_frag = 0;
        reply_ip->ttl = 64;
        reply_ip->protocol = 1;                // ICMP
        reply_ip->checksum = 0;
        mem_copy(reply_ip->src_ip, ip_address, 4);
        mem_copy(reply_ip->dst_ip, ip->src_ip, 4);
        
        // Calculate IP checksum
        reply_ip->checksum = htons(checksum((unsigned char *)reply_ip, 20));
        
        // ICMP reply (copy original, change type)
        struct icmp_header *reply_icmp = (struct icmp_header *)(reply + 14 + 20);
        mem_copy((unsigned char *)reply_icmp, (unsigned char *)icmp, icmp_len);
        reply_icmp->type = ICMP_ECHO_REPLY;
        reply_icmp->checksum = 0;
        reply_icmp->checksum = htons(checksum((unsigned char *)reply_icmp, icmp_len));
        
        // Send the raw packet (bypass net_send since we built full packet)
        net_send(reply, ETH_TYPE_IP, reply + 14, 20 + icmp_len);
    }
}

// ============== Main Packet Handler ==============

// Process incoming packets - call this in a loop
static void net_poll(void) {
    unsigned char packet[1500];
    int len;
    
    while ((len = net_receive(packet, sizeof(packet))) > 0) {
        if (len < 14) continue;  // Too small for ethernet header
        
        // Get ethertype
        unsigned short ethertype = (packet[12] << 8) | packet[13];
        
        if (ethertype == ETH_TYPE_ARP) {
            handle_arp(packet, len);
        }
        else if (ethertype == ETH_TYPE_IP) {
            struct ip_header *ip = (struct ip_header *)(packet + 14);
            if (ip->protocol == 1) {  // ICMP
                handle_icmp(packet, len);
            }
        }
    }
}

#endif