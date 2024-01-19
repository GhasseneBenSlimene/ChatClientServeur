#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>

#include "../include/server.h"

ChannelNode *channel_head = NULL;
ClientNode *head = NULL;

bool name_accepted(char *str)
{
	bool valid = true;

	for (int i = 0; i < strlen(str); i++)
	{
		if (!((str[i] >= 'a' && str[i] <= 'z') ||
			  (str[i] >= 'A' && str[i] <= 'Z') ||
			  (str[i] >= '0' && str[i] <= '9')))
		{
			valid = false; // Invalid character found
		}
	}

	return valid;
}

bool channel_already_exist(char *name)
{
	ChannelNode *current = channel_head;
	while (current)
	{
		if (strcmp(current->name, name) == 0)
		{
			return true;
		}
		current = current->next;
	}
	return false;
}

ChannelNode *create_channel(char *str, int fd)
{

	bool valid = name_accepted(str);
	if (valid)
	{
		if (channel_already_exist(str))
			return NULL;

		ChannelNode *newNode = (ChannelNode *)malloc(sizeof(ChannelNode));

		test_error(!newNode, "Failed to allocate memory for new ChannelNodenode");
		memset(newNode->name, '\0', CHANNEL_LEN);
		strcpy(newNode->name, str);
		newNode->nb_client = 0;
		newNode->next = channel_head;
		channel_head = newNode;

		return newNode;
	}
	return NULL;
}


bool is_empty(ChannelNode *channel)
{
	if (channel->nb_client == 0)
	{
		ChannelNode *current = channel_head;
		ChannelNode *previous = NULL;

		while (current != NULL)
		{
			if (strcmp(current->name, channel->name) == 0)
			{
				if (previous == NULL)
				{
					channel_head = current->next;
				}
				else
				{
					previous->next = current->next;
				}
				free(current);
				break;
			}
			previous = current;
			current = current->next;
		}
		return true;
	}
	return false;
}

void quit_channel(ClientNode *client)
{
	struct message msg;
	char buf[MSG_LEN] = {'\0'};
	ChannelNode *current = channel_head;
	while (current)
	{
		if (strcmp(client->channel, current->name) == 0)
		{
			strcpy(buf, "You have quit the channel ");
			strcat(buf, current->name);
			current->nb_client--;
			if (is_empty(current))
			{
				strcat(buf, "\nINFO> You were the last member, the channel has been destroyed");
			}
			break;
		}
		current = current->next;
	}
	memset(client->channel, '\0', CHANNEL_LEN);
	response_server(&msg, "Server", "\0", strlen(buf), MULTICAST_QUIT);
	write_full(client->client_fd, &msg, sizeof(msg));
	write_full(client->client_fd, buf, strlen(buf));
	
}

void inform_channel_members_quit(char* channel, char *client)
{
	struct message msg;
	memset(&msg, 0, sizeof(msg));
	char buf[MSG_LEN] = {'\0'};
	strcpy(buf, "INFO> ");
	strcat(buf, client);
	strcat(buf, " has quit ");
	strcat(buf, channel);

	response_server(&msg, "Server", "\0", strlen(buf), MULTICAST_QUIT);

	ClientNode *current = head;
	while (current)
	{
		if (strcmp(current->channel, channel) == 0)
		{
			if (strcmp(current->nickname, client) != 0)
			{
				write_full(current->client_fd, &msg, sizeof(msg));
				write_full(current->client_fd, buf, strlen(buf));
			}
		}
		current = current->next;
	}
}

void inform_channel_members_arrive(char* channel, char *client)
{
	struct message msg;
	memset(&msg, 0, sizeof(msg));
	char buf[MSG_LEN] = {'\0'};
	strcpy(buf, "INFO> ");
	strcat(buf, client);
	strcat(buf, " has joined ");
	strcat(buf, channel);

	response_server(&msg, "Server", "\0", strlen(buf), MULTICAST_JOIN);
	ClientNode *current = head;
	while (current)
	{
		if (strcmp(current->channel, channel) == 0)
		{
			if (strcmp(current->nickname, client) != 0)
			{
				write_full(current->client_fd, &msg, sizeof(msg));
				write_full(current->client_fd, buf, strlen(buf));
			}
		}
		current = current->next;
	}
}

void add_client_to_channel(int fd, ChannelNode *channel)
{
	ClientNode *current = head;
	while (current)
	{
		if (fd == current->client_fd)
		{
			if (strcmp(current->channel, "\0") != 0)
			{
				quit_channel(current);
			}
			channel->nb_client++;
			strcpy(current->channel, channel->name);
			break;
		}
		current = current->next;
	}
}


void freeAllChannels(ChannelNode *head) 
{
    ChannelNode *current = head;
    while (current) {
        ChannelNode *next = current->next;
        free(current);
        current = next;
    }
}

ChannelNode *research_channel(char *name)
{
	ChannelNode *current_channel = channel_head;

	while (current_channel)
	{
		if (strcmp(current_channel->name, name) == 0)
		{
			return current_channel;
		}
		current_channel = current_channel->next;
	}
	return NULL;
}