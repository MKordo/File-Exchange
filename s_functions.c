#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "queue.h"
#include "s_functions.h"

#define perror2(s, e) fprintf(stderr , "%s: %s\n", s, strerror (e))

//////////////////////////////////
enum { BLACK=0, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

extern int client_color;

//////////////////////////////////

extern QueueNode *queue;
extern int queue_size;
extern pthread_mutex_t queue_lock;
extern pthread_cond_t insert;
extern pthread_cond_t worker_cond;

// find ouy what it is written in command line
int cmd_arg_parser(int argc, char **argv, int *port, int *thread_pool_size, int *queue_size)
{
    int i;

    argc--;
    argv++;

    if(argc!=6) {
        printf("Wrong number of arguments\n");
        return -1;
    }

    for (i=0; i<6; i+=2) {
//    for (i=1; i<7; i+=2) {
        if (strcmp("-p",argv[i])==0) {
            *port = atoi(argv[i+1]);
        }
        else if (strcmp("-s",argv[i])==0) {
            *thread_pool_size = atoi(argv[i+1]);
        }
        else if (strcmp("-q",argv[i])==0) {
            *queue_size = atoi(argv[i+1]);
        }
        else {
            fprintf(stderr,"cmd_arg_parser: something went wrong\n");
            return -1;
        }
    }
    return 0;
}

////////////////////////////////
////////////////////////////////

int find_files(char* dirname, int socket, int *number, pthread_mutex_t *mtx)
{
        int return_code, i, err;
        DIR *dir_ptr;
        struct dirent entry;
        struct dirent *result;

        struct stat mybuf;
        char buf[256];

        if ((dir_ptr = opendir (dirname)) == NULL) {
                fprintf(stderr,"find files: couldn't open folder");
                return -1;
        }
        else {

                while( ( return_code = readdir_r(dir_ptr, &entry, &result) ) == 0 && result!=NULL) {

                        if (entry.d_ino==0) continue;
                        if ( strcmp(entry.d_name,".")==0 || strcmp(entry.d_name,"..")==0) continue;

                        strcpy(buf,dirname);
                        strcat(buf,"/");
                        strcat(buf, entry.d_name);

                        if ( stat (buf, & mybuf ) < 0) {
                                perror ( buf ) ; continue ; }

                        if (( mybuf.st_mode & S_IFMT ) == S_IFDIR )  // directory encountered
                        {
//                                printf("\ndirectory: %s\n", buf);
                                if ( find_files(buf, socket, number, mtx) == -1)
                                { fprintf(stderr,"find files recursion error"); return -1; }
                        }

                        else  // file encountered
                        {
                                QueueNode *node;
                                int filesize = (strlen(dirname)+strlen(entry.d_name)+2);

                                node = create_node();

                                node->number = number;
                                node->mutex = mtx;
                                node->clientSock = socket;
                                node->filesize = mybuf.st_size;
                                node->mode = mybuf.st_mode;

                                node->filename = malloc( filesize * sizeof(char) );
                                for (i=0; i<filesize; i++) node->filename[i] = '\0';
                                for (i=0; i<strlen(dirname); i++) node->filename[i] = dirname[i];
                                strcat(node->filename, "/");
                                strcat(node->filename, entry.d_name);

                                node->filename_size = filesize-1;

                                if (err = pthread_mutex_lock (& queue_lock )) {
                                    perror2 ("insert queue pthread_mutex_lock ", err ); exit (1); }

                                while(size_queue(queue) == queue_size) {
                                    pthread_cond_wait ( &insert , &queue_lock );
                                }

                                insert_queue(queue, node);
                                pthread_cond_signal ( &worker_cond );

                                color_text(client_color);
                                printf("[Client Thread: %ld]: Just added to queue: %s\n", pthread_self(), node->filename);
                                if (err = pthread_mutex_unlock (& queue_lock )) {
                                    perror2 ("insernt queue pthread_mutex_unlock ", err ); exit (1) ; }

//                                printf("file: %d,       %s/%s\n", mybuf.st_size ,dirname, entry.d_name);
                        }
                }
                closedir(dir_ptr);
        }
        return 0;
}

int count_files(char* dirname)
{
        int return_code, i;
        DIR *dir_ptr;
        struct dirent entry;
        struct dirent *result;

        struct stat mybuf;
        char buf[256];
        int num_of_files=0;

        if ((dir_ptr = opendir (dirname)) == NULL) {
                fprintf(stderr,"count files: couldn't open folder");
                return -1;
        }
        else {

                while( ( return_code = readdir_r(dir_ptr, &entry, &result) ) == 0 && result!=NULL) {

                        if (entry.d_ino==0) continue;
                        if ( strcmp(entry.d_name,".")==0 || strcmp(entry.d_name,"..")==0) continue;

                        strcpy(buf,dirname);
                        strcat(buf,"/");
                        strcat(buf, entry.d_name);

                        if ( stat (buf, & mybuf ) < 0) {
                                perror ( buf ) ; continue ; }

                        if (( mybuf.st_mode & S_IFMT ) == S_IFDIR )  // directory encountered
                        {
                                if ( (num_of_files += count_files(buf)) == -1)
                                { fprintf(stderr,"count files recursion error"); return -1; }
                        }
                        else  // file encountered
                        {
                            num_of_files++;
                        }
                }
                closedir(dir_ptr);
        }
        return num_of_files;
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
