/**************************************************************************
 *                           app_1threads.c                               *
 **************************************************************************                                                                   
 * Application which purpose is to test the overall functionalities of    *
 * clipboard applications                                                 *
 *************************************************************************/
#define MSG_MAX 100 /* Maximum lenght of a message*/
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "clipboard.h"

int fd; /*local app socket descriptor*/

/* Function that deals with the signal SIGINT*/
void ctrl_c_callback_handler()
{
	clipboard_close(fd);
	exit(0);
}

/*Client Application*/
int main()
{
	int region = 10;
	int ret, i;
	struct sigaction act;
	act.sa_sigaction = &ctrl_c_callback_handler;
	char dados[MSG_MAX];
	size_t count;

	/*Sigaction setup*/
	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGINT, &act, NULL) < 0)
	{
		perror("Sigaction");
		exit(-1);
	}

	fd = clipboard_connect("./");
	while (1)
	{
		printf("\n------------------APP--------------------\n");
		printf("There's a server runnning with 10 regions\n");
		printf("-You may read or modify what is in those-\n");
		printf("----Please type copy or paste or wait----\n");
		printf("-----------------------------------------\n");
		fgets(dados, MSG_MAX, stdin);
		strtok(dados, "\n");
		if (!strcmp(dados, "copy"))
		{
			printf("---Which region do you want to modify?---\n");
			ret = scanf(" %d", &region);
			getchar();
			if (ret != 1)
			{
				getchar();
				printf("Invalid input. Returning to menu...\n");
				continue;
			}

			printf("-------What do you want to insert?-------\n");
			fgets(dados, MSG_MAX, stdin);
			strtok(dados, "\n");
			count = strlen(dados) + 1;
			printf("--- Inserting [%s] (%ld bytes) in region [%d] ---\n", dados, count, region);

			if (clipboard_copy(fd, region, dados, count) == 0)
			{
				printf("Clipboard error\n");
				continue;
			}

			printf("In region [%d] was inserted: [%s]\n", region, dados);
			printf("-----------Insertion complete------------\n");
		}
		else if (!strcmp(dados, "paste"))
		{
			printf("--From which region do you want to read?-\n");
			ret = scanf("%d", &region);
			getchar();
			if (ret != 1)
			{
				getchar();
				printf("Invalid region. Returning to menu...\n");
				continue;
			}

			printf("----How many bytes you want to read?----\n");
			ret = scanf("%ld", &count);

			getchar();
			if (ret != 1)
			{
				getchar();
				printf("Invalid size. Returning to menu...\n");
				continue;
			}
			printf("--------Reading from region [%d]---------\n\n", region);
			for (i = 0; i < MSG_MAX; i++)
				dados[i] = 0;
			if (clipboard_paste(fd, region, dados, count) == 0)
			{
				printf("Clipboard error\n");
				continue;
			}

			printf("In region [%d] there is the corresponding string: %s\n\n", region, dados);
			printf("--------------Read complete-------------\n");
		}
		else if (!strcmp(dados, "wait"))
		{
			printf("--Which region do you want to wait for?-\n");
			ret = scanf("%d", &region);
			getchar();
			if (ret != 1)
			{
				getchar();
				printf("Invalid region. Returning to menu...\n");
				continue;
			}
			printf("-----Waiting for changes in region [%d]----\n\n", region);
			for (i = 0; i < MSG_MAX; i++)
				dados[i] = 0;
			if (clipboard_wait(fd, region, dados, (long int)100) == 0)
			{
				printf("Clipboard error\n");
				continue;
			}

			printf("Region[%d] content was changed to the string: %s\n\n", region, dados);
			printf("--------------Wait complete-------------\n");
		}
		else
		{
			printf("-Invalid option, type 'paste' or 'copy' or 'wait'-\n\n");
		}
	}
	clipboard_close(fd);
	exit(0);
}
