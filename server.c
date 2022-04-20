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

char buffer[256];

struct cliente{
	char username[32];
	int fd_toserver;
	int fd_toclient;
};

struct cliente clientes[100];
pthread_mutex_t critical;

struct missatge{
	int len;
	char *txt;
};

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
	char len[256];
	char buffer2[270];
	sprintf(buffer2,"Thread username: %s Born\n",cliente->username);
	write(1,buffer2,strlen(buffer2));
	while(1){
		read(cliente->fd_toserver,len,256);
		sprintf(buffer2,"missatge rebut!!\n");
		write(1,buffer2,strlen(buffer2));
		
		sprintf(buffer2,"len=%s\n",len);
		write(1,buffer2,strlen(buffer2));
		sprintf(buffer2,"%s said:",cliente->username);
		write(1,buffer2,strlen(buffer2));
		txt=(char*)malloc(atoi(len));
		read(cliente->fd_toserver,txt,atoi(len));

		write(1,txt,atoi(len));
		pthread_mutex_lock(&critical);
		while(clientes[i].fd_toserver!=0){
			if(clientes[i].fd_toclient != cliente->fd_toclient){
				write(clientes[i].fd_toclient,cliente->username,strlen(cliente->username));
				usleep(25);
				write(clientes[i].fd_toclient,len,256);
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

	//Creacio de les adreces pels 2 sockets: addresin i addresout
	int sAddrLen;
	int sdin,sdout,j=0,in,out;
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
				read(in,clientes[j].username,32);
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

