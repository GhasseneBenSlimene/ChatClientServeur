#ifndef SERVER_H
#define SERVER_H

#include <poll.h>


#include "msg_struct.h"
#include "structs.h"
#include "channel.h"
#include "listclients.h"


void handle_ctrlc(int signal);
void response_server(struct message *msg, char *nickname, char *info, int pld_len, int TYPE);
int handle_bind(const char *port);
void initialize_fds(struct pollfd *fds, int sfd);
void add_new_client(struct pollfd *fds, int sfd);
void clean_up_client(struct pollfd *fds, int i);
void handle_disconnection(struct pollfd *fds, int i);
void edit_Nickname(int fd, char *nickname);
void echo_server(struct pollfd *fds, int i);
void handle_clients(struct pollfd *fds, int sfd);



#endif