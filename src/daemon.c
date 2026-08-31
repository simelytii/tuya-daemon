#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int daemonize(void)
{
    pid_t pid = fork();

    if (pid < 0) {
        return -1;
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        return -1;
    }

    pid = fork();

    if (pid < 0) {
        return -1;
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (chdir("/") < 0) {
        return -1;
    }

    umask(0);

    return 0;
}