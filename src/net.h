#ifndef NET_H
#define NET_H

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
int parseIP(const char *str, unsigned char *out);

#endif