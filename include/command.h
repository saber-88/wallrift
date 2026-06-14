#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum State {
    eTrue,
    eError,
    eFalse,
};

#define MAX_PATH_LEN 512
#define MAX_TRANS_LEN 64

struct __attribute__((packed)) Command {
    uint8_t img_s;
    uint8_t speed_s;
    uint8_t query_s;
    uint8_t transition_s;

    char path[MAX_PATH_LEN];
    char transition[MAX_TRANS_LEN];
    float speed;
};

ssize_t send_all(int fd, const void *buf, size_t len);
ssize_t recv_all(int fd, void *buf, size_t len);
