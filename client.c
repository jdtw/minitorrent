#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <linux/limits.h>
#include <signal.h>
#include "torrent.h"
#include "tracker.h"
#include "connections.h"
#include "uploadhash.h"
#include "client.h"

#define THREAD_MAX 4096

/* client_cmd sends a local command through the upload
   fifo to the upload daemon.
*/
static void client_cmd(int, char *, unsigned char *);

/* readchunk reads a size chunk of the file described by
   fd. It makes sure that all size bytes are read, as opposed
   to read, which does not make that gaurantee.
*/
static int readchunk(int fd, char *buffy, int size) {
    char *ptr = buffy;
    int n, total = 0, eof = 0;
    while(!eof && total < size) {
        n = read(fd, (void*)ptr, size-total);
        switch(n) {
        case -1:
            perror("read");
            exit(0);
        case 0:
            eof = 1;
            break;
        default:
            ptr += n;
            total += n;
            break;
        }
    }
    return total;
}


/* writechunk writes a size chunk of the file described at 
   fd. It makes sure that all size bytes are written, as opposed
   to write, which does not make that gaurantee.
*/
static int writechunk(int fd, char *buffy, int size) {
    char *ptr = buffy;
    int n, total = 0, eof = 0;
    while(!eof && total < size) {
        n = write(fd, (void*)ptr, size-total);
        switch(n) {
        case -1:
            perror("write");
            exit(0);
        case 0:
            eof = 1;
            break;
        default:
            ptr += n;
            total += n;
            break;
        }
    }
    return total;
}

typedef struct thread_arg_send {
    int cfd;
    int read_pipefd;
    download_request rq;
} thread_arg_send;

void *send_chunk(void *arg) {
    thread_arg_send *targ = (thread_arg_send*)arg;
    int fd, total;
    char buffy[CHUNK_SIZE];
    char file[PATH_MAX];
    unsigned char hash[SHA1_SIZE];
    memcpy((void*)hash, (void*)targ->rq.hash, SHA1_SIZE);

    // send a request to the table manager to find the filename
    client_cmd(UPLOAD_CMD_FIND, NULL, hash);
    memset(file, 0, PATH_MAX);
    read(targ->read_pipefd, (void*)file, PATH_MAX);

    // open the file to read a chunk starting at chunk*CHUNK_SIZE
    fd = open(file, O_RDONLY);
    if (fd < 0) {
        perror("open");
        fprintf(stderr, "send_chunk: couldn't open %s\n", file);
        exit(0);
    }
    if (lseek(fd, CHUNK_SIZE*targ->rq.chunk, SEEK_SET) < 0) {
        perror("lseek");
        exit(0);
    }

    // now read it and send it to the client who requested it.
    total = readchunk(fd, buffy, CHUNK_SIZE);
    total = writechunk(targ->cfd, buffy, total);
    close(fd);
    close(targ->cfd);
    pthread_exit(NULL);
}

static int stop_serving = 0;    

// a handler to intercept the SIGINT signal sent
// by upload_daemon to stop the upload_server.
void handler(int n) {
    stop_serving = 1;
}

void upload_server(int read_pipefd) {
    int sfd, cfd;
    sfd = server_listen(CLIENT_PORT);
    pthread_t threads[THREAD_MAX];
    thread_arg_send args[THREAD_MAX];
    int num_threads = 0;
    signal(SIGINT, handler);
    while (!stop_serving) {
        cfd = accept(sfd, NULL, NULL);
        args[num_threads].read_pipefd = read_pipefd;
        args[num_threads].cfd = cfd;
        // read the request from cfd into the thread argument
        read(cfd, (void*)&args[num_threads].rq, sizeof(args[num_threads].rq));
        // create a thread to send a chunk back to cfd
        pthread_create(&threads[num_threads], NULL, send_chunk, (void*)&args[num_threads]);
        num_threads++;
    }
    close(sfd);
    close(read_pipefd);
    upload_free_table();
}

void upload_daemon() {
    int pipefd[2];
    int pid;
    int stop;
    upload_local_request ur;
    char *filename;
    int sfd, cfd;
    sfd = server_listen(LOCAL_PORT);
    for (stop=0; !stop; ) {
        cfd = accept(sfd, NULL, NULL);
        if (cfd < 0) {
            perror("upload_daemon: accept");
            exit(0);
        }
        memset((void*)&ur, 0, sizeof(ur));
        read(cfd, (void*)&ur, sizeof(ur));
        switch(ur.cmd) {
        case UPLOAD_CMD_START:
            upload_init_table();
            // this pipe is used to send the results of upload_find_hash
            // to the server:
            pipe(pipefd);
            // fork the upload server here so we can kill it
            // when we receive the UPLOAD_CMD_STOP_ALL command.
            if ((pid = fork()) == 0) {
                close(pipefd[1]);
                upload_server(pipefd[0]);
                exit(0);
            }
            close(pipefd[0]);
            break;
        case UPLOAD_CMD_STOP_ALL:
            // kill the server.
            kill(pid, SIGINT);
            close(pipefd[1]);
            close(cfd);
            stop=1;
            break;
        case UPLOAD_CMD_UPLOAD:
            upload_add_hash(ur.name, ur.hash);
            break;
        case UPLOAD_CMD_DUMP:
            upload_dump_table();
            break;
        case UPLOAD_CMD_FIND:
            filename = upload_find_hash(ur.hash);
            // send the results to the upload server.
            write(pipefd[1], (void*)filename, PATH_MAX);
            break;
        default:
            break;
        }
        close(cfd);
    }
}

static void client_cmd(int cmd, char *fn, unsigned char *fh) {
    int fd;
    char me[HOST_MAX];
    upload_local_request ur;
    getmyaddr(me);
    fd = client_connect(me, LOCAL_PORT);
    if(fd < 0) {
        fprintf(stderr, "client_cmd: could not connect to %s\n", me);
        exit(0);
    }
    memset((void*)&ur, 0, sizeof(ur));
    ur.cmd = cmd;
    if (fn) {
        strcpy(ur.name, fn);
    }
    if (fh) {
        memcpy((void*)ur.hash, (void*)fh, SHA1_SIZE);
    }
    write(fd, (void*)&ur, sizeof(ur));
    close(fd);
}

void client_start() {
    char me[HOST_MAX];
    int cfd;
    getmyaddr(me);
    if((cfd = client_connect(me, LOCAL_PORT)) < 0) {
        if(fork() == 0) {
            upload_daemon();
            exit(0);
        }
        while((cfd = client_connect(me, LOCAL_PORT)) < 0) continue;
        close(cfd);
        client_cmd(UPLOAD_CMD_START, NULL, NULL);
    } else {
        close(cfd);
    }
}

void client_upload(char *fn, unsigned char *fh) {
    client_cmd(UPLOAD_CMD_UPLOAD, fn, fh);
}

void client_dump() {
    client_cmd(UPLOAD_CMD_DUMP, NULL, NULL);
}

void client_stop() {
    client_cmd(UPLOAD_CMD_STOP_ALL, NULL, NULL);
}

typedef struct thread_arg_receive {
    int fd;
    int chunk;
    int buffy_size;
    char buffy[CHUNK_SIZE];
} thread_arg_receive;

void *receive_chunk(void *arg) {
    thread_arg_receive *targ = (thread_arg_receive*)arg;
    int total = readchunk(targ->fd, targ->buffy, CHUNK_SIZE);
    targ->buffy_size = total;
    close(targ->fd);
    pthread_exit(NULL);
}

static int new_outfd(char *fn) {
    struct stat sbuf;
    char new[PATH_MAX];
    int i;
    strcpy(new, fn);
    for (i=1; stat(new, &sbuf) == 0; i++) {
        memset(new, 0, PATH_MAX);
        sprintf(new, "(%d)%s", i, fn);
    }
    return creat(new, 0666);
}
    
void client_download(torrent t) {
    download_request dr;
    int sfd = tracker_download(t.tracker, t.file_hash);
    pthread_t *threads;
    thread_arg_receive *args;
    int i, j, outfd, peeri, valid_peers, npeers;
    char **peers;
    if (read(sfd, (void*)&npeers, sizeof(npeers)) < 0) {
        perror("read");
        exit(0);
    }
    if (npeers == 0) {
        return;
    }
    peers = malloc(sizeof(*peers) * npeers);
    threads = malloc(sizeof(*threads) * t.num_chunks);
    args = malloc(sizeof(*args) * t.num_chunks);
    
    // read the peers from the tracker
    for (i=0;i<npeers;i++) {
        peers[i] = (char*) malloc(HOST_MAX+1);
        memset(peers[i], 0, HOST_MAX+1);
        read(sfd, (void*)peers[i], HOST_MAX);
    }

    outfd = new_outfd(t.friendly_name); 
    if (outfd < 0) {
        perror("creat");
        exit(0);
    }

    // send a request for each chunk
    memcpy(dr.hash, t.file_hash, SHA1_SIZE);
    valid_peers = npeers;
    for (j=0; j<t.num_chunks; j+=npeers*CHUNKS_PER_PEER) {
        for (i=j; i<j+npeers*CHUNKS_PER_PEER && i<t.num_chunks; i++) {
            peeri = i%npeers;
            while (peers[peeri] == NULL) peeri = (peeri+1)%npeers;
            dr.chunk = i;
            args[i].chunk = i;
            args[i].fd = client_connect(peers[peeri], CLIENT_PORT);
            if (args[i].fd < 0) { // if we couldn't connect to the peer...
                fprintf(stderr, "client_download: could not connect to %s, removing it for the peer list\n", peers[peeri]);
                free(peers[peeri]);
                peers[peeri] = NULL;
                valid_peers--;
                if (valid_peers == 0) { 
                    fprintf(stderr, "client_download: couldn't connect to any peers!\n");
                    exit(0);
                }
                // try getting the chunk from another peer
                i--;
                continue;
            }
            write(args[i].fd, (void*)&dr, sizeof(dr));
            pthread_create(&threads[i], NULL, receive_chunk, (void*)&args[i]);
        }
        for(i=j; i<j+npeers*CHUNKS_PER_PEER && i<t.num_chunks; i++) {
            pthread_join(threads[i], NULL);
            // TODO: verify the hash
            writechunk(outfd, args[i].buffy, args[i].buffy_size);
        }
    }
    close(outfd);
    close(sfd);
    for (i=0; i<npeers; i++) {
        free(peers[i]);
    }
    free(peers);
    free(threads);
    free(args);
}
