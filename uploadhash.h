#include "common.h"
#include <linux/limits.h>
#define UPLOAD_TABLE_SIZE 0x100

typedef struct upload_node {
    unsigned char file_hash[SHA1_SIZE];
    // this is the path to the file on the local machine:
    char file_name[PATH_MAX];
    struct upload_node *next;
} upload_node;
    
// must be called before any other upload hash commands
void upload_init_table();

int upload_add_hash(char *, unsigned char *);

char *upload_find_hash(unsigned char *);

// mostly for debugging, shows what the client is uploading:
void upload_dump_table();


void upload_free_table();

