/*****************************************************************
*    main.c - Gusty - began with code from https://codereview.stackexchange.com/questions/67746/simple-shell-in-c
********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include "types.h"
#include "defs.h"
#include "proc.h"

void parseCmd(char *cmd, char **params, int *nparams);
int executeCmd(char **params, int nparams);

#define MAX_COMMAND_LENGTH 100
#define MAX_NUMBER_OF_PARAMS 10
#define TIME_SLICE 48
#define MIN_GRAN 12

enum cmds
{
    FORK = 0,
    SETPID,
    SHOWPID,
    WAIT,
    EXIT,
    SLEEP,
    WAKEUP,
    PS,
    SCHEDULE,
    TIMER,
    HELP,
    QUIT
};
char *cmdstr[] = {"fork", "Setpid", "currpid", "wait", "exit", "sleep", "wakeup", "ps", "schedule", "timer", "help", "quit"};

int curr_proc_id = 0;

int chosenScheduler = 0;

int local_scheduler()
{
    if (chosenScheduler == 0)
    {
        scheduler();
    }
    else if (chosenScheduler == 1)
    {
        completlyFairScheduler();
    }
    else if (chosenScheduler == 2)
    {
        //round robin
    }
    else
    {
        printf("please choose a scheduling algorithm. 0 is default 1 is completly fair 2 is round robin:\n");
        char buf[1];
        scanf("%s", buf);
        chosenScheduler = atoi(buf);
    }
    struct proc *p = curr_proc;
    return p->pid;
}

int main()
{
    //allow the user to pick which algorythm to use. this is done on good faith, not a lot of error checking
    //printf("Select a scheduling algorithm. 0 is default 1 is completly fair 2 is round robin:");
    //char buf[101];
    //scanf("%c%*c", buf);
    chosenScheduler = 1; //atoi(buf);

    pinit();                   // initialize process table
    curr_proc_id = userinit(); // create first user process
    char cmd[MAX_COMMAND_LENGTH + 1];
    char *params[MAX_NUMBER_OF_PARAMS + 1];
    int cmdCount = 0, nparams = 0;

    while (1)
    {
        nparams = 0; // > Fork 4 command sets nparams to 2
        char *username = getenv("USER");
        printf("%s@shell %d> ", username, ++cmdCount);
        if (fgets(cmd, sizeof(cmd), stdin) == NULL)
            break;
        if (cmd[strlen(cmd) - 1] == '\n')
            cmd[strlen(cmd) - 1] = '\0';
        parseCmd(cmd, params, &nparams);
        if (strcmp(params[0], "Quit") == 0)
            break;
        if (executeCmd(params, nparams) == 0)
            break;
    }

    return 0;
}

// Split cmd into array of parameters
void parseCmd(char *cmd, char **params, int *nparams)
{
    for (int i = 0; i < MAX_NUMBER_OF_PARAMS; i++)
    {
        params[i] = strsep(&cmd, " ");
        if (params[i] == NULL)
            break;
        (*nparams)++;
    }
}

int executeCmd(char **params, int nparams)
{
    int pid, rc = 1, chan;
    int ncmds = sizeof(cmdstr) / sizeof(char *);
    int cmd_index;
    for (cmd_index = 0; cmd_index < ncmds; cmd_index++)
        if (strcmp(params[0], cmdstr[cmd_index]) == 0)
            break;

    //for (int i = 0; i < nparams; i++)
    //printf("Param %d: %s\n", i, params[i]);
    //printf("ncmds: %d, cmd_index: %d\n", ncmds, cmd_index);

    switch (cmd_index)
    {
    case FORK:
        if (nparams > 1)
            pid = atoi(params[1]);
        else
            pid = curr_proc->pid;

        if (chosenScheduler == 0)
        {
            int fpid = uniqueFork(pid, -100);
            printf("pid: %d forked: %d\n", pid, fpid);
        }
        else if (chosenScheduler == 1)
        {
            //They chose completly fair, and should enter a niceness
            printf("Please enter a niceness for this process, min is -20 max is 20\n");
            int niceness = -100;
            scanf("%d%*c", &niceness);
            printf("you chose niceness:%d\n", niceness);
            int fpid = uniqueFork(pid, niceness);
        }
        break;
    case SETPID:
        if (nparams == 1)
            printf("setpid cmd requires pid parameter\n");
        else
            curr_proc_id = atoi(params[1]);
        break;
    case SHOWPID:
        //printf("Current pid: %d\n", curr_proc_id);
        printf("Current pid: %d\n", curr_proc->pid);
        break;
    case WAIT:
        if (nparams > 1)
            pid = atoi(params[1]);
        else
            pid = curr_proc->pid;
        int wpid = Wait(pid);
        if (wpid == -1)
            printf("pid: %d has no children to wait for.\n", pid);
        else if (wpid == -2)
            printf("pid: %d has children, but children still running.\n", pid);
        else
            printf("pid: %d child %d has terminated.\n", pid, wpid);
        break;
    case EXIT:
        if (nparams > 1)
            pid = atoi(params[1]);
        else
            pid = curr_proc->pid;
        pid = Exit(pid);
        printf("Exit Status:: %d .\n", pid);
        break;
    case SLEEP:
        if (nparams < 2)
            printf("Sleep chan [pid]\n");
        else
        {
            chan = atoi(params[1]);
            if (nparams > 2)
                pid = atoi(params[2]);
            else
                pid = curr_proc->pid;
            pid = Sleep(pid, chan);
            printf("Sleep Status:: %d .\n", pid);
        }
        break;
    case WAKEUP:
        if (nparams < 2)
            printf("Wakeup chan\n");
        else
        {
            chan = atoi(params[1]);
            Wakeup(chan);
        }
        break;
    case PS:
        procdump();
        break;
    case SCHEDULE:
        pid = local_scheduler();
        printf("Scheduler selected pid: %d\n", pid);
        break;
    case TIMER:
        if (nparams < 2)
            printf("timer quantums\n");
        else
        {
            // printf("Changing current proc, pid:%d, weight:%d\n", curr_proc->pid, curr_proc->weight);

            //get total weight
            double curWeight = getProcWeight();
            printf("curr_proc->vruntime:%d, curWeight:%f \n", curr_proc->vruntime, curWeight);

            //get schedule latency
            double scheduleLatency = (getTotalProcs() / TIME_SLICE);
            if (scheduleLatency < MIN_GRAN)
            {
                scheduleLatency = MIN_GRAN;
            }
            printf("Hi\n\n");
            int quantums = atoi(params[1]);
            for (int i = 0; i < quantums; i++)
            {
                curr_proc->runtime += 1;

                double time_slice = curr_proc->weight / curWeight * scheduleLatency;
                //printf("curWeight:%lf, curr_proc->runtime:%d weight:%lf\n",curWeight,curr_proc->runtime,curr_proc->weight);
                curr_proc->vruntime += ((1024 / curr_proc->weight) * curr_proc->runtime);

                pid = local_scheduler();
                printf("Scheduler selected pid: %d\n", pid);
            }
        }
        break;
    case HELP:
        printf("Commands: Fork, Wait, Exit, Setpid, Showpid, Sleep, Wakeup, Help, PS\n");
        break;
    case QUIT:
        rc = 0;
        break;
    default:
        printf("Invalid command! Enter Help to see commands.\n");
    }

    return rc;
}
