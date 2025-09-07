#define VERSION "1.0.0"
#define BUFFER_IN_LEN 2048
#define BUF_SIZE 500
#define MAX_CLIENT_COUNT 3
#define SILENCE 1
#define ACTIVE 1
#define MAX_ATTEMPTS_ID_GEN 100

/* USE PORT 2145 */

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <malloc.h>

struct client {
	uint8_t id;		/*max 128 ids be careful*/
	unsigned int position;	/*position in client_sockets array*/
	struct pollfd socket;
	struct sockaddr_in addr;
	struct client * next_ptr;
};

void print_ver_and_id(void) {
	printf("Simple HTTP server\n");
	printf("Version: %s\n", VERSION);
	printf("Author: Giacomo Leandrini\n");
}

void error(char *msg) {
 perror(msg);
 exit(1);
}

void debug_printf(char buf[]) {
	if(!SILENCE) printf("%s", buf);
}

uint8_t id_gen(struct client * tail) {
	if (tail.next_ptr == NULL) return rand();
	for(int attempts = 1; i < MAX_ATTEMPTS_ID_GEN; attempts++) {
		uint8_t id = rand();
		while (tail->next_ptr != NULL) {
			if (tail->id == id) break;
			if (tail->next_ptr == NULL) return id;
			tail = tail->nextptr;
		}
	}
	return 0; //cant find a suitable id or max attempts reached
}

uint8_t add_client(struct client * tail, int sockfd) {
	struct client * ttail = tail;
	unsigned int ctr = 1
	if(tail == NULL) {	/* zero clients rn, crating first element */
		tail = malloc(sizeof(struct client));
	} else {	/* every other element */
		ctr++;
		while (tail->next_ptr != NULL) {
			tail = tail->next_ptr;
			ctr++;
		}
		tail->next_ptr = malloc(sizeof(struct client));
		tail = tail->next_ptr;
	}
		tail->next_ptr = NULL;
		tail->id = id_gen(tail);		/* to implement error check (id_gen can return 0) */
		tail->position = ctr;						
		memset(&(tail->socket), 0, sizeof(struct pollfd));
		tail->socket.fd = sockfd;
		tail->socket.events = POLLIN;
}

void remove_client(unsigned int position, struct client * tail) {
	struct client * ttail = tail;
	while (ttail->next_ptr->position != (position - 1)) {
		ttail = ttail->next_ptr;
	}
	struct client * temp = ttail->next_ptr;
	free(temp);
	ttail->next_ptr = ttail->next_ptr->next_ptr;
}

int client_count(struct client * tail)
{
	int count = 0;
	while (tail->next_ptr != NULL) count++;
	return count;

}

int main(int argc, char * argv[]) {

	unsigned int seed = arc4random();
	srand(seed);

	char buffer[BUFFER_IN_LEN];
	struct client * tail = NULL;
	int sockfd, newsockfd, portno, clilen;
	struct sockaddr_in serv_addr, cli_addr;

	//struct pollfd * client_sockets = malloc( sizeof(struct pollfd) * MAX_CLIENT_COUNT);
	//struct client *clients = malloc(sizeof(struct client) * MAX_CLIENT_COUNT);

	print_ver_and_id();

	if (argc != 2) {
		fprintf(stderr, "Usage: %s port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);	/*requesting a fd for a new socket to the kernel*/
	if (sockfd < 0)
		error("ERROR opening socket");

	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));	/*socket option to be reused asap?*/

	bzero((char *) &serv_addr, sizeof(serv_addr));	/*setting serv_addr to zero (bzero is a memset wrapper as far as I know)*/

	portno = atoi(argv[1]);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");

	int flags = fcntl(sockfd, F_GETFD, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	listen(sockfd,5);

	clilen = sizeof(cli_addr);

	printf("Listening for TCP on port: %d\n", portno);
	printf("Max clients number: %d\n", MAX_CLIENT_COUNT);

	while (1) {
		debug_printf("start infinite loop\n");
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd > 0) {

			add_client(tail, newsockfd);
			printf("Client %d connesso\n", client_count());
			memset(&client_sockets[client_count], 0, sizeof(struct pollfd));
			client_sockets[client_count].fd 	= newsockfd;
			client_sockets[client_count].events = POLLIN;
		}
		//debug_printf("prima del if client count %d\n", client_count);
		if(client_count >= 1) {
			//printf("if client_count maggiore di 1?");
			int ret = poll(client_sockets, MAX_CLIENT_COUNT, 0);
			for (int i = 1; i <= client_count; i++) {
				//printf("Itearazione %d",i);
				if (client_sockets[i].revents & POLLIN) {
					//memset(&buffer, 0, sizeof(buffer));
					memset(&buffer, 0, sizeof(buffer));
					n = read(client_sockets[i].fd, buffer, sizeof(buffer) - 1);
					if (n == 0) printf("client %d disconnected", client_count);
					if (n < 0) error("Error reading from the socket");
					printf("Client %d says: %s",i , buffer);
				}
			}
		}
	}
	return 0;
}