#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void sighandler(int signo) {
	if(signo == SIGINT) {
		printf("Ctrl+C received \n");
		//sem_unlink("");
		//shmctl (id , IPC_RMID , 0)
		exit(0);
	}
}

/*
int main() {
	signal(SIGINT, sighandler);
	while(1) {}
}*/