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
#define MAXCLIENTS 2

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

void printclientdetails(cli *a) {
	printf("Printing details of %s: ", a->name);
	printf("%d %d %ld\n", a->id, a->fd, a->conntime);
}

char* showonline(cli* clilist) {
	cli *curr = clilist;
	int i;
	char *b = (char*)calloc(256,sizeof(char));
	strcat(b, "Online users: \n");
	for(i=0;i<MAXCLIENTS;i++) {
		strcat(b, (curr+i)->name); 
		strcat(b, "\n");
	}
	printf("%s", b);
	return b;
}

void retrievemessages(msg *mbuffer, int *q, cli* clilist) {
	//Send message to correct client
	msg *curr = mbuffer + q[1];
	int i;
	while(1) {
		if(q[1]==q[0]) {
			usleep(100*1000); 
			continue;
		}
		char *r = curr->recname;
		for(i=0;i<5;i++) {
			if(strcmp(clilist[i].name, r)==0) {
				break;
			}
		}
		if(i==5) printf("Receiver %s does not exist\n", r);
		else {
			printf("Value of i: %d\n", i);
			printf("Intended receiver: %s id: %d fd: %d\n", clilist[i].name, clilist[i].id, clilist[i].fd);
			printf("Message: %s\n", curr->message);
		}
		//write(clilist[i].fd, curr->message, 255);
		curr = curr+1;
		q[1]++;
	}
}

void enqueue(msg newmsg, msg* mbuffer, int *q) {
	//Put message in message buffer queue
	msg* curr = mbuffer + q[0];
	q[0]++;
	strcpy(curr->message, newmsg.message);
	strcpy(curr->recname, newmsg.recname);
}

char* getrecname(char *rawmsg) {
	//Get receiver name from the message
	int colonpos = (int)(strchr(rawmsg, ':')-rawmsg);
	char *cliname = (char*)malloc(30*sizeof(char));
	strncpy(cliname, rawmsg, colonpos);
	cliname[colonpos+1] = '\0';
	return cliname;
}

char *getmsg(char *rawmsg) {
	int colonpos = (int)(strchr(rawmsg, ':')-rawmsg);
	char *msg = (char*)malloc(256*sizeof(char));
	strncpy(msg, rawmsg+colonpos+1, strlen(rawmsg)-colonpos);
	msg[100] = '\0';
	return  msg;
}

void fillclientdetails(cli *a, int *ctr, int newsockfd) {
	a->id = *ctr;
	sprintf(a->name, "client%d", *ctr);
	a->fd = newsockfd;
	a->conntime = time(0)%10000;
	printclientdetails(a);
	showonline(a-(*ctr));
}

void handleclients(cli* clilist, int* ctr, msg* mbuffer, int *q) {
	cli* me = clilist+((*ctr)-1);
	char b[256] = {0}; 
	/*
	while(1) {
		bzero((char*)b, 256);
		read(me->fd, b, 255);
		write(me->fd, b, 255);
	} */
	sprintf(b, "ID: %d Name: %d fd: %d\n", me->id, me->name, me->fd);
	write(me->fd, b, 256);
	write(me->fd, showonline(clilist), 256);
	int counter = 0;
	while(1) {
		msg newmsg;
		read(me->fd, newmsg.message, 255);
		if(strcmp(newmsg.message, "+online")==0) {
			showonline(clilist-(*ctr));
		}
		char* rec = getrecname(newmsg.message);
		char *msgtxt = getmsg(newmsg.message);
		strcpy(newmsg.recname, rec);
		strcpy(newmsg.message, msgtxt);
		newmsg.mtime = time(0);
		enqueue(newmsg, mbuffer, q);
		counter++;
		//if(counter%3==0) retrievemessages(mbuffer, q, clilist);
	}

}

int allowconnection(int *ctr) {
	if(*ctr >1) return 0;
	return 1;
}

int main() {
	int sockfd, newsockfd, n, cli_len, portno, pid;
	struct sockaddr_in serv;
	struct sockaddr_in cli_addr;

	portno = 5001;
	signal(SIGINT, sighandler);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	int reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse));
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
	q[0] = 0; q[1] = 0;
	
	int pid1 = fork(); 							//Create a new process P for delivering messages
	if(pid1==0) {
		retrievemessages(mbuffer, q, clilist);
	}
	while(1) {
		sleep(1);
		newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len);
		if(allowconnection(ctr)) {
			fillclientdetails(clilist+(*ctr), ctr, newsockfd);
			(*ctr)++;
			pid = fork();
			if(pid == 0) {	//Child process
				handleclients(clilist, ctr, mbuffer, q);
			}
			else if(pid >0) {	//Parent process
				close(newsockfd);
				continue;
			}
		}
		else {
			write(newsockfd, "Connection limit exceeded\n", 30);
			close(newsockfd);
			continue;
		}
	}
}