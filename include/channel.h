#ifndef CHANNEL_H
#define CHANNEL_H


#include "server.h"
#include "structs.h"
#include <netinet/in.h>


#define MAX_CLIENTS 10
#define DATE_LEN 26

extern ChannelNode *channel_head;
extern ClientNode *head;


ChannelNode *create_channel(char *str, int fd);
bool is_empty(ChannelNode *channel);
bool channel_already_exist(char *name);
void quit_channel(ClientNode *client);
void inform_channel_members_arrive(char* channel, char *client);
void inform_channel_members_quit(char* channel, char *client);
void add_client_to_channel(int fd, ChannelNode *channel);
void freeAllChannels(ChannelNode *head);
bool name_accepted(char *str);
ChannelNode *research_channel(char *name);


#endif