#ifndef MSG_STRUCT_H
#define MSG_STRUCT_H

#define INFOS_LEN 128

#include "common.h"

enum msg_type
{
    NICKNAME_NEW,
    NICKNAME_LIST,
    NICKNAME_INFOS,
    ECHO_SEND,
    UNICAST_SEND,
    BROADCAST_SEND,
    MULTICAST_CREATE,
    MULTICAST_LIST,
    MULTICAST_JOIN,
    MULTICAST_SEND,
    MULTICAST_QUIT,
    MULTICAST_INFO,
    MULTICAST_FILE_REQUEST,
    MULTICAST_RENAME,
    FILE_REQUEST,
    FILE_ACCEPT,
    FILE_REJECT,
    FILE_SEND,
    FILE_ACK,
    DISCONECT
};

struct message
{
    int pld_len;
    char nick_sender[NICK_LEN];
    enum msg_type type;
    char infos[INFOS_LEN];
};

#endif