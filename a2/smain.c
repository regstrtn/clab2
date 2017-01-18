#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "sighandler.c"

typedef struct {
	int id;
	char name[30];
	int fd;
	time_t conntime;
} cli;

typedef struct {
	char message[256];
	char recname[30];
	char sendername[30];
	int sender;
	int receiver;
	time_t mtime;
} msg;

void enqueue(char *str, msg* mbuffer, int *q) {
	//Put message in message buffer queue
	strcpy(mbuffer->message, str);
}

void retrievemessages(msg *mbuffer) {
	//Send message to correct client
	printf("I will send message to correct client");
}

char* getrecname(char *rawmsg) {
	//Get receiver name from the message
	int colonpos = (int)(strchr(rawmsg, ':')-rawmsg);
	char *cliname = (char*)malloc(30*sizeof(char));
	strncpy(cliname, rawmsg, colonpos);
	cliname[colonpos+1] = '\0';
	return cliname;
}

void fillclientdetails(cli *a, int *ctr) {
	a->id = *ctr;
	sprintf(a->name, "client%d ", *ctr);
	a->conntime = time(0)%10000;
	printf("%d %s %ld\n", a->id, a->name, a->conntime);
}

void handleclients(cli* clilist, int* ctr, msg* mbuffer, int *q) {
	cli* me = clilist+(*ctr);
	printf("Printing my information: %s\n", me->name);
	printf("Receiver name: %s\n", getrecname("Hello:World"));
	enqueue("foobar", mbuffer, q);
}

int allowconnection(int *ctr) {
	if(*ctr >1) return 0;
	return 1;
}

int main() {
	int sockfd, newsockfd, n, cli_len, portno, pid;
	struct sockaddr_in serv;
	struct sockaddr_in cli_addr;

	portno = 5000;
	signal(SIGINT, sighandler);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		perror("socket: ");
		return 1;
	}
	bzero((char*)&serv, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_port = htons(portno);
	serv.sin_addr.s_addr = INADDR_ANY;

	bzero((char*)&cli_addr, sizeof(cli_addr));
	cli_len = sizeof(cli_addr);
	if(bind(sockfd, (struct sockaddr*)&serv, sizeof(serv)) == -1) {
		perror("bind: ");
	}
	listen(sockfd, 10);

	int ctrid = shmget(IPC_PRIVATE, sizeof(int), 0666|IPC_CREAT);
	int* ctr = (int*)shmat(ctrid, 0, 0);
	int clilistid = shmget(IPC_PRIVATE, 5*sizeof(cli), 0666|IPC_CREAT);
	cli* clilist = (cli*)shmat(clilistid, 0, 0);
	int mbufferid = shmget(IPC_PRIVATE, 100*sizeof(msg), 0666|IPC_CREAT);
	msg *mbuffer = (msg*)shmat(mbufferid, 0, 0);
	int qid = shmget(IPC_PRIVATE, 3*sizeof(int), 0666|IPC_CREAT);
	int *q = (int*)shmat(qid, 0, 0);
	*ctr = 0;
	newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len);
	
	while(1) {
		//newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len);
		if(allowconnection(ctr)) {
			write(newsockfd, "3 clients connected\n", 40); 
			(*ctr)++;
			fillclientdetails(clilist+(*ctr), ctr);
			pid = 0;
			//pid = fork();
			if(pid == 0) {	//Child process
				handleclients(clilist, ctr, mbuffer, q);
			}
			else if(pid >0) {	//Parent process
				//close(newsockfd);
				continue;
			}
			usleep(1000*100);
		}
		else {
			usleep(100*1000);
			write(newsockfd, "Connection limit exceeded\n", 30);
			//close(newsockfd);
			continue;
		}
		retrievemessages(mbuffer);
	}
}