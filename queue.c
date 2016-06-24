#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "queue.h"

// Queue's first node will be just a pointer to the rest of the list
QueueNode* create_queue()
{
    return create_node();
}

QueueNode* create_node()
{
    QueueNode * node;
    int i;

    node = malloc(sizeof(QueueNode));
    if (node==NULL) { perror("error: create node\n"); return; }

    node->next=NULL;
    node->number = NULL;
    node->clientSock=-1;
    node->filesize=-1;
    node->filename_size=-1;
    node->filename = NULL;

    return node;
}

void insert_queue(QueueNode *parent, QueueNode *to_be_inserted)
{
    QueueNode *ptr;

    ptr = parent;
    while(ptr->next!=NULL)
    {
        ptr = ptr->next;
    }
    ptr->next = to_be_inserted;
}

void delete_queue(QueueNode* parent)
{
    QueueNode* tmp;

    while(parent!=NULL)
    {
        tmp = parent;
        parent = parent->next;
        delete_node2(tmp);
    }
    printf("\nQueue deleted...\n");
}

void delete_node2(QueueNode *node)
{
    free(node->filename);
    free(node->number);
    free(node->mutex);
    free(node);
}

void delete_node(QueueNode *node)
{
    free(node->filename);
    free(node);
}

QueueNode* remove_queue(QueueNode* parent)
{
    QueueNode *ptr;

    if(parent->next!=NULL) {

        ptr = parent->next;
        parent->next = (parent->next)->next;

        return ptr;
    }
    else return NULL;
}

void print_queue(QueueNode* parent)
{
    QueueNode* tmp;

    printf("Printing queue:\n");
    tmp = parent->next;
    while(tmp!=NULL)
    {
        print_node(tmp);
        tmp = tmp->next;
    }
    printf("\n");
}

void print_node(QueueNode* node)
{
    printf("%d: client socket: %d, size: %lu,	and filename: (%d) %s \n",
           node->number[0], node->clientSock, node->filesize, node->filename_size, node->filename);
}

int size_queue(QueueNode *parent)
{
    int size=0, err;
    QueueNode *tmp;

    tmp=parent->next;
    while(tmp!=NULL)
    {
        size++;
        tmp = tmp->next;
    }

    return size;
}

/*
int main(void)
{
	QueueNode *queue;
	QueueNode *node;

	queue = create_queue();
	print_node(queue);
	printf("queue size: %d\n",size_queue(queue));

	delete_queue(queue);
	return 0;
}
*/
