#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "trackerhash.h"
#include "torrent.h"
#include "common.h"

tracker_node *tracker_table[TRACKER_TABLE_SIZE];

void tracker_init_table() {
    memset(tracker_table, 0, TRACKER_TABLE_SIZE*(sizeof(*tracker_table)));
}

int tracker_hash(unsigned char *h) {
    return (int) *h;
}

int tracker_add_hash(char *host, unsigned char *fh) {
    tracker_node *n = tracker_table[tracker_hash(fh)];
    for(; n != NULL; n=n->next) {
        if(memcmp((void*)fh, (void*)n->file_hash, SHA1_SIZE) == 0) {
            break; // the file has already been hashed.
        }
    }
    if (n == NULL) { // not in table
        n = malloc(sizeof(tracker_node));
        n->next = tracker_table[tracker_hash(fh)];
        n->p = NULL;
        memcpy((void*)n->file_hash, (void*)fh, SHA1_SIZE);
        tracker_table[tracker_hash(fh)] = n;
    }
    
    peer *p = n->p;
    for(; p != NULL; p=p->next) {
        if(strcmp(p->host, host) == 0) {
            break;
        }
    }
    if (p == NULL) { // new peer
        p = malloc(sizeof(peer));
        p->npeers = (n->p) ? n->p->npeers + 1 : 1;
        p->next = n->p;
        strcpy(p->host, host);
        n->p = p;
    }
    return 0;
}

peer *tracker_find_hash(unsigned char *fh) {
    tracker_node *n = tracker_table[tracker_hash(fh)];
    for(; n != NULL; n=n->next) {
        if(memcmp((void*)fh, (void*)n->file_hash, SHA1_SIZE) == 0) {
            break; // the file has already been hashed.
        }
    }
    if (n == NULL) {
        printf("file not found!\n");
        return NULL;
    }
    return n->p;
}
    
void tracker_dump_table() {
    int i;
    tracker_node *n;
    peer *p;
    for(i=0; i<TRACKER_TABLE_SIZE; i++) {
        if(tracker_table[i] == NULL) {
            continue;
        }
        for(n=tracker_table[i]; n != NULL; n = n->next) {
            printf("%02x: ", i); print_hash(n->file_hash);
            printf("%d peer(s):\n", ((n->p) ? n->p->npeers : 0));
            for(p=n->p; p != NULL; p=p->next) {
                printf("\t%s\n", p->host);
            }
        }
    }
}

void tracker_free_table() {
    int i;
    tracker_node *n, *n_next;
    peer *p, *p_next;
    for(i=0; i<TRACKER_TABLE_SIZE; i++) {
        if(tracker_table[i] == NULL) {
            continue;
        }
        for(n=tracker_table[i]; n != NULL; n=n_next) {
            n_next=n->next;
            for(p=n->p; p != NULL; p=p_next) {
                p_next=p->next;
                free(p);
            }
            free(n);
        }
        tracker_table[i]=NULL;
    }
}
