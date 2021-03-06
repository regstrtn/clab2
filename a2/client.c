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
#include "sighandler.c"

extern int sockfd;

void clearinput(char *incoming, int prev) {
	//if(prev)	printf("%c[2A\r%c[2K\r", 27, 27);
	//if(prev)	printf("%c[1A\r%c[2K\r",27,27);
	printf("%c[1A\r%c[2K\r",27,27);
	int charstoreplace = 8;
	if(strncmp(incoming, "broadcast:", 10)==0) printf("%s", incoming);
	else {
		printf("\r%c[2K\r",27);
		if(strchr(incoming, '+')==NULL) 
			printf("%*s%s", 10, "ME:",incoming+charstoreplace);
	}
		//printf("%s------------------------------------------------------\n", incoming);
}

void sendlotsmessages(int sockfd) {
	char *clients[] = {"broadcast", "client0", "client1", "client3", "client4", "client5"};
	char *msg[] = {"lorem", "ipsum", "dolor", "sit", "amet", "computing lab", "assignment"};
	int i;
	sleep(30);
	int num = rand()%100+100;
	for(i=0;i<num;i++) {
		char b[256] = {0};
		int c = rand()%6;
		int m = rand()%7;
		sprintf(b, "%s:%s\n", clients[c], msg[m]);
		write(sockfd, b, 255);
	}
}

int main() {
	//int sockfd;
	int portno, n;
	struct hostent* host;
	struct sockaddr_in serv;
	char *buffer = NULL;
	char incoming[256];
	size_t len;
	portno = 5001;
	signal(SIGINT, sighandler);		//Register signal handler
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	int reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse));
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
	//sendlotsmessages(sockfd);
	int prev = 0;
	while(1) {
		usleep(1000*100);
		int r = poll(fds, 2, -1);
		int i; 
		for(i = 0;i<2;i++) {
			if(fds[i].revents & POLLIN) {
				bzero((char*)incoming, 256);
				int bytesread = read(fds[i].fd, incoming, 255);
				if(i==0) {
					prev = 0;	
					if(bytesread<=0) {
						printf("Server disconnected\n");
						exit(0);
					}
					else {
						//printf("%c[1A\r%c[2K\r\n\r%c[2K\r%s\n--------------------------------------------\n", 27,27, 27, incoming);
						printf("%s", incoming);
						//printf("--------------------------------------------\n");
						fflush(NULL);
						bzero((char*)incoming, 256);
					}
				}
				else if(i==1) {							//Send message to server
					clearinput(incoming, prev);
					prev = 1;
					write(sockfd, incoming, 255);
					bzero((char*)incoming, 256);
				}
			}
		}
	}
}