#include "common.h"

#define TRACK_FIFO ".tracker"

#define TRACKER_CMD_UPLOAD 1
#define TRACKER_CMD_DOWNLOAD 2
#define TRACKER_CMD_DUMP 3
#define TRACKER_CMD_STOP 4

typedef struct tracker_request {
    int cmd;
    unsigned char hash[SHA1_SIZE];
} tracker_request;

void tracker_start();
void tracker_stop();
void tracker_upload(char *, unsigned char *);
int tracker_download(char *, unsigned char *);
void tracker_dump(char *);
