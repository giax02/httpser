#define VERSION "2.0.0"
#define BUFFER_IN_LEN 16
#define BUF_SIZE 500
/*#define MAX_CLIENT_COUNT 3*/
#define SILENCE 0
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
#include <netinet/tcp.h>

char cmd1[] = "clientcount\n";
char cmd2[] = "help\n";

struct client {
	unsigned int position;		/*position in client_sockets array, and a sort of id of the client*/
	struct pollfd socket;		/*holds the ad-client socket given by accept()*/
	struct sockaddr_in addr;	/*holds the IP of the client:)*/
	struct client * next_ptr;	/*points to the next client in the queue*/
};

void print_ver_and_id(void) {
	printf("Simple HTTP server\n");
	printf("Version: %s\n", VERSION);
	printf("Author: Giacomo Leandrini\n");
}

char html_response[] = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\r\n\
<html>\n\
<head><title>Sito web di Giacomol02</title></head>\n\
<body>Ciao, questo sito e' hostato dal PC di Giacomol02 e sul server HTTP di Giacomol02 :)</body>\n\
</html>";

char html_payload[] = "<html>\n\
<head><title>Test</title></head>\n\
<body>Ciao mondo!</body>\n\
</html>";

void error(char *msg) {
 perror(msg);
 exit(1);
}

void debug_printf(char buf[]) {
	if(!SILENCE) printf("%s", buf);
}

void refresh_positions(struct client * tail) {
		int ctr = 1;
		while(tail != NULL)
		{
			tail->position = ctr;
			ctr++;
			tail = tail->next_ptr;
		}
		return;
}

void add_client(struct client ** tail, int sockfd, struct sockaddr_in cli_addr) {
	struct client * ttail = *tail;				/*ttail = temporary tail, used to leave *tail untouched*/
	if(*tail == NULL){
		*tail = malloc(sizeof(struct client));
		if(*tail == NULL)
			error("Could not allocate memory for the first first client");
		//printf("Memoria allocata, anche se non funzia! %x\n",*tail); /* ora funzia :)*/
		(*tail)->next_ptr = NULL;						
		memset(&((*tail)->socket), 0, sizeof((*tail)->socket));
		(*tail)->addr = cli_addr;
		(*tail)->socket.fd = sockfd;
		(*tail)->socket.events = POLLIN;
		refresh_positions(*tail);
		return;
	} else {  
		while (ttail->next_ptr != NULL) {
			ttail = ttail->next_ptr;
		}
	}
	ttail->next_ptr = malloc(sizeof(struct client));
	ttail->next_ptr->next_ptr = NULL;
	memset(&(ttail->next_ptr->socket), 0, sizeof(ttail->next_ptr->socket));
	ttail->next_ptr->addr = cli_addr;
	ttail->next_ptr->socket.fd = sockfd;
	ttail->next_ptr->socket.events = POLLIN;
	refresh_positions(*tail);
	return;
}

void remove_client(unsigned int position, struct client ** tail) {
	struct client * ttail = *tail;				/*ttail = temporary tail, used to leave *tail untouched*/
	if (position == 1) {
		if((*tail)->next_ptr == NULL) {			/*first element, none ahead	|O            */
			free(*tail);						/*free to ttail, not tail, just in case the free modifies the address stored in ttail*/
			*tail = NULL;
		} else {								/*first element, some ahead	|Oo...o       */
			free(*tail);						/*same as above*/
			*tail = (*tail)->next_ptr;
		}
	} else {
		while (ttail->position != (position-1)) {
				ttail = ttail->next_ptr;
		}
		if(ttail->next_ptr->next_ptr == NULL) { /*n# element, none ahead 	|o...oooO      */
			free(ttail->next_ptr);
			ttail->next_ptr = NULL;				/*here we are not touching the actual tail (tail) so we end up using ttail*/
		} else {								/*n# element, some ahead 	|o...ooOo...o  */
			free(ttail->next_ptr);
			ttail->next_ptr = ttail->next_ptr->next_ptr;
		}
	}
	refresh_positions(*tail);
}

int client_count(struct client * tail)
{	
	int count = 0;
	while (tail != NULL) {
		count++;
		tail = tail->next_ptr;
	}
	return count;
}

/*
void check_for_new_client(int sockfd, struct sockaddr_in cli_addr, int clilen, struct client * tail, char * cli_addr_buf) {
	int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd > 0) {
		add_client(&tail, newsockfd, cli_addr);
		if (inet_ntop(AF_INET, &(cli_addr.sin_addr), cli_addr_buf, sizeof(cli_addr_buf)) == NULL) error("Error converting IPV4 addr to string");
		printf("Client %d connected from %s \n", client_count(tail), cli_addr_buf);
	}
}
*/

void shell_parse(char * buf, struct client * tail) {
	
	if (strcmp(buf, cmd1) == 0)
		printf("Connected clients: %d\n", client_count(tail));
	else if (strcmp(buf, cmd2) == 0)
		printf("help in progress\n");
	else
		printf("Unknown command\n");
	return;
}

int parse_http_get(char * buffer) {
	char get_str[] = "GET / HTTP/1.1";
	int i = 0;
	while(buffer[i] == get_str[i]) 
	{
		if (i == (strlen(get_str) - 1))
			return 1;
		i++;
	}
	return 0;
}

int main(int argc, char * argv[]) {

	unsigned int seed = arc4random();
	srand(seed);
	char buffer[BUFFER_IN_LEN];
	char shell_buf[128];
	char cli_addr_buf[INET_ADDRSTRLEN];
	struct client * tail = NULL;	/*mythical client forward linked list tail (or head?)*/
									/*it looks like this(TAIL)->()->()->()->(NULL)*/
									/*ok it is the head but it works the same so whatever */

	int sockfd, newsockfd, portno, clilen, n;
	struct sockaddr_in serv_addr, cli_addr;

	print_ver_and_id();

	if (argc != 2) {
		fprintf(stderr, "Usage: %s port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);	/*requesting a fd for a new socket to the kernel*/
	if (sockfd < 0)
		error("ERROR opening socket");

	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));	/*to be able to reuse the socket soon*/
	
	bzero((char *) &serv_addr, sizeof(serv_addr));	/*setting serv_addr to zero (bzero is a memset wrapper afaik)*/

	portno = atoi(argv[1]);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");

	int flags = fcntl(sockfd, F_GETFD, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);	/*rendo non bloccante stdin*/

	listen(sockfd,5);

	clilen = sizeof(cli_addr);

	printf("Listening for TCP on port: %d\n", portno);
	//printf("Max clients number: %d\n", MAX_CLIENT_COUNT);

	while (1) {


		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd > 0) {
			add_client(&tail, newsockfd, cli_addr);

			setsockopt(newsockfd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

			int idle = 2, interval = 3, probes = 3;
			setsockopt(newsockfd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
			setsockopt(newsockfd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
			setsockopt(newsockfd, IPPROTO_TCP, TCP_KEEPCNT, &probes, sizeof(probes));

			if (inet_ntop(AF_INET, &(cli_addr.sin_addr), cli_addr_buf, sizeof(cli_addr_buf)) == NULL)
				error("Error converting IPV4 addr to string");
			printf("Client %d connected from %s \n", client_count(tail), cli_addr_buf);
		}
		
		/*local stdin shell*/
		memset(&shell_buf, 0, sizeof(shell_buf));	/*cleaning buffer*/
		if (read(STDIN_FILENO, shell_buf, sizeof(shell_buf) - 1) > 0) {	/*reading n bytes, expects terminal in canonical mode*/
			char command_symbol[] = "/";
			if(strncmp(shell_buf, command_symbol, 1) == 0)	/*check for the command symbol aka is the message a command?*/
				shell_parse(shell_buf + 1, tail);
		}

		//check_for_new_client(sockfd, cli_addr, clilen, tail, cli_addr_buf);

		/* Ok now lets check for incoming data*/
		if (client_count(tail) > 0) {	/*but only if there is any client connected*/
			struct client * index  = tail;
			while(index != NULL)	/*THis cycles through the client forward linked list via index*/
			{
				//struct client * tclient = *tail;
				int ret = poll(&(index->socket), 1, 0);	/*fetch events*/
				if (index->socket.revents & POLLIN){	/*simply put if there is something to read in the current client fd*/
					memset(&buffer, 0, sizeof(buffer));
					n = read(index->socket.fd, buffer, sizeof(buffer) - 1);
					if (n == 0) {
						printf("Client %d disconnected\n", index->position);
						remove_client(index->position, &tail);
						goto end;	/* client disconnected? :( let's move onto the next one :)*/
					}
					if (n < 0) error("Error reading from the socket");
					//printf("Client %d says: %s", index->position, buffer);	/*land here only if everything works out fine*/
					printf("%s", buffer);

					if(parse_http_get(buffer)) {
						write(index->socket.fd, html_response, strlen(html_response));
					}
				}
				end:
				index = index->next_ptr;
			}
		}
	}
	return 0;
}