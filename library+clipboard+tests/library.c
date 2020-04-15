/**************************************************************************
 *                               library.c                                *
 **************************************************************************                                                                   
 * Library of functions that constitute an API for the app to interact    *
 * with the clipboard. Functions we're defined in the file clipboard.h	  *
 *************************************************************************/
#include "clipboard.h"
#include "topsecret.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>


/* Function that deals with the signal SIGINT*/
void send_error_callback_handler()
{
	printf("\nSend: Clipboard Closed\n");
	exit(0);
}

/******************************************************************************
* clipboard_connect - creates a local socket to communicate with the clipboard*
*                                        			                          *
* Receives: char *clipboard_dir - directory of the local clipboard            *   
* Returns: int sock_fd - the socket descriptor		                          *
******************************************************************************/
int clipboard_connect(char *clipboard_dir)
{
	struct sockaddr_un server_addr;
	int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	char wDir[108];
	if (sock_fd == -1)
	{
		perror("Socket");
		exit(-1);
	}
	strcpy(wDir, clipboard_dir);
	strcat(wDir, "CLIPBOARD_SO");
	strcpy(server_addr.sun_path, wDir);
	server_addr.sun_family = AF_UNIX;
	if (-1 == connect(sock_fd, (const struct sockaddr *)&server_addr, sizeof(struct sockaddr)))
	{
		perror("Connect");
		exit(-1);
	}

	printf("App socket created\n");
	return sock_fd;
}

/******************************************************************************
* clipboard_copy -  copies the data pointed by buf to a region on the local   *
*  					clipboard	       										  *
*                                        			                          *
* Receives: int clipboard_id - clipboard socket descriptor                    *
*			int region - region to interact with                              *
*			void *buf - buffer where the region info will be stored			  *
*			size_t count - size of the region to store in the buffer          *   
* Returns: int nbytes - number of bytes copied or pasted                      *
******************************************************************************/
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count)
{
	struct _data d;
	char *msg = (char *)malloc(sizeof(struct _data));
	struct sigaction act;
	act.sa_sigaction = &send_error_callback_handler;

	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGPIPE, &act, NULL) < 0)
	{
		perror("Sigaction");
		exit(-1);
	}

	if (msg == NULL)
	{
		printf("Malloc failed\n");
		return 0;
	}

	if (clipboard_id < 1)
	{
		printf("Invalid clipboard ID\n");
		return 0;
	}

	if (region < 0 || region > 9)
	{
		printf("Invalid region\n");
		return 0;
	}
	/*Prepare data struct to send*/
	d.copy_paste = COPY;
	d.region = region;
	d.count = count;
	memcpy(d.buf, buf, count);
	/*Convert it to a bytestream so it is able to be sent accordingly*/
	memcpy(msg, &d, sizeof(struct _data));

	if (send(clipboard_id, msg, sizeof(struct _data), 0) == -1)
	{
		perror("Send");
		exit(-1);
	}
	free(msg);
	return 1;
}

/******************************************************************************
* clipboard_paste - copies from the system to the application the data in a   *
*					certain region											  *
* Receives: int clipboard_id - clipboard socket descriptor                    *
*			int region - region to interact with                              *
*			void *buf - buffer where the region info will be stored			  *
*			size_t count - size of the region to store in the buffer          *   
* Returns: int nbytes - number of bytes copied or pasted                      *
******************************************************************************/
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count)
{
	struct _data d;
	int nbytes;
	long int msgSize;
	char *msg = (char *)malloc(sizeof(struct _data));

	struct sigaction act;
	act.sa_sigaction = &send_error_callback_handler;

	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGPIPE, &act, NULL) < 0)
	{
		perror("Sigaction");
		exit(-1);
	}

	if (msg == NULL)
	{
		printf("Malloc failed\n");
		return 0;
	}

	if (clipboard_id < 1)
	{
		printf("Invalid clipboard ID\n");
		return 0;
	}

	if (region < 0 || region > 9)
	{
		printf("Invalid region\n");
		return 0;
	}

	/*Prepare data struct to send*/
	d.copy_paste = PASTE;
	d.region = region;
	strcpy(d.buf, "\0");
	memcpy(msg, &d, sizeof(struct _data));

	/* First send the region we want to read*/
	if ((nbytes = send(clipboard_id, msg, sizeof(struct _data), 0)) == -1)
	{
		perror("Send");
		exit(-1);
	}
	/* Receive the number of bytes of what is in the region */
	if ((nbytes = recv(clipboard_id, &msgSize, sizeof(long int), 0)) == -1)
	{
		perror("Recv");
		exit(-1);
	}
	/* If region is empty*/
	if (msgSize < 1)
	{
		free(msg);
		return 1;
	}

	/* Buffer that will store the received info that is in the region, alloced 
	precisely with its size*/
	char *buffer = NULL;
	buffer = (char *)malloc(sizeof(char) * msgSize +1);

	if (buffer == NULL)
	{
		printf("Malloc failed\n");
		return 0;
	}
	/* Receive what is in the region*/
	if ((nbytes = recv(clipboard_id, buffer, msgSize * sizeof(char), 0)) == -1)
	{
		perror("Recv");
		exit(-1);
	}

	/* Truncate message according to count */
	memcpy(buf, buffer, count);

	free(buffer);
	free(msg);
	return 1;
}

/******************************************************************************
* clipboard_wait -   waits for a change on a certain region                   *
*                                        			                          *
* Receives: int clipboard_id - clipboard socket descriptor                    *
*			int region - region to interact with                              *
*			void *buf - buffer where the region info will be stored			  *
*			size_t count - size of the region to store in the buffer          *   
* Returns: int nbytes - number of bytes copied or pasted                      *
******************************************************************************/
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count)
{
	struct _data d;
	int nbytes;
	long int msgSize;
	char *msg = (char *)malloc(sizeof(struct _data));
	char *buffer;

	struct sigaction act;
	act.sa_sigaction = &send_error_callback_handler;

	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGPIPE, &act, NULL) < 0)
	{
		perror("Sigaction");
		exit(-1);
	}

	if (msg == NULL)
	{
		printf("Malloc failed\n");
		return 0;
	}

	if (clipboard_id < 1)
	{
		printf("Invalid clipboard ID\n");
		return 0;
	}

	if (region < 0 || region > 9)
	{
		printf("Invalid region\n");
		return 0;
	}
	/*Prepare data struct to send*/
	d.copy_paste = WAIT;
	d.region = region;
	strcpy(d.buf, "\0");
	memcpy(msg, &d, sizeof(struct _data));

	/* First ssend the region we want to read*/
	if ((nbytes = send(clipboard_id, msg, sizeof(struct _data), 0)) == -1)
	{
		perror("Send");
		exit(-1);
	}
	/* Receive the number of bytes of what is in the region */
	if ((nbytes = recv(clipboard_id, &msgSize, sizeof(long int), 0)) == -1)
	{
		perror("Recv");
		exit(-1);
	}

	/* Buffer that will store the received info that is in the region, alloced 
	precisely with its size*/
	buffer = NULL;
	buffer = (char *)malloc(sizeof(char) * msgSize + 1);

	if (buffer == NULL)
	{
		printf("Malloc failed\n");
		return 0;
	}
	/* Receive what is in the region*/
	if ((nbytes = recv(clipboard_id, buffer, msgSize * sizeof(char), 0)) == -1)
	{
		perror("Recv");
		exit(-1);
	}
	free(buffer);

	/*Wait for an update in the region*/
	if ((nbytes = recv(clipboard_id, &msgSize, sizeof(long int), 0)) == -1)
	{
		perror("Recv");
		exit(-1);
	}
	printf("----- Region %d was updated -----\n", d.region);

	buffer = NULL;
	buffer = (char *)malloc(sizeof(char) * msgSize + 1);

	if (buffer == NULL)
	{
		printf("Malloc failed\n");
		return 0;
	}
	/*Receive what was updated in the region*/
	if ((nbytes = recv(clipboard_id, buffer, msgSize * sizeof(char), 0)) == -1)
	{
		perror("Recv");
		exit(-1);
	}

	/*Truncate the new message*/
	memcpy(buf, buffer, count);
	free(buffer);
	free(msg);
	return 1;
}

/******************************************************************************
* clipboard_close - closes the connection to the clipboard socket			  *
*                                        			                          *
* Receives: int clipboard_id - descriptor of the socket of the local clipboard*   
* Returns: void                  											  *
******************************************************************************/
void clipboard_close(int clipboard_id)
{
	printf("\nClosing CLIPBOARD_SOCKET socket\n");
	close(clipboard_id);
	return;
}
