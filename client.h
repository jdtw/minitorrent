#include "common.h"
#include <linux/limits.h>

#define UPLOAD_FIFO ".upload"

#define CHUNKS_PER_PEER 16

// local commands sent to the table manager
#define UPLOAD_CMD_START 1
#define UPLOAD_CMD_UPLOAD 2
#define UPLOAD_CMD_STOP_ALL 3
#define UPLOAD_CMD_DUMP 4
#define UPLOAD_CMD_FIND 5

// this struct is sent to a client to initiate a download
// hash is the sha1 hash of the file requested, and 
// chunk is the zero-indexed chunk number.
typedef struct download_request {
    unsigned char hash[SHA1_SIZE];
    int chunk;
} download_request;

// struct sent to the hash table manager via the upload fifo
typedef struct upload_local_request {
    int cmd;
    char name[PATH_MAX];
    unsigned char hash[SHA1_SIZE];
} upload_local_request;

void client_download(torrent t);
void client_start();
void client_upload(char *, unsigned char *);
void client_stop();
void client_dump();
