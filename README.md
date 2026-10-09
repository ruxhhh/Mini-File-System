# Mini File System (RAM Disk) — Enhanced OS Project

A lightweight user-space simulation of a block-based file system written in C.

## What makes this version stronger?

- Multi-block file storage
- Inode table for file metadata
- Block bitmap for allocation tracking
- Persistent binary filesystem image (`mini_fs.img`)
- Create, read, append and delete
- Directory-style file listing
- Detailed `stat` information
- Disk usage/status reporting
- `fsck`-style consistency checking
- Format support with confirmation
- GCC, Make, GDB and Valgrind support

## Storage model

- 32 blocks
- 64 bytes per block
- 2048 bytes total simulated storage
- Maximum 16 files
- Maximum 16 blocks per file

## Build

```bash
make
```

## Run

```bash
./mini_fs
```

or

```bash
make run
```

## Debug

```bash
gdb ./mini_fs
```

## Memory checking

```bash
make valgrind
```

## Persistence

The in-memory file system is saved into `mini_fs.img`, a binary image file. The program loads it automatically on the next start.

## Core mapping

`filename -> inode -> block list -> RAM disk blocks -> file data`

## Important technical note

This is a user-space simulation for learning Operating Systems concepts. It is not a kernel filesystem, filesystem driver, or replacement for ext4/NTFS.