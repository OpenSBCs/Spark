#ifndef STRINGS_H
#define STRINGS_H

// Help text - displayed in two columns to fit on screen
static const char *HELP_TEXT = 
    "+====================================================================+\n"
    "|                                                                    |\n"
    "|  GENERAL                           FILESYSTEM                      |\n"
    "|    help        Show help menu        ls          List files        |\n"
    "|    credits     Show credits          cat <file>  Show file content |\n"
    "|    repo        Show source link      touch <f>   Create empty file |\n"
    "|    exit        Shutdown Spark        mkdir <dir> Create directory  |\n"
    "|                                      rm <name>   Remove file/dir   |\n"
    "|  NETWORK INFO                                                      |\n"
    "|    net / ip / subnet / router / dns NETWORK TOOLS                  |\n"
    "|                                      ping <host> Ping IP/domain    |\n"
    "|  NETWORK CONFIG                      scan        Scan hosts (ARP)  |\n"
    "|    dhcp / setip / subnet / router    listen      Listen packets    |\n"
    "|    dns <ip>    Set DNS server                                      |\n"
    "|                                                                    |\n"
    "+====================================================================+\n";

static const char *CREDITS = "Spark is made and developed by syntaxMORG0 and Samuraien2\n";
static const char *REPO_DETAILS = "View the spark project here: https://github.com/syntaxMORG0/Spark\n";

// Welcome message
static const char *WELCOME_TEXT = "Hello from Spark!";

// Error messages
static const char *ERR_INVALID_CMD = "Invalid command: ";

#endif
