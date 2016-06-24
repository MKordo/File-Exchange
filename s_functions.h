#ifndef SERVER_FUNCTIONS_H
#define SERVER_FUNCTIONS_H

#include <pthread.h>

int cmd_arg_parser(int, char **, int *, int *, int *);
int find_files(char*,int,int*,pthread_mutex_t*);
int count_files(char*);
void color_text(int);

#endif
