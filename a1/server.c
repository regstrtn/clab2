#include <stdio.h>
#include <unistd.h> //read, write, fork
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int search(char buffer[]) {
	int count = 0;
	char *token, *haystack, *needle;
	char s[2] = ";";
	printf("%s\n", buffer);
	
	/*Split input into haystack and needle*/
	haystack = strtok(buffer, s);
	needle = strtok(NULL, s);
	needle[strlen(needle)-1] = '\0';
	char *tmp = haystack;
	while((tmp = strstr(tmp, needle))!=NULL) {
		tmp++;
		count++;
	}
	return count;
}

int main() {
	int sockfd, newsockfd, n, cli_len, port_no;
	struct sockaddr_in serv;
	port_no = 5000;
	char buffer[256];
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero((char*)&serv, sizeof(serv));
	//AF_INET - IPv4. SOCK_STREAM - TCP Protocol. 0 - Assign a protocol for the address type, family combination. Here TCP will be assigned.  
	serv.sin_family = AF_INET;
	serv.sin_port = htons(port_no);
	serv.sin_addr.s_addr = INADDR_ANY;
	struct sockaddr_in cli_addr;
	bzero((char*)&cli_addr, sizeof(cli_addr)); 
	cli_len = sizeof(cli_addr);
	bind(sockfd, (struct sockaddr*)&serv, sizeof(serv));
	int pid;
	listen(sockfd, 5);

	while(1) { 											//continue to loop forever
		newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &cli_len);
		
		if((pid = fork()) > 0) { 						//Parent
			close(newsockfd); 
			continue;
		}
		else if(pid == 0) {								//Child
			bzero(buffer, 256);
			read(newsockfd, buffer, 255);
			printf("Client message: %s\n", buffer);
			int n = search(buffer);						//Count occurrences of pattern in text
			bzero(buffer, 256);
			sprintf(buffer, "%d\0", n);
			printf("My response: %s\n", buffer);
			write(newsockfd, buffer, sizeof(buffer));
			break;
		}
		else if(pid<0) {								//Fork fail
			close(newsockfd);
			write(newsockfd, "Fork call failed\n", sizeof(buffer));
			break;
		}
	}
}