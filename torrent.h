#include <linux/limits.h>
#include <stdint.h>
#include "common.h"

typedef struct torrent {
    char tracker[HOST_MAX];
    unsigned char file_hash[SHA1_SIZE];
    int32_t num_chunks;
    unsigned char **chunks;
    char friendly_name[PATH_MAX];
} torrent;

int create_torrent(char *, char *);
void parse_torrent(char *, torrent *);
void dump_torrent(torrent);
void free_torrent(torrent);
void print_hash(unsigned char *);

