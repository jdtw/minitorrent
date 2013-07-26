# Minitorrent

## Disclaimer
I haven't even looked at this code in years. I just found it sitting
on an old hard drive and figured I'd put it up. I don't even know if
it still compiles...

## What is it?
A toy implementation of a custom torrent implementation. Contains code
for the tracker and a client.

## Installation
- Run `make` to build the executables `minitorrent` and `minitracker`.
- `make clean` will remove all binaries
- `make cleant` will remove all .torrent files

## USAGE

### minitracker

#### Starting the tracker
`minitracker` will start the tracker.

### minitorrent 

#### Creating/displaying a torrent file
`minitorrent --create-torrent tracker file(s)` will create .torrent
files to be tracked by the tracker specified by 'tracker' for each
file in 'file(s).' `minitorrent --dump-torrent .torrent_file(s)` will
display the .torrent file(s) in a human readable form.

#### Uploading a file
`minitorrent --upload .torrent_file(s)` will tell the tracker specified
in the .torrent file that we are uploading the file described by the
.torrent file.

#### Downloading a file
`minitorrent --download .torrent_file` download's the file described
by the .torrent file.
#### Dumping the tracker
`minitorrent --tracker-dump tracker` will dump the tracker's hash
table of files being tracked, and the peers tracking those files.
But it does not send them to the client. The information is displayed
on whatever screen 'minitracker' was run on.

## Protocol

### Torrent files
A torrent file has the following format:
The name of the tracker (`HOST_MAX` [64] bytes)
The sha1 hash of the file (`SHA1_SIZE` [20] bytes)
The number of chunks as a 32-bit integer (4 bytes) 
The sha1 hash of each of the 256kb chunks of the file 
(`SHA1_SIZE` * num_chunks bytes)
The friendly name of the file (up to `PATH_MAX` bytes)

Example from torrent.c:
    
    write(tfd, (void*)host_buf, HOST_MAX);
    write(tfd, (void*)file_hash, SHA1_SIZE);
    write(tfd, (void*)&num_chunks, sizeof(num_chunks)); 
    for(i=0; i<num_chunks; i++) {
        write(tfd, (void*)chunks[i], SHA1_SIZE);
        free(chunks[i]);
    }
    write(tfd, (void*)stripped_path, strlen(stripped_path));
    
### Ports 
The minitracker runs on port 20000. Clients talk to the tracker on that
port. Clients talk to eachother on port 30000. (May add support to have 
user-specified ports later).

### Talking to the tracker
There are currently three commands that the client can send to the
tracker:

    #define TRACKER_CMD_UPLOAD 1
    #define TRACKER_CMD_DOWNLOAD 2
    #define TRACKER_CMD_DUMP 3

They are sent to the tracker in the following struct:

    typedef struct tracker_request {
        int cmd;
        unsigned char hash[SHA1_SIZE];
    } tracker_request;

Example from tracker.c: 

    int tracker_cmd(int cmd, char *host, unsigned char *fh) {
        int sfd;
        tracker_request rq;
        sfd = tracker_connect(host);
        rq.cmd = cmd;
        if (fh != NULL)
            memcpy(rq.hash, fh, SHA1_SIZE);
        write(sfd, (void *)&rq, sizeof(rq));
        return sfd;
    }
    
#### TRACKER_CMD_UPLOAD
The client sends this command to let the tracker know that it
is uploading a file. tracker_request.fh should be the sha1 hash
of the file to be uploaded

#### TRACKER_CMD_DOWNLOAD 
The client sends this command to let the tracker know that it
wants to download a file. tracker_request.fh should be the sha1
hashof the file to be downloaded.

#### TRACKER_CMD_DUMP 
This is a debugging command, and will be removed from the final
version. It causes the tracker to dump it's list of hashed files
and the peers downloading those files

### Listening to the tracker
#### Getting a peer-list
After the tracker receives a `TRACKER_CMD_DOWNLOAD` command, it sends
the following information to the client:
The number of peers uploading the file as a 32-bit int (4 bytes)
For each peer, it sends the host-name of that peer (`HOST_MAX` bytes)

Example from tracker.c:

    write(cfd, (void*)&npeers, sizeof(npeers));
    for(i=0; plist != NULL; plist = plist->next, i++) {
        write(cfd, (void*)plist->host, HOST_MAX);
    }
    assert(i==npeers);
    
### Client to client communication
#### Downloading a file
A download request is sent in the following struct:

    typedef struct download_request {
        unsigned char hash[SHA1_SIZE];
        int chunk;
    } download_request;

hash is the sha1 hash of the file you want to download,
chunk is the 0 indexed chunk.

The response will be the `CHUNK_SIZE` chunk of the file.
