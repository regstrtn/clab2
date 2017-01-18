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

int main() {
	int sockfd, portno, n;
	struct hostent* host;
	struct sockaddr_in serv;
	char *buffer = NULL;
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
	while(1) {
		getline(&buffer, &len, stdin);
		printf("%s\n", buffer);
		write(sockfd, buffer, strlen(buffer));
		buffer = NULL;
	}
}