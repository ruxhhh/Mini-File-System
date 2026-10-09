#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"

static void read_line(char *buffer, size_t size)
{
    if (fgets(buffer, (int)size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
    }
}

static void show_menu(void)
{
    printf("\n");
    printf("=================================================\n");
    printf("              MINI FILE SYSTEM\n");
    printf("             RAM DISK SIMULATOR\n");
    printf("=================================================\n");
    printf("1.  Create File\n");
    printf("2.  Read File\n");
    printf("3.  Append Data\n");
    printf("4.  Delete File\n");
    printf("5.  List Files\n");
    printf("6.  File Details (STAT)\n");
    printf("7.  Disk Status\n");
    printf("8.  File-System Check (FSCK)\n");
    printf("9.  Save File System\n");
    printf("10. Format File System\n");
    printf("11. Exit\n");
    printf("=================================================\n");
    printf("Enter your choice: ");
}

int main(void)
{
    char input[32];
    char filename[128];
    char data[1024];

    fs_init();

    printf("\nMini File System initialized.\n");
    printf("Storage: %d blocks x %d bytes = %d bytes\n",
           BLOCKS, BLOCK_SIZE, BLOCKS * BLOCK_SIZE);

    while (1) {
        show_menu();
        read_line(input, sizeof(input));

        int choice = atoi(input);

        switch (choice) {
            case 1:
                printf("\nEnter file name: ");
                read_line(filename, sizeof(filename));

                printf("Enter file content: ");
                read_line(data, sizeof(data));

                create_file(filename, data);
                break;

            case 2:
                printf("\nEnter file name: ");
                read_line(filename, sizeof(filename));
                read_file(filename);
                break;

            case 3:
                printf("\nEnter file name: ");
                read_line(filename, sizeof(filename));

                printf("Enter data to append: ");
                read_line(data, sizeof(data));

                append_file(filename, data);
                break;

            case 4:
                printf("\nEnter file name: ");
                read_line(filename, sizeof(filename));
                delete_file(filename);
                break;

            case 5:
                list_files();
                break;

            case 6:
                printf("\nEnter file name: ");
                read_line(filename, sizeof(filename));
                stat_file(filename);
                break;

            case 7:
                show_status();
                break;

            case 8:
                fs_check();
                break;

            case 9:
                if (fs_save()) {
                    printf("File system saved to %s successfully.\n", FS_IMAGE);
                }
                break;

            case 10:
                printf("\nWARNING: This will erase all files.\n");
                printf("Type FORMAT to continue: ");
                read_line(input, sizeof(input));

                if (strcmp(input, "FORMAT") == 0) {
                    fs_format();
                    fs_save();
                    printf("File system formatted successfully.\n");
                } else {
                    printf("Format cancelled.\n");
                }
                break;

            case 11:
                fs_save();
                printf("\nFile system saved. Goodbye!\n");
                return 0;

            default:
                printf("Error: Invalid choice.\n");
        }
    }
}
