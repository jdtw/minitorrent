#define MAXBACKLOG 5

int gethostname(char *, size_t);

int client_connect(char *, char *);

int server_listen(char *);

void getmyaddr(char *);
