#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

char buffer[256];
int sdout,sdin;

struct cliente{
	char *username;
	int fd_toserver;
	int fd_toclient;
	char userlen[16];
};

struct cliente clientes[100];
pthread_mutex_t critical;

struct missatge{
	int len;
	char *txt;
};

//Handler que s'executa al rebre SIGUSR2
void int_handler(){
	char buffer[256];
	int i=0;
	sprintf(buffer,"SIGUSR2 recieved Closing sockets...\n");
	write(1,buffer,strlen(buffer));

	//Recorre tot vector de clientes xapant els sockets
	while(clientes[i].fd_toserver!=0){
		close(clientes[i].fd_toserver);
		close(clientes[i].fd_toclient);
	}
	sprintf(buffer,"All clients closed\n");
	write(1,buffer,strlen(buffer));

	//Xapa sockets del server i finalitza
	close(sdin);
	close(sdout);
	sprintf(buffer,"Server Sockets closed\n");
	write(1,buffer,strlen(buffer));
	exit(0);
}


int createsocket(int port, struct sockaddr_in address){
	int x;

	x=socket(AF_INET,SOCK_STREAM,0);
	if (x==-1) {
		sprintf(buffer,"ERROR socket creation port: %d \n",port);
		write(1,buffer,strlen(buffer));
		return 0;
	}else{
		sprintf(buffer,"OK SCreation port: %d \n",port);
		write(1,buffer,strlen(buffer));
	}


	if (bind(x,(struct sockaddr*) &address, sizeof(address)) == -1) {
		sprintf(buffer,"ERROR Bind port: %d \n",port);
		write(1,buffer,strlen(buffer));
		return 0;
	}else{
		sprintf(buffer,"OK Bind port: %d \n",port);
		write(1,buffer,strlen(buffer));
	}

	if(listen(x,2) == -1) {
		sprintf(buffer,"ERROR listen port: %d \n",port);
		write(1,buffer,strlen(buffer));
		return 0;
	}else{
		sprintf(buffer,"OK Listen  port: %d \n",port);
		write(1,buffer,strlen(buffer));
	}
	sprintf(buffer,"------------------------\n");
	write(1,buffer,strlen(buffer));
	return x;
}

void *read_write (void *asdf){
	struct cliente *cliente;
	cliente=(struct cliente*)asdf;
	int i=0;
	char *txt;
	char len[32];
	char buffer2[270];
	sprintf(buffer2,"Thread username: %s Born\n",cliente->username);
	write(1,buffer2,strlen(buffer2));
	while(1){
		//Rep longitud missatge
		read(cliente->fd_toserver,len,32);
		sprintf(buffer2,"missatge rebut!!\n");
		write(1,buffer2,strlen(buffer2));
		/* COMPROVACIONS
		sprintf(buffer2,"len=%s\n",len);
		write(1,buffer2,strlen(buffer2));
		sprintf(buffer2,"%s said:",cliente->username);
		write(1,buffer2,strlen(buffer2));
		*/
		//Posem mida missatge
		txt=(char*)malloc(atoi(len));
		read(cliente->fd_toserver,txt,atoi(len));
		//write(1,txt,atoi(len));
		pthread_mutex_lock(&critical);
		while(clientes[i].fd_toserver!=0){
			if(clientes[i].fd_toclient != cliente->fd_toclient){
				
				write(clientes[i].fd_toclient,cliente->userlen,strlen(cliente->userlen));
				usleep(25);
				write(clientes[i].fd_toclient,cliente->username,atoi(cliente->userlen));
				usleep(25);
				write(clientes[i].fd_toclient,len,32);
				usleep(25);
				write(clientes[i].fd_toclient,txt,atoi(len));
			}
			i++;
		}
		pthread_mutex_unlock(&critical);
		free(txt);
		i=0;
	}
}

int main(int argc, char**argv)
{
	//Llegir els 2 ports per teclat
	if (argc!=1){
		sprintf(buffer,"ERROR input params\n");
		write(1,buffer,strlen(buffer));
		return 0;
	}
	/*
	int portin=atoi(argv[1]);
	sprintf(buffer,"primer port %d \n",atoi(argv[1]));
	write(1,buffer,strlen(buffer));
	int portout=atoi(argv[2]);;
	sprintf(buffer,"segon port %d \n",atoi(argv[2]));
	write(1,buffer,strlen(buffer));
	*/
	int portin=11002;
	int portout=12002;

	//Creacio cosetes de signals
	struct sigaction hand;
	sigset_t mask;
	sigemptyset (&mask);
	hand.sa_mask=mask;
	hand.sa_flags=0;
	hand.sa_handler=int_handler;
	sigaction(SIGUSR2,&hand,NULL);

	//Creacio de les adreces pels 2 sockets: addresin i addresout
	int sAddrLen;
	int j=0,in,out;
	pthread_t id[100];
	struct sockaddr_in addressin, addressout, retSin;
	memset(&addressin,0,sizeof(addressin));
	addressin.sin_family = AF_INET;
	addressin.sin_addr.s_addr = INADDR_ANY;
	addressin.sin_port = htons(portin);
	memset(&addressout,0,sizeof(addressout));
	addressout.sin_family = AF_INET;
	addressout.sin_addr.s_addr = INADDR_ANY;
	addressout.sin_port = htons(portout);

	//Creació dels 2 sockets, retorna el sd corresponent a cada canal.
	sdin=createsocket(portin,addressin);
	if (sdin==0)
	{
		return 0;
	}
	sdout=createsocket(portout,addressout);
	if (sdout==0)
	{
		return 0;
	}

	pthread_mutex_init(&critical,NULL);
	sprintf(buffer,"Entrant bucle accepts\n");
	write(1,buffer,strlen(buffer));
	while(1){
		sprintf(buffer,"------------------------\n");
		write(1,buffer,strlen(buffer));
		sprintf(buffer,"Waiting IN...\n");
		write(1,buffer,strlen(buffer));
		in=accept(sdin, (struct sockaddr *)&retSin, &sAddrLen);
		if(in==-1){
			sprintf(buffer,"IN accept error\n");
			write(1,buffer,strlen(buffer));
		}else{
			sprintf(buffer,"Accepted IN\n");
			write(1,buffer,strlen(buffer));
			sprintf(buffer,"Waiting OUT...\n");
			write(1,buffer,strlen(buffer));
			out=accept(sdout, (struct sockaddr *)&retSin, &sAddrLen);
			if(out==-1){
				sprintf(buffer,"OUT accept error\n");
				write(1,buffer,strlen(buffer));
			}else{
				sprintf(buffer,"Accepted OUT:\n");
				write(1,buffer,strlen(buffer));
				pthread_mutex_lock(&critical);

				//Rep mida UserName
				read(in,clientes[j].userlen,16);
				//Assigna mida a username
				clientes[j].username=(char*)malloc(atoi(clientes[j].userlen));
				//Rep Username
				read(in,clientes[j].username,sizeof(clientes[j].username));
				clientes[j].fd_toclient=out;
				clientes[j].fd_toserver=in;
				sprintf(buffer,"Creating thread...\n");
				write(1,buffer,strlen(buffer));
				pthread_create(&id[j],NULL,read_write,(void *)&clientes[j]);
				pthread_mutex_unlock(&critical);
				j++;
			}
		}
	}
	return 0;
}

