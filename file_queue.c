#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "file_queue.h"

//! MUTEX MAKES SURE ENQUEUE AND DEQUEUE ARE THREAD SAFE AND CANNOT BE INTERRUPTED BY EACHOTHER

// Structure for the queue

// Function to initialize the queue
void initializeQueue(queue *queue)
{
	queue->front = NULL;
	queue->rear = NULL;
	pthread_mutex_init(&queue->mutex, NULL);
}

// Function to check if the queue is empty
int isEmpty(queue *queue)
{
	return queue->front == NULL;
}

// Function to enqueue a value into the queue
void enqueue(queue *queue, file_info value)
{
	pthread_mutex_lock(&queue->mutex);

	Node *newNode = (Node *)malloc(sizeof(Node));
	if (!newNode)
	{
		printf("Memory allocation failed. Cannot enqueue.\n");
		pthread_mutex_unlock(&queue->mutex);
		return;
	}

	newNode->data = value;
	newNode->next = NULL;

	if (isEmpty(queue))
	{
		queue->front = newNode;
		queue->rear = newNode;
	}
	else
	{
		queue->rear->next = newNode;
		queue->rear = newNode;
	}

	pthread_mutex_unlock(&queue->mutex);
}

// Function to dequeue a value from the queue
file_info dequeue(queue *queue)
{
	file_info removedItem;
	removedItem.file_name = NULL;

	pthread_mutex_lock(&queue->mutex);

	if (isEmpty(queue))
	{
		printf("Queue is empty. Cannot dequeue.\n");
		pthread_mutex_unlock(&queue->mutex);
		return removedItem;
	}

	Node *temp = queue->front;
	removedItem = temp->data;

	if (queue->front == queue->rear)
	{
		queue->front = NULL;
		queue->rear = NULL;
	}
	else
	{
		queue->front = queue->front->next;
	}

	free(temp);

	pthread_mutex_unlock(&queue->mutex);

	return removedItem;
}

// Function to destroy the queue
void destroyQueue(queue *queue)
{
	pthread_mutex_lock(&queue->mutex);

	while (!isEmpty(queue))
	{
		dequeue(queue);
	}

	pthread_mutex_unlock(&queue->mutex);

	pthread_mutex_destroy(&queue->mutex);
}
