#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMDLINE_MAX 512
#define ARG_MAX 16
#define PRO_MAX 4

struct cmdLine
{
    char o_cmd[CMDLINE_MAX];
    char *command;
    char *arg[ARG_MAX];
    char *file;
    char *process_arr[PRO_MAX];
    int process_size;
    int error;
};

struct listNode
{
    char *directory;
    struct listNode *next;
};

void arg_parser(struct cmdLine *cl, char cmd[CMDLINE_MAX])
{
    char *token = cmd;
    if (strstr(token, "|"))
    {
        cl->command = "";
        char *pipe_token = strtok(token, "|");
        int i = 0;
        while (pipe_token != NULL)
        {
            cl->process_arr[i] = pipe_token;
            pipe_token = strtok(NULL, "|");
            i++;
        }
        cl->process_size = i;
        return;
    }

    if (strstr(cmd, ">"))
    {
        token = strtok(cmd, ">");
        cl->file = strtok(NULL, " ");
    }

    token = strtok(token, " ");
    if (token == NULL)
    {
        cl->command = cmd;
    }
    else
    {
        cl->command = token;
    }
    cl->arg[0] = cl->command;
    int i = 1;
    while (token != NULL)
    {
        token = strtok(NULL, " ");
        cl->arg[i] = token;
        i++;
    }
}

void openFile(struct cmdLine *cl)
{
    int fd;
    char *filename = malloc(strlen(cl->file) + 1);
    sprintf(filename, "%s", cl->file);
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    dup2(fd, STDOUT_FILENO);
    close(fd);
}

void piping(char *p[PRO_MAX], int size)
{
    pid_t pid;
    int fd0[2];
    pipe(fd0);
    int fd1[2];
    pipe(fd1);
    int fd2[2];
    pipe(fd2);

    for (int i = 0; i < size; i++)
    {
        struct cmdLine process;
        arg_parser(&process, p[i]);

        pid = fork();
        if (pid)
        {
            if (i == 1)
            {
                close(fd0[1]);
                close(fd0[0]);
            }
            if (i == 2)
            {
                close(fd1[1]);
                close(fd1[0]);
            }
            if (i == 3)
            {
                close(fd2[1]);
                close(fd2[0]);
            }
        }
        else
        {
            if (i == 0)
            {
                close(fd0[0]);
                dup2(fd0[1], STDOUT_FILENO);
                close(fd0[1]);
            }
            else if (i == 1)
            {
                close(fd0[1]);
                dup2(fd0[0], STDIN_FILENO);
                close(fd0[0]);
                if (i != size - 1)
                {
                    close(fd1[0]);
                    dup2(fd1[1], STDOUT_FILENO);
                    close(fd1[1]);
                }
            }
            else if (i == 2)
            {
                close(fd1[1]);
                dup2(fd1[0], STDIN_FILENO);
                close(fd1[0]);

                if (i != size - 1)
                {
                    close(fd2[0]);
                    dup2(fd2[1], STDOUT_FILENO);
                    close(fd2[1]);
                }
            }
            else if (i == 3)
            {
                close(fd2[1]);
                dup2(fd2[0], STDIN_FILENO);
                close(fd2[0]);
            }
            execvp(process.command, process.arg);
            perror(process.command);
        }
    }
    int status;
    waitpid(pid, &status, 0);
    exit(1);
}

void system_call(pid_t pid, struct cmdLine *cl)
{

    if (pid == 0)
    {
        // child
        if (strstr(cl->o_cmd, ">"))
        {
            openFile(cl);
        }

        if (strstr(cl->o_cmd, "|"))
        {
            piping(cl->process_arr, cl->process_size);
        }
        else
        {
            execvp(cl->command, cl->arg);
            fprintf(stderr, "Error: command not found\n");
            exit(1);
        }
    }
    else if (pid > 0)
    {
        // parent
        int status;
        waitpid(pid, &status, 0);
        fprintf(stderr, "+ completed '%s': [%d]\n", cl->o_cmd, WEXITSTATUS(status));
    }
    else
    {
        perror("fork");
        exit(1);
    }
}

void pwd_function(struct cmdLine *cl)
{
    char *pwd = NULL;
    char *res = getcwd(pwd, 0);
    if (strcmp(cl->o_cmd, cl->command))
    {
        cl->error = 1;
        fprintf(stderr, "Error: too many process arguments\n");
    }
    else if (res == NULL)
    {
        cl->error = 1;
        perror("Error");
    }
    else
    {
        cl->error = 0;
        printf("%s\n", res);
        fflush(stdout);
    }
}

void cd_function(char *path, struct cmdLine *cl)
{
    if (chdir(path) == -1)
    {
        cl->error = 1;
        fprintf(stderr, "Error: cannot cd into directory\n");
    }
    else
    {
        cl->error = 0;
    }
}

void push_directory(char *dir, struct listNode *ds, struct cmdLine *cl)
{
    char *pwd = NULL;
    char *res = getcwd(pwd, 0);
    if (res == NULL)
    {
        perror("getcwd");
    }
    else
    {
        struct listNode *new_node = (struct listNode *)malloc(sizeof(struct listNode));
        new_node->next = NULL;
        new_node->directory = res;
        if (ds->next)
        {
            struct listNode *next_node = (struct listNode *)malloc(sizeof(struct listNode));
            next_node = ds->next;
            next_node->directory = ds->next->directory;
            ds->next = new_node;
            new_node->next = next_node;
        }
        else
        {
            ds->next = new_node;
            new_node->next = NULL;
        }
    }
    cd_function(dir, cl);
    if (cl->error == 0)
    {
        struct listNode *curr_dir = (struct listNode *)malloc(sizeof(struct listNode));
        curr_dir->next = NULL;
        curr_dir->directory = getcwd(pwd, 0);
        struct listNode *next_dir = (struct listNode *)malloc(sizeof(struct listNode));
        next_dir = ds->next;
        next_dir->directory = ds->next->directory;

        ds->next = NULL;
        ds->next = curr_dir;
        ds->next->directory = curr_dir->directory;
        curr_dir->next = next_dir;
    }
}

void pop_directory(struct listNode *ds, struct cmdLine *cl)
{
    if (strcmp(cl->o_cmd, cl->command))
    {
        cl->error = 1;
        fprintf(stderr, "Error: too many process arguments\n");
    }
    else if (ds->next->next != NULL)
    {
        struct listNode *new_node = (struct listNode *)malloc(sizeof(struct listNode));
        new_node = ds->next->next;
        char *path = new_node->directory;
        cd_function(path, cl);
        ds->next->next = NULL;
        ds->next = NULL;
        ds->next = new_node;
        ds->next->directory = new_node->directory;
        free(new_node);
    }
    else if (ds->next->next == NULL)
    {
        char *path = ds->next->directory;
        cd_function(path, cl);
        free(ds);
    }
    else
    {
        cl->error = 1;
        fprintf(stderr, "Error: directory stack empty\n");
    }
}

void print_all_directoryies(struct listNode *ds, struct cmdLine *cl)
{
    if (ds->next == NULL)
    {
        cl->error = 1;
        fprintf(stderr, "Error: directory stack empty\n");
    }
    else
    {
        struct listNode *top = (struct listNode *)malloc(sizeof(struct listNode));
        top = ds;
        while (top->next != NULL)
        {
            top = top->next;
            printf("%s\n", top->directory);
            fflush(stdout);
        }
        free(top);
    }
}

int command_handler(struct cmdLine *cl, struct listNode *stack)
{
    if (!strcmp(cl->command, "exit"))
    {
        if (strcmp(cl->o_cmd, cl->command))
        {
            cl->error = 1;
            fprintf(stderr, "Error: too many process arguments\n");
        }
        else
        {
            fprintf(stderr, "Bye...\n");
        }
        fprintf(stderr, "+ completed '%s' [%d]\n", cl->o_cmd, 0);
        return 1;
    }
    else if (!strcmp(cl->command, "pwd"))
    {
        pwd_function(cl);
        return 2;
    }
    else if (!strcmp(cl->command, "cd"))
    {
        cd_function(cl->arg[1], cl);
        return 2;
    }
    else if (!strcmp(cl->command, "pushd"))
    {
        push_directory(cl->arg[1], stack, cl);
        return 2;
    }
    else if (!strcmp(cl->command, "dirs"))
    {
        print_all_directoryies(stack, cl);
        return 2;
    }
    else if (!strcmp(cl->command, "popd"))
    {
        pop_directory(stack, cl);
        return 2;
    }
    else
        return 0;
}

int main(void)
{
    char cmd[CMDLINE_MAX];
    pid_t pid;
    struct cmdLine cl;
    struct listNode directory_stack;
    /* initialize the directory stack */
    directory_stack.next = NULL;
    directory_stack.directory = "";

    while (1)
    {
        // argument(cmd);
        char *nl;

        /* Print prompt */
        printf("sshell@ucd$ ");
        fflush(stdout);

        /* Get command line */
        fgets(cmd, CMDLINE_MAX, stdin);

        /* Print command line if stdin is not provided by terminal */
        if (!isatty(STDIN_FILENO))
        {
            printf("%s", cmd);
            fflush(stdout);
        }

        /* Remove trailing newline from command line */
        nl = strchr(cmd, '\n');
        if (nl)
            *nl = '\0';

        /* copy the original cmd to prevent revising it */
        strncpy(cl.o_cmd, cmd, CMDLINE_MAX);

        /* argument parser */
        arg_parser(&cl, cmd);

        /* System call command*/
        int command_flag = command_handler(&cl, &directory_stack);
        if (command_flag == 0)
        {
            pid = fork();
            system_call(pid, &cl);
        }
        /*Exit*/
        else if (command_flag == 1)
        {
            break;
        }
        /* Builtin command */
        else if (command_flag == 2)
        {
            fprintf(stderr, "+ completed '%s' [%d]\n", cl.o_cmd, cl.error);
        }
    }
    return EXIT_SUCCESS;
}
