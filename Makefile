CC = gcc
CFLAGS = -Wall -pedantic -pthread
INCLUDE_DIR = src
FILES_SERVER = src/common.c src/messenger.c src/request_handler.c src/lists.c src/board_handler.c src/thread_handler.c
FILES_CLIENT = src/common.c src/messenger.c src/request_sender.c src/client_message.c

all: client server
debug: client_debug server_debug

client: src/client.c ${FILES_CLIENT}	
	${CC} ${CFLAGS} -L${INCLUDE_DIR} -o client src/client.c ${FILES_CLIENT}

server: src/server.c ${FILES_SERVER}	
	${CC} ${CFLAGS} -L${INCLUDE_DIR} -o server src/server.c ${FILES_SERVER}

client_debug: src/client.c ${FILES_CLIENT}
	${CC} ${CFLAGS} -g -L${INCLUDE_DIR} -o client src/client.c ${FILES_CLIENT}

server_debug: src/server.c ${FILES_SERVER}
	${CC} ${CFLAGS} -g -L${INCLUDE_DIR} -o server src/server.c ${FILES_SERVER}

.PHONY: clean
clean:
	rm client server
