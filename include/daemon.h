#ifndef DAEMON_H
#define DAEMON_H

#define BD_MAX_CLOSE 8192 /* Maximum number of file descriptors to close */

int daemonize(void);

#endif