#ifndef NETWORK_DRIVER_H
#define NETWORK_DRIVER_H

#include "../../package.h"

// Software division helpers for ARM (no hardware divider on ARM926EJ-S)
static inline unsigned int udiv10(unsigned int n) {
    // Fast divide by 10 using multiplication by reciprocal
    // 0xCCCCCCCD is approximately 2^35 / 10
    unsigned long long x = (unsigned long long)n * 0xCCCCCCCDULL;
    return (unsigned int)(x >> 35);
}

static inline unsigned int umod10(unsigned int n) {
    return n - udiv10(n) * 10;
}

static inline unsigned int udiv100(unsigned int n) {
    return udiv10(udiv10(n));
}

static inline unsigned int umod100(unsigned int n) {
    return n - udiv100(n) * 100;
}

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
static unsigned char dns_server[4] = {10, 0, 2, 3};  // QEMU default DNS

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
            buf[pos++] = '0' + udiv100(val);
            val = umod100(val);
            buf[pos++] = '0' + udiv10(val);
            buf[pos++] = '0' + umod10(val);
        } else if (val >= 10) {
            buf[pos++] = '0' + udiv10(val);
            buf[pos++] = '0' + umod10(val);
        } else {
            buf[pos++] = '0' + val;
        }
        if (i < 3) buf[pos++] = '.';
    }
    buf[pos] = '\0';
}

// Set gateway/router address
static void net_set_gateway(unsigned char a, unsigned char b, unsigned char c, unsigned char d) {
    gateway[0] = a;
    gateway[1] = b;
    gateway[2] = c;
    gateway[3] = d;
}

// Get gateway address as string
static void net_get_gateway_string(char *buf) {
    int pos = 0;
    for (int i = 0; i < 4; i++) {
        unsigned char val = gateway[i];
        if (val >= 100) {
            buf[pos++] = '0' + udiv100(val);
            val = umod100(val);
            buf[pos++] = '0' + udiv10(val);
            buf[pos++] = '0' + umod10(val);
        } else if (val >= 10) {
            buf[pos++] = '0' + udiv10(val);
            buf[pos++] = '0' + umod10(val);
        } else {
            buf[pos++] = '0' + val;
        }
        if (i < 3) buf[pos++] = '.';
    }
    buf[pos] = '\0';
}

// Set subnet mask
static void net_set_subnet(unsigned char a, unsigned char b, unsigned char c, unsigned char d) {
    subnet_mask[0] = a;
    subnet_mask[1] = b;
    subnet_mask[2] = c;
    subnet_mask[3] = d;
}

// Get subnet mask as string
static void net_get_subnet_string(char *buf) {
    int pos = 0;
    for (int i = 0; i < 4; i++) {
        unsigned char val = subnet_mask[i];
        if (val >= 100) {
            buf[pos++] = '0' + udiv100(val);
            val = umod100(val);
            buf[pos++] = '0' + udiv10(val);
            buf[pos++] = '0' + umod10(val);
        } else if (val >= 10) {
            buf[pos++] = '0' + udiv10(val);
            buf[pos++] = '0' + umod10(val);
        } else {
            buf[pos++] = '0' + val;
        }
        if (i < 3) buf[pos++] = '.';
    }
    buf[pos] = '\0';
}

// Get CIDR prefix length from subnet mask
static int net_get_cidr(void) {
    int bits = 0;
    for (int i = 0; i < 4; i++) {
        unsigned char val = subnet_mask[i];
        while (val & 0x80) {
            bits++;
            val <<= 1;
        }
    }
    return bits;
}

// Set DNS server
static void net_set_dns(unsigned char a, unsigned char b, unsigned char c, unsigned char d) {
    dns_server[0] = a;
    dns_server[1] = b;
    dns_server[2] = c;
    dns_server[3] = d;
}

// Get DNS server as string
static void net_get_dns_string(char *buf) {
    int pos = 0;
    for (int i = 0; i < 4; i++) {
        unsigned char val = dns_server[i];
        if (val >= 100) {
            buf[pos++] = '0' + udiv100(val);
            val = umod100(val);
            buf[pos++] = '0' + udiv10(val);
            buf[pos++] = '0' + umod10(val);
        } else if (val >= 10) {
            buf[pos++] = '0' + udiv10(val);
            buf[pos++] = '0' + umod10(val);
        } else {
            buf[pos++] = '0' + val;
        }
        if (i < 3) buf[pos++] = '.';
    }
    buf[pos] = '\0';
}

// Format any IP address to string buffer
static void format_ip(unsigned char *ip, char *buf) {
    int pos = 0;
    for (int i = 0; i < 4; i++) {
        unsigned char val = ip[i];
        if (val >= 100) {
            buf[pos++] = '0' + udiv100(val);
            val = umod100(val);
            buf[pos++] = '0' + udiv10(val);
            buf[pos++] = '0' + umod10(val);
        } else if (val >= 10) {
            buf[pos++] = '0' + udiv10(val);
            buf[pos++] = '0' + umod10(val);
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
    char gw_str[16];
    char subnet_str[16];
    char dns_str[16];
    net_get_mac_string(mac_str);
    net_get_ip_string(ip_str);
    net_get_gateway_string(gw_str);
    net_get_subnet_string(subnet_str);
    net_get_dns_string(dns_str);

    writeOut("Network Driver: SMC91C111\n");
    writeOut("MAC Address:  ");
    writeOut(mac_str);
    writeOut("\n");
    writeOut("IP Address:   ");
    writeOut(ip_str);
    writeOut("/");
    writeOutNum(net_get_cidr());
    writeOut("\n");
    writeOut("Subnet Mask:  ");
    writeOut(subnet_str);
    writeOut("\n");
    writeOut("Gateway:      ");
    writeOut(gw_str);
    writeOut("\n");
    writeOut("DNS Server:   ");
    writeOut(dns_str);
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

// Ping state
static volatile int ping_reply_received = 0;
static unsigned short ping_seq = 0;

// Send ICMP echo request (ping)
// Returns: 0 on success sending, -1 on failure
static int net_send_ping(unsigned char *dst_ip) {
    unsigned char packet[74];  // 14 eth + 20 ip + 8 icmp + 32 data
    int icmp_data_len = 32;
    int icmp_len = 8 + icmp_data_len;
    int ip_len = 20 + icmp_len;

    // We need ARP to get the MAC - for simplicity, use gateway MAC (broadcast for now)
    // In QEMU user networking, the gateway handles routing
    unsigned char dst_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    // Ethernet header
    mem_copy(packet, dst_mac, 6);              // Destination MAC
    mem_copy(packet + 6, mac_address, 6);      // Source MAC
    packet[12] = 0x08; packet[13] = 0x00;      // EtherType = IPv4

    // IP header
    struct ip_header *ip = (struct ip_header *)(packet + 14);
    ip->version_ihl = 0x45;                    // IPv4, 20 byte header
    ip->tos = 0;
    ip->total_len = htons(ip_len);
    ip->id = htons(ping_seq);
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = 1;                          // ICMP
    ip->checksum = 0;
    mem_copy(ip->src_ip, ip_address, 4);
    mem_copy(ip->dst_ip, dst_ip, 4);
    ip->checksum = htons(checksum((unsigned char *)ip, 20));

    // ICMP echo request
    struct icmp_header *icmp = (struct icmp_header *)(packet + 34);
    icmp->type = ICMP_ECHO_REQUEST;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->id = htons(0x1234);
    icmp->seq = htons(ping_seq++);

    // Fill data with pattern
    unsigned char *data = packet + 42;
    for (int i = 0; i < icmp_data_len; i++) {
        data[i] = (unsigned char)(i & 0xFF);
    }

    // Calculate ICMP checksum
    icmp->checksum = htons(checksum((unsigned char *)icmp, icmp_len));

    // Reset reply flag
    ping_reply_received = 0;

    // Send packet
    return net_send(dst_mac, ETH_TYPE_IP, packet + 14, ip_len);
}

// Wait for ping reply with timeout
// Returns: 1 if reply received, 0 if timeout
static int net_wait_ping_reply(int timeout_ms) {
    // Simple timeout loop (approximate, no real timer)
    for (int i = 0; i < timeout_ms * 100; i++) {
        unsigned char packet[1500];
        int len = net_receive(packet, sizeof(packet));

        if (len > 0) {
            unsigned short ethertype = (packet[12] << 8) | packet[13];

            if (ethertype == ETH_TYPE_ARP) {
                handle_arp(packet, len);
            }
            else if (ethertype == ETH_TYPE_IP) {
                struct ip_header *ip = (struct ip_header *)(packet + 14);
                if (ip->protocol == 1) {  // ICMP
                    int ip_hdr_len = (ip->version_ihl & 0x0F) * 4;
                    struct icmp_header *icmp = (struct icmp_header *)(packet + 14 + ip_hdr_len);

                    if (icmp->type == ICMP_ECHO_REPLY) {
                        return 1;  // Got reply!
                    }
                    else if (icmp->type == ICMP_ECHO_REQUEST) {
                        handle_icmp(packet, len);  // Reply to incoming pings
                    }
                }
            }
        }
    }
    return 0;  // Timeout
}

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

// ============== DHCP Protocol ==============

#define DHCP_SERVER_PORT    67
#define DHCP_CLIENT_PORT    68

#define DHCP_DISCOVER       1
#define DHCP_OFFER          2
#define DHCP_REQUEST        3
#define DHCP_DECLINE        4
#define DHCP_ACK            5
#define DHCP_NAK            6
#define DHCP_RELEASE        7

// DHCP magic cookie
#define DHCP_MAGIC_COOKIE   0x63825363

// DHCP packet structure (simplified)
struct dhcp_packet {
    unsigned char op;           // 1 = request, 2 = reply
    unsigned char htype;        // Hardware type (1 = Ethernet)
    unsigned char hlen;         // Hardware address length (6)
    unsigned char hops;         // Hops
    unsigned int xid;           // Transaction ID
    unsigned short secs;        // Seconds elapsed
    unsigned short flags;       // Flags (0x8000 = broadcast)
    unsigned char ciaddr[4];    // Client IP address
    unsigned char yiaddr[4];    // Your IP address
    unsigned char siaddr[4];    // Server IP address
    unsigned char giaddr[4];    // Gateway IP address
    unsigned char chaddr[16];   // Client hardware address
    unsigned char sname[64];    // Server name
    unsigned char file[128];    // Boot filename
    unsigned char options[312]; // Options (variable)
};

// UDP header structure
struct udp_header {
    unsigned short src_port;
    unsigned short dst_port;
    unsigned short length;
    unsigned short checksum;
};

static unsigned int dhcp_xid = 0x12345678;
static unsigned char dhcp_server_ip[4] = {0, 0, 0, 0};
static unsigned char offered_ip[4] = {0, 0, 0, 0};

// Build and send DHCP Discover
static int dhcp_send_discover(void) {
    unsigned char packet[590];  // Eth(14) + IP(20) + UDP(8) + DHCP(548)
    unsigned char broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    unsigned char broadcast_ip[4] = {255, 255, 255, 255};
    unsigned char zero_ip[4] = {0, 0, 0, 0};

    // Clear packet
    for (int i = 0; i < 590; i++) packet[i] = 0;

    // Ethernet header
    mem_copy(packet, broadcast_mac, 6);
    mem_copy(packet + 6, mac_address, 6);
    packet[12] = 0x08; packet[13] = 0x00;  // IPv4

    // IP header
    struct ip_header *ip = (struct ip_header *)(packet + 14);
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_len = htons(20 + 8 + 548);  // IP + UDP + DHCP
    ip->id = htons(1);
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = 17;  // UDP
    ip->checksum = 0;
    mem_copy(ip->src_ip, zero_ip, 4);
    mem_copy(ip->dst_ip, broadcast_ip, 4);
    ip->checksum = htons(checksum((unsigned char *)ip, 20));

    // UDP header
    struct udp_header *udp = (struct udp_header *)(packet + 34);
    udp->src_port = htons(DHCP_CLIENT_PORT);
    udp->dst_port = htons(DHCP_SERVER_PORT);
    udp->length = htons(8 + 548);
    udp->checksum = 0;  // Optional for IPv4

    // DHCP packet
    struct dhcp_packet *dhcp = (struct dhcp_packet *)(packet + 42);
    dhcp->op = 1;        // Boot request
    dhcp->htype = 1;     // Ethernet
    dhcp->hlen = 6;      // MAC = 6 bytes
    dhcp->hops = 0;
    dhcp->xid = dhcp_xid;
    dhcp->secs = 0;
    dhcp->flags = htons(0x8000);  // Broadcast
    mem_copy(dhcp->chaddr, mac_address, 6);

    // DHCP options
    unsigned char *opt = dhcp->options;
    // Magic cookie
    opt[0] = 0x63; opt[1] = 0x82; opt[2] = 0x53; opt[3] = 0x63;
    opt += 4;

    // Option 53: DHCP Message Type = Discover
    *opt++ = 53; *opt++ = 1; *opt++ = DHCP_DISCOVER;

    // Option 55: Parameter Request List
    *opt++ = 55; *opt++ = 3;
    *opt++ = 1;   // Subnet Mask
    *opt++ = 3;   // Router
    *opt++ = 6;   // DNS

    // End option
    *opt++ = 255;

    // Send packet
    return net_send(broadcast_mac, ETH_TYPE_IP, packet + 14, 20 + 8 + 548);
}

// Build and send DHCP Request
static int dhcp_send_request(void) {
    unsigned char packet[590];
    unsigned char broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    unsigned char broadcast_ip[4] = {255, 255, 255, 255};
    unsigned char zero_ip[4] = {0, 0, 0, 0};

    // Clear packet
    for (int i = 0; i < 590; i++) packet[i] = 0;

    // Ethernet header
    mem_copy(packet, broadcast_mac, 6);
    mem_copy(packet + 6, mac_address, 6);
    packet[12] = 0x08; packet[13] = 0x00;

    // IP header
    struct ip_header *ip = (struct ip_header *)(packet + 14);
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_len = htons(20 + 8 + 548);
    ip->id = htons(2);
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = 17;
    ip->checksum = 0;
    mem_copy(ip->src_ip, zero_ip, 4);
    mem_copy(ip->dst_ip, broadcast_ip, 4);
    ip->checksum = htons(checksum((unsigned char *)ip, 20));

    // UDP header
    struct udp_header *udp = (struct udp_header *)(packet + 34);
    udp->src_port = htons(DHCP_CLIENT_PORT);
    udp->dst_port = htons(DHCP_SERVER_PORT);
    udp->length = htons(8 + 548);
    udp->checksum = 0;

    // DHCP packet
    struct dhcp_packet *dhcp = (struct dhcp_packet *)(packet + 42);
    dhcp->op = 1;
    dhcp->htype = 1;
    dhcp->hlen = 6;
    dhcp->hops = 0;
    dhcp->xid = dhcp_xid;
    dhcp->secs = 0;
    dhcp->flags = htons(0x8000);
    mem_copy(dhcp->chaddr, mac_address, 6);

    // DHCP options
    unsigned char *opt = dhcp->options;
    // Magic cookie
    opt[0] = 0x63; opt[1] = 0x82; opt[2] = 0x53; opt[3] = 0x63;
    opt += 4;

    // Option 53: DHCP Message Type = Request
    *opt++ = 53; *opt++ = 1; *opt++ = DHCP_REQUEST;

    // Option 50: Requested IP Address
    *opt++ = 50; *opt++ = 4;
    *opt++ = offered_ip[0]; *opt++ = offered_ip[1];
    *opt++ = offered_ip[2]; *opt++ = offered_ip[3];

    // Option 54: Server Identifier
    *opt++ = 54; *opt++ = 4;
    *opt++ = dhcp_server_ip[0]; *opt++ = dhcp_server_ip[1];
    *opt++ = dhcp_server_ip[2]; *opt++ = dhcp_server_ip[3];

    // Option 55: Parameter Request List
    *opt++ = 55; *opt++ = 3;
    *opt++ = 1; *opt++ = 3; *opt++ = 6;

    // End option
    *opt++ = 255;

    return net_send(broadcast_mac, ETH_TYPE_IP, packet + 14, 20 + 8 + 548);
}

// Parse DHCP options and return message type
static int dhcp_parse_options(unsigned char *options, int len) {
    int msg_type = 0;
    int i = 4;  // Skip magic cookie

    while (i < len && options[i] != 255) {
        unsigned char opt = options[i++];
        if (opt == 0) continue;  // Padding

        unsigned char opt_len = options[i++];

        switch (opt) {
            case 53:  // DHCP Message Type
                msg_type = options[i];
                break;
            case 1:   // Subnet Mask
                if (opt_len == 4) {
                    mem_copy(subnet_mask, &options[i], 4);
                }
                break;
            case 3:   // Router
                if (opt_len >= 4) {
                    mem_copy(gateway, &options[i], 4);
                }
                break;
            case 54:  // Server Identifier
                if (opt_len == 4) {
                    mem_copy(dhcp_server_ip, &options[i], 4);
                }
                break;
        }
        i += opt_len;
    }
    return msg_type;
}

// Handle incoming DHCP packet
static int handle_dhcp(unsigned char *packet, int len) {
    struct ip_header *ip = (struct ip_header *)(packet + 14);
    int ip_hdr_len = (ip->version_ihl & 0x0F) * 4;
    struct udp_header *udp = (struct udp_header *)(packet + 14 + ip_hdr_len);

    // Check if it's a DHCP response (from server port 67 to client port 68)
    if (htons(udp->src_port) != DHCP_SERVER_PORT) return 0;
    if (htons(udp->dst_port) != DHCP_CLIENT_PORT) return 0;

    struct dhcp_packet *dhcp = (struct dhcp_packet *)(packet + 14 + ip_hdr_len + 8);

    // Check transaction ID
    if (dhcp->xid != dhcp_xid) return 0;

    // Check it's a reply
    if (dhcp->op != 2) return 0;

    // Parse options
    int msg_type = dhcp_parse_options(dhcp->options, 312);

    if (msg_type == DHCP_OFFER) {
        // Save offered IP
        mem_copy(offered_ip, dhcp->yiaddr, 4);
        return DHCP_OFFER;
    }
    else if (msg_type == DHCP_ACK) {
        // Apply the IP configuration
        mem_copy(ip_address, dhcp->yiaddr, 4);
        return DHCP_ACK;
    }
    else if (msg_type == DHCP_NAK) {
        return DHCP_NAK;
    }

    return 0;
}

// Main DHCP client function - returns 1 on success, 0 on failure
static int dhcp_request(void) {
    unsigned char packet[1500];
    int len;
    int state = 0;  // 0 = send discover, 1 = wait offer, 2 = send request, 3 = wait ack
    int timeout;
    int retries = 3;

    // Generate random-ish transaction ID based on MAC
    dhcp_xid = (mac_address[2] << 24) | (mac_address[3] << 16) |
               (mac_address[4] << 8) | mac_address[5];
    dhcp_xid ^= 0xDEADBEEF;

    while (retries > 0) {
        if (state == 0) {
            // Send DHCP Discover
            writeOut("Sending DHCP Discover...\n");
            dhcp_send_discover();
            state = 1;
            timeout = 300000;
        }

        if (state == 1) {
            // Wait for DHCP Offer
            while (timeout-- > 0) {
                len = net_receive(packet, sizeof(packet));
                if (len > 0) {
                    unsigned short ethertype = (packet[12] << 8) | packet[13];
                    if (ethertype == ETH_TYPE_IP) {
                        struct ip_header *ip = (struct ip_header *)(packet + 14);
                        if (ip->protocol == 17) {  // UDP
                            int result = handle_dhcp(packet, len);
                            if (result == DHCP_OFFER) {
                                char ip_str[16];
                                format_ip(offered_ip, ip_str);
                                writeOut("Received offer: ");
                                writeOut(ip_str);
                                writeOut("\n");
                                state = 2;
                                break;
                            }
                        }
                    }
                    // Handle ARP during DHCP
                    if (ethertype == ETH_TYPE_ARP) {
                        handle_arp(packet, len);
                    }
                }
            }
            if (state == 1) {
                writeOut("Timeout waiting for offer\n");
                retries--;
                state = 0;
                continue;
            }
        }

        if (state == 2) {
            // Send DHCP Request
            writeOut("Sending DHCP Request...\n");
            dhcp_send_request();
            state = 3;
            timeout = 300000;
        }

        if (state == 3) {
            // Wait for DHCP ACK
            while (timeout-- > 0) {
                len = net_receive(packet, sizeof(packet));
                if (len > 0) {
                    unsigned short ethertype = (packet[12] << 8) | packet[13];
                    if (ethertype == ETH_TYPE_IP) {
                        struct ip_header *ip = (struct ip_header *)(packet + 14);
                        if (ip->protocol == 17) {
                            int result = handle_dhcp(packet, len);
                            if (result == DHCP_ACK) {
                                writeOut("DHCP ACK received!\n");
                                writeOut("IP configured: ");
                                char ip_str[16];
                                net_get_ip_string(ip_str);
                                writeOut(ip_str);
                                writeOut("\n");
                                writeOut("Gateway: ");
                                char gw_str[16];
                                net_get_gateway_string(gw_str);
                                writeOut(gw_str);
                                writeOut("\n");
                                return 1;  // Success!
                            }
                            else if (result == DHCP_NAK) {
                                writeOut("DHCP NAK received, retrying...\n");
                                state = 0;
                                retries--;
                                break;
                            }
                        }
                    }
                }
            }
            if (state == 3) {
                writeOut("Timeout waiting for ACK\n");
                retries--;
                state = 0;
            }
        }
    }

    writeOut("DHCP failed\n");
    return 0;
}

// ============== Network Scan (ARP Discovery) ==============

// Send ARP request to a specific IP
static void send_arp_request(unsigned char *target_ip) {
    unsigned char packet[42];  // 14 (eth) + 28 (arp)
    unsigned char broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    unsigned char zero_mac[6] = {0, 0, 0, 0, 0, 0};

    // Ethernet header
    mem_copy(packet, broadcast_mac, 6);        // Broadcast destination
    mem_copy(packet + 6, mac_address, 6);      // Our source MAC
    packet[12] = 0x08; packet[13] = 0x06;      // EtherType = ARP

    // ARP request
    struct arp_packet *arp = (struct arp_packet *)(packet + 14);
    arp->hw_type = htons(1);                   // Ethernet
    arp->proto_type = htons(0x0800);           // IPv4
    arp->hw_len = 6;
    arp->proto_len = 4;
    arp->opcode = htons(ARP_REQUEST);
    mem_copy(arp->sender_mac, mac_address, 6);
    mem_copy(arp->sender_ip, ip_address, 4);
    mem_copy(arp->target_mac, zero_mac, 6);
    mem_copy(arp->target_ip, target_ip, 4);

    net_send(broadcast_mac, ETH_TYPE_ARP, packet + 14, 28);
}

// Format MAC address to string buffer
static void format_mac(unsigned char *mac, char *buf) {
    const char hex[] = "0123456789ABCDEF";
    int pos = 0;
    for (int i = 0; i < 6; i++) {
        buf[pos++] = hex[(mac[i] >> 4) & 0x0F];
        buf[pos++] = hex[mac[i] & 0x0F];
        if (i < 5) buf[pos++] = ':';
    }
    buf[pos] = '\0';
}

// Scan network for hosts using ARP
// Scans the local subnet based on current IP and subnet mask
static void net_scan(void) {
    unsigned char network[4];
    unsigned char scan_ip[4];
    int hosts_found = 0;

    // Calculate network address
    for (int i = 0; i < 4; i++) {
        network[i] = ip_address[i] & subnet_mask[i];
    }

    // Calculate number of hosts to scan based on subnet
    int host_bits = 0;
    for (int i = 0; i < 4; i++) {
        unsigned char inv = ~subnet_mask[i];
        while (inv) {
            host_bits++;
            inv >>= 1;
        }
    }

    int max_hosts = (1 << host_bits) - 2;  // Exclude network and broadcast
    if (max_hosts > 254) max_hosts = 254;  // Limit scan to /24 max for speed

    writeOut("Scanning network: ");
    char net_str[16];
    format_ip(network, net_str);
    writeOut(net_str);
    writeOut("/");
    writeOutNum(net_get_cidr());
    writeOut("\n");
    writeOut("Scanning ");
    writeOutNum(max_hosts);
    writeOut(" hosts...\n\n");

    writeOut("IP Address       MAC Address\n");
    writeOut("---------------- -----------------\n");

    // Scan each host in the subnet
    for (int host = 1; host <= max_hosts; host++) {
        // Calculate host IP
        scan_ip[0] = network[0];
        scan_ip[1] = network[1];
        scan_ip[2] = network[2];
        scan_ip[3] = network[3] + host;

        // Handle overflow for larger subnets
        if (host_bits > 8) {
            int h = host;
            scan_ip[3] = network[3] | (h & 0xFF);
            scan_ip[2] = network[2] | ((h >> 8) & (~subnet_mask[2]));
        }

        // Skip our own IP
        if (ip_match(scan_ip, ip_address)) continue;

        // Send ARP request
        send_arp_request(scan_ip);

        // Wait for reply (short timeout)
        for (int wait = 0; wait < 5000; wait++) {
            unsigned char packet[1500];
            int len = net_receive(packet, sizeof(packet));

            if (len > 0) {
                unsigned short ethertype = (packet[12] << 8) | packet[13];

                if (ethertype == ETH_TYPE_ARP) {
                    struct arp_packet *arp = (struct arp_packet *)(packet + 14);

                    if (htons(arp->opcode) == ARP_REPLY) {
                        char ip_str[16];
                        char mac_str[18];
                        format_ip(arp->sender_ip, ip_str);
                        format_mac(arp->sender_mac, mac_str);

                        // Pad IP for alignment
                        writeOut(ip_str);
                        int pad = 17 - 15;  // Approximate padding
                        for (int p = 0; p < pad; p++) writeOut(" ");
                        writeOut(mac_str);
                        writeOut("\n");
                        hosts_found++;
                    }
                }
            }
        }
    }

    writeOut("\n");
    writeOutNum(hosts_found);
    writeOut(" host(s) found\n");
}

// ============== DNS Resolution ==============

#define DNS_PORT 53

// DNS header structure
struct dns_header {
    unsigned short id;
    unsigned short flags;
    unsigned short qdcount;  // Question count
    unsigned short ancount;  // Answer count
    unsigned short nscount;  // Authority count
    unsigned short arcount;  // Additional count
};

static unsigned short dns_id = 0x1234;

// Get string length
static int str_len(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

// Encode domain name in DNS format (e.g., "google.com" -> "\x06google\x03com\x00")
static int dns_encode_name(const char *domain, unsigned char *buf) {
    int pos = 0;
    int label_start = 0;
    int i = 0;

    while (1) {
        if (domain[i] == '.' || domain[i] == '\0') {
            int label_len = i - label_start;
            buf[pos++] = (unsigned char)label_len;
            for (int j = label_start; j < i; j++) {
                buf[pos++] = domain[j];
            }
            label_start = i + 1;
            if (domain[i] == '\0') break;
        }
        i++;
    }
    buf[pos++] = 0;  // Null terminator
    return pos;
}

// Send DNS query
static int dns_send_query(const char *domain) {
    unsigned char packet[512];
    unsigned char broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    // Clear packet
    for (int i = 0; i < 512; i++) packet[i] = 0;

    // Ethernet header
    mem_copy(packet, broadcast_mac, 6);
    mem_copy(packet + 6, mac_address, 6);
    packet[12] = 0x08; packet[13] = 0x00;  // IPv4

    // IP header
    struct ip_header *ip = (struct ip_header *)(packet + 14);
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->id = htons(dns_id);
    ip->flags_frag = 0;
    ip->ttl = 64;
    ip->protocol = 17;  // UDP
    mem_copy(ip->src_ip, ip_address, 4);
    mem_copy(ip->dst_ip, dns_server, 4);

    // UDP header
    struct udp_header *udp = (struct udp_header *)(packet + 34);
    udp->src_port = htons(12345);  // Random source port
    udp->dst_port = htons(DNS_PORT);

    // DNS header
    struct dns_header *dns = (struct dns_header *)(packet + 42);
    dns->id = htons(dns_id++);
    dns->flags = htons(0x0100);  // Standard query, recursion desired
    dns->qdcount = htons(1);     // One question
    dns->ancount = 0;
    dns->nscount = 0;
    dns->arcount = 0;

    // DNS question
    unsigned char *qname = packet + 42 + 12;  // After DNS header
    int name_len = dns_encode_name(domain, qname);

    // QTYPE and QCLASS
    unsigned char *qtype = qname + name_len;
    qtype[0] = 0; qtype[1] = 1;   // Type A (IPv4 address)
    qtype[2] = 0; qtype[3] = 1;   // Class IN (Internet)

    // Calculate lengths
    int dns_len = 12 + name_len + 4;  // Header + name + type/class
    int udp_len = 8 + dns_len;
    int ip_len = 20 + udp_len;

    // Fill in lengths
    ip->total_len = htons(ip_len);
    ip->checksum = 0;
    ip->checksum = htons(checksum((unsigned char *)ip, 20));

    udp->length = htons(udp_len);
    udp->checksum = 0;  // Optional for IPv4

    // Send packet
    return net_send(broadcast_mac, ETH_TYPE_IP, packet + 14, ip_len);
}

// Parse DNS response and extract IP address
// Returns 1 on success, 0 on failure
static int dns_parse_response(unsigned char *packet, int len, unsigned char *result_ip) {
    struct ip_header *ip = (struct ip_header *)(packet + 14);
    int ip_hdr_len = (ip->version_ihl & 0x0F) * 4;
    struct udp_header *udp = (struct udp_header *)(packet + 14 + ip_hdr_len);

    // Check if it's a DNS response
    if (htons(udp->src_port) != DNS_PORT) return 0;

    struct dns_header *dns = (struct dns_header *)(packet + 14 + ip_hdr_len + 8);

    // Check if it's a response (QR bit set) and no error
    unsigned short flags = htons(dns->flags);
    if (!(flags & 0x8000)) return 0;  // Not a response
    if (flags & 0x000F) return 0;     // Error code present

    int ancount = htons(dns->ancount);
    if (ancount == 0) return 0;  // No answers

    // Skip question section
    unsigned char *ptr = (unsigned char *)dns + 12;
    while (*ptr != 0) {
        if ((*ptr & 0xC0) == 0xC0) {
            ptr += 2;  // Compression pointer
            break;
        }
        ptr += *ptr + 1;
    }
    if (*ptr == 0) ptr++;  // Skip null terminator
    ptr += 4;  // Skip QTYPE and QCLASS

    // Parse answer section
    for (int i = 0; i < ancount; i++) {
        // Skip name (handle compression)
        if ((*ptr & 0xC0) == 0xC0) {
            ptr += 2;  // Compression pointer
        } else {
            while (*ptr != 0) ptr += *ptr + 1;
            ptr++;  // Skip null
        }

        unsigned short type = (ptr[0] << 8) | ptr[1];
        ptr += 2;
        ptr += 2;  // Class
        ptr += 4;  // TTL
        unsigned short rdlen = (ptr[0] << 8) | ptr[1];
        ptr += 2;

        if (type == 1 && rdlen == 4) {  // Type A, 4 bytes
            result_ip[0] = ptr[0];
            result_ip[1] = ptr[1];
            result_ip[2] = ptr[2];
            result_ip[3] = ptr[3];
            return 1;  // Success!
        }

        ptr += rdlen;  // Skip to next record
    }

    return 0;
}

// Resolve domain name to IP address
// Returns 1 on success, 0 on failure
static int dns_resolve(const char *domain, unsigned char *result_ip) {
    writeOut("Resolving ");
    writeOut(domain);
    writeOut("...\n");

    // Send DNS query
    if (dns_send_query(domain) < 0) {
        writeOut("Failed to send DNS query\n");
        return 0;
    }

    // Wait for response
    for (int timeout = 0; timeout < 500000; timeout++) {
        unsigned char packet[1500];
        int len = net_receive(packet, sizeof(packet));

        if (len > 0) {
            unsigned short ethertype = (packet[12] << 8) | packet[13];

            if (ethertype == ETH_TYPE_ARP) {
                handle_arp(packet, len);
            }
            else if (ethertype == ETH_TYPE_IP) {
                struct ip_header *ip = (struct ip_header *)(packet + 14);
                if (ip->protocol == 17) {  // UDP
                    if (dns_parse_response(packet, len, result_ip)) {
                        char ip_str[16];
                        format_ip(result_ip, ip_str);
                        writeOut("Resolved to: ");
                        writeOut(ip_str);
                        writeOut("\n");
                        return 1;
                    }
                }
            }
        }
    }

    writeOut("DNS resolution timeout\n");
    return 0;
}

// Check if string is a valid IP address (contains only digits and dots)
static int is_ip_address(const char *str) {
    int i = 0;
    int dots = 0;
    while (str[i]) {
        if (str[i] == '.') {
            dots++;
        } else if (str[i] < '0' || str[i] > '9') {
            return 0;  // Contains non-digit, non-dot character
        }
        i++;
    }
    return (dots == 3);  // Valid IP has exactly 3 dots
}

#endif