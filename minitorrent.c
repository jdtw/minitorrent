#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "torrent.h"
#include "tracker.h"
#include "uploadhash.h"
#include "client.h"

int is_torrent_file(char *fn) {
    char *ext = ".torrent";
    char *ptr = fn;
    int is_torrent = 0;
    while (strstr(ptr, ext))  {
        ptr = strstr(ptr, ext);
        if(strcmp(ptr, ext) == 0) {
            is_torrent = 1;
            break;
        }
        ptr += strlen(ext);
    }
    return is_torrent;
}

int main(int argc, char **argv) {
    int i;
    if (argc < 2) {
        fprintf(stderr, "usage: %s <command> [args]\n", argv[0]);
        exit(0);
    }

    if (strcmp(argv[1], "--create-torrent") == 0 || strcmp(argv[1], "-ct") == 0) {
        if (argc < 4) {
            fprintf(stderr, "usage: %s --create-torrent tracker [files]\n", argv[0]);
            exit(0);
        }
        for (i=3; i<argc; i++) {
            create_torrent(argv[i], argv[2]);
        }
    } else if (strcmp(argv[1], "--dump-torrent") == 0 || strcmp(argv[1], "-dt") == 0) {
        if (argc < 3) {
            fprintf(stderr, "usage: %s --dump-torrent [files]\n", argv[0]);
            exit(0);
        }
        torrent t;
        for (i=2; i<argc; i++) {
            if(!is_torrent_file(argv[i])) {
                fprintf(stderr, "%s is not a .torrent file\n", argv[i]);
                continue;
            }
            memset(&t, 0, sizeof(t));
            parse_torrent(argv[i], &t);
            dump_torrent(t);
            free_torrent(t);
        }
    } else if (strcmp(argv[1], "--upload") == 0 || strcmp(argv[1], "-u") == 0) {
        if (argc < 3) {
            fprintf(stderr, "usage: %s --upload file(s)\n", argv[0]);
            exit(0);
        }
        client_start();
        torrent t;
        for(i=2; i<argc; i++) {
            if(!is_torrent_file(argv[i])) {
                fprintf(stderr, "%s is not a .torrent file\n", argv[i]);
                continue;
            }
            parse_torrent(argv[i], &t);
            tracker_upload(t.tracker, t.file_hash);
            client_upload(t.friendly_name, t.file_hash);
            free_torrent(t);
        }
    } else if (strcmp(argv[1], "--stop-upload") == 0 || strcmp(argv[1], "-su") == 0) {
        client_stop();
    } else if (strcmp(argv[1], "--dump-upload") == 0 || strcmp(argv[1], "-du") == 0) {
        client_dump();
    } else if (strcmp(argv[1], "--tracker-dump") == 0 || strcmp(argv[1], "-td") == 0) {
        if (argc < 3) {
            fprintf(stderr, "usage: %s --tracker-dump tracker\n", argv[0]);
            exit(0);
        }
        tracker_dump(argv[2]);
    } else if (strcmp(argv[1], "--download") == 0 || strcmp(argv[1], "-d") == 0) {
        if (argc < 3) {
            fprintf(stderr, "usage: %s --download file(s)\n", argv[0]);
            exit(0);
        }
        torrent t;
        for(i=2; i<argc; i++) {
            if(!is_torrent_file(argv[i])) {
                fprintf(stderr, "%s is not a .torrent file\n", argv[i]);
                continue;
            }
            parse_torrent(argv[i], &t);
            client_download(t);
            free_torrent(t);
        }
    } else {
        fprintf(stderr, "invalid argument\n");
        exit(0);
    }

    return 0;
}

