/**************************************************************************
 *                           app_3massive.c                               *
 **************************************************************************                                                                   
 * Application which purpose is to test massive alterations to clipboards *
 *************************************************************************/

#include "clipboard.h"
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define MAX_ITERATION 10000
#define MSG_MAX 100 /* Maximum lenght of a message*/

int main()
{
    int fd;
    int pid;
    pid = fork();
    char dados[MSG_MAX];
    strcpy(dados, "\0");
    int i = 0;
    srand(time(NULL)); // should only be called once
    int r = 0;
    char str[12];

    if (pid == 0)
    {
        fd = clipboard_connect("./");
        sleep(1);
        printf("[%d]---\n", pid);
    }
    else
    {
        sleep(1);
        fd = clipboard_connect("./");
        printf("[%d]---\n", pid);
    }

    while (i < MAX_ITERATION)
    {
        i++;
        printf("i = %d\n", i);
        r = rand();
        printf("r = %d\n", r);
        sprintf(str, "%d", r);
        if (clipboard_copy(fd, 1, str, sizeof(str)) == 0)
        {
            printf("Clipboard error\n");
            break;
        }
    }

    printf("[%d]-Operation complete\n", pid);
    if (pid == 0)
    {
    	sleep(1);
        clipboard_close(fd);
    }
    else
    {
        clipboard_close(fd);
        sleep(1);
    }
    exit(0);
}
