/**************************************************************************
 *                              clipboard.c                               *
 **************************************************************************                                                                   
 * Clipboard application that consists of ten regions synchronized and    *
 * modifiable over multiple clipboards and application, respectively      *
 * through INET domain sockets and Unix domain sockets.					  *
 * Applications can copy, paste and wait for a region's content to change *
 *************************************************************************/
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/wait.h>
#include "clipboard.h"
#include "topsecret.h"

/* Array that holds all local regions*/
struct _region regions[10];

/* List of socket fd's of the clipboards connected from below*/
struct _sockList *iniSockList = NULL;

/* Array for the locks when accessing regions */
pthread_rwlock_t rwlock[10];

/* File descriptor of the upper clipboard*/
int net_sock_fd = -1;

/******************************************************************************
* ctrl_c_callback_handler - Deals with the ctrl C signal, so that we can launch a new clipboard afterwards with no problems *
******************************************************************************/

void ctrl_c_callback_handler()
{
	printf("Caught signal Ctr-C\n");
	int i;
	for (i = 0; i < 10; i++)
	{
		free(regions[i].buf);
	}
	
	struct _sockList *aux;
	while(iniSockList!=NULL){
		aux = iniSockList->next;
		free(iniSockList);
		iniSockList = aux;
	}
	
	close(net_sock_fd);
	unlink(SOCK_ADDRESS);
	exit(0);
}

void printClipboard()
{
	int i;
	printf("\n----- Clipboard -----\n");
	for (i = 0; i < 10; i++)
	{
		printf("Region[%d]: %s, size: %ld\n", i, (char *)regions[i].buf, regions[i].count);
	}
}

/******************************************************************************
* clipboard_receiver - Deals with the communication between this clipboard and one another that was previous linked to this one
*                                        
*
* Receives: void *remote_fdd - file descriptor of the previous accepted clipboard *
******************************************************************************/

void *clipboard_receiver(void *remote_fdd)
{
	int *pin = (int *)remote_fdd;
	int nbytes, client_fd = *pin;
	struct _sockList *aux;
	struct _sockList *iterator;
	void *buf = NULL;
	long int count = 0;
	printf("My remote fd: [%d]\n", client_fd);

	while (1)
	{
		struct _data d;
		char *msg = (char *)malloc(sizeof(struct _data));
		if (msg == NULL)
		{
			printf("Malloc failed\n");
			return 0;
		}

		printf("WAITING TO RECEIVE FROM %d\n", client_fd);
		if ((nbytes = recv(client_fd, msg, sizeof(struct _data), 0)) == -1)
		{
			perror("Recv");
			exit(-1);
		}
		printf("RECEIVED FROM %d\n", client_fd);
		fflush(stdout);
		if (nbytes == 0)
		{
			printf("Clipboard [%d] closed. Closing this thread.\n", client_fd);
			fflush(stdout);
			/* Iterates through the connected clipboard list, and erases the socket of the closed clipboard*/
			if (client_fd == net_sock_fd)
			{
				net_sock_fd = -1;
				return 0;
			}
			
			if(iniSockList->value == client_fd){
				aux = iniSockList;
				iniSockList = iniSockList->next;
				free(aux);
			}else{
				for (iterator = iniSockList; iterator->next != NULL; iterator = iterator->next)
				{
					if (iterator->next->value == client_fd)
					{
						aux = iterator->next->next;
						free(iterator->next);
						iterator->next = aux;
						break;
					}
				}
			}
			return 0;
		}

		/* From bystream to data struct*/
		memcpy(&d, msg, sizeof(struct _data));

		/* Store in the corresponding region */
		if (d.copy_paste == COPY)
		{
			/*If this thread is not dealing with the upper clipboard and is not the root
			* it will forward everything only upwards and do no updates */
			if (net_sock_fd != -1 && client_fd != net_sock_fd)
			{
				printf("I received info from clipboard below\nIm not root I will send up\n");
				printf("\nInfo is buf:[%s] region:[%d]\n", d.buf, d.region);
				printf("Waiting for message from upwards so I can update\n");
				if (send(net_sock_fd, msg, sizeof(struct _data), 0) == -1)
				{
					perror("Send");
					exit(-1);
				}
			}else{ 
				/* If it is the root or received update from upper node,
				 * it will update and send everything to all the list downwards */
				if(net_sock_fd == -1 && client_fd != net_sock_fd){
					printf("I received info from clipboard below\nIm root i will update and send down\n");
				}else if(client_fd == net_sock_fd){
					printf("I received info from clipboard above\nWill update and send down\n");
				}

				/* Iterates through the connected clipboard list, and updates them*/
				for (iterator = iniSockList; iterator != NULL; iterator = iterator->next)
				{
					if (send(iterator->value, msg, sizeof(struct _data), 0) == -1)
					{
						perror("Send");
						exit(-1);
					}
				}

				buf = malloc(d.count);
				memcpy(buf, d.buf, d.count);
				/* Optimized write lock */
				pthread_rwlock_wrlock(&rwlock[d.region]);
				/* Cleans memory in region */
				if (regions[d.region].buf != NULL)
					free(regions[d.region].buf);

				regions[d.region].buf = buf;
				regions[d.region].count = d.count;
				pthread_rwlock_unlock(&rwlock[d.region]);
				
				printf("REAL UPDATE: Region [%d] with [%s]\n", d.region, (char *)regions[d.region].buf);
			}
		}
		else if (d.copy_paste == PASTE)
		{
			/* Optimized read lock */
			pthread_rwlock_rdlock(&rwlock[d.region]);
			buf = malloc(regions[d.region].count);
			buf = regions[d.region].buf;
			count = regions[d.region].count;
			pthread_rwlock_unlock(&rwlock[d.region]);

			if (buf != NULL)
			{
				printf("Clipboard wants to read from backup from region [%d]: [%s]\n", d.region, (char *)buf);

				if (send(client_fd, buf, count, 0) == -1)
				{
					perror("Send");
					exit(-1);
				}
			}
			else
			{ /* handles situation where the region has nothing */
				printf("Clipboard wants to read from backup from region [%d], it is empty\n", d.region);
				if (send(client_fd, "\0", 1, 0) == -1)
				{ //send "/0"
					perror("Send");
					exit(-1);
				}
			}
		}
		free(msg);
	}
	close(client_fd);
	return 0;
}

/******************************************************************************
* app_receiver - Deals with the communication between this clipboard and a local application
*                                        
*
* Receives: void *client_fdd - file descriptor of the previous accepted application *
******************************************************************************/

void *app_receiver(void *client_fdd)
{
	int *pin = (int *)client_fdd;
	int nbytes, client_fd = *pin;
	printf("My client fd: [%d]\n", client_fd);
	struct _data d;
	void *firstFlag = NULL;
	struct _sockList *iterator;
	void *buf = NULL;
	long int count = 0;

	while (1)
	{
		printf(" pid - %d\t threadID %lu\n", getpid(), pthread_self());
		char *msg = (char *)malloc(sizeof(struct _data));
		if (msg == NULL)
		{
			printf("malloc failed\n");
			return 0;
		}

		if ((nbytes = recv(client_fd, msg, sizeof(struct _data), 0)) == -1)
		{
			perror("Recv");
			return 0;
		}
		if (nbytes == 0)
		{
			printf("App [%d] closed. Closing this thread.\n", client_fd);
			break;
		}

		/* From bytestream to data struct*/
		memcpy(&d, msg, sizeof(struct _data));

		if (d.copy_paste == COPY)
		{
			if (d.region < 10 && d.region > -1)
			{
				printf("App wants to write [%s] into region [%d]\n", d.buf, d.region);
				if (net_sock_fd != -1)
				{
					printf("\n----------- Updating Parent Clipboard -----------\n\n");
					if (send(net_sock_fd, msg, sizeof(struct _data), 0) == -1)
					{
						perror("Send");
						break;
					}
					printf("\n----- Parent Clipboard Updated Successfully -----\n");
				}else{
					/* If we are (g)root, update and send everything downwards*/
					buf = malloc(d.count);
					memcpy(buf, d.buf, d.count);

					/* Optimized write lock */
					pthread_rwlock_wrlock(&rwlock[d.region]);
					/* Cleans memory in region */
					if (regions[d.region].buf != NULL)
						free(regions[d.region].buf);

					regions[d.region].buf = buf;
					regions[d.region].count = d.count;
					pthread_rwlock_unlock(&rwlock[d.region]);
					printf("Region [%d] update saved with [%s]!\n", d.region, (char *)regions[d.region].buf);

					/* Send update upwards if there is upward clipboard*/
					printf("\n----------- Updating Children Clipboards -----------\n\n");
					for (iterator = iniSockList; iterator != NULL; iterator = iterator->next)
					{
						printf("I will send to %d child\n", iterator->value);
						if (send(iterator->value, msg, sizeof(struct _data), 0) == -1)
						{
							perror("Send");
							exit(-1);
						}
					}
					printf("\n----- Children Clipboards Updated Successfully -----\n");
				}
			}
			else
			{
				printf("Invalid Region\n");
				break;
			}
		}
		else if (d.copy_paste == PASTE)
		{
			if (d.region < 10 && d.region > -1)
			{

				/* Optimized read lock */
				pthread_rwlock_rdlock(&rwlock[d.region]);
				buf = malloc(regions[d.region].count);
				buf = regions[d.region].buf;
				count = regions[d.region].count;
				pthread_rwlock_unlock(&rwlock[d.region]);

				printf("Will send as msgSize: %ld\n", count);
				if (send(client_fd, &count, sizeof(long int), 0) == -1)
				{
					perror("Send");
					break;
				}

				printf("Will send as buf: %s\n", (char *)buf);
				if (send(client_fd, buf, count, 0) == -1)
				{
					perror("Send");
					break;
				}
			}
			else
			{
				printf("Invalid Region\n");
				break;
			}
		}
		else if (d.copy_paste == WAIT)
		{
			printf("Will wait for changes in region [%d]\n", d.region);
			if (d.region < 10 && d.region > -1)
			{
				/* Optimized read lock*/
				pthread_rwlock_rdlock(&rwlock[d.region]);
				buf = malloc(regions[d.region].count);
				buf = regions[d.region].buf;
				count = regions[d.region].count;
				pthread_rwlock_unlock(&rwlock[d.region]);

				if (send(client_fd, &count, sizeof(long int), 0) == -1)
				{
					perror("Send");
					break;
				}

				if (send(client_fd, buf, count, 0) == -1)
				{
					perror("Send");
					break;
				}

				firstFlag = buf;
				buf = NULL; 
				/* We are stuck in this cycle until an update is made in regions buffer */
				pthread_rwlock_rdlock(&rwlock[d.region]);
				while (firstFlag == regions[d.region].buf)
				{
					pthread_rwlock_unlock(&rwlock[d.region]);
					pthread_rwlock_rdlock(&rwlock[d.region]);
				}
				/* Put again in buffer */
				buf = malloc(regions[d.region].count);
				buf = regions[d.region].buf;
				count = regions[d.region].count;
				pthread_rwlock_unlock(&rwlock[d.region]);

				/* When region is updated, repeat the sending process*/
				if (send(client_fd, &count, sizeof(long int), 0) == -1)
				{
					perror("Send");
					break;
				}

				if (send(client_fd, buf, count, 0) == -1)
				{
					perror("Send");
					break;
				}
				buf = NULL;
				free(firstFlag);
				printf("Region [%d] was updated\n", d.region);
			}
			else
			{
				printf("Invalid Region\n");
				break;
			}
		}
		free(msg);
	}
	close(client_fd);
	return (void *)0;
}

/******************************************************************************
* remote_connect - Ensures the connection to other clipboards *
******************************************************************************/

void *remote_connect()
{
	int remote_fd;
	pthread_t thread_id = 0;
	struct _sockList *aux,*insertAux;

	/*------------ Start of socket setup ----------------*/
	struct sockaddr_in local_addr;
	struct sockaddr_in client_addr, foo;
	unsigned int len = sizeof(struct sockaddr);
	socklen_t size_addr;

	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (sock_fd == -1)
	{
		perror("socket: ");
		exit(-1);
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(0);
	local_addr.sin_addr.s_addr = INADDR_ANY;
	int err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if (err == -1)
	{
		perror("Bind");
		exit(-1);
	}
	getsockname(sock_fd, (struct sockaddr *)&foo, &len);
	fprintf(stderr, "listening on %s:%d\n", inet_ntoa(foo.sin_addr),
			ntohs(foo.sin_port));

	if (listen(sock_fd, 5) == -1)
	{
		perror("Listen");
		exit(-1);
	}
	/*------------ End of socket setup -------------------*/

	while (1)
	{
		size_addr = sizeof(struct sockaddr);
		printf("Ready to accept connections from Clipboards\n\n");

		/*Print my clipboard fd's list!!*/
		if(iniSockList == NULL){
			printf("\nMy clipboard list is empty\n\n");
		}else{
			for(aux = iniSockList; aux != NULL; aux = aux->next){
				printf("One of my elements is: %d\n", aux->value);
			}
		}

		if ((remote_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &size_addr)) == -1)
		{
			perror("Accept");
			exit(-1);
		}

		/* Insert remote_fd in list of adjacent clipboards*/
		if (iniSockList == NULL)
		{
			iniSockList = malloc(sizeof(struct _sockList));
			iniSockList->next = NULL;
			iniSockList->value = remote_fd;
		}
		else
		{
			aux =  malloc(sizeof(struct _sockList));
			aux->next = NULL;
			aux->value = remote_fd;
			for(insertAux = iniSockList; insertAux->next != NULL; insertAux = insertAux->next){}
				insertAux->next = aux;
		}
		/*Send to clipboard_receiver its clipboard fd*/
		if (pthread_create(&thread_id, NULL, clipboard_receiver, &remote_fd) != 0)
		{
			perror("pthread_create() error");
			exit(-1);
		}
	}

	close(sock_fd);
	return 0;
}

/******************************************************************************
* main - Ensures the correct initialization of this clipboard, launch the necessary threads, and then works as an app_receiver, accepting connections from local applications
*                                        
*
* Receives: argc, and argv, which will contain the ip adress and port of another clipboard if we want to make a connection as we launch *
******************************************************************************/

int main(int argc, char *argv[])
{
	unlink(SOCK_ADDRESS);
	struct sockaddr_un local_addr;
	struct sockaddr_un client_addr;
	struct sockaddr_in backup_addr;
	pthread_t thread_id = 0;
	int nbytes;
	socklen_t size_addr;
	signal(SIGINT, ctrl_c_callback_handler);
	struct _data d;
	char buff[MSG_MAX];
	int client_fd;
	int i;

	/* Initializing regions */
	for (i = 0; i < 10; i++)
		regions[i].buf = NULL;

	/* Initializing read-write lock */
	for (i = 0; i < 10; i++)
	{
		if (pthread_rwlock_init(&rwlock[i], NULL) != 0)
		{
			perror("Rwlock");
			exit(-1);
		}
	}

	/*-------Connected mode------*/
	if (argc == 4 && !strcmp(argv[1], "-c"))
	{
		char *msg = (char *)malloc(sizeof(struct _data));
		if (msg == NULL)
		{
			printf("Malloc failed\n");
			return 0;
		}
		net_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

		if (net_sock_fd == -1)
		{
			perror("Socket");
			exit(-1);
		}
		printf("Clipboard INET socket created\n");

		backup_addr.sin_family = AF_INET;
		backup_addr.sin_port = htons(atoi(argv[3]));
		inet_aton(argv[2], &backup_addr.sin_addr);

		if (-1 == connect(net_sock_fd,
						  (const struct sockaddr *)&backup_addr,
						  sizeof(backup_addr)))
		{
			perror("Connect");
			exit(-1);
		}
		/* Send a struct of type PASTE */
		d.copy_paste = PASTE;
		strcpy(d.buf, "\0");

		/* Pastes every region of the connected clipboard*/
		for (i = 0; i < 10; i++)
		{
			d.region = i;
			memcpy(msg, &d, sizeof(struct _data));
			if (send(net_sock_fd, msg, sizeof(struct _data), 0) == -1)
			{
				perror("Send");
				exit(-1);
			}
			if ((nbytes = recv(net_sock_fd, buff, MSG_MAX, 0)) == -1)
			{
				perror("Recv");
				exit(-1);
			}

			void *buf = malloc(nbytes);
			memcpy(buf, buff, nbytes);
			/* Optimized pthread write lock*/
			pthread_rwlock_wrlock(&rwlock[d.region]);
			regions[i].buf = buf;
			regions[i].count = nbytes;
			pthread_rwlock_unlock(&rwlock[d.region]);
		}
		printClipboard();
		/* Opening a thread to receive updates from the connected "backup" clipboard*/
		if (pthread_create(&thread_id, NULL, clipboard_receiver, &net_sock_fd) != 0)
		{
			perror("pthread_create() error");
			exit(-1);
		}
		//TODO
		free(msg);
		printf("\n---------- Backup finished ----------\n\n");
	}
	/*-------------End of interaction with backup---------------*/

	/*Unix socket creation*/
	int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock_fd == -1)
	{
		perror("Socket");
		exit(-1);
	}
	local_addr.sun_family = AF_UNIX;
	strcpy(local_addr.sun_path, SOCK_ADDRESS);

	/*Bind and Listen*/
	int err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if (err == -1)
	{
		perror("Bind");
		exit(-1);
	}
	printf("Clipboard Unix socket created and binded \n");

	if (listen(sock_fd, 5) == -1)
	{
		perror("Listen");
		exit(-1);
	}

	/* One thread deals with the connection of clipboards*/
	if (pthread_create(&thread_id, NULL, remote_connect, NULL) != 0)
	{
		perror("pthread_create() error");
		exit(-1);
	}

	/* One thread deals with the connection of apps*/
	while (1)
	{
		size_addr = sizeof(struct sockaddr);

		printf("Ready to accept connections from Apps\n");
		client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &size_addr);

		if (client_fd == -1)
		{
			perror("Accept");
			exit(-1);
		}
		/*Send to app_receiver its app' fd*/
		if (pthread_create(&thread_id, NULL, app_receiver, &client_fd) != 0)
		{
			perror("pthread_create() error");
			exit(-1);
		}
	}
	// Socket closing
	close(net_sock_fd);
	close(sock_fd);
	exit(0);
}
