CC = gcc
CFLAGS = -Wall
SRC_DIR = src
OBJ_DIR = obj
INCLUDE_DIR = include

OBJ_COMMON = $(OBJ_DIR)/common.o
OBJ_CHANNEL = $(OBJ_DIR)/channel.o
OBJ_SERV = $(OBJ_DIR)/server.o
OBJ_LISTCLIENT = $(OBJ_DIR)/listclients.o
OBJ_CLIENT = $(OBJ_DIR)/client.o $(OBJ_COMMON)
OBJ_SERVER = $(OBJ_DIR)/server.o $(OBJ_COMMON) $(OBJ_CHANNEL) $(OBJ_LISTCLIENT)

all: client server

client: $(OBJ_CLIENT)
	$(CC) $(CFLAGS) -o $@ $^

server: $(OBJ_SERVER)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/common.h $(INCLUDE_DIR)/server.h $(INCLUDE_DIR)/listclients.h $(INCLUDE_DIR)/channel.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/*.o client server
