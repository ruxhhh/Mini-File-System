#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stddef.h>
#include <time.h>

#define FS_MAGIC "MINIFS1"
#define FS_IMAGE "mini_fs.img"

#define BLOCKS 32
#define BLOCK_SIZE 64
#define MAX_FILES 16
#define FILENAME_SIZE 32
#define MAX_BLOCKS_PER_FILE 16
#define FS_VERSION 1

typedef struct {
    char filename[FILENAME_SIZE];
    size_t size;
    int block_count;
    int blocks[MAX_BLOCKS_PER_FILE];
    int used;
    time_t created_at;
    time_t modified_at;
} Inode;

typedef struct {
    char magic[8];
    int version;
    int block_count;
    int block_size;
    int max_files;
} Superblock;

void fs_init(void);
int fs_save(void);
int fs_load(void);
void fs_format(void);

int create_file(const char *filename, const char *data);
int read_file(const char *filename);
int append_file(const char *filename, const char *data);
int delete_file(const char *filename);
void list_files(void);
int stat_file(const char *filename);
void show_status(void);
int fs_check(void);

#endif
