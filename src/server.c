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

extern ChannelNode *channel_head;
extern ClientNode *head;


void handle_ctrlc(int signal) {
    freeAllClients(head);
    freeAllChannels(channel_head);

    exit(EXIT_SUCCESS);
}


void response_server(struct message *msg, char *nickname, char *info, int pld_len, int TYPE)
{
	char name[strlen(nickname)];
	strcpy(name, nickname);
	memset(msg, 0, sizeof(struct message));
	strcpy(msg->nick_sender, name);
	strcpy(msg->infos, info);
	msg->pld_len = pld_len;
	msg->type = TYPE;
}

int handle_bind(const char *port)
{
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	test_error(getaddrinfo(NULL, port, &hints, &result) != 0, "getaddrinfo()\n");

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype,
					 rp->ai_protocol);
		if (sfd == -1)
		{
			continue;
		}
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			break;
		}
		close(sfd);
	}

	test_error(rp == NULL, "Could not bind\n");
	freeaddrinfo(result);
	return sfd;
}

void initialize_fds(struct pollfd *fds, int sfd)
{
	fds[0].fd = sfd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	for (int i = 1; i < MAX_CLIENTS; i++)
	{
		fds[i].fd = -1;
		fds[i].events = POLLIN;
		fds[i].revents = 0;
	}
}

void add_new_client(struct pollfd *fds, int sfd)
{
	struct sockaddr_in cli;
	socklen_t len = sizeof(cli);
	int new_fd = accept(sfd, (struct sockaddr *)&cli, &len);
	test_error(new_fd == -1, "accept\n");

	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(cli.sin_addr), client_ip, INET_ADDRSTRLEN);
	ClientNode *newNode = add_client_list(new_fd, client_ip, cli);
	memset(newNode->nickname, 0, NICK_LEN); // Initialize nickname

	for (int j = 1; j < MAX_CLIENTS; j++)
	{
		if (fds[j].fd == -1)
		{
			fds[j].fd = new_fd;
			fds[j].events = POLLIN | POLLHUP;
			fds[j].revents = 0;
			break;
		}
	}
	printf("New client connected with IP address: %s and port number: %u on socket %d\n", client_ip, newNode->client_port, new_fd);

	// Inform the new client to set their nickname
	struct message msg;
	memset(&msg, 0, sizeof(msg));
	msg.pld_len = 0;
	strcpy(msg.nick_sender, "Server");
	msg.type = NICKNAME_INFOS;
	strcpy(msg.infos, "please login with /nick <your pseudo>");
	write_full(new_fd, &msg, sizeof(msg));
}


void clean_up_client(struct pollfd *fds, int i)
{
	close(fds[i].fd);
	fds[i].fd = -1;
	fds[i].events = 0;
	fds[i].revents = 0;
}

void handle_disconnection(struct pollfd *fds, int i)
{
	int fdi = fds[i].fd;
	remove_client(fds[i].fd);
	clean_up_client(fds, i);
	printf("socket %d disconnected\n", fdi);
}

void echo_server(struct pollfd *fds, int i)
{
	struct message msg;
	char buf[MSG_LEN];
	memset(&msg, 0, sizeof(struct message));
	memset(buf, 0, MSG_LEN);
	int received = read_full(fds[i].fd, &msg, sizeof(msg));

	if (received > 0)
	{
		switch (msg.type)
		{
		case DISCONECT:
			handle_disconnection(fds, i);
		case NICKNAME_NEW:

			// Extract nickname from the infos field
			if (strlen(msg.infos) > 0 && strlen(msg.infos) < NICK_LEN)
			{
				char proposed_nick[strlen(msg.infos) + 1];
				strcpy(proposed_nick, msg.infos);
				proposed_nick[strlen(msg.infos)] = '\0'; // Null terminate

				// Validate nickname - Add checks for valid characters here

				bool valid = name_accepted(proposed_nick);

				if (strlen(proposed_nick) <= NICK_LEN && valid)
				{
					
					bool nick_exists = nickname_already_exist(proposed_nick);
					if (!nick_exists)
					{
						edit_Nickname(fds[i].fd, proposed_nick);
						strcpy(buf, "Your nickname is accepted.");
						response_server(&msg, "Server", "y", strlen(buf), NICKNAME_NEW);
					}

					else
					{
						strcpy(buf, "Nickname already exists, choose another one.");
						response_server(&msg, "Server", "n", strlen(buf), NICKNAME_NEW);
					}
				}
				else
				{
					strcpy(buf, "Invalid nickname. Only use alphanumeric characters and no spaces.");
					response_server(&msg, "Server", "n", strlen(buf), NICKNAME_NEW);
				}
			}
			else
			{
				strcpy(buf, "Invalid nickname. Choose a shorter nickname.");
				response_server(&msg, "Server", "n", strlen(buf), NICKNAME_NEW);
			}

			write_full(fds[i].fd, &msg, sizeof(msg));
			write_full(fds[i].fd, buf, strlen(buf));
			break;

		case NICKNAME_LIST:
			strcpy(buf, "Online users are :\n");
			ClientNode *current = head;
			while (current)
			{
				if (current->client_fd != fds[i].fd)
				{
					strcat(buf, "				- ");
					strcat(buf, current->nickname);
					strcat(buf, "\n");
				}
				current = current->next;
			}
			response_server(&msg, "Server", "\0", strlen(buf), NICKNAME_LIST);
			write_full(fds[i].fd, &msg, sizeof(msg));
			write_full(fds[i].fd, buf, strlen(buf));
			break;

		case NICKNAME_INFOS:

			current = research_client_name(msg.infos);
			if (current)
			{
				strcpy(buf, current->nickname);
				strcat(buf, " connected since ");
				strcat(buf, current->connection_time);
				strcat(buf, " with IP address ");
				strcat(buf, current->client_ip);
				strcat(buf, " and port number ");
				char port[10];
				memset(port, 0, 10);
				sprintf(port, "%u", current->client_port);
				strcat(buf, port);
			}
			else
			{
				strcpy(buf, "User ");
				strcat(buf, msg.infos);
				strcat(buf, " does not exist.");
			}

			response_server(&msg, "Server", "\0", strlen(buf), NICKNAME_INFOS);
			write_full(fds[i].fd, &msg, sizeof(msg));
			write_full(fds[i].fd, buf, strlen(buf));

		case UNICAST_SEND:
			read_full(fds[i].fd, buf, msg.pld_len);
			buf[msg.pld_len] = '\0';
			current = research_client_name(msg.infos);
			if (current)
			{
				response_server(&msg, msg.nick_sender, "\0", strlen(buf), UNICAST_SEND);
				write_full(current->client_fd, &msg, sizeof(msg));
				write_full(current->client_fd, buf, strlen(buf));
			}
			else
			{
				strcpy(buf, "User ");
				strcat(buf, msg.infos);
				strcat(buf, " does not exist.");
				response_server(&msg, "Server", "\0", strlen(buf), UNICAST_SEND);
				write_full(fds[i].fd, &msg, sizeof(msg));
				write_full(fds[i].fd, buf, strlen(buf));
			}

			break;

		case BROADCAST_SEND:

			read_full(fds[i].fd, buf, msg.pld_len);
			buf[msg.pld_len] = '\0';
			char nick_sender[NICK_LEN];
			strcpy(nick_sender, msg.nick_sender);
			response_server(&msg, msg.nick_sender, "\0", msg.pld_len, BROADCAST_SEND);
			current = head;
			while (current)
			{
				if (strcmp(current->nickname, nick_sender) != 0)
				{
					write_full(current->client_fd, &msg, sizeof(msg));
					write_full(current->client_fd, buf, strlen(buf));
				}
				current = current->next;
			}
			break;

		case ECHO_SEND:
			read_full(fds[i].fd, buf, msg.pld_len);
			buf[msg.pld_len] = '\0';
			response_server(&msg, msg.nick_sender, "\0", msg.pld_len, ECHO_SEND);

			write_full(fds[i].fd, &msg, sizeof(msg));
			write_full(fds[i].fd, buf, strlen(buf));
			break;

		case MULTICAST_CREATE:
			char proposed_channel[CHANNEL_LEN];
			char channel_leaving[CHANNEL_LEN];
			current = research_client_fd(fds[i].fd);

			if (strcmp(current->channel, "\0") != 0)
				strcpy(channel_leaving, current->channel);

			strcpy(proposed_channel, msg.infos);
			proposed_channel[strlen(msg.infos)] = '\0';
			ChannelNode *channel = create_channel(proposed_channel, fds[i].fd);
			if (channel)
			{
				strcpy(channel->creator, msg.nick_sender);
				time_t current_time;
				time(&current_time);
				strncpy(channel->connection_time, ctime(&current_time), DATE_LEN - 1);
				channel->connection_time[DATE_LEN - 1] = '\0';

				strcpy(buf, "You have created channel ");
				strcat(buf, msg.infos);
				char channel_name[CHANNEL_LEN] = {'\0'};
				strcpy(channel_name, msg.infos);
				response_server(&msg, "Server", channel_name, strlen(buf), MULTICAST_CREATE);
			}
			else
			{
				strcpy(buf, "Creative failure");
				response_server(&msg, "Server", "\0", strlen(buf), MULTICAST_CREATE);
			}

			write_full(fds[i].fd, &msg, sizeof(msg));
			write_full(fds[i].fd, buf, strlen(buf));

			if (channel) {

				add_client_to_channel(fds[i].fd, channel);

				if (strcmp(channel_leaving, "\0") != 0)
					inform_channel_members_quit(channel_leaving, current->nickname);

			}

			break;

		case MULTICAST_LIST:
			strcpy(buf, "The channels are :\n");
			ChannelNode *current_channel = channel_head;
			while (current_channel)
			{

				strcat(buf, "				- ");
				strcat(buf, current_channel->name);
				strcat(buf, "\n");

				current_channel = current_channel->next;
			}

			response_server(&msg, "Server", "\0", strlen(buf), MULTICAST_LIST);
			write_full(fds[i].fd, &msg, sizeof(msg));
			write_full(fds[i].fd, buf, strlen(buf));

			break;

		case MULTICAST_JOIN:
			channel = research_channel(msg.infos);
			current = research_client_fd(fds[i].fd);
			char leaving_channel[CHANNEL_LEN] = {'\0'};

			if (strcmp(current->channel, "\0") != 0)
				strcpy(leaving_channel, current->channel);

			char user[NICK_LEN] = {'\0'};
			strcpy(user, msg.nick_sender);
			if (channel)
			{
				add_client_to_channel(fds[i].fd, channel);

				strcpy(buf, "INFO> You have joined channel ");
				strcat(buf, channel->name);
				char channel_name[CHANNEL_LEN] = {'\0'};
				strcpy(channel_name, channel->name);
				response_server(&msg, "Server", channel_name, strlen(buf), MULTICAST_JOIN);
			}
			else
			{
				strcpy(buf, "INFO> This channel doesn't exist");
				response_server(&msg, "Server", "\0", strlen(buf), MULTICAST_JOIN);
			}

			write_full(fds[i].fd, &msg, sizeof(msg));
			write_full(fds[i].fd, buf, strlen(buf));

			if (channel)
			{
				inform_channel_members_arrive(channel->name, user);
				
				if (strcmp(leaving_channel, "\0") != 0)
					inform_channel_members_quit(leaving_channel, user);
			}

			break;

		case MULTICAST_SEND:
			char channel_multicast[CHANNEL_LEN] = {'\0'};
			strcpy(channel_multicast, msg.infos);
			read_full(fds[i].fd, buf, msg.pld_len);
			buf[msg.pld_len] = '\0';
			response_server(&msg, msg.nick_sender, "\0", msg.pld_len, MULTICAST_SEND);
			current = head;
			while (current)
			{
				if ((strcmp(current->channel, channel_multicast) == 0) && (current->client_fd != fds[i].fd))
				{
					write_full(current->client_fd, &msg, sizeof(msg));
					write_full(current->client_fd, buf, strlen(buf));
				}
				current = current->next;
			}
			break;

		case MULTICAST_QUIT:
			current = research_client_fd(fds[i].fd);
			memset(leaving_channel, 0, CHANNEL_LEN);
			strcpy(leaving_channel, current->channel);

			if (strcmp(msg.infos, current->channel) != 0)
			{
				strcpy(buf, "INFO> You are not in this channel");
				response_server(&msg, "Server", "\0", strlen(buf), MULTICAST_QUIT);
				write_full(fds[i].fd, &msg, sizeof(msg));
				write_full(fds[i].fd, buf, strlen(buf));
			}
			else
			{
				quit_channel(current);
				inform_channel_members_quit(leaving_channel, current->nickname);
			}
			break;

		case MULTICAST_INFO:

			current_channel = research_channel(msg.infos);

			strcpy(buf, "This channel does not exist.");

			if(current_channel)
			{
				strcpy(buf, "The channel ");
				strcat(buf, msg.infos);
				strcat(buf, " has been created by ");
				strcat(buf, current_channel->creator);
				strcat(buf, " since ");
				strcat(buf, current_channel->connection_time);
				strcat(buf, ".\nThere are ");

				char members[sizeof(int)] = {'\0'};
				sprintf(members, "%i", current_channel->nb_client);
				strcat(buf, members);

				strcat(buf, " members :\n");

				current = head;
				while(current){
					if (strcmp(current->channel, current_channel->name) == 0)
					{
						strcat(buf, "			- ");
						strcat(buf, current->nickname);
						strcat(buf, "\n");

					}
					current = current->next;
				}


			}

			response_server(&msg, "Server", "\0", strlen(buf), MULTICAST_INFO);
			write_full(fds[i].fd, &msg, sizeof(msg));
			write_full(fds[i].fd, buf, strlen(buf));
			break;

		case MULTICAST_RENAME:

			char name_tmp[CHANNEL_LEN] = {'\0'};
			strcpy(name_tmp, msg.infos);

			ClientNode * client = research_client_fd(fds[i].fd);
			channel = research_channel(client->channel);

			strcpy(buf, "The channel name is not valid.");

			if ((name_accepted(name_tmp)) && !(channel_already_exist(name_tmp)))
			{

				strcpy(buf, "You cannot change the name of the channel.");

				if (strcmp(client->nickname, channel->creator) == 0)
				{

					strcpy(buf, "The channel was renamed by ");
					strcat(buf, name_tmp);

					response_server(&msg, "Server", name_tmp, strlen(buf), MULTICAST_RENAME);

					current = head;
					while(current){
						if ((strcmp(current->channel, client->channel) == 0) && (current->client_fd != fds[i].fd))
							{
								strcpy(current->channel, name_tmp);
								write_full(current->client_fd, &msg, sizeof(msg));
								write_full(current->client_fd, buf, strlen(buf));

							}

						current = current->next;
					}

					strcpy(client->channel, name_tmp);
					strcpy(channel->name, name_tmp);

					response_server(&msg, "Server", name_tmp, strlen(buf), MULTICAST_RENAME);
					write_full(fds[i].fd, &msg, sizeof(msg));
					write_full(fds[i].fd, buf, strlen(buf));

				}
			}
			
			response_server(&msg, "Server", "\0", strlen(buf), MULTICAST_RENAME);
			write_full(fds[i].fd, &msg, sizeof(msg));
			write_full(fds[i].fd, buf, strlen(buf));


			break;

		case FILE_REQUEST:
			read_full(fds[i].fd, buf, msg.pld_len);
			buf[msg.pld_len] = '\0';
			char file_name[MSG_LEN] = {'\0'};
			strcpy(file_name, buf);
			memset(buf, '\0', MSG_LEN);

			current = research_client_name(msg.infos);

			if (current)
			{
				strcpy(buf, msg.nick_sender);
				strcat(buf, " wants you to accept the transfer of the file named \"");
				strcat(buf, file_name);
				strcat(buf, "\". Do you accept ? [Y/N]");
				response_server(&msg, msg.nick_sender, "\0", strlen(buf), FILE_REQUEST);
				write_full(current->client_fd, &msg, sizeof(msg));
				write_full(current->client_fd, buf, strlen(buf));

			}
			
			else {
				strcpy(buf, "user ");
				strcat(buf, msg.infos);
				strcat(buf, "does not exist.");
				response_server(&msg, msg.nick_sender, "\0", strlen(buf), ECHO_SEND);
				write_full(fds[i].fd, &msg, sizeof(msg));
				write_full(fds[i].fd, buf, strlen(buf));
				}	
			break;

        case MULTICAST_FILE_REQUEST:

            char channel_name[CHANNEL_LEN] = {'\0'};
            current = research_client_fd(fds[i].fd);

            if (current->channel)
            {

                strcpy(channel_name, current->channel);

                // Lire le nom du fichier depuis le payload
				char file_name[MSG_LEN] = {'\0'};
				strcpy(file_name, msg.infos);

                // Construire le message à envoyer à chaque membre du canal
                strcpy(buf, msg.nick_sender);
                strcat(buf, " wants you to accept the transfer of the file named \"");
                strcat(buf, file_name);
                strcat(buf, "\". Do you accept? [Y/N]");

                // Envoyer la demande à tous les membres du channel
                ClientNode* current = head;
                while (current)
                {
                if (strcmp(current->channel, channel_name) == 0 && current->client_fd != fds[i].fd)
                {
                    response_server(&msg, msg.nick_sender, "\0", strlen(buf), FILE_REQUEST);
                    write_full(current->client_fd, &msg, sizeof(msg));
                    write_full(current->client_fd, buf, strlen(buf));
                }
                current = current->next;
                }
            }
            break;

		case FILE_REJECT:

			strcpy(buf, msg.nick_sender);
			strcat(buf, " cancelled file transfer");

			current = research_client_name(msg.infos);
			response_server(&msg, msg.nick_sender, "\0", strlen(buf), FILE_REJECT);
			write_full(current->client_fd, &msg, sizeof(msg));
			write_full(current->client_fd, buf, strlen(buf));

			break;


		case FILE_ACCEPT:
			char port[10];
			memset(port, 0, 10);

			char name_author[NICK_LEN] = {'\0'};
			strcpy(name_author, msg.nick_sender);
			current = research_client_name(msg.nick_sender);
			strcpy(buf, current->client_ip);
			sprintf(port, ":%u", current->client_port+1);
			strcat(buf, port);

			current = research_client_name(msg.infos);
			
			response_server(&msg, "Server", name_author, strlen(buf), FILE_ACCEPT);
			write_full(current->client_fd, &msg, sizeof(msg));
			write_full(current->client_fd, buf, strlen(buf));
			break;

		case FILE_ACK:

			char *str = NULL;
			char filename[INFOS_LEN] = {'\0'};
			str = strtok(msg.infos, ":");
			strcpy(filename, str);

			char *author = NULL;
			author = strtok(NULL, ":");

			current = research_client_name(author);
			response_server(&msg, msg.nick_sender, filename, 0, FILE_ACK);

			write_full(current->client_fd, &msg, sizeof(msg));
			break;

		default:
			printf("Unsupported message type received.\n");
			break;
		}
	}
	else if (received == 0) // Client has closed the connection
	{
		handle_disconnection(fds, i);
	}
	else
	{
		perror("recv");
	}
}

void handle_clients(struct pollfd *fds, int sfd)
{
	while (1)
	{
		test_error(poll(fds, MAX_CLIENTS, -1) == -1, "poll\n");

		if (fds[0].fd == sfd && ((fds[0].revents & POLLIN) == POLLIN))
		{
			add_new_client(fds, sfd);
		}

		for (int i = 1; i < MAX_CLIENTS; i++)
		{
			if (fds[i].fd != -1 && ((fds[i].revents & POLLIN) == POLLIN))
			{
				echo_server(fds, i);
			}
			else if (fds[i].fd != -1 && ((fds[i].revents & POLLHUP) == POLLHUP))
			{
				clean_up_client(fds, i);
			}
		}
	}
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	int sfd;
	
	signal(SIGINT, handle_ctrlc);

	sfd = handle_bind(argv[1]);
	struct pollfd fds[MAX_CLIENTS];

	test_error((listen(sfd, SOMAXCONN)) != 0, "listen()\n");
	initialize_fds(fds, sfd);
	handle_clients(fds, sfd);

	close(sfd);
	return EXIT_SUCCESS;
}
