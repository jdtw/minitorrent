#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <semaphore.h>
#include "uploadhash.h"
#include "torrent.h"
#include "common.h"

upload_node *upload_table[UPLOAD_TABLE_SIZE];
sem_t s;

void upload_init_table() {
    sem_init(&s, 0, 1);
    memset(upload_table, 0, UPLOAD_TABLE_SIZE*(sizeof(upload_table[0])));
}

int upload_hash(unsigned char *h) {
    return (int) *h;
}

int upload_add_hash(char *fn, unsigned char *fh) {
    sem_wait(&s);
    upload_node *n = upload_table[upload_hash(fh)];
    for(; n != NULL; n=n->next) {
        if(memcmp((void*)fh, (void*)n->file_hash, SHA1_SIZE) == 0) {
            break; // the file has already been hashed.
        }
    }
    if (n == NULL) { // not in table
        n = malloc(sizeof(upload_node));
        n->next = upload_table[upload_hash(fh)];
        memset(n->file_name, 0, PATH_MAX);
        strcpy(n->file_name, fn);
        memcpy((void*)n->file_hash, (void*)fh, SHA1_SIZE);
        upload_table[upload_hash(fh)] = n;
    }
    sem_post(&s);
    return 0;
}

char *upload_find_hash(unsigned char *fh) {
    sem_wait(&s);
    upload_node *n = upload_table[upload_hash(fh)];
    for(; n != NULL; n=n->next) {
        if(memcmp((void*)fh, (void*)n->file_hash, SHA1_SIZE) == 0) {
            break; // the file has already been hashed.
        }
    }
    if (n == NULL) {
        return NULL;
    }
    sem_post(&s);
    return n->file_name;
}
    
void upload_free_table() {
    int i;
    upload_node *n, *n_next;
    for(i=0; i<UPLOAD_TABLE_SIZE; i++) {
        if(upload_table[i] == NULL) {
            continue;
        }
        for(n=upload_table[i]; n != NULL; n = n_next) {
            n_next = n->next;
            free(n);
        }
    }
    sem_destroy(&s);
}

    
void upload_dump_table() {
    int i;
    upload_node *n;
    for(i=0; i<UPLOAD_TABLE_SIZE; i++) {
        if(upload_table[i] == NULL) {
            continue;
        }
        for(n=upload_table[i]; n != NULL; n = n->next) {
            printf("%02x: ", i); print_hash(n->file_hash);
            printf("\t%s\n", n->file_name);
        }
    }
}


