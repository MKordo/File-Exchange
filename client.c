#include <sys/types.h>                                   /* For sockets */
#include <sys/socket.h>                                  /* For sockets */
#include <netinet/in.h>                         /* For Internet sockets */
#include <netdb.h>                                 /* For gethostbyname */
#include <stdio.h>                                           /* For I/O */
#include <stdlib.h>                                         /* For exit */
#include <string.h>                         /* For strlen, bzero, bcopy */
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define BLOCKSIZE sysconf (_SC_PAGESIZE)
#define DIR_MODE (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)

void make_directories(char *, mode_t);

int read_file(int, char *, long, mode_t);
int write_all(int, void *, size_t);
int read_all(int, void *, size_t);
int cmd_arg_parser(int, char **, struct hostent **, int *, char **);
void color_text(int);

int main(int argc, char **argv)
{
    int port, sock; char buf[BLOCKSIZE];
    unsigned int serverlen;
    struct sockaddr_in server;
    struct sockaddr *serverptr;
    struct hostent *rem;
    char *dir;

    int color=0;
    int name_len;
    long filesize;
    mode_t mode;
    pid_t pid;

   // Create socket
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket"); exit(1); }

    if( cmd_arg_parser(argc, argv, &rem, &port, &dir) == -1)
        return -1;
    printf("Client's parameters are:\n");
    printf("\tserver_ip: ");
    puts(rem->h_name);
    printf("\tserver_port: %d \n\tdirectory: %s\n\n", port, dir);

    server.sin_family = AF_INET; /* Internet domain */
    bcopy((char *) rem -> h_addr, (char *) &server.sin_addr, rem -> h_length);
    server.sin_port = htons(port); /* Server's Internet address and port */
    serverptr = (struct sockaddr *) &server;
    serverlen = sizeof server;

    if (connect(sock, serverptr, serverlen) < 0) { /* Request connection */
        perror("connect"); exit(1); }
    printf("Requested connection to host %s port %d\n", argv[1], port);

    name_len = strlen(dir);

    // Send length of name of directory
    if (write_all(sock, (void *) &name_len, sizeof(int)) < 0) {
        perror("client write"); exit(1); }

    // Send name of directory
    if (write_all(sock, dir, name_len) < 0) {
        perror("client write"); exit(1); }

    pid = getpid();
    color = 6;

////////////////////////////////////////////////////////

    while (1) {

        bzero(buf, BLOCKSIZE);
        filesize=0;
        name_len=0;

        // Read filename_size
        if (read_all(sock, &name_len, sizeof(int)) < 0) {
            perror("client read filename_size"); exit(1); }

        if(name_len<0) break;

        // Read filename
        if (read_all(sock, buf, name_len) < 0) {
            perror("client read filename"); exit(1); }

        // Read filesize
        if (read_all(sock, &filesize, sizeof(long)) < 0) {
            perror("client read filesize"); exit(1); }

        if (read_all(sock, &mode, sizeof(mode_t)) < 0) {
            perror("client read file mode"); exit(1); }

        make_directories(buf, DIR_MODE);

        if (read_file(sock, buf, filesize, mode) < 0) {
            perror("client read file!"); exit(1); }

        color_text(color);
        printf("[Client: %d received]: file: %s, size: %lu, mode: %o\n", pid, buf, filesize, mode);
        color_text(-1);
        fflush(stdout);
    }

    printf("\nExiting client...\n\n");
    color_text(-1);
    close(sock); exit(0);
    return 0;
}

int read_file(int sock, char *filename, long size, mode_t mode)
{
    int fp;
    char *buffer;
    long read, n;

    fp = open(filename, O_CREAT | O_WRONLY | O_TRUNC, mode);
    for(read=0, n=0; read < size; read+=n) {

        if ( read_all(sock, &n, sizeof(long)) < 0 ) {
            close(fp); fprintf (stderr ,"inside read file\n"); return -1; }

        if (( buffer = malloc(n*sizeof(char)) ) == NULL ) { close(fp); perror("client malloc"); return -1; }

        if ( read_all(sock, buffer, n) < 0 ) {
            close(fp); fprintf (stderr ,"inside read file\n"); return -1; }

        write(fp, buffer, n);
        free(buffer);
    }
    close(fp);
    return read;
}

int write_all(int fd, void *buff, size_t size)
{
    int sent, n;
    for(sent = 0; sent < size; sent+=n) {
        if ( ( n = write(fd, buff+sent, size-sent) ) == -1 )
             return -1;
    }
    return sent;
}

int read_all(int fd, void *buff, size_t size)
{
    int receive, n;
    for(receive = 0; receive < size; receive+=n) {
        if ( ( n = read(fd, buff+receive, size-receive) ) == -1 )
            return -1;
    }
    return receive;
}

void make_directories(char *path, mode_t mode)
{
    int i;
    int len = strlen(path);
    for (i = 0; i < len; i++) {
        if (path[i]=='/') {
            char *dir = malloc( (i+1) * sizeof(char));
            strncpy(dir, path, i);
            dir[i]='\0';
            mkdir(dir, mode);
            free(dir);
            i++;
        }
    }
//    mkdir(path, mode);
}

// find ouy what it is written in command line
int cmd_arg_parser(int argc, char **argv, struct hostent **server_ip, int *server_port, char **directory)
{
    int i;

    argc--;
    argv++;

    if(argc!=6) {
        printf("Wrong number of arguments\n");
        return -1;
    }

    for (i=0; i<6; i+=2) {
        if (strcmp("-i",argv[i])==0) {
            if ((*server_ip = gethostbyname(argv[i+1])) == NULL) {
                perror("gethostbyname"); exit(1); }
        }
        else if (strcmp("-p",argv[i])==0) {
            *server_port = atoi(argv[i+1]);
        }
        else if (strcmp("-d",argv[i])==0) {
            *directory = argv[i+1];
        }
        else {
            fprintf(stderr,"cmd_arg_parser: something went wrong\n");
            return -1;
        }
    }
    return 0;
}

void color_text(int color)
{
    if(color == -1)
    {
        printf("\x1B[0;39m");
        return;
    }
    if (color == 0 || color == 4) // use bold mode for black and blue
        printf("\x1B[1;%dm", 30 + color);
    else                          // use normal mode for everything else
        printf("\x1B[0;%dm", 30 + color);
}

