CC = gcc
CFLAGS = -Wall -Wextra -g

all: ; $(CC) $(CFLAGS) main.c filesystem.c -o mini_fs

clean: ; rm -f mini_fs
 