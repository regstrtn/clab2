#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

int wellformatted(char *buffer) {
	char c = ';';
	int count = 0, i = 0;
	for(i=0;i<strlen(buffer); i++) {
		if(buffer[i] == ';') count++;
	}
	if(count == 1 && buffer[strlen(buffer)-2] == ';') return 0;			//if no pattern
	if(count == 1 && buffer[0] == ';') return 0;						//if no text
	if(count>1) return 0;												//if multiple delimiters
	if(count<1) return 0;												//if no delimiter
	return 1;
}

void readinput(char *buffer) {
	int len;
	getline(&buffer, &len, stdin);
	while(!wellformatted(buffer)) {												//Validate user input
		printf("Enter input in the following format - 'haystack;needle': ");
		getline(&buffer, &len, stdin);
	}
	char op[256];
	strcpy(op, buffer);
	char *haystack = strtok(op, ";");
	char *needle = strtok(NULL, ";");
	printf("Haystack: %s Needle: %s", haystack, needle);
}

int main() {
	int sockfd, newsockfd, n, cli_len, port_no;
	struct sockaddr_in serv;
	
	//Server Information. Port 5000;
	port_no = 5000;
	char buffer[256], haystack[256], needle[256];
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct hostent *server;							//<netdb.h>
	server = gethostbyname("127.0.0.1"); 			//localhost
	bzero((char*)&serv, sizeof(serv));
	//AF_INET - IPv4. SOCK_STREAM - TCP Protocol. 0 - Assign a protocol for the address type, family combination. Here TCP will be assigned.  

	serv.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *) &serv.sin_addr.s_addr, server->h_length);
	serv.sin_port = htons(port_no);
	

	bzero(buffer, 256);
	readinput(buffer); 								//Process user input and store in buffer
	connect(sockfd, (struct sockaddr *) &serv, sizeof(serv));
	write(sockfd, buffer, strlen(buffer));
	bzero(buffer, 256);
	read(sockfd, buffer, 255);
	printf("Occurrences: %s\n", buffer);
}