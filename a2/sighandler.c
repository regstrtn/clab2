#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

int sockfd;

void sighandler(int signo) {
	if(signo == SIGINT) {
		printf("Ctrl+C received \n");
		char b[256] = {0};
		strcpy(b, "+exit\n");
		write(sockfd, b, 255);
		exit(0);
	}
}

/*
Do not delete this comment block. Tells you how to register a signal handler
int main() {
	signal(SIGINT, sighandler);
	while(1) {}
}*/