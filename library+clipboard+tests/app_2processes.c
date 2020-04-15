/**************************************************************************
 *                           app_2processes.c                             *
 **************************************************************************                                                                   
 * Application which purpose is to test the locks and syncronism in the   *
 * clipboards applications                                                *
 *************************************************************************/
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "clipboard.h"

/*0 for simultaneous copies, 1 for pastes, and 2 for both*/
#define H 2
#define MSG_MAX 100 /* Maximum lenght of a message*/

int main()
{
    int fd;
    int pid;
    pid = fork();
    char dados[MSG_MAX];
    strcpy(dados, "\0");

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

    switch (H)
    {
    case 0:
        if (pid == 0)
        {
            if (clipboard_copy(fd, 5, "dados errados", 20) == 0)
            {
                printf("Clipboard error\n");
                exit(-1);
            }
        }
        else
        {
            printf("\nSimultaneous copy:\n\n");
            if (clipboard_copy(fd, 5, "dados corretos", 20) == 0)
            {
                printf("Clipboard error\n");
                exit(-1);
            }
        }
        break;
    case 1:
        if (pid == 0)
        {
            if (clipboard_paste(fd, 5, dados, 9) == 0)
            {
                printf("Clipboard error\n");
                exit(-1);
            }
        }
        else
        {
            printf("\nSimultaneous paste:\n\n");
            if (clipboard_paste(fd, 5, dados, 20) == 0)
            {
                printf("Clipboard error\n");
                exit(-1);
            }
        }
        break;
    case 2:
        if (pid == 0)
        {
            if (clipboard_paste(fd, 5, dados, 20) == 0)
            {
                printf("Clipboard error\n");
                exit(-1);
            }
        }
        else
        {
            printf("\nSimultaneous copy and paste\n\n");
            if (clipboard_copy(fd, 5, "dados corretos", 20) == 0)
            {
                printf("Clipboard error\n");
                exit(-1);
            }
        }
    }
    printf("[%d]-Operation complete\n", pid);
    clipboard_close(fd);
    exit(0);
}
