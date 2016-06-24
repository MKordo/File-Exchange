#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>                                         /* strerror */

#include <sys/types.h>                                   /* For sockets */
#include <signal.h>
#include <sys/socket.h>                                  /* For sockets */
#include <netinet/in.h>                         /* For Internet sockets */
#include <netdb.h>                                 /* For gethostbyaddr */
#include <fcntl.h>
#include <pthread.h>

#include "s_functions.h"
#include "queue.h"

#define perror2(s, e) fprintf(stderr , "%s: %s\n", s, strerror (e))
#define BLOCKSIZE sysconf (_SC_PAGESIZE)

//////////////////////////////////
enum { BLACK=0, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

int worker_color = BLACK;
int client_color = CYAN;
int beg_end_color = RED;

/*
int worker_color = -1;
int client_color = -1;
int beg_end_color = -1;
*/
//////////////////////////////////
void * worker_thread();
void * client_thread(void *);

int write_file(int, char*, size_t);
int write_all(int, void *, size_t);
int read_all(int, void *, size_t);
void end();

QueueNode *queue;
int queue_size;
int tps;
int flag=1;
int sock;
pthread_t *worker_threads;

pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;     // Mutex for queue
pthread_cond_t insert;                                      // Condition for not inserting into queue when its full
pthread_cond_t worker_cond;                                 // Stop worker thread when there's nothing to do

int main(int argc, char **argv)
{
//    int tps=-1;                                  // thread_pool_size
    int port, newsock;
    unsigned int serverlen, clientlen;
    struct sockaddr_in server, client;
    struct sockaddr *serverptr, *clientptr;
    struct hostent *rem;
    QueueNode *node;

    pthread_t worker, t1;

    int errnum, i;
    int client_num=0;

    // block_sz = sysconf (_SC_PAGESIZE);           // block size
    queue = create_queue();
    pthread_cond_init ( &insert , NULL);
    pthread_cond_init ( &worker_cond, NULL );
    setbuf(stdout, NULL);                           // printf immidiately

    if( cmd_arg_parser(argc, argv, &port, &tps, &queue_size) == -1)
        return -1;

    color_text(beg_end_color);                      // color text
    printf("Server's parameters are:\n");
    printf("\tport: %d\n\ttps: %d\n\tqueue size: %d\n\n",port, tps, queue_size);

    if ( ( worker_threads = malloc( tps*sizeof(pthread_t)) ) == NULL)
         { perror("worker array -> malloc error "); }

    for (i=0; i<tps; i++) {
        if ( errnum = pthread_create(&worker_threads[i], NULL, worker_thread, NULL) ) {
            fprintf (stderr , "worker create: %s\n", strerror (errnum) );
        }
    }

    signal(SIGTERM, end);

    // create socket
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket"); exit(1); }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);  // My Internet address
    server.sin_port = htons(port);               // The given port
    serverptr = (struct sockaddr *) &server;
    serverlen = sizeof server;

    if (bind(sock, serverptr, serverlen) < 0) {  // Bind socket to address
        perror("bind"); exit(1); }

    if (listen(sock, 15) < 0) {                  // Listen for connections
        perror("listen"); exit(1); }
    printf("Listening for connections to port %d\n", port);
    printf("Waiting for clients...\n");

//    while(client_num<4) {
    while(flag==1) {

        clientptr = (struct sockaddr *) &client;
        clientlen = sizeof client;

        if ((newsock = accept(sock, clientptr, &clientlen)) < 0) {
            perror("accept"); exit(1); } /* Accept connection */

         // Find client's address
         if ((rem = gethostbyaddr((char *) &client.sin_addr.s_addr,
              sizeof client.sin_addr.s_addr, client.sin_family)) == NULL) {
             perror("gethostbyaddr"); exit(1); }

         color_text(BLACK);
         printf("\nAccepted connection from %s\n\x1B[0;33mFor source release termination please send SIGTEMR with \"kill %d\"...\n\n", rem -> h_name, getpid());
//         color_text(3); printf("\x1B[0;%dm", 30 + color);
//         printf("For source release termination please send SIGTEMR with \"kill %d\"...\n\n", getpid());

         if ( ( node = malloc(sizeof(QueueNode)) ) == NULL)
         { perror("while -> malloc error "); }

         node->clientSock = newsock;

         if ( ( errnum = pthread_create( &t1, NULL, client_thread, (void *) node ) ) != 0)
         { delete_node(node); fprintf(stderr, "%s\n", strerror(errnum)); }

         client_num++;
    }

    end();
    return 0;
}

void end()
{
    int err, status;
    int i;

    color_text(beg_end_color);

    close(sock);

    flag=0;
    pthread_cond_broadcast(& worker_cond);
    sleep(1);

    printf("\n\n");
    for (i=0; i<tps; i++) {
//        if ( err = pthread_cancel(worker_threads[i]) ) {
//            fprintf (stderr , "worker cancel: %s\n", strerror (err) );
//        }

        if ( err = pthread_join(worker_threads[i], (void **) &status) ) {
            fprintf (stderr , "worker join: %s\n", strerror (err) );
        }
        printf ("Worker thread %ld exited with status: %d\n", worker_threads[i], status);
    }
    free(worker_threads);

    if (err = pthread_mutex_destroy (& queue_lock )) {
        perror2 (" pthread_mutex_destroy ", err ); exit (1) ; }

    if (err = pthread_cond_destroy (& insert )) {
        perror2 (" pthread_cond_destroy ", err ); exit (1) ; }

    if (err = pthread_cond_destroy (& worker_cond)) {
        perror2 (" pthread_cond_destroy ", err ); exit (1) ; }

    delete_queue(queue);
    color_text(-1);
    exit(1);
}

void worker_cleanup(void *node)
{
    if (node!=NULL)
        delete_node2(node);
    pthread_mutex_unlock (& queue_lock );
}

void * worker_thread()
{
    QueueNode *node;
    int current_size=0;
    int err ;
//    if ( err = pthread_detach ( pthread_self () ) ) { /* Detach thread */
//        fprintf (stderr ,"worker pthread_detach: %s\n", strerror (err));
//        exit (1);
//    }

    pthread_cleanup_push(worker_cleanup, (void *) node);

    while (flag==1) {

        // Lock queue mutex
        if (err = pthread_mutex_lock (& queue_lock )) {
            perror2 ("worker thread pthread_mutex_lock queue", err ); exit (1); }

            while ( size_queue(queue) == 0 ) {
                pthread_cond_wait ( &worker_cond , &queue_lock );

                if (flag == 0) {                       // termination time
                    if (err = pthread_mutex_unlock (& queue_lock )) {
                        perror2 ("worker thread pthread_mutex_unlock queue", err ); exit (1) ; }
                    break;
                }
            }
            if (flag == 0) break;

            node = remove_queue(queue);                // Remove node to send
            pthread_cond_signal (& insert );           // If queue was full, new files will be inserted

        if (err = pthread_mutex_unlock (& queue_lock )) {
            perror2 ("worker thread pthread_mutex_unlock queue", err ); exit (1) ; }

            color_text(worker_color);
            printf("[Worker Thread: %ld]: socket: %d, size: %lu, and filename: %s \n",
                    pthread_self(), node->clientSock, node->filesize, node->filename);

        if (err = pthread_mutex_lock ( node->mutex )) {
            perror2 ("worker thread pthread_mutex_lock ", err ); exit (1); }

            // Sent file related information
            if (write_all(node->clientSock, &node->filename_size, sizeof(int)) < 0) {
                fprintf (stderr ,"worker thread write filename_size\n"); exit(1); }

            if (write_all(node->clientSock, node->filename, node->filename_size) < 0) {
                fprintf (stderr ,"worker thread write filename\n"); exit(1); }

            if (write_all(node->clientSock, &node->filesize, sizeof(long)) < 0) {
                fprintf (stderr ,"worker thread write filename\n"); exit(1); }

            if (write_all(node->clientSock, &node->mode, sizeof(mode_t)) < 0) {
                fprintf (stderr ,"worker thread write file mode\n"); exit(1); }

            if ( write_file(node->clientSock, node->filename, node->filesize) < 0 ){
                fprintf (stderr ,"worker thread write file!!\n"); exit(1); }

            // One less file to the list
            node->number[0] = node->number[0]-1;

        if (err = pthread_mutex_unlock ( node->mutex )) {
            perror2 ("worker thread pthread_mutex_unlock ", err ); exit (1); }

            if(node->number[0]==0) {
                int close_client=-1;
                free(node->number);
                if (write_all(node->clientSock, &close_client, sizeof(int)) < 0) {
                    fprintf (stderr ,"worker thread write filename_size\n"); exit(1); }
                close(node->clientSock);

                if ( pthread_mutex_destroy(node->mutex) )
                { fprintf (stderr ,"worker thread mutex destroy\n"); exit(1); }
                free(node->mutex);
            }
            delete_node(node);
            node = NULL;
    }

    pthread_cleanup_pop(0);
    pthread_exit ( NULL );
}



void * client_thread(void *n)
{
    QueueNode *node = (QueueNode *) n;
    int namesize;
    int err, i;
    int *filenum;
    pthread_mutex_t *mtx;

    if (err = pthread_detach ( pthread_self ())) {/* Detach thread */
        free(n);
        fprintf (stderr ,"client pthread_detach: %s\n", strerror (err));
        exit (1);
    }

    // Read size of namefile
    if (read_all(node->clientSock, &namesize, sizeof(int)) < 0) {
        fprintf (stderr ,"client thread read\n"); exit(1); }

//    printf("Size of name: %d\n", namesize);

    // Allocate space for the name
    if ( ( node->filename = malloc((namesize+1)*sizeof(char)) ) == NULL)
    { fprintf (stderr ,"client malloc\n"); exit(1); }
    for (i=0; i<namesize+1; i++) node->filename[i] = '\0';

    // Read name of requested directory
    if (read_all(node->clientSock, node->filename, namesize) < 0) {
        fprintf (stderr ,"client thread read\n"); exit(1); }

//    printf("Just read: %s\n", node->filename);

    if ( (filenum = malloc(sizeof(int)) ) == NULL )
    { fprintf (stderr ,"client malloc\n"); exit(1); }
    filenum[0] = count_files(node->filename);

    if ( (mtx = malloc(sizeof(pthread_mutex_t)) ) == NULL )
    { fprintf (stderr ,"client mutex malloc\n"); exit(1); }
    pthread_mutex_init(mtx,NULL);

    color_text(client_color);
    printf("[Client Thread: %ld]: socket: %d, directory name: %s \n",
             pthread_self(), node->clientSock, node->filename);

    if ( find_files(node->filename, node->clientSock, filenum, mtx) == -1) {
        free(node);
        free(filenum);
        delete_queue(queue);
        exit (1);
    }

//    print_queue(queue);

    delete_node(node);
//    printf("[Client Thread: %ld]: Exiting client thread...\n", pthread_self());
    return NULL;
}

int write_file(int sock, char *namefile, size_t size)
{
    int fp;
    char buffer[BLOCKSIZE];
    long sent, n;

    fp = open(namefile, O_RDONLY);
    for(sent=0, n=0; sent < size; sent+=n) {
        bzero(buffer,BLOCKSIZE);
        n = read(fp, buffer, BLOCKSIZE);

//        printf("%d read\n",n); fflush(stdout);

        if (write_all(sock, &n, sizeof(long)) < 0) {
            fprintf (stderr ,"inside write file\n"); return -1; }

        if (write_all(sock, buffer, n) < 0) {
            fprintf (stderr ,"inside write file\n"); return -1; }
    }
    close(fp);
//    printf("Got out\n");
    return sent;
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



