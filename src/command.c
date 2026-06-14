#include "command.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

ssize_t send_all(int fd, const void *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = write(fd, (const char *)buf + sent, len - sent);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        sent += n;
    }
    return (ssize_t)sent;
}

ssize_t recv_all(int fd, void *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t n = read(fd, (char*)buf + got, len - got);
        if (n == 0) return 0;   // peer closed
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        got += n;
    }
    return got;
}
