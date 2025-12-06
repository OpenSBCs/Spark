int parseIP(const char *str, unsigned char *out) {
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