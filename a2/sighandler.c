#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

<<<<<<< Updated upstream
int sockfd;

void sighandler(int signo) {
	if(signo == SIGINT) {
		printf("Ctrl+C received \n");
		char b[256] = {0};
		strcpy(b, "+exit\n");
		write(sockfd, b, 255);
=======
void sighandler(int signo) {
	if(signo == SIGINT) {
		printf("Ctrl+C received \n");
		//sem_unlink("");
		//shmctl (id , IPC_RMID , 0)
>>>>>>> Stashed changes
		exit(0);
	}
}

/*
<<<<<<< Updated upstream
Do not delete this comment block. Tells you how to register a signal handler
=======
>>>>>>> Stashed changes
int main() {
	signal(SIGINT, sighandler);
	while(1) {}
}*/