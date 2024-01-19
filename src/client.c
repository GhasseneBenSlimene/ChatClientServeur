#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <stdbool.h>
#include <ctype.h>
#include <poll.h>
#include <libgen.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#include "../include/common.h"
#include "../include/msg_struct.h"

#define NB_SOCKETS 3

const char *ServAddr;
const char *ServPort;
char nickname[NICK_LEN];
char nickname_tmp[NICK_LEN];
char channel_tmp[CHANNEL_LEN] = {'\0'};
char channel[CHANNEL_LEN] = {'\0'};
int client_port = 0;
char str_tmp[FILE_LEN];

struct pollfd fds[NB_SOCKETS];

void save_str(char *str)
{
	strcpy(str_tmp, str);
}

void delete_str()
{
	memset(str_tmp, 0, 260);
}
void freeAllSockets()
{

	int i = 0;
	for (i = 0; i < NB_SOCKETS; i++)
	{
		close(fds[i].fd);
	}

	return;
}

void disconnection(int signal)
{

	struct message msg;
	memset(&msg, 0, sizeof(msg));
	msg.type = DISCONECT;
	strcpy(msg.nick_sender, nickname);

	write_full(fds[1].fd, &msg, sizeof(msg));

	freeAllSockets();

	exit(EXIT_SUCCESS);
	return;
}

void display_prompt(const char *username, const char *channel)
{
	if (strcmp(channel, "") == 0)
	{
		printf("%%terminal_%s> ", username);
	}
	else
	{
		printf("%%terminal_%s[%s]> ", username, channel);
	}
	fflush(stdout); // Force l'écriture du buffer pour afficher le prompt
}

void define_server(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("Usage: ./client <server_name> <server_port>\n");
		exit(EXIT_FAILURE);
	}

	ServAddr = argv[1];
	ServPort = argv[2];
}

void sendfile_to_client(int sockfd, const char *filename)
{
	struct message response;
	memset(&response, 0, sizeof(struct message));

	strcpy(response.nick_sender, nickname);
	strcpy(response.infos, filename);
	response.type = FILE_SEND;

	int filefd = open(filename, O_RDONLY);
	if (filefd < 0)
	{
		response.pld_len = 0;
		write_full(sockfd, &response, sizeof(response));
		perror("Failed to open the file");
		return;
	}

	// Obtenir la taille du fichier
	struct stat file_stat;
	if (fstat(filefd, &file_stat) < 0)
	{
		perror("Failed to get file stats");
		close(filefd);
		return;
	}
	response.pld_len = file_stat.st_size;
	write_full(sockfd, &response, sizeof(response));

	off_t offset = 0;
	ssize_t sent_bytes = 0;
	printf("sending the file ..");
	while (offset < file_stat.st_size)
	{
		sent_bytes = sendfile(sockfd, filefd, &offset, file_stat.st_size - offset);
		printf(".");
		if (sent_bytes <= 0)
		{
			perror("Failed to send file");
			break;
		}
	}
	printf(" done\n");

	close(filefd);
	close(sockfd);
}

int receive_file(int sockfd, int file_len, char *sender_name)
{
	if (file_len == 0)
	{
		printf("info>  Transmission stopped\n");
		return 0;
	}
	char new_path[1024] = "./recieved_files/";
	char temp_path[1024];
	char count_str[3];
	int filefd, count = 0;

	// Utilise la fonction basename pour extraire le nom du fichier du chemin
	strcat(new_path, sender_name);
	strcat(new_path, "_file");

	// Copie le chemin initial dans temp_path
	strcpy(temp_path, new_path);
	struct stat st = {0};

	if (stat("recieved_files", &st) == -1)
	{
		mkdir("recieved_files", 0700);
	}
	int max_attempts = 100;
	// Vérifie si le fichier existe et ajoute un chiffre si c'est le cas
	while ((filefd = open(temp_path, O_WRONLY | O_CREAT | O_EXCL, 0666)) == -1 && count < max_attempts)
	{
		sprintf(count_str, "%d", ++count); // Convertit le compteur en chaîne
		strcpy(temp_path, new_path);	   // Réinitialise temp_path
		strcat(temp_path, count_str);	   // Ajoute le compteur à temp_path
	}
	if (count >= max_attempts)
	{
		filefd = -1;
	}
	if (filefd < 0)
	{
		perror("Failed to open file for writing");
		return 0;
	}

	ssize_t bytes_received;
	char buffer[file_len];

	while ((bytes_received = recv(sockfd, buffer, file_len, 0)) > 0)
	{
		write_full(filefd, buffer, bytes_received);
	}

	if (bytes_received < 0)
	{
		perror("Failed to receive file");
		return 0;
	}
	else
		printf("File saved in %s\n", temp_path);

	close(filefd);
	return 1;
}

void ask_nickname(int sockfd)
{
	struct message msg;
	char buf[MSG_LEN];
	char *str;

	memset(&msg, 0, sizeof(struct message));
	memset(buf, 0, MSG_LEN);

	read_full(sockfd, &msg, sizeof(struct message));
	printf("[%s]: %s\n", msg.nick_sender, msg.infos);

	char nickname_tmp[NICK_LEN];
	bool nickname_set = false;

	while (!nickname_set)
	{
		memset(&msg, 0, sizeof(struct message));
		memset(buf, 0, MSG_LEN);

		int n = 0;
		while ((buf[n] = getchar()) != '\n')
		{
			n++;
		}

		// Begin determine_msg logic
		char message[strlen(buf) + 1];
		strcpy(message, buf);
		str = strtok(buf, " ");

		if (strcmp(str, "/nick") == 0)
		{
			msg.type = NICKNAME_NEW;
			str = strtok(NULL, "\n");
			strncpy(msg.infos, str, INFOS_LEN);
		}
		else
		{
			msg.type = -1; // Invalid command for nickname setting
		}
		// End of determine_msg logic

		if (msg.type != NICKNAME_NEW)
		{
			printf("Use /nick followed by your nickname.\n");
		}
		else
		{
			write_full(sockfd, &msg, sizeof(msg));
			strcpy(nickname_tmp, msg.infos);

			memset(&msg, 0, sizeof(struct message));
			memset(buf, 0, MSG_LEN);

			// server's response
			read_full(sockfd, &msg, sizeof(msg));
			read_full(sockfd, buf, msg.pld_len);

			printf("[%s] : %s\n", msg.nick_sender, buf);

			if (strcmp(msg.infos, "y") == 0)
			{
				memset(nickname, 0, NICK_LEN);
				strcpy(nickname, nickname_tmp);
				printf("Welcome to the chat %s\n", nickname);
				nickname_set = true;
			}
		}
	}
}

int handle_connect(const char *Addr, const char *Port)
{
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	test_error(getaddrinfo(Addr, Port, &hints, &result) != 0, "getaddrinfo()");
	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if (sfd == -1)
		{
			continue;
		}

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
		{
			// Obtenez les détails de l'adresse locale associés au socket client
			struct sockaddr_in client_address;
			socklen_t client_address_len = sizeof(client_address);
			if (getsockname(sfd, (struct sockaddr *)&client_address, &client_address_len) == -1)
			{
				perror("Error obtaining the local address of the client.");
				close(sfd);
				exit(EXIT_FAILURE);
			}

			// Le numéro de port local du client est maintenant dans client_address.sin_port
			client_port = ntohs(client_address.sin_port);

			break;
		}
		close(sfd);
	}
	test_error(rp == NULL, "Could not connect\n");
	printf("Connection ... done \n");
	freeaddrinfo(result);
	return sfd;
}

int accept_connection(int listening_scoket)
{
	struct sockaddr_in client_address;
	socklen_t client_address_len = sizeof(client_address);

	int client_socket = accept(listening_scoket, (struct sockaddr *)&client_address, &client_address_len);
	if (client_socket == -1)
	{
		perror("Erreur lors de l'acceptation de la connexion entrante");
	}

	return client_socket;
}

void response(struct message msgstruct, int sockfd)
{
	struct message response;
	memset(&response, 0, sizeof(struct message));

	strcpy(response.nick_sender, nickname);
	strcpy(response.infos, msgstruct.nick_sender);
	response.pld_len = 0;
	response.type = FILE_REJECT;

	char buf[MSG_LEN];
	memset(buf, 0, MSG_LEN);

	int n = 0;
	int cmpt = 0;

	while (cmpt < 3)
	{
		char a = getchar();
		while (a != '\n' && a != EOF)
		{
			buf[n] = a;
			a = getchar();
			n++;
		}

		if (n == 1 && buf[0] == 'Y')
		{
			response.type = FILE_ACCEPT;
			break;
		}
		else if (n == 1 && buf[0] == 'N')
		{
			break;
		}
		else
		{
			printf("Do you accept ? [Y/N]\n");
			cmpt++;
		}
	}

	write_full(sockfd, &response, sizeof(response));
}

void send_message(int sockfd)
{
	struct message msgstruct;
	char buf[MSG_LEN];
	int n = 0;

	memset(&msgstruct, 0, sizeof(struct message));
	memset(buf, 0, MSG_LEN);

	char a = getchar();
	while (a != '\n' && a != EOF)
	{
		buf[n] = a;
		a = getchar();
		n++;
	}

	char *str = NULL;

	if (n > 0)
	{
		// Begin determine_msg logic
		char message[strlen(buf) + 1];
		strcpy(message, buf);
		message[strlen(buf)] = '\0';
		str = strtok(buf, " ");

		if (strcmp(str, "/nick") == 0)
		{
			memset(nickname_tmp, 0, NICK_LEN);
			msgstruct.type = NICKNAME_NEW;
			str = strtok(NULL, "\n");
			if (str != NULL)
			{
				strncpy(msgstruct.infos, str, INFOS_LEN);
				strncpy(nickname_tmp, str, NICK_LEN);
			}
		}
		else if (strcmp(str, "/who") == 0)
		{
			msgstruct.type = NICKNAME_LIST;
			strncpy(msgstruct.infos, "\0", INFOS_LEN);
		}
		else if (strcmp(str, "/whois") == 0)
		{
			msgstruct.type = NICKNAME_INFOS;
			str = strtok(NULL, "\n");
			if (str != NULL)
				strncpy(msgstruct.infos, str, INFOS_LEN);
		}
		else if (strcmp(str, "/msg") == 0)
		{
			msgstruct.type = UNICAST_SEND;
			str = strtok(NULL, " ");
			strncpy(msgstruct.infos, str, INFOS_LEN);
			str = strtok(NULL, "\n");
		}
		else if (strcmp(str, "/msgall") == 0)
		{
			msgstruct.type = BROADCAST_SEND;
			strncpy(msgstruct.infos, "\0", INFOS_LEN);
			str = strtok(NULL, "\n");
		}

		else if (strcmp(str, "/quit") == 0)
		{
			str = strtok(NULL, "\n");

			// Si l'utilisateur n'a pas fourni de nom de channel et qu'il n'est pas dÃ©jÃ  dans un channel
			if ((str == NULL || strcmp(str, "") == 0) && strcmp(channel, "") == 0)
			{
				disconnection(SIGINT);
				exit(EXIT_SUCCESS);
			}
			// Si l'utilisateur n'a pas fourni de nom de channel mais qu'il est dans un channel
			else if (str == NULL || strcmp(str, "") == 0)
			{
				printf("You need to quit the channel first.\n");
				return;
			}
			// Si l'utilisateur a fourni un nom de channel, mais qu'il n'est pas Ã©gal au channel actuel
			else if (strcmp(str, channel) != 0)
			{
				printf("You are not in this channel.\n");
				return;
			}

			// Si l'utilisateur souhaite quitter le channel actuel
			msgstruct.type = MULTICAST_QUIT;

			strcpy(channel_tmp, "");
			strcpy(channel, "");

			if (str != NULL)
				strncpy(msgstruct.infos, str, INFOS_LEN);
		}

		else if (strcmp(str, "/create") == 0)
		{
			memset(channel_tmp, 0, CHANNEL_LEN);
			msgstruct.type = MULTICAST_CREATE;
			str = strtok(NULL, "\n");
			if (str != NULL)
			{
				strncpy(msgstruct.infos, str, INFOS_LEN);
				strncpy(channel_tmp, str, CHANNEL_LEN);
			}
		}
		else if (strcmp(str, "/join") == 0)
		{
			memset(channel_tmp, 0, CHANNEL_LEN);
			msgstruct.type = MULTICAST_JOIN;
			str = strtok(NULL, "\n");
			if (str != NULL)
			{
				strncpy(msgstruct.infos, str, INFOS_LEN);
				strncpy(channel_tmp, str, CHANNEL_LEN);
			}
		}
		else if (strcmp(str, "/channel_list") == 0)
		{
			msgstruct.type = MULTICAST_LIST;
			strncpy(msgstruct.infos, "\0", INFOS_LEN);
		}

		else if (strcmp(str, "/channel_info") == 0)
		{
			msgstruct.type = MULTICAST_INFO;
			str = strtok(NULL, "\n");
			if (str != NULL)
				strncpy(msgstruct.infos, str, INFOS_LEN);
		}

		else if (strcmp(str, "/channel_name") == 0)
		{
			if (strcmp(channel, "") != 0)
			{
				msgstruct.type = MULTICAST_RENAME;
				str = strtok(NULL, "\n");
				if (str != NULL)
					strncpy(msgstruct.infos, str, INFOS_LEN);
			}
			else
			{
				printf("You are not in a channel.\n");
				return;
			}
		}

		else if (strcmp(str, "/send") == 0)
		{
			msgstruct.type = FILE_REQUEST;
			str = strtok(NULL, " ");
			strncpy(msgstruct.infos, str, INFOS_LEN);
			str = strtok(NULL, "\n");
			save_str(str);
		}

		else if (strcmp(str, "/sendall") == 0)
		{
			str = strtok(NULL, "\n");
			if (strcmp(channel, "") != 0)
			{
				msgstruct.type = MULTICAST_FILE_REQUEST;
				strncpy(msgstruct.infos, str, INFOS_LEN);
				save_str(str);
			}
			else
			{
				printf("Usage: /send <receiver_name> <file_name>\n");
				return;
			}
		}

		else if (str[0] == '/')
		{
			printf("The command is not recognised. Please use : </command> <information> <message>\n");
			return;
		}
		else // echo_send ou multicast_send
		{
			if (strcmp(channel, "") != 0)
			{
				msgstruct.type = MULTICAST_SEND;
				strncpy(msgstruct.infos, channel, INFOS_LEN);
			}
			else
			{
				msgstruct.type = ECHO_SEND;
				strncpy(msgstruct.infos, "\0", INFOS_LEN);
			}

			strcpy(str, message);
		}

		if (str == NULL)
		{
			printf("You have not used the correct method.\n");
			return;
		}

		msgstruct.pld_len = strlen(str);
		strncpy(msgstruct.nick_sender, nickname, strlen(nickname));

		write_full(sockfd, &msgstruct, sizeof(msgstruct));

		if (msgstruct.type == UNICAST_SEND || msgstruct.type == ECHO_SEND || msgstruct.type == BROADCAST_SEND || msgstruct.type == MULTICAST_SEND || msgstruct.type == FILE_REQUEST)
		{
			write_full(sockfd, str, msgstruct.pld_len);
		}
	}
	else if (n == MSG_LEN)
	{
		printf("the message is too long, write a shorter message\n");
	}
}

void receive_message(int sockfd)
{
	struct message msgstruct;
	char buf[MSG_LEN];

	memset(&msgstruct, 0, sizeof(struct message));
	memset(buf, 0, MSG_LEN);

	int len = read_full(sockfd, &msgstruct, sizeof(struct message));

	if (len > 0)
	{
		if (msgstruct.pld_len > MSG_LEN)
		{
			fprintf(stderr, "Message length received exceeds buffer size\n");
			return; // Don't proceed further if the message is too long
		}

		read_full(sockfd, buf, msgstruct.pld_len);
		printf("\n");
		switch (msgstruct.type)
		{
		case NICKNAME_NEW:
			printf("[%s]: %s\n", msgstruct.nick_sender, buf);
			if (strcmp(msgstruct.infos, "y") == 0)
			{
				memset(nickname, 0, NICK_LEN);
				strcpy(nickname, nickname_tmp);
				printf("Your new nickname is %s\n", nickname);
			}
			break;
		case NICKNAME_LIST:
		case NICKNAME_INFOS:
		case UNICAST_SEND:
		case BROADCAST_SEND:
		case MULTICAST_INFO:

			printf("[%s]: %s\n", msgstruct.nick_sender, buf);
			break;

		case MULTICAST_RENAME:
			printf("[%s]: %s\n", msgstruct.nick_sender, buf);

			if (strcmp(msgstruct.infos, "\0") != 0)
			{
				strcpy(channel, msgstruct.infos);
			}

			break;

		case ECHO_SEND:
		case FILE_REJECT:
			printf("%s\n", buf);
			break;

		case MULTICAST_CREATE:
		case MULTICAST_JOIN:
			if (strcmp(msgstruct.infos, "\0") != 0)
			{
				memset(channel, 0, CHANNEL_LEN);
				strcpy(channel, msgstruct.infos);
				printf("[%s] %s\n", channel, buf);
				if (msgstruct.type == MULTICAST_CREATE)
				{
					printf("[%s] INFO> You have joined the channel %s\n", channel, channel);
				}
			}

			else
			{
				printf("%s\n", buf);
			}
			break;
		case MULTICAST_QUIT:
		case MULTICAST_LIST:
			printf("%s\n", buf);
			break;

		case MULTICAST_SEND:
			printf("[%s] %s : %s\n", channel, msgstruct.nick_sender, buf);
			break;

		case FILE_REQUEST:
			printf("%s\n", buf);
			response(msgstruct, sockfd);
			break;

		case FILE_ACCEPT:
			printf("%s accepted file client\n", msgstruct.infos);

			const char *addr = NULL;
			addr = strtok(buf, ":");

			const char *port = NULL;
			port = strtok(NULL, "\"");

			int fdc = handle_connect(addr, port);
			sendfile_to_client(fdc, str_tmp);
			break;

		case FILE_ACK:
			printf("%s has received the file %s.\n", msgstruct.nick_sender, msgstruct.infos);
			break;

		default:
			printf("the received message is not available\n");
			break;
		}
	}
	else
	{
		fprintf(stderr, "Connection closed\n");
		disconnection(SIGINT);
	}
}

void initialize_fds(int sfd, int listening_socket)
{
	// surveiller stdin pour l'entrÃƒÂ©e
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

	// surveiller sockfd pour les donnÃƒÂ©es entrantes
	fds[1].fd = sfd;
	fds[1].events = POLLIN;

	fds[2].fd = listening_socket;
	fds[2].events = POLLIN;
	fds[2].revents = 0;
}

void echo_client()
{

	while (1)
	{
		display_prompt(nickname, channel);
		int ret = poll(fds, 3, -1); // attend indÃƒÂ©finiment qu'un ÃƒÂ©vÃƒÂ©nement se produise

		if (ret == -1)
		{
			perror("poll");
			exit(EXIT_FAILURE);
		}

		// si des donnÃƒÂ©es sont prÃƒÂªtes sur stdin
		if ((fds[0].revents & POLLIN) == POLLIN)
		{
			send_message(fds[1].fd);
		}

		// si des donnÃƒÂ©es sont prÃƒÂªtes sur sockfd
		if ((fds[1].revents & POLLIN) == POLLIN)
		{
			receive_message(fds[1].fd);
		}

		if ((fds[2].revents & POLLIN) == POLLIN)
		{
			int fd_client_accepted = accept_connection(fds[2].fd);

			// reception de fichier
			struct message msgstruct;
			memset(&msgstruct, 0, sizeof(struct message));

			int len = read_full(fd_client_accepted, &msgstruct, sizeof(struct message));

			if (len > 0)
			{
				if (msgstruct.pld_len > MSG_LEN)
				{
					fprintf(stderr, "Message length received exceeds buffer size\n");
					return; // Don't proceed further if the message is too long
				}

				if (msgstruct.type == FILE_SEND)
					if (receive_file(fd_client_accepted, msgstruct.pld_len, msgstruct.nick_sender))
					{
						struct message msg;
						memset(&msg, 0, sizeof(struct message));
						strcpy(msg.nick_sender, nickname);
						strcpy(msg.infos, msgstruct.infos);
						strcat(msg.infos, ":");
						strcat(msg.infos, msgstruct.nick_sender);
						strcat(msg.infos, ":");
						msg.type = FILE_ACK;

						write_full(fds[1].fd, &msg, sizeof(msg));
					}
				close(fd_client_accepted);
			}
		}
	}
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

int main(int argc, char *argv[])
{
	int sfd;
	int listening_socket;
	define_server(argc, argv);
	sfd = handle_connect(ServAddr, ServPort);

	signal(SIGINT, disconnection);

	char port[10];
	memset(port, 0, 10);
	sprintf(port, "%i", client_port + 1);
	listening_socket = handle_bind(port);
	test_error((listen(listening_socket, SOMAXCONN)) != 0, "listen()\n");
	initialize_fds(sfd, listening_socket);
	ask_nickname(sfd);
	echo_client();
	close(sfd);
	return EXIT_SUCCESS;
}
