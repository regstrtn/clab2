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
#include <pthread.h>
#define MAXCLIENTS 40

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

//Struct for multithread argument passing
typedef struct {
	msg* mbuffer;
	cli* clients;
	int* q;
} thread_arg;

//Unlink semaphores before exiting
void sighandler(int signo) {
	if(signo == SIGINT) {
		sem_unlink("/mbuffsem");
		sem_unlink("/q");
		exit(0);
	}
}

void printclientdetails(cli *a) {
	printf("Printing details of %s : ", a->name);
	printf("%d %d %ld random id: %d random name: %s\n", a->id, a->fd, a->conntime, a->rid, a->rname);
	write(a->fd, "--------------------------------------------\n", 255);
	write(a->fd, "\nCommands available - \n1.+online            -- Show online users\n2.broadcast:<msg>    -- Broadcast to all clients\n3.Ctrl+c             -- Exit chat.\n4.To send message, type <client>:<message>\n\n", 255);
	write(a->fd, "--------------------------------------------\n", 255);
}

//Return receiver name from the message
char* getrecname(char *rawmsg) {
	int colonpos = (int)(strchr(rawmsg, ':')-rawmsg);
	char *cliname = (char*)malloc(30*sizeof(char));
	strncpy(cliname, rawmsg, colonpos);
	cliname[colonpos+1] = '\0';
	return cliname;
}

//Get message text from raw input stream
char *getmsg(char *rawmsg) {
	int colonpos = (int)(strchr(rawmsg, ':')-rawmsg);
	char *msg = (char*)malloc(256*sizeof(char));
	strncpy(msg, rawmsg+colonpos+1, strlen(rawmsg)-colonpos);
	msg[strlen(rawmsg)-colonpos] = '\0';
	return  msg;
}

//Show online users
char* showonline(cli* clilist) {
	cli *curr = clilist;
	int i;
	char *b = (char*)calloc(256,sizeof(char));
	strcat(b, "Online users:\n");
	for(i=0;i<MAXCLIENTS;i++) {
		if(clilist[i].status>0) {
			sprintf(b, "%s %d.%s\n", b, i+1, clilist[i].name);
		}
	}
	sprintf(b, "%s\n--------------------------------------------\n", b);
	return b;
}

//Check message queue on demand and deliver message if present
void checkmsgq(cli* me, int* ctr, msg *mbuffer, int *q) {
	if(q[1]>=q[0]) return;
	int rear = q[1];
	int i;
	cli * clilist = me - (*ctr-1);
	//Delete message if receiver has gone offline
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

//This function will be used by multithreaded chat version
void *retrieve(void* vargp) {
	thread_arg* args = vargp;
	cli* clilist = args->clients;
	msg* mbuffer = args->mbuffer;
	int*q = args->q;
	int rear = q[1];
	int i;
	printf("I will print details of all clients\n");
	while(1) {
		for(i=q[1];i<q[0];i++) {
			printf("message: %s to %s:%s\n",mbuffer[i].sendername, mbuffer[i].recname, mbuffer[i].message);
		}
		if(q[1]>=q[0]) continue;
		rear = q[1];
		msg m1 = mbuffer[rear];
		for(i=0;i<MAXCLIENTS;i++) {
			if(strcmp(clilist[i].name, m1.recname)==0) {
				if(clilist[i].status<0) {
					q[1]++;
					break;
				}
				char b[256] = {0};
				//printf("Message is for %s %s. FD: %d\n", me->name, mbuffer[rear].message, me->fd);
				sprintf(b, "%s:%s", mbuffer[rear].sendername, mbuffer[rear].message);
				write(clilist[i].fd, b, 255);
				q[1]++;
			}
		}
	}
}

//Enqueue messages to message queue
void enqueue(msg m1, msg* mbuffer, int *q, cli* clilist, cli* sender) {
	//Put message in message buffer queue
	int i;
	if(strchr(m1.message, ':')!=NULL && m1.recname[0]==0) {			
		//Check if receiver name is mentioned. If not, then check if receivername is already mentioned in m1.recname
		//For broadcast messages, receiver name is already set in m1.recname
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
		//Client name not present in either message or m1.recname variable
		write(sender->fd, "Please include client name.\n", 255);
		return;
	}
	printf("Enqueueing messages of %s. Message: %s", m1.recname, m1.message);
	for(i=0;i<MAXCLIENTS;i++) {
		if(strcmp(m1.recname, sender->name)==0) {
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
	sem_t* mbuffsem = sem_open("/mbuff", 0);
	sem_wait(mbuffsem);
	FILE *fp = fopen("log", "a");
	msg* curr = mbuffer + q[0];	
	q[0]++;
	*curr = m1; 													//Structs can be copied in C
	fprintf(fp, "%s|%s|%ld|%s", m1.sendername, m1.recname, m1.mtime, m1.message);
	fclose(fp);
	sem_post(mbuffsem);
}

//Gracefully disconnect a client and close server process
void disconnect(msg* mbuffer, int *q, cli* clilist, cli* sender) {
	sender->status = -1;
	if(strcmp(mbuffer[q[1]].recname, sender->name)==0) {
		q[1]++;
	}
	close(sender->fd);
	printf("%s disconnected\n", sender->name);
	exit(0);
}

//Send messages to all clients except itself
void broadcast(msg m1, msg* mbuffer, int *q, cli* clilist, cli* sender) {
	int i;
	msg marr[MAXCLIENTS];
	for(i=0;i<MAXCLIENTS;i++) {
		if(clilist[i].status!=1) continue;
		if(strncmp(m1.message, "broadcast:", 10)==0) {
			printf("Broadcast message to all clients.\n");
			char *text = getmsg(m1.message);
			strcpy(m1.message, text);
		}
		else if(strcmp(m1.message, "+exit\n")==0) {
			//printf("So you want to exit?\n");
			sprintf(m1.message, "%s is now offline.\n", sender->name);
		}
		strcpy(m1.recname, clilist[i].name);
		if(strcmp(m1.recname, sender->name)!=0)
			enqueue(m1, mbuffer, q, clilist, sender);
	}
}

//Initialise client on connection. Assign ID and show online users
void fillclientdetails(cli *a, int *ctr, int newsockfd) {
	a->id = *ctr;
	int r = 10000+rand()%89999;
	a->rid = r;																//Assign random ID as rid
	sprintf(a->name, "client%d", *ctr);
	int length = 10;
	char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    while (length-- > 0) {													//Assign random string as rname
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        a->rname[length] = charset[index];
    }
    a->rname[10] = '\0';
	a->fd = newsockfd;
	a->status = 1;
	a->conntime = time(0);
	char b[256] = {0};
	write(a->fd, "Welcome to RobustChat. You have been connected. Happy chatting. :)\n\n", 255);
	sprintf(b, "Name        : %s \nRandomID    : %d\nRandomstring: %s\nTime        : %s\n", a->name, a->rid, a->rname, ctime(&(a->conntime)));
	write(a->fd, b, 255);
	bzero(b, 256);
	printclientdetails(a);
	printf("%s", showonline(a-(*ctr)));
}

//Server process handling all client communications
void handlemessage(cli* clilist, int* ctr, msg* mbuffer, int *q) {
		cli* me = clilist+((*ctr)-1);
		char *onlineusers = showonline(clilist);
		write(me->fd, onlineusers, 255);			//Show online users on connection
		while(1) {
			msg m1;
			checkmsgq(me, ctr, mbuffer, q);			//Check for messages
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
					broadcast(m1, mbuffer, q, clilist, me);
					disconnect(mbuffer, q, clilist, me);
					continue;
				}
				else if(strncmp(m1.message, "broadcast:", 10)==0) {
					broadcast(m1, mbuffer, q, clilist, me);
					continue;
				}
				else if(strncmp(m1.message, "+talkto", 7)==0) {				//Choose one client to talk to .Under development
					write(me->fd, "This feature is not available yet.\n--------------------------------------------\n", 255);
					//talktox(m1, mbuffer, q, clilist, me);
					continue;
				}
				bzero(m1.recname, 30);
				enqueue(m1, mbuffer, q, clilist, me);
			}
		}
}

//Check if number of clients exceeds max allowed number
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
	fcntl(sockfd, F_SETFL, O_NONBLOCK);				//Make socket non blocking
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

	//Allocate shared memory
	int ctrid = shmget(IPC_PRIVATE, sizeof(int), 0666|IPC_CREAT);
	int* ctr = (int*)shmat(ctrid, 0, 0);
	int clilistid = shmget(IPC_PRIVATE, MAXCLIENTS*sizeof(cli), 0666|IPC_CREAT);
	cli* clilist = (cli*)shmat(clilistid, 0, 0);
	int mbufferid = shmget(IPC_PRIVATE, 1000*sizeof(msg), 0666|IPC_CREAT);
	msg *mbuffer = (msg*)shmat(mbufferid, 0, 0);
	int qid = shmget(IPC_PRIVATE, 3*sizeof(int), 0666|IPC_CREAT);
	int *q = (int*)shmat(qid, 0, 0);
	*ctr = 0;
	q[0] = 0; q[1] = 0;

	//Create semaphores for synchronization
	sem_t *qsem = sem_open("/q", O_CREAT|O_EXCL, 0644, 1);
	sem_t *mbuffsem = sem_open("/mbuff", O_CREAT|O_EXCL, 0644, 1);
	signal(SIGINT, sighandler);	
	
	/*
	//Uncomment this block for multithreaded chat
	pthread_t tid;
	thread_arg arg;
	arg.mbuffer = mbuffer;
	arg.clients = clilist;
	arg.q = q;
	pthread_create(&tid, NULL, retrieve, (void*)&arg);
	*/
	int i; for(i=0;i<MAXCLIENTS;i++) {clilist[i].status = -10;}

	while(1) {
		if((newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len))==-1) {
			continue;
		}
		fcntl(newsockfd, F_SETFL, O_NONBLOCK);					//Make socket non blocking
		if(allowconnection(ctr)) {								//check if clients exceed MAXCLIENTS
			fillclientdetails(clilist+(*ctr), ctr, newsockfd);	//Assign ID and name
			(*ctr)++;
			pid = fork();
			if(pid == 0) {										//Child process
				handlemessage(clilist, ctr, mbuffer, q);		//Handle clients
			}
			else if(pid >0) {	//Parent process
				//close(newsockfd);
				continue;
			}
		}
		else {
			write(newsockfd, "Connection limit exceeded\n", 255);		//Limit exceeded error
			close(newsockfd);
			continue;
		}
	}
}
