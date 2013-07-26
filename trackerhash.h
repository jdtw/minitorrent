#include "common.h"
#define TRACKER_TABLE_SIZE 0x100

typedef struct peer {
    int npeers;
    char host[HOST_MAX+1];
    struct peer *next;
} peer;

typedef struct tracker_node {
    unsigned char file_hash[SHA1_SIZE];
    struct peer *p;
    struct tracker_node *next;
} tracker_node;
    
void tracker_init_table();
int tracker_add_hash(char *, unsigned char *);
peer *tracker_find_hash(unsigned char *);
void tracker_dump_table();
void tracker_free_table();
