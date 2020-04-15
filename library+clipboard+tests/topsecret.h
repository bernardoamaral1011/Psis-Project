/**************************************************************************
 *                              topsecret.h                               *
 **************************************************************************                                                                   
 * Structs and defines that the application doesn't need to know          *
 *************************************************************************/
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define MSG_MAX 100 /* Maximum lenght of a message*/
#define SOCK_ADDRESS "./CLIPBOARD_SO"
#define COPY 0
#define PASTE 1
#define WAIT 2

/* Struct for data transmission*/
typedef struct _data
{
	int region;
	int copy_paste;
	size_t count;
	char buf[MSG_MAX];
} _data;

/*Struct which stores a region*/
typedef struct _region
{
	size_t count;
	void *buf;
} _region;

/*Struct that represents a linked list of sockets*/
typedef struct _sockList
{
	int value;
	struct _sockList *next;
} _sockList;
