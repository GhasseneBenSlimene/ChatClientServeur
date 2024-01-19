#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stddef.h>

#define MSG_LEN 1024
#define CHANNEL_LEN 128
#define NICK_LEN 128
#define FILE_LEN 260

void test_error(bool test, char *str);
size_t read_full(int fd, void *buf, size_t count);
size_t write_full(int fd, const void *buf, size_t count);

#endif