#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h>
#include "connections.h"
#include "tracker.h"
#include "trackerhash.h"

static int tracker_cmd(int, char *, unsigned char *);

void tracker_start() {
    // run in background
    if (fork() == 0) {
        int sfd, cfd, s, stop;
        peer *plist;
        sfd = server_listen(TRACKER_PORT);
        tracker_init_table();
        struct sockaddr p;
        socklen_t plen = sizeof(p);

        for (stop=0; !stop; ) {
            char host[HOST_MAX+1];
            tracker_request rq;
            int i, npeers;

            // accept a connection and read the request
            cfd = accept(sfd, (struct sockaddr *) &p, &plen);
            read(cfd, (void*)&rq, sizeof(rq));

            // find out who we are talking to...
            if((s = getnameinfo((struct sockaddr *) &p, sizeof(peer), host, HOST_MAX, NULL, 0, 0)) <0) {
                fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
            }

            switch(rq.cmd) {
            case TRACKER_CMD_UPLOAD:
                tracker_add_hash(host, rq.hash);
                break;
            case TRACKER_CMD_DOWNLOAD:
                plist = tracker_find_hash(rq.hash);
                npeers = (plist) ? plist->npeers : 0;
                // write the number of peers
                write(cfd, (void*)&npeers, sizeof(npeers));
                // and then write each of the peer hostnames
                for(i=0; plist != NULL; plist = plist->next, i++) {
                    write(cfd, (void*)plist->host, HOST_MAX);
                }
                assert(i==npeers); // sanity check...
                break;
            case TRACKER_CMD_DUMP:
                // mostly a debugging command since the clients who send this request cannot
                // see the results...
                tracker_dump_table();
                break;
            case TRACKER_CMD_STOP:
                stop = 1;
                break;
            default:
                fprintf(stderr, "TRACKER: invalid command\n");
                exit(0);
                break;
            }
            close(cfd);
        }
        close(sfd);
        tracker_free_table();
        exit(0);
    }
}

void tracker_stop() {
    char self[HOST_MAX];
    gethostname(self, HOST_MAX);
    tracker_cmd(TRACKER_CMD_STOP, self, NULL);
}

static int tracker_cmd(int cmd, char *host, unsigned char *fh) {
    int sfd;
    tracker_request rq;
    sfd = client_connect(host, TRACKER_PORT);
    if (sfd < 0) {
        fprintf(stderr, "tracker_cmd: could not connect to tracker %s\n", host);
        exit(0);
    }
    rq.cmd = cmd;
    if (fh != NULL)
        memcpy(rq.hash, fh, SHA1_SIZE);
    write(sfd, (void *)&rq, sizeof(rq));
    return sfd;
}
void tracker_upload(char *host, unsigned char *fh) {
    int sfd = tracker_cmd(TRACKER_CMD_UPLOAD, host, fh);
    close(sfd);
}

void tracker_dump(char *host) {
    int sfd = tracker_cmd(TRACKER_CMD_DUMP, host, NULL);
    close(sfd);
}

int tracker_download(char *host, unsigned char *fh) {
    return tracker_cmd(TRACKER_CMD_DOWNLOAD, host, fh);
}


