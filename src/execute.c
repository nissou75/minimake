#include "execute.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int wait_command(pid_t process)
{
    int status = 0;
    pid_t result = waitpid(process, &status, 0);
    while (result < 0 && errno == EINTR)
        result = waitpid(process, &status, 0);
    if (result < 0)
    {
        perror("minimake: waitpid");
        return 2;
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        return 0;
    if (WIFEXITED(status))
        fprintf(stderr, "minimake: *** Command exited with status %d. Stop.\n",
                WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        fprintf(stderr, "minimake: *** Command killed by signal %d. Stop.\n",
                WTERMSIG(status));
    return 2;
}

int execute_command(const char *command, bool *executed)
{
    while (*command == ' ' || *command == '\t')
        command++;
    bool silent = *command == '@';
    if (silent)
        command++;
    while (*command == ' ' || *command == '\t')
        command++;
    if (!*command)
        return 0;
    *executed = true;
    if (!silent)
        printf("%s\n", command);
    if (fflush(stdout) == EOF || fflush(stderr) == EOF)
    {
        perror("minimake: fflush");
        return 2;
    }
    pid_t process = fork();
    if (process < 0)
    {
        perror("minimake: fork");
        return 2;
    }
    if (process == 0)
    {
        execl("/bin/sh", "/bin/sh", "-c", command, NULL);
        perror("minimake: /bin/sh");
        _exit(127);
    }
    return wait_command(process);
}
