#ifndef QUEUE_H
#define QUEUE_H

#define NAMESIZE 100

typedef struct queue QueueNode;

struct queue {
    int clientSock;
    long filesize;
    int filename_size;
    char *filename;
    int *number;
    mode_t mode;
    pthread_mutex_t *mutex;
    QueueNode * next;
};

QueueNode* create_queue();
QueueNode* create_node();
void delete_node(QueueNode*);
void delete_node2(QueueNode*);
void insert_queue(QueueNode*, QueueNode*);
QueueNode* stop_id_from_queue(QueueNode*, int);
QueueNode* remove_queue(QueueNode*);
QueueNode* remove_from_running(pid_t);
void delete_node(QueueNode*);
void delete_queue(QueueNode*);
void print_queue(QueueNode*);
void print_node(QueueNode*);
int size_queue(QueueNode*);


#endif
