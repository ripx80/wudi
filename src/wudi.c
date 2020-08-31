#include <stdio.h>
#include <string.h>
#include <sys/socket.h> // for functions
#include <sys/types.h> //
#include <netinet/in.h> // for sockaddr_in
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include <pthread.h>

//msg queue
#include <sys/ipc.h>
#include <sys/msg.h>


#define MXVRT 25
#define BUF 4069
#define LOGBUF 128
#define DAEMON_NAME "wudi"
#define PID "/var/run/wudi.pid"
#define LOG "/var/log/wudi.log"

pthread_t threads[MXVRT];
pthread_attr_t attr;
uint8_t childs=0;

typedef struct msgbuf {
         long    mtype;
         char    mtext[LOGBUF+100];
} message_buf;
int msqid;

struct virtual {
	uint16_t rport,port;
	char rip[16],ip[16];
}vrt;



pthread_t log_t;
key_t key = 12;
message_buf sbuf;
size_t buf_length;

void send_queue(char *message){
	sbuf.mtype = 1;
	printf("%s\n",message);
   	strcpy(sbuf.mtext, message);
	buf_length = strlen(sbuf.mtext) + 1 ;
	if (msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT) < 0)
			perror("ERROR:Cannot send message to Queue\n");
}

void *log_thread(){
	if ((msqid = msgget(key, IPC_CREAT | 0666 )) < 0) {
        perror("ERROR no Queue are initialize");
        return NULL;
    }
	FILE *log;
	message_buf rbuf;
	memset(&rbuf,0,sizeof(rbuf));
    while(1){
		log=fopen(LOG,"a");
		if (log==NULL) {
			perror("Cannot open Log File");
			pthread_exit(NULL);
		}
		if (msgrcv(msqid, &rbuf, LOGBUF+100, 1, MSG_NOERROR) < 0)
			perror("msgrcv");

		if(rbuf.mtext){
			time_t x = time (NULL);
			struct tm * timeinfo = localtime ( &x );
			fprintf (log,"%02d.%02d.%04d %02d:%02d\t%s\n", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900,timeinfo->tm_hour,timeinfo->tm_min,rbuf.mtext);
		}
		fclose(log);
	}
	pthread_exit(NULL);
}

void *strvrt(void *arg){
	//printf("Starting thread number %d\n",childs);
	fflush(stdout);
	int *client=(int *) arg;
	int ext;
	size_t l;
	char buf[BUF];
	memset(buf,0,BUF);

	struct sockaddr_in ext_addr;

	if(recv (*client, buf, BUF, MSG_NOSIGNAL) == -1){
		perror ("recv :");
		pthread_exit(NULL);
	}

	if((ext=socket(AF_INET,SOCK_STREAM,0))==-1){perror("Socket");pthread_exit(NULL);;}

	ext_addr.sin_family = AF_INET;
	ext_addr.sin_port = htons (vrt.rport);
	ext_addr.sin_addr.s_addr = inet_addr(vrt.rip);
	memset(&(ext_addr.sin_zero),0,8);

	if((connect(ext,(struct sockaddr*)&ext_addr,sizeof(struct sockaddr))) == -1){perror("Whereis secret");pthread_exit(NULL);}

	if((send(ext,buf,strlen(buf),MSG_NOSIGNAL))==-1){perror("konnt nit sende");pthread_exit(NULL);}

	while(1){
		l=recv(ext,buf,BUF,MSG_NOSIGNAL);
		if(l>0){
			if((send(*client,buf,l,MSG_NOSIGNAL))==-1){perror("konnt nit sende");pthread_exit(NULL);}
		}else
			break;
	}

//printf("thread say good bye!\n");
close(ext);
--childs;
pthread_exit(NULL);

}

int startDaemon(){



	memset(&sbuf,0,sizeof(sbuf));
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	if(pthread_create(&log_t,&attr,log_thread,NULL)){
		perror("ERROR: Failed to spawn log thread!");
		return 1;
	}
	char msg[LOGBUF+100];
	memset(msg,0,LOGBUF);
	sprintf(msg,"starting server\nlisten IP: %s\n listen Port: %d\n Repo IP: %s\n Repo Port: %d\n",vrt.ip,vrt.port,vrt.rip,vrt.rport);
	send_queue(msg);
	int sock;
	struct sockaddr_in local_addr;
	struct sockaddr_in client_addr;
	socklen_t len;

	char buf[BUF];
	memset(buf,0,BUF);

	if((sock = socket (AF_INET, SOCK_STREAM, 0))==-1){perror ("socket :");return 1;}
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons (vrt.port);
	local_addr.sin_addr.s_addr = inet_addr(vrt.ip);
	memset(&(local_addr.sin_zero),0,8);

	if(bind (sock, (struct sockaddr*)&local_addr, sizeof (local_addr)) == -1){perror ("bind :");return 1;}
	listen (sock, 1);

	while(1){
		//printf("habe %d Kinder\n",childs);
		len = sizeof (client_addr);
		int client= accept (sock, (struct sockaddr*)&client_addr, &len);
		if(sock == -1){perror ("accept :");return 1;}

		if(childs==MXVRT){
			strcpy(buf,"HTTP/1.1 503 Service Unavailable\r\n\r\n");
			send(client,buf,BUF,MSG_NOSIGNAL);
		}else
			if(pthread_create(&threads[childs], &attr, strvrt, (void *) &client)==-1)
				perror("can not creat a thread");
			else
				++childs;

	}
	close (sock);
	return 0;

}

int main(int argc, char *argv[0]) {

	if(argc!=5){
		send_queue("USE: wudi listenip listenport reposhost reposport - only four arguments accepted");
	}else{
		strcpy(vrt.ip,argv[1]);
		vrt.port=atoi(argv[2]);
		strcpy(vrt.rip,argv[3]);
		vrt.rport=atoi(argv[4]);

		startDaemon();
	}
	return 0;
}
