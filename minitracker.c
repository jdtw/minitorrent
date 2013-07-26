#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tracker.h"

int main(int argc, char **argv) {
    if (argc==2) {
        if (strcmp(argv[1], "--stop")==0 || strcmp(argv[1], "-s")==0) {
            tracker_stop();
        } else {
            fprintf(stderr, "usage: minitracker [--stop]\n");
            exit(0);
        }
    } else if(argc == 1) {
        tracker_start();
    } else {
        fprintf(stderr, "usage: minitracker [--stop]\n");
        exit(0);
    }
    return 0;
}
