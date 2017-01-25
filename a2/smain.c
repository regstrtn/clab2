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
#define MAXCLIENTS 4

typedef struct {
	int id;
	char name[30];
	int fd;
	int rid;
	char rname[30];
	int status;
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

//Return receiver name from the message
char* getrecname(char *rawmsg) {
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
	msg[strlen(rawmsg)-colonpos] = '\0';
	return  msg;
}

void printclientdetails(cli *a) {
	printf("Printing details of %s : ", a->name);
	printf("%d %d %ld random id: %d random name: %s\n", a->id, a->fd, a->conntime, a->rid, a->rname);
}

char* showonline(cli* clilist) {
	cli *curr = clilist;
	int i;
	char *b = (char*)calloc(256,sizeof(char));
	strcat(b, "Online users:\n");
	for(i=0;i<MAXCLIENTS;i++) {
		if(clilist[i].status>0) {
			strcat(b, (curr+i)->name); 
			strcat(b, "\n");
		}
	}
	return b;
}

void checkmsgq(cli* me, int* ctr, msg *mbuffer, int *q) {
	if(q[1]>=q[0]) return;
	int rear = q[1];
	int i;
	cli * clilist = me - (*ctr-1);
	for(i=0;i<MAXCLIENTS;i++) {
		if(strcmp(mbuffer[rear].recname, clilist[i].name)==0) {
			if(clilist[i].status<0) {
				q[1]++;
				rear = q[1];
			}
		}
	}
	//printf("q0 %d q1 %d procname %s recname %s message %s\n", q[0], q[1], me->name, mbuffer[rear].recname, mbuffer[rear].message);
	if(strcmp(mbuffer[rear].recname, me->name)==0) {
		char b[256] = {0};
		//printf("Message is for %s %s. FD: %d\n", me->name, mbuffer[rear].message, me->fd);
		sprintf(b, "%s:%s", mbuffer[rear].sendername, mbuffer[rear].message);
		write(me->fd, b, 255);
		q[1]++;
	}
}

void enqueue(msg m1, msg* mbuffer, int *q, cli* clilist, cli* sender) {
	//Put message in message buffer queue
	int i;
	if(strchr(m1.message, ':')!=NULL && m1.recname[0]==0) {
		char *r = getrecname(m1.message);
		if(r[0]==0) {
			write(sender->fd, "Please include client name.\n", 255);
			return;
		}
		char *text = getmsg(m1.message);
		strcpy(m1.recname, r);
		strcpy(m1.message, text);
	}
	else if(m1.recname[0]==0 && strchr(m1.message, ':')==NULL){
		write(sender->fd, "Please include client name.\n", 255);
		return;
	}
	printf("Enqueueing messages of %s. Message: %s", m1.recname, m1.message);
	for(i=0;i<MAXCLIENTS;i++) {
		if(strcmp(m1.recname, sender->name)==0) {
			//Uncomment the following two lines to stop clients sending messages to themselves
			write(sender->fd, "Cannot send messages to yourself.\n", 255); 
			return;
		}
		if(strcmp(m1.recname, clilist[i].name)==0) break;
	}
	if(i==MAXCLIENTS) {
		write(sender->fd, "This client does not exist\n", 255);
		return;
	}
	else if(clilist[i].status < 0) {
		write(sender->fd, "This client has disconnected\n", 255);
		return;
	}
	msg* curr = mbuffer + q[0];
	q[0]++;
	*curr = m1; 				//Structs can be copied in C
}


void disconnect(msg* mbuffer, int *q, cli* clilist, cli* sender) {
	sender->status = -1;
	if(strcmp(mbuffer[q[1]].recname, sender->name)==0) {
		q[1]++;
	}
	close(sender->fd);
	printf("%s disconnected\n", sender->name);
	exit(0);
}

void broadcast(msg m1, msg* mbuffer, int *q, cli* clilist, cli* sender) {
	int i;
	msg marr[MAXCLIENTS];
	for(i=0;i<MAXCLIENTS;i++) {
		if(strncmp(m1.message, "broadcast:", 10)==0) {
			printf("So you want to broadcast?\n");
			char *text = getmsg(m1.message);
			strcpy(m1.message, text);
		}
		else if(strcmp(m1.message, "+exit\n")==0) {
			printf("So you want to exit?\n");
			sprintf(m1.message, "%s is now offline.", sender->name);
		}
		strcpy(m1.recname, clilist[i].name);
		enqueue(m1, mbuffer, q, clilist, sender);
	}
}

void fillclientdetails(cli *a, int *ctr, int newsockfd) {
	a->id = *ctr;
	int r = 10000+rand()%89999;
	a->rid = r;
	sprintf(a->name, "client%d", *ctr);
	int length = 10;
	char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    while (length-- > 0) {			//Assign random string as rname
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        a->rname[length] = charset[index];
    }
    a->rname[10] = '\0';
	a->fd = newsockfd;
	a->status = 1;
	a->conntime = time(0)%10000;
	printclientdetails(a);
	printf("%s", showonline(a-(*ctr)));
}

void handlemessage(cli* clilist, int* ctr, msg* mbuffer, int *q) {
		cli* me = clilist+((*ctr)-1);
		char b[256] = {0};
		sprintf(b, "ID: %d Name: %s fd: %d randomid: %d randomstring: %s\n", me->id, me->name, me->fd, me->rid, me->rname);
		write(me->fd, b, 255);
		bzero(b, 256);
		char *onlineusers = showonline(clilist);
		write(me->fd, onlineusers, 255);
		while(1) {
			msg m1;
			checkmsgq(me, ctr, mbuffer, q);
			int bytesread = read(me->fd, m1.message, 255);
			usleep(1000*10); //printf("Message: %s From: %s\n", m1.message, me->name);
			if(bytesread>0) {
				strcpy(m1.sendername, me->name);
				m1.mtime = time(0);
				if(strcmp(m1.message, "+online\n")==0) {
					onlineusers = showonline(clilist);
					write(me->fd, onlineusers, 255);
					continue;
				}
				if(strcmp(m1.message, "+exit\n")==0) {
					disconnect(mbuffer, q, clilist, me);
					continue;
				}
				else if(strncmp(m1.message, "broadcast:", 10)==0) {
					broadcast(m1, mbuffer, q, clilist, me);
					continue;
				}
				bzero(m1.recname, 30);
				enqueue(m1, mbuffer, q, clilist, me);
			}
		}
}

int allowconnection(int *ctr) {
	if(*ctr >= MAXCLIENTS) return 0;
	return 1;
}

int main() {
	int sockfd, newsockfd, n, cli_len, portno, pid;
	struct sockaddr_in serv;
	struct sockaddr_in cli_addr;

	portno = 5001;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
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
	
	/*
	int pid1 = fork(); 							//Create a new process P for delivering messages
	if(pid1==0) {
		retrievemessages(mbuffer, q, clilist);
	} 
	//Actually this way of retrieving messages will not work. Forget about it. 
	*/
	while(1) {
		if((newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len))==-1) {
			continue;
		}
		fcntl(newsockfd, F_SETFL, O_NONBLOCK);
		if(allowconnection(ctr)) {
			fillclientdetails(clilist+(*ctr), ctr, newsockfd);
			(*ctr)++;
			pid = fork();
			if(pid == 0) {	//Child process
				handlemessage(clilist, ctr, mbuffer, q);
			}
			else if(pid >0) {	//Parent process
				//close(newsockfd);
				continue;
			}
		}
		else {
			write(newsockfd, "Connection limit exceeded\n", 255);
			close(newsockfd);
			continue;
		}
	}
}