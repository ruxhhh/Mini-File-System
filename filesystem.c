#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "filesystem.h"

static Superblock superblock;
static char disk[BLOCKS][BLOCK_SIZE];
static Inode inode_table[MAX_FILES];
static int block_bitmap[BLOCKS];

static int find_file(const char *filename)
{
    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].used && strcmp(inode_table[i].filename, filename) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_free_inode(void)
{
    for (int i = 0; i < MAX_FILES; i++) {
        if (!inode_table[i].used) {
            return i;
        }
    }
    return -1;
}

static int find_free_block(void)
{
    for (int i = 0; i < BLOCKS; i++) {
        if (!block_bitmap[i]) {
            return i;
        }
    }
    return -1;
}

static int blocks_needed(size_t size)
{
    if (size == 0) {
        return 1;
    }
    return (int)((size + BLOCK_SIZE - 1) / BLOCK_SIZE);
}

static void set_metadata(void)
{
    memcpy(superblock.magic, FS_MAGIC, sizeof(superblock.magic));
    superblock.version = FS_VERSION;
    superblock.block_count = BLOCKS;
    superblock.block_size = BLOCK_SIZE;
    superblock.max_files = MAX_FILES;
}

void fs_format(void)
{
    memset(disk, 0, sizeof(disk));
    memset(inode_table, 0, sizeof(inode_table));
    memset(block_bitmap, 0, sizeof(block_bitmap));
    set_metadata();

    for (int i = 0; i < MAX_FILES; i++) {
        for (int j = 0; j < MAX_BLOCKS_PER_FILE; j++) {
            inode_table[i].blocks[j] = -1;
        }
    }
}

void fs_init(void)
{
    if (!fs_load()) {
        fs_format();
        fs_save();
    }
}

int fs_save(void)
{
    FILE *fp = fopen(FS_IMAGE, "wb");
    if (fp == NULL) {
        perror("Error opening filesystem image");
        return 0;
    }

    if (fwrite(&superblock, sizeof(superblock), 1, fp) != 1 ||
        fwrite(block_bitmap, sizeof(block_bitmap), 1, fp) != 1 ||
        fwrite(inode_table, sizeof(inode_table), 1, fp) != 1 ||
        fwrite(disk, sizeof(disk), 1, fp) != 1) {
        fclose(fp);
        fprintf(stderr, "Error: Failed to save filesystem image.\n");
        return 0;
    }

    fclose(fp);
    return 1;
}

int fs_load(void)
{
    FILE *fp = fopen(FS_IMAGE, "rb");
    if (fp == NULL) {
        return 0;
    }

    int ok = 1;

    if (fread(&superblock, sizeof(superblock), 1, fp) != 1 ||
        fread(block_bitmap, sizeof(block_bitmap), 1, fp) != 1 ||
        fread(inode_table, sizeof(inode_table), 1, fp) != 1 ||
        fread(disk, sizeof(disk), 1, fp) != 1) {
        ok = 0;
    }

    fclose(fp);

    if (!ok || strcmp(superblock.magic, FS_MAGIC) != 0 ||
        superblock.version != FS_VERSION ||
        superblock.block_count != BLOCKS ||
        superblock.block_size != BLOCK_SIZE ||
        superblock.max_files != MAX_FILES) {
        return 0;
    }

    return 1;
}

int create_file(const char *filename, const char *data)
{
    size_t name_len = strlen(filename);
    size_t size = strlen(data);

    if (name_len == 0 || name_len >= FILENAME_SIZE) {
        printf("Error: Invalid file name.\n");
        return 0;
    }

    if (find_file(filename) != -1) {
        printf("Error: File Already Exists.\n");
        return 0;
    }

    int required_blocks = blocks_needed(size);

    if (required_blocks > MAX_BLOCKS_PER_FILE) {
        printf("Error: File Too Large. Maximum file size is %d bytes.\n",
               MAX_BLOCKS_PER_FILE * BLOCK_SIZE);
        return 0;
    }

    int inode_index = find_free_inode();
    if (inode_index == -1) {
        printf("Error: Maximum number of files reached.\n");
        return 0;
    }

    int free_blocks = 0;
    for (int i = 0; i < BLOCKS; i++) {
        if (!block_bitmap[i]) {
            free_blocks++;
        }
    }

    if (free_blocks < required_blocks) {
        printf("Error: Disk Storage Full. Need %d block(s), only %d free.\n",
               required_blocks, free_blocks);
        return 0;
    }

    Inode *node = &inode_table[inode_index];
    memset(node, 0, sizeof(*node));

    strncpy(node->filename, filename, FILENAME_SIZE - 1);
    node->filename[FILENAME_SIZE - 1] = '\0';
    node->size = size;
    node->block_count = required_blocks;
    node->used = 1;
    node->created_at = time(NULL);
    node->modified_at = node->created_at;

    for (int i = 0; i < MAX_BLOCKS_PER_FILE; i++) {
        node->blocks[i] = -1;
    }

    size_t copied = 0;

    for (int i = 0; i < required_blocks; i++) {
        int block = find_free_block();
        if (block == -1) {
            fprintf(stderr, "Error: Internal block allocation failure.\n");
            return 0;
        }

        node->blocks[i] = block;
        block_bitmap[block] = 1;

        size_t remaining = size - copied;
        size_t chunk = remaining < BLOCK_SIZE ? remaining : BLOCK_SIZE;

        if (chunk > 0) {
            memcpy(disk[block], data + copied, chunk);
        }

        if (chunk < BLOCK_SIZE) {
            memset(disk[block] + chunk, 0, BLOCK_SIZE - chunk);
        }

        copied += chunk;
    }

    fs_save();

    printf("File created successfully using %d block(s).\n", required_blocks);
    return 1;
}

int read_file(const char *filename)
{
    int index = find_file(filename);
    if (index == -1) {
        printf("Error: File Not Found.\n");
        return 0;
    }

    Inode *node = &inode_table[index];

    printf("\nFile Content:\n");

    size_t remaining = node->size;
    for (int i = 0; i < node->block_count; i++) {
        size_t chunk = remaining < BLOCK_SIZE ? remaining : BLOCK_SIZE;
        if (chunk > 0) {
            fwrite(disk[node->blocks[i]], 1, chunk, stdout);
        }
        remaining -= chunk;
    }

    printf("\n");
    return 1;
}

int append_file(const char *filename, const char *data)
{
    int index = find_file(filename);
    if (index == -1) {
        printf("Error: File Not Found.\n");
        return 0;
    }

    Inode *node = &inode_table[index];
    size_t append_size = strlen(data);
    size_t new_size = node->size + append_size;
    int new_block_count = blocks_needed(new_size);

    if (new_block_count > MAX_BLOCKS_PER_FILE) {
        printf("Error: File Too Large after append.\n");
        return 0;
    }

    int extra_blocks = new_block_count - node->block_count;
    int free_blocks = 0;

    for (int i = 0; i < BLOCKS; i++) {
        if (!block_bitmap[i]) {
            free_blocks++;
        }
    }

    if (free_blocks < extra_blocks) {
        printf("Error: Disk does not have enough free blocks for append.\n");
        return 0;
    }

    /* Allocate any extra blocks before writing data. */
    for (int i = node->block_count; i < new_block_count; i++) {
        int block = find_free_block();
        if (block == -1) {
            fprintf(stderr, "Error: Internal block allocation failure.\n");
            return 0;
        }
        node->blocks[i] = block;
        block_bitmap[block] = 1;
        memset(disk[block], 0, BLOCK_SIZE);
    }

    size_t old_size = node->size;
    size_t remaining = append_size;
    size_t source_offset = 0;

    while (remaining > 0) {
        int block_position = (int)(old_size / BLOCK_SIZE);
        size_t offset = old_size % BLOCK_SIZE;
        size_t room = BLOCK_SIZE - offset;
        size_t chunk = remaining < room ? remaining : room;

        memcpy(disk[node->blocks[block_position]] + offset,
               data + source_offset,
               chunk);

        old_size += chunk;
        source_offset += chunk;
        remaining -= chunk;
    }

    node->size = new_size;
    node->block_count = new_block_count;
    node->modified_at = time(NULL);

    fs_save();

    printf("Data appended successfully. File now uses %d block(s).\n",
           new_block_count);
    return 1;
}

int delete_file(const char *filename)
{
    int index = find_file(filename);
    if (index == -1) {
        printf("Error: File Not Found.\n");
        return 0;
    }

    Inode *node = &inode_table[index];

    for (int i = 0; i < node->block_count; i++) {
        int block = node->blocks[i];
        if (block >= 0 && block < BLOCKS) {
            memset(disk[block], 0, BLOCK_SIZE);
            block_bitmap[block] = 0;
        }
    }

    memset(node, 0, sizeof(*node));
    for (int i = 0; i < MAX_BLOCKS_PER_FILE; i++) {
        node->blocks[i] = -1;
    }

    fs_save();

    printf("File deleted successfully.\n");
    return 1;
}

void list_files(void)
{
    int count = 0;

    printf("\n================ FILE LIST ================\n");

    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].used) {
            printf("%-20s %6zu bytes   %2d block(s)\n",
                   inode_table[i].filename,
                   inode_table[i].size,
                   inode_table[i].block_count);
            count++;
        }
    }

    if (count == 0) {
        printf("No files stored.\n");
    }

    printf("===========================================\n");
}

int stat_file(const char *filename)
{
    int index = find_file(filename);
    if (index == -1) {
        printf("Error: File Not Found.\n");
        return 0;
    }

    Inode *node = &inode_table[index];

    printf("\n=============== FILE STAT ================\n");
    printf("Name        : %s\n", node->filename);
    printf("Size        : %zu bytes\n", node->size);
    printf("Block Count : %d\n", node->block_count);
    printf("Blocks      : ");

    for (int i = 0; i < node->block_count; i++) {
        printf("%d", node->blocks[i]);
        if (i + 1 < node->block_count) {
            printf(", ");
        }
    }

    printf("\nCreated     : %s", ctime(&node->created_at));
    printf("Modified    : %s", ctime(&node->modified_at));
    printf("==========================================\n");

    return 1;
}

void show_status(void)
{
    int used_blocks = 0;
    int used_files = 0;
    size_t logical_bytes = 0;

    for (int i = 0; i < BLOCKS; i++) {
        if (block_bitmap[i]) {
            used_blocks++;
        }
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].used) {
            used_files++;
            logical_bytes += inode_table[i].size;
        }
    }

    int free_blocks = BLOCKS - used_blocks;
    size_t total_bytes = (size_t)BLOCKS * BLOCK_SIZE;
    size_t allocated_bytes = (size_t)used_blocks * BLOCK_SIZE;

    printf("\n=============== FILESYSTEM STATUS ===============\n");
    printf("Block Size       : %d bytes\n", BLOCK_SIZE);
    printf("Total Blocks     : %d\n", BLOCKS);
    printf("Used Blocks      : %d\n", used_blocks);
    printf("Free Blocks      : %d\n", free_blocks);
    printf("Total Capacity   : %zu bytes\n", total_bytes);
    printf("Allocated Space  : %zu bytes\n", allocated_bytes);
    printf("Logical File Data: %zu bytes\n", logical_bytes);
    printf("Files Stored     : %d / %d\n", used_files, MAX_FILES);
    printf("Persistent Image : %s\n", FS_IMAGE);
    printf("==================================================\n");
}

int fs_check(void)
{
    int errors = 0;
    int referenced[BLOCKS] = {0};

    for (int i = 0; i < MAX_FILES; i++) {
        if (!inode_table[i].used) {
            continue;
        }

        if (inode_table[i].block_count < 1 ||
            inode_table[i].block_count > MAX_BLOCKS_PER_FILE) {
            printf("FSCK: File '%s' has invalid block count.\n",
                   inode_table[i].filename);
            errors++;
            continue;
        }

        int expected = blocks_needed(inode_table[i].size);
        if (expected != inode_table[i].block_count) {
            printf("FSCK: File '%s' has incorrect block count.\n",
                   inode_table[i].filename);
            errors++;
        }

        for (int j = 0; j < inode_table[i].block_count; j++) {
            int block = inode_table[i].blocks[j];

            if (block < 0 || block >= BLOCKS) {
                printf("FSCK: File '%s' has invalid block %d.\n",
                       inode_table[i].filename, block);
                errors++;
                continue;
            }

            if (referenced[block]) {
                printf("FSCK: Block %d is referenced by multiple files.\n", block);
                errors++;
            }

            referenced[block] = 1;

            if (!block_bitmap[block]) {
                printf("FSCK: Block %d is referenced but bitmap says FREE.\n", block);
                errors++;
            }
        }
    }

    for (int i = 0; i < BLOCKS; i++) {
        if (block_bitmap[i] && !referenced[i]) {
            printf("FSCK: Block %d is marked USED but not referenced.\n", i);
            errors++;
        }
    }

    if (errors == 0) {
        printf("\nFSCK: File system is consistent. No errors found.\n");
        return 1;
    }

    printf("\nFSCK: Found %d consistency problem(s).\n", errors);
    return 0;
}
