#include "../package.h"
#include "strings.h"
#include "../print.h"
#include "../functions/drivers/fat32Driver.h"

void setup_process(void) {
    char buffer[100]; // predefine input data on RAM
    char prompt[] = "(SSW) > ";

    int running = 1;

    print("Spark Setup Wizard\n");

    while (running == 1) {
        print(prompt);
        readLine(buffer, sizeof(buffer));
        if (strcmp(buffer, "exit") == 0) {
            running--;
        }
        else if (strcmp(buffer, "part") == 0) {
            print("Mounting FAT32 filesystem...\n");
            int result = fat32_init(0);  // Partition at LBA 0
            if (result == 0) {
                print("FAT32 filesystem mounted successfully!\n");
            } else if (result == -1) {
                print("Error: Disk read failed\n");
            } else if (result == -2) {
                print("Error: Invalid boot signature\n");
            } else if (result == -3) {
                print("Error: Not a FAT32 filesystem\n");
            } else {
                print("Error: Mount failed\n");
            }
        }
        else if (strcmp(buffer, "help") == 0 || strcmp(buffer, "") == 0) {
            print(setup_help, "\n");
        }
        else if (strcmp(buffer, "spm") == 0 || strcmp(buffer, "snatch") == 0) {
            print("snatch (Spark Package Manager) comming soon\n");
        }
        else {
            print("unknown command: ", buffer, "\n");
        }
    }
    
    return;
};