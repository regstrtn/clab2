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

int exceedsize(char *buffer) {
	int delimpos = (int) (strchr(buffer, ';') - buffer);
	if(delimpos > 30) return 1;
	int patsize = strlen(buffer) - delimpos;
	if(patsize>7) return 1; //Taking into account the newline character and index offset
	return 0;
}

int validchar(char *buffer) {
	int i;
	int delimpos = (int) (strchr(buffer, ';') - buffer);
	//for(i=0;i<delimpos;i++) printf("%d ", buffer[i]); printf("\n");
	for(i=0;i<delimpos;i++) {
		if(buffer[i]<97 || buffer[i]>122) {
			if(buffer[i]<65 || buffer[i]>90) {
				if(buffer[i]!=32) {
					return 0;
				}
			}
		}
	}
	for(i=delimpos+1;i<strlen(buffer)-1; i++) {
		if(buffer[i]>'z' || buffer[i]<'a') { 
			if(buffer[i]>'Z' || buffer[i]<'A')
				return 0;
		}
	}
	return 1;
}

void readinput(char *buffer) {
	int len;
	getline(&buffer, &len, stdin);
	int correct = 0;
	while(!correct) {
		correct = 1;
		while(exceedsize(buffer)) {                                                //check input size
			printf("String or pattern exceeds size limits. Text <=30. Pattern <=5: ");
			correct = 0;
			getline(&buffer, &len, stdin);
		}
		while(!wellformatted(buffer)) {												//Validate user input
			printf("Enter input in the following format - 'haystack;needle': ");
			correct = 0;
			getline(&buffer, &len, stdin);
		}
		while(!validchar(buffer)) {
			printf("Invalid input. Allowed characters: Text [a-zA-Z blank]. Pattern [a-z]. Enter again: ");
			correct = 0;
			getline(&buffer, &len, stdin);
		}
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
	
	if(sockfd == -1) {   						//Could not create socket. Error.
		printf("Could not create socket\n");
		return 1;
	}
	
	struct hostent *server;							//<netdb.h>
	server = gethostbyname("127.0.0.1"); 			//localhost
	bzero((char*)&serv, sizeof(serv));
	//AF_INET - IPv4. SOCK_STREAM - TCP Protocol. 0 - Assign a protocol for the address type, family combination. Here TCP will be assigned.  

	serv.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *) &serv.sin_addr.s_addr, server->h_length);
	serv.sin_port = htons(port_no);
	bzero(buffer, 256);
	readinput(buffer); 								//Process user input and store in buffer
	if(connect(sockfd, (struct sockaddr *) &serv, sizeof(serv)) == -1) {
		printf("Could not connect to server on port: %d\n", port_no);
		return 1;
	}
	write(sockfd, buffer, strlen(buffer));
	bzero(buffer, 256);
	read(sockfd, buffer, 255);
	printf("Occurrences: %s\n", buffer);
	close(sockfd);
}