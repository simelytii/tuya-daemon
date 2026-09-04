#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "daemon.h"

int daemonize(void)
{
    int maxfd, fd;

    switch (fork()) {
        case -1:
            return -1;

        case 0:
            break;

        default:
            _exit(EXIT_SUCCESS);
    }

    if (setsid() == -1)
        return -1;

    switch (fork()) {
        case -1:
            return -1;

        case 0:
            break;

        default:
            _exit(EXIT_SUCCESS);
    }

    umask(0);

    if (chdir("/") < 0)
        return -1;

    maxfd = sysconf(_SC_OPEN_MAX);

    if (maxfd == -1)
        maxfd = BD_MAX_CLOSE;
    for (fd = 0; fd < maxfd; fd++)
        close(fd);

    fd = open("/dev/null", O_RDWR);

    if (fd != STDIN_FILENO)
        return -1;

    if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
        return -1;

    if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
        return -1;

    return 0;
}