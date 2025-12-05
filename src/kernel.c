#include "functions/drivers/networkDriver.h"
#include "package.h"
#include "strings.h"

// Parse IP address from string like "192.168.1.1"
// Returns 1 on success, 0 on failure
static int parseIP(const char *str, unsigned char *out) {
    int i = 0;
    int octet = 0;
    int digits = 0;
    
    for (int part = 0; part < 4; part++) {
        octet = 0;
        digits = 0;
        
        while (str[i] >= '0' && str[i] <= '9') {
            octet = octet * 10 + (str[i] - '0');
            digits++;
            i++;
            if (octet > 255) return 0;  // Invalid
        }
        
        if (digits == 0) return 0;  // No digits found
        out[part] = (unsigned char)octet;
        
        if (part < 3) {
            if (str[i] != '.') return 0;  // Expected dot
            i++;
        }
    }
    
    return (str[i] == '\0') ? 1 : 0;  // Must end here
}

void kernel_main(void) {
    char buffer[100];
    char prefix[] = "> ";
    int running = 1;
    
    writeOut(WELCOME_TEXT), BreakLine(2);
    
    while (running == 1) {
        writeOut(prefix), readLine(buffer, sizeof(buffer));
        if (strcmp(buffer, "help") == 0) {
            writeOut(HELP_TEXT), BreakLine(2);
        }
        else if (strcmp(buffer, "credits") == 0) {
            writeOut(CREDITS);
        }
        else if (strcmp(buffer, "net") == 0) {
            net_init();
            net_info();
        }
        else if (strcmp(buffer, "repo") == 0) {
            writeOut(REPO_DETAILS);
        }
        else if (startsWith(buffer, "setip ")) {
            unsigned char new_ip[4];
            if (parseIP(buffer + 6, new_ip)) {
                net_set_ip(new_ip[0], new_ip[1], new_ip[2], new_ip[3]);
                writeOut("IP address set to: ");
                char ip_str[16];
                net_get_ip_string(ip_str);
                writeOut(ip_str);
                BreakLine(1);
            } else {
                writeOut("Invalid IP format. Use: setip x.x.x.x\n");
            }
        }
        else if (strcmp(buffer, "ip") == 0) {
            char ip_str[16];
            net_get_ip_string(ip_str);
            writeOut("Current IP: ");
            writeOut(ip_str);
            BreakLine(1);
        }
        else if (strcmp(buffer, "listen") == 0) {
            net_init();
            writeOut("Listening for network packets (ping, ARP)...\n");
            writeOut("Press Ctrl+C to stop\n");
            char ip_str[16];
            net_get_ip_string(ip_str);
            writeOut("IP: ");
            writeOut(ip_str);
            BreakLine(1);
            
            // Network listening loop
            while (1) {
                net_poll();  // Handle ARP and ICMP
            }
        }
        else if (strcmp(buffer, "exit") == 0) {
            running--;
        }
        else {
            writeOut(ERR_INVALID_CMD), writeOut(buffer), BreakLine(1);
        }
    }
    BreakLine(1), exit();
    return;
}