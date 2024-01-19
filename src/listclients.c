#include <unistd.h>
#include <time.h>

#include "../include/server.h"
#include "../include/listclients.h"

void remove_client(int fd)
{
	ClientNode *current = head;
	ClientNode *previous = NULL;

	while (current != NULL)
	{
		if (current->client_fd == fd)
		{
			if (previous == NULL)
			{
				head = current->next;
			}
			else
			{
				previous->next = current->next;
			}
			close(current->client_fd);
			free(current);
			return;
		}
		previous = current;
		current = current->next;
	}
}

void edit_Nickname(int fd, char *nickname)
{
	ClientNode *current = head;
	while (current)
	{
		if (current->client_fd == fd)
		{
			strcpy(current->nickname, nickname);
			break;
		}
		current = current->next;
	}
}

void freeAllClients(ClientNode *head)
{
    ClientNode *current = head;
    while (current) {
        ClientNode *next = current->next;
		close(current->client_fd);
        free(current);
        current = next;
    }
}

bool nickname_already_exist(char *name)
{
	ClientNode *current = head;
	while (current)
	{
		if (strcmp(current->nickname, name) == 0)
		{
			return true;
		}
		current = current->next;
	}
	return false;
}

ClientNode *add_client_list(int fd, char *ip, struct sockaddr_in addr)
{
	ClientNode *newNode = (ClientNode *)malloc(sizeof(ClientNode));
	test_error(!newNode, "Failed to allocate memory for new client node");
	newNode->client_fd = fd;
	memset(newNode->client_ip, '\0', INET_ADDRSTRLEN);
	strcpy(newNode->client_ip, ip);
	newNode->client_port = ntohs(addr.sin_port);
	newNode->next = head;
	time_t current_time;
	time(&current_time);
	strncpy(newNode->connection_time, ctime(&current_time), DATE_LEN - 1);
	newNode->connection_time[DATE_LEN - 1] = '\0';
	memset(newNode->channel, '\0', CHANNEL_LEN);
	head = newNode;

	return newNode;
}


ClientNode *research_client_fd(int fd_client)
{
	ClientNode *current_client = head;

	while (current_client)
	{
		if (current_client->client_fd == fd_client)
		{
			return current_client;
		}
		current_client = current_client->next;
	}
	return NULL;
}

ClientNode *research_client_name(char *name_client)
{
	ClientNode *current_client = head;

	while (current_client)
	{
		if (strcmp(current_client->nickname, name_client) == 0)
		{
			return current_client;
		}
		current_client = current_client->next;
	}
	return NULL;
}
