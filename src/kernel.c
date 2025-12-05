/*
 * ============================================================================
 *                              SPARK KERNEL
 * ============================================================================
 * 
 * This is the main kernel file for Spark OS.
 * It handles the command-line interface and processes user commands.
 * 
 * Author: syntaxMORG0 and Samuraien2
 * 
 * ============================================================================
 */

#include "functions/drivers/networkDriver.h"
#include "functions/drivers/filesystem.h"
#include "package.h"
#include "strings.h"


/* ============================================================================
 *                           HELPER FUNCTIONS
 * ============================================================================ */

/*
 * parseIP - Convert an IP address string to bytes
 * 
 * Takes a string like "192.168.1.1" and converts it to 4 bytes.
 * 
 * Parameters:
 *   str - The IP address as a string (e.g., "192.168.1.1")
 *   out - Array of 4 bytes to store the result
 * 
 * Returns:
 *   1 if successful, 0 if the format is invalid
 * 
 * Example:
 *   unsigned char ip[4];
 *   if (parseIP("10.0.2.15", ip)) {
 *       // ip[0] = 10, ip[1] = 0, ip[2] = 2, ip[3] = 15
 *   }
 */
static int parseIP(const char *str, unsigned char *out) {
    int i = 0;          // Current position in the string
    int octet = 0;      // Current number being parsed (0-255)
    int digits = 0;     // How many digits we've read for this octet
    
    // An IP address has 4 parts (octets) separated by dots
    for (int part = 0; part < 4; part++) {
        octet = 0;
        digits = 0;
        
        // Read digits until we hit a non-digit
        while (str[i] >= '0' && str[i] <= '9') {
            // Build the number: "192" = 1*100 + 9*10 + 2
            octet = octet * 10 + (str[i] - '0');
            digits++;
            i++;
            
            // Each octet must be 0-255
            if (octet > 255) {
                return 0;  // Invalid: number too big
            }
        }
        
        // We must have at least one digit
        if (digits == 0) {
            return 0;  // Invalid: no digits found
        }
        
        // Store this octet
        out[part] = (unsigned char)octet;
        
        // After the first 3 octets, we need a dot
        if (part < 3) {
            if (str[i] != '.') {
                return 0;  // Invalid: expected a dot
            }
            i++;  // Skip the dot
        }
    }
    
    // Make sure there's nothing after the last octet
    if (str[i] != '\0') {
        return 0;  // Invalid: extra characters at the end
    }
    
    return 1;  // Success!
}


/* ============================================================================
 *                           COMMAND HANDLERS
 * ============================================================================
 * 
 * Each function below handles a specific command.
 * This makes the main loop cleaner and easier to understand.
 * ============================================================================ */

/*
 * cmd_setip - Set a new IP address
 */
static void cmd_setip(const char *arg) {
    unsigned char new_ip[4];
    
    if (parseIP(arg, new_ip)) {
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

/*
 * cmd_router - Show or set the gateway/router address
 */
static void cmd_router(const char *arg) {
    // If no argument, just show current router
    if (arg == 0 || arg[0] == '\0') {
        char gw_str[16];
        net_get_gateway_string(gw_str);
        writeOut("Router/Gateway: ");
        writeOut(gw_str);
        BreakLine(1);
        return;
    }
    
    // Otherwise, set a new router
    unsigned char new_gw[4];
    if (parseIP(arg, new_gw)) {
        net_set_gateway(new_gw[0], new_gw[1], new_gw[2], new_gw[3]);
        writeOut("Router set to: ");
        
        char gw_str[16];
        net_get_gateway_string(gw_str);
        writeOut(gw_str);
        BreakLine(1);
    } else {
        writeOut("Invalid IP format. Use: router x.x.x.x\n");
    }
}

/*
 * cmd_subnet - Show or set the subnet mask
 */
static void cmd_subnet(const char *arg) {
    // If no argument, just show current subnet
    if (arg == 0 || arg[0] == '\0') {
        char mask_str[16];
        net_get_subnet_string(mask_str);
        writeOut("Subnet Mask: ");
        writeOut(mask_str);
        writeOut(" (/");
        writeOutNum(net_get_cidr());
        writeOut(")\n");
        return;
    }
    
    // Otherwise, set a new subnet mask
    unsigned char new_mask[4];
    if (parseIP(arg, new_mask)) {
        net_set_subnet(new_mask[0], new_mask[1], new_mask[2], new_mask[3]);
        writeOut("Subnet mask set to: ");
        
        char mask_str[16];
        net_get_subnet_string(mask_str);
        writeOut(mask_str);
        BreakLine(1);
    } else {
        writeOut("Invalid format. Use: subnet x.x.x.x\n");
    }
}

/*
 * cmd_dns - Show or set the DNS server
 */
static void cmd_dns(const char *arg) {
    // If no argument, just show current DNS
    if (arg == 0 || arg[0] == '\0') {
        char dns_str[16];
        net_get_dns_string(dns_str);
        writeOut("DNS Server: ");
        writeOut(dns_str);
        BreakLine(1);
        return;
    }
    
    // Otherwise, set a new DNS server
    unsigned char new_dns[4];
    if (parseIP(arg, new_dns)) {
        net_set_dns(new_dns[0], new_dns[1], new_dns[2], new_dns[3]);
        writeOut("DNS server set to: ");
        
        char dns_str[16];
        net_get_dns_string(dns_str);
        writeOut(dns_str);
        BreakLine(1);
    } else {
        writeOut("Invalid IP format. Use: dns x.x.x.x\n");
    }
}

/*
 * cmd_dhcp - Get network configuration automatically via DHCP
 */
static void cmd_dhcp(void) {
    net_init();
    writeOut("Starting DHCP client...\n");
    
    if (dhcp_request()) {
        writeOut("DHCP configuration complete!\n");
    } else {
        writeOut("DHCP failed. Using static IP.\n");
    }
}

/*
 * cmd_listen - Listen for incoming network packets (ARP, ping)
 */
static void cmd_listen(void) {
    net_init();
    writeOut("Listening for network packets (ping, ARP)...\n");
    writeOut("Press Ctrl+C to stop\n");
    
    // Show current IP
    char ip_str[16];
    net_get_ip_string(ip_str);
    writeOut("IP: ");
    writeOut(ip_str);
    BreakLine(1);
    
    // Infinite loop to handle incoming packets
    while (1) {
        net_poll();
    }
}

/*
 * cmd_ping - Ping a host by IP address or domain name
 */
static void cmd_ping(const char *target) {
    unsigned char target_ip[4];
    int resolved = 0;
    
    net_init();
    
    // Check if it's an IP address or domain name
    if (is_ip_address(target)) {
        // It's an IP address like "10.0.2.2"
        if (parseIP(target, target_ip)) {
            resolved = 1;
        }
    } else {
        // It's a domain name like "google.com"
        // Use DNS to find the IP address
        if (dns_resolve(target, target_ip)) {
            resolved = 1;
        }
    }
    
    // If we couldn't resolve the target, show an error
    if (!resolved) {
        writeOut("Could not resolve host: ");
        writeOut(target);
        BreakLine(1);
        return;
    }
    
    // Convert IP to string for display
    char ip_str[16];
    format_ip(target_ip, ip_str);
    
    // Show what we're pinging
    writeOut("PING ");
    writeOut(target);
    writeOut(" (");
    writeOut(ip_str);
    writeOut(") - sending 4 packets\n");
    
    // Send 4 ping packets
    int received = 0;
    for (int seq = 1; seq <= 4; seq++) {
        // Try to send the ping
        if (net_send_ping(target_ip) >= 0) {
            // Wait for a reply (1 second timeout)
            if (net_wait_ping_reply(1000)) {
                writeOut("Reply from ");
                writeOut(ip_str);
                writeOut(" seq=");
                writeOutNum(seq);
                BreakLine(1);
                received++;
            } else {
                writeOut("Request timeout seq=");
                writeOutNum(seq);
                BreakLine(1);
            }
        } else {
            writeOut("Send failed\n");
        }
    }
    
    // Show statistics
    writeOut("--- ");
    writeOut(target);
    writeOut(" ping statistics ---\n");
    writeOut("4 packets sent, ");
    writeOutNum(received);
    writeOut(" received\n");
}


/* ============================================================================
 *                              MAIN KERNEL
 * ============================================================================ */

/*
 * kernel_main - The main entry point of the kernel
 * 
 * This function:
 * 1. Initializes the graphics/display
 * 2. Shows a welcome message
 * 3. Runs a loop that reads commands and executes them
 * 4. Exits when the user types "exit"
 */
void kernel_main(void) {
    // Buffer to store the user's input
    char buffer[100];
    
    // The prompt shown before each command
    char prompt[] = "> ";
    
    // Flag to keep the kernel running (1 = running, 0 = exit)
    int running = 1;
    
    // Initialize the graphics display
    initGraphics();
    
    // Show welcome message
    writeOut(WELCOME_TEXT);
    BreakLine(2);
    
    /* ------------------------------------------------------------------------
     * MAIN COMMAND LOOP
     * 
     * This loop continuously:
     * 1. Shows the prompt (">")
     * 2. Waits for the user to type a command
     * 3. Checks what command was entered
     * 4. Runs the appropriate code for that command
     * ------------------------------------------------------------------------ */
    while (running == 1) {
        // Show prompt and read user input
        writeOut(prompt);
        readLine(buffer, sizeof(buffer));
        
        /* --------------------------------------------------------------------
         * GENERAL COMMANDS
         * -------------------------------------------------------------------- */
        
        if (strcmp(buffer, "help") == 0) {
            // Show the help menu
            writeOut(HELP_TEXT);
            BreakLine(2);
        }
        else if (strcmp(buffer, "credits") == 0) {
            // Show credits
            writeOut(CREDITS);
        }
        else if (strcmp(buffer, "repo") == 0) {
            // Show repository link
            writeOut(REPO_DETAILS);
        }
        else if (strcmp(buffer, "exit") == 0) {
            // Exit the kernel
            running = 0;
        }
        
        /* --------------------------------------------------------------------
         * NETWORK INFO COMMANDS
         * -------------------------------------------------------------------- */
        
        else if (strcmp(buffer, "net") == 0) {
            // Show network status
            net_init();
            net_info();
        }
        else if (strcmp(buffer, "ip") == 0) {
            // Show current IP address
            char ip_str[16];
            net_get_ip_string(ip_str);
            writeOut("Current IP: ");
            writeOut(ip_str);
            BreakLine(1);
        }
        else if (strcmp(buffer, "router") == 0) {
            // Show current router (no argument)
            cmd_router(0);
        }
        else if (strcmp(buffer, "subnet") == 0) {
            // Show current subnet (no argument)
            cmd_subnet(0);
        }
        else if (strcmp(buffer, "dns") == 0) {
            // Show current DNS (no argument)
            cmd_dns(0);
        }
        
        /* --------------------------------------------------------------------
         * NETWORK CONFIG COMMANDS (with arguments)
         * -------------------------------------------------------------------- */
        
        else if (startsWith(buffer, "setip ")) {
            // Set IP address: "setip 192.168.1.100"
            cmd_setip(buffer + 6);  // Skip "setip "
        }
        else if (startsWith(buffer, "router ")) {
            // Set router: "router 192.168.1.1"
            cmd_router(buffer + 7);  // Skip "router "
        }
        else if (startsWith(buffer, "subnet ")) {
            // Set subnet: "subnet 255.255.255.0"
            cmd_subnet(buffer + 7);  // Skip "subnet "
        }
        else if (startsWith(buffer, "dns ")) {
            // Set DNS: "dns 8.8.8.8"
            cmd_dns(buffer + 4);  // Skip "dns "
        }
        
        /* --------------------------------------------------------------------
         * NETWORK TOOLS COMMANDS
         * -------------------------------------------------------------------- */
        
        else if (strcmp(buffer, "dhcp") == 0) {
            // Get IP automatically via DHCP
            cmd_dhcp();
        }
        else if (strcmp(buffer, "scan") == 0) {
            // Scan network for other devices
            net_init();
            net_scan();
        }
        else if (strcmp(buffer, "listen") == 0) {
            // Listen for incoming packets
            cmd_listen();
        }
        else if (startsWith(buffer, "ping ")) {
            // Ping a host: "ping google.com" or "ping 10.0.2.2"
            cmd_ping(buffer + 5);  // Skip "ping "
        }
        
        /* --------------------------------------------------------------------
         * UNKNOWN COMMAND
         * -------------------------------------------------------------------- */
        
        /* --------------------------------------------------------------------
         * FILESYSTEM COMMANDS
         * -------------------------------------------------------------------- */
        
        else if (strcmp(buffer, "ls") == 0) {
            fs_list();
        }
        else if (startsWith(buffer, "cat ")) {
            fs_cat(buffer + 4);
        }
        else if (startsWith(buffer, "touch ")) {
            fs_touch(buffer + 6);
        }
        else if (startsWith(buffer, "mkdir ")) {
            fs_mkdir(buffer + 6);
        }
        else if (startsWith(buffer, "rm ")) {
            fs_rm(buffer + 3);
        }
        else if (startsWith(buffer, "write ")) {
            // Parse: write filename content
            const char *args = buffer + 6;
            char filename[32];
            int i = 0;
            
            // Get filename (until space)
            while (args[i] != ' ' && args[i] != '\0' && i < 31) {
                filename[i] = args[i];
                i++;
            }
            filename[i] = '\0';
            
            // Get content (after space)
            if (args[i] == ' ') {
                fs_write(filename, args + i + 1);
            } else {
                writeOut("Usage: write <filename> <content>\n");
            }
        }
        
        /* --------------------------------------------------------------------
         * UNKNOWN COMMAND
         * -------------------------------------------------------------------- */
        
        else if (buffer[0] != '\0') {
            // Only show error if user typed something
            writeOut(ERR_INVALID_CMD);
            writeOut(buffer);
            BreakLine(1);
        }
    }
    
    // Kernel is shutting down
    BreakLine(1);
    exit();
}