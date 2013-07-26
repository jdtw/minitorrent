#include <gcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <stdint.h>
#include "torrent.h"
#include "common.h"

void hash_to_hex(unsigned char *, char **);
char *strip_path(char *path);
void copy_hash(unsigned char *, unsigned char **);

int create_torrent(char *path, char *host) {
    gcry_md_hd_t hd;
    int tfd, fd, num_read, i;
    int32_t num_chunks;
    char buffy[CHUNK_SIZE];
    unsigned char hash[SHA1_SIZE];
    unsigned char *file_hash = NULL;
    struct stat st;
    unsigned char **chunks;
    char *torrent_path, *stripped_path;
    char host_buf[HOST_MAX];

    if (path == NULL) {
        return -1;
    }
    stripped_path = strip_path(path);

    if ((fd = open(path, O_RDONLY)) == -1) {
        perror("open");
        return -1;
    }
    
    // create the .torrent file.
    torrent_path = (char*) malloc(strlen(stripped_path)+strlen(".torrent")+1);
    sprintf(torrent_path, "%s.torrent", stripped_path);
    if ((tfd = creat(torrent_path, 0644)) == -1) {
        perror("creat");
        free(torrent_path);
        return -1;
    }
    
    // calculate the number of chunks, and allocate space for them
    fstat(fd, &st);
    num_chunks = (int32_t)ceil((double)st.st_size/(double)CHUNK_SIZE);
    chunks = malloc(sizeof(char*) * num_chunks);

    // calculate the hashes
    gcry_md_open(&hd, GCRY_MD_SHA1, 0);
    for(i=0;i<num_chunks;i++) {
        num_read = read(fd, (void*)buffy, CHUNK_SIZE);
        gcry_md_hash_buffer(GCRY_MD_SHA1, hash, buffy, num_read); 
        copy_hash(hash, chunks+i);
        gcry_md_write(hd, (void*)buffy, num_read);
    }

    // sanity check: we should be at EOF...
    assert(read(fd, (void*)buffy, CHUNK_SIZE) == 0);

    memset(host_buf, 0, HOST_MAX);
    strcpy(host_buf, host);
    copy_hash(gcry_md_read(hd, GCRY_MD_SHA1), &file_hash);

    // make the actual hash file.
    write(tfd, (void*)host_buf, HOST_MAX);
    write(tfd, (void*)file_hash, SHA1_SIZE);
    write(tfd, (void*)&num_chunks, sizeof(num_chunks)); 
    for(i=0; i<num_chunks; i++) {
        write(tfd, (void*)chunks[i], SHA1_SIZE);
        free(chunks[i]);
    }
    write(tfd, (void*)stripped_path, strlen(stripped_path));

    free(chunks);
    free(torrent_path);
    free(file_hash);
    gcry_md_close(hd);
    close(fd);
    close(tfd);
    return 0;
}

void copy_hash(unsigned char *src, unsigned char **dst) {
    *dst = (unsigned char *) malloc(SHA1_SIZE);
    memcpy((void*)*dst, (void*)src, SHA1_SIZE);
}

void print_hash(unsigned char *hash) {
    int i;
    unsigned char *p = hash;

    for (i=0; i < SHA1_SIZE; i++, p += 2) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

char *strip_path(char *path) {
    char *p;
    if (strchr(path, '/') == NULL)
        p = path;
    else
        p = strrchr(path, '/') + 1;
    return p;
}

void parse_torrent(char *path, torrent *t) {
    int fd, i;
    if((fd = open(path, O_RDONLY)) == -1) {
        perror("open");
        return;
    }
    memset(t, 0, sizeof(*t));
    read(fd, (void*)t->tracker, HOST_MAX);
    read(fd, (void*)t->file_hash, SHA1_SIZE);   
    read(fd, (void*)&t->num_chunks, sizeof(t->num_chunks)); 
    t->chunks = malloc(sizeof(unsigned char*) * t->num_chunks);
    for(i=0;i<t->num_chunks;i++) {
        t->chunks[i] = (unsigned char *) malloc(SHA1_SIZE);
        read(fd, (void*)t->chunks[i], SHA1_SIZE);
    }
    read(fd, (void*)t->friendly_name, PATH_MAX);
    close(fd);
}

void dump_torrent(torrent t) {
    int i;
    printf("%s: ", t.friendly_name);
    print_hash(t.file_hash);
    printf("%s\n", t.tracker);
    printf("%d chunk(s)\n", t.num_chunks);
    for(i=0;i<t.num_chunks;i++) {
        printf("%d: ", i+1);
        print_hash(t.chunks[i]);
    }
}

void free_torrent(torrent t) {
    int i;
    for(i=0; i<t.num_chunks; i++) {
        free(t.chunks[i]);
    }
    free(t.chunks);
}
