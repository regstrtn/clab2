#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>

int main() {
	int sockfd, portno, n;
	struct hostent* host;
	struct sockaddr_in serv;
	char *buffer = NULL;
	char incoming[256];
	size_t len;
	portno = 5000;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		perror("socket: ");
		exit(0);
	}
	bzero((char*)&serv, sizeof(serv));
	host = gethostbyname("127.0.0.1");
	serv.sin_family = AF_INET;
	serv.sin_port = htons(portno);
	bcopy((char*)host->h_addr, (char*)&serv.sin_addr.s_addr, host->h_length);
	if(connect(sockfd, (struct sockaddr *)&serv, sizeof(serv))==-1) {
		printf("Could not connect to server on port %d\n", portno);
	}
	struct pollfd fds[] = {
		{sockfd, POLLIN},
		{0, POLLIN}
	};
	while(1) {
		//printf("I keep coming here");
		usleep(1000*100);
		//buffer = NULL;			//required to be null by getline
		//getline(&buffer, &len, stdin);
		//write(sockfd, buffer, strlen(buffer));
		//read(sockfd, incoming, 255);
		int r = poll(fds, 2, -1);
		int i;
		for(i = 0;i<2;i++) {
			if(fds[i].revents & POLLIN) {
				read(fds[i].fd, incoming, 255);
				printf("Incoming from %d: %s\n", fds[i].fd, incoming);
			}
		}
		bzero((char*)incoming, 256);
	}
}