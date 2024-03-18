#ifndef FILE_QUEUE_H
#define FILE_QUEUE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <dirent.h>
#include <limits.h>

typedef struct file_info
{
	char *file_name;
	size_t file_size;
	int array_number;
} file_info;

typedef struct Node
{
	file_info data;
	struct Node *next;
} Node;

typedef struct queue
{
	Node *front;
	Node *rear;
	pthread_mutex_t mutex;
} queue;

void initializeQueue(queue *queue);
int isEmpty(queue *queue);
void enqueue(queue *queue, file_info value);
file_info dequeue(queue *queue);
void destroyQueue(queue *queue);

#endif // FILE_QUEUE_H
