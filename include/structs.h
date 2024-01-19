#ifndef STRUCTS_H
#define STRUCTS_H

#include <netinet/in.h>
#include <stddef.h>

#include "common.h"


#define MAX_CLIENTS 10
#define DATE_LEN 26

typedef struct ChannelNode
{
	int nb_client;
	struct ChannelNode *next;
	char name[CHANNEL_LEN];
	char creator[NICK_LEN];
	char connection_time[DATE_LEN];

} ChannelNode;


typedef struct ClientNode
{
	int client_fd;
	char client_ip[INET_ADDRSTRLEN];
	unsigned short client_port;
	char nickname[NICK_LEN];
	struct ClientNode *next;
	char connection_time[DATE_LEN];
	char channel[CHANNEL_LEN];
} ClientNode;

#endif