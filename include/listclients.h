#ifndef LISTCLIENTS_H
#define LISTCLIENTS_H

#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "structs.h"
#include "channel.h"


#define MAX_CLIENTS 10
#define DATE_LEN 26

extern ChannelNode *channel_head;
extern ClientNode *head;

void remove_client(int fd);
void freeAllClients(ClientNode *head);
void edit_Nickname(int fd, char *nickname);
bool nickname_already_exist(char *name);
ClientNode *add_client_list(int fd, char *ip, struct sockaddr_in addr);
ClientNode *research_client_fd(int fd_client);
ClientNode *research_client_name(char *name_client);


#endif