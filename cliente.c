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


int sdin;
int sdout;
//char buffer[256];
struct missatge{
	int len;
	char *txt;
};


//GET MSG
void *getmsg(){
	char buffer[256];
	int i=0;
	int a=1;
	char *msg;
	char c;
	struct missatge *mis1;
	mis1=(struct missatge*)malloc(sizeof(struct missatge));
	msg =(char*)malloc(32*sizeof(char)+1);
	while(c!= '\n'){	
		c=getchar();	
		msg[i]=c;
		i++;	
		if (i==(32*a)-1){
			a++;
			msg= (char *) realloc(msg,32*a+1);	
		}
	}
	msg[i]='\0';
	mis1->len=i+1;
	mis1->txt=(char*)realloc(mis1->txt,mis1->len); 
	strcpy(mis1->txt,msg);

	//Comprobacions
	/*sprintf(buffer,"len=%d\n",mis1->len);
	write(1,buffer,strlen(buffer));
	sprintf(buffer,"txt=%s",mis1->txt);
	write(1,buffer,strlen(buffer));*/

	free(msg);
	return(mis1);
}

//REBRE MSG I ESCRIURE
void *read_print(void *x){
	int *sdin=(int*)x;
	char *username;
	char usrlen[16];
	char buffer[50];
	char len[16];
	char *miss;
	int l,i=0;
	while(1){
		//Rep allargada del username, li dona mida i el rep
		read(*sdin,usrlen,16);
		username=(char*)malloc(atoi(usrlen));
		read(*sdin,username,atoi(usrlen));
	
		//Comprovacions
		/*
		sprintf(buffer,"<%s>: ",username);
		write(1,buffer,strlen(buffer));
		*/
		
		//Rep len de text, marca mida i rep text
		read(*sdin,len,16);
		miss=(char*)malloc(atoi(len));
		read(*sdin,miss,atoi(len));

		//Escriu text i allibera memoria
		write(1,miss,strlen(miss));
		free(miss);
		memset(username,0,sizeof(username));
		free(username);
		i++;
	}
}


//Handler que s'executa al rebre SIGUSR1
void int_handler(){
	char buffer[256];
	sprintf(buffer,"SIGUSR1 recieved\n");
	write(1,buffer,strlen(buffer));
	shutdown(sdout,1);
	shutdown(sdin,0);
	close(sdout);
	close(sdin);
	sprintf(buffer,"Client Terminated\n");
	write(1,buffer,strlen(buffer));
	exit(0);
}


int main(int argc, char**argv){
	char buffer[256];
	//Llegir username IP i ports pel teclat
	if (argc!=2){
		sprintf(buffer,"ERROR input params\n");
		write(1,buffer,strlen(buffer));
		return 0;
	}

	//Creacio cosetes de signals
	struct sigaction hand;
	sigset_t mask;
	sigemptyset (&mask);
	hand.sa_mask=mask;
	hand.sa_flags=0;
	hand.sa_handler=int_handler;
	sigaction(SIGUSR1,&hand,NULL);


	//Llegim username IP i PORTS
	char *username ;
	char IP[128];
	char portin[10];
	char portout[10];	
	username=(char*)malloc(strlen(argv[1]));
	strcpy(username,argv[1]);
	/*strcpy(IP,argv[2]);
	strcpy(portin,argv[3]);
	strcpy(portout,argv[4]);
	*/
	char IP2[128]="0.0.0.0";
	char portin2[10]="12005";
	char portout2[10]="11005";
	strcpy(IP,IP2);
	strcpy(portin,portin2);
	strcpy(portout,portout2);
	sprintf(buffer,"Username:%s -- IP: %s -- IN: %s -- OUT: %s \n",username, IP,portin,portout);
	write(1,buffer,strlen(buffer));

	//Creaci?? dels 2 SOCKETS
	sdin = socket(AF_INET,SOCK_STREAM,0);
	if (sdin == -1) {
		sprintf(buffer,"ERROR IN s.creation\n");
		write(1,buffer,strlen(buffer));
		return 0;
	}else{
		sprintf(buffer,"OK IN s.creation\n");
		write(1,buffer,strlen(buffer));
	}
	sdout = socket(AF_INET,SOCK_STREAM,0);
	if (sdout == -1) {
		sprintf(buffer,"ERROR OUT s.creation\n");
		write(1,buffer,strlen(buffer));
		return 0;
	}else{
		sprintf(buffer,"OK OUT s.creation\n");
		write(1,buffer,strlen(buffer));
	}


	//Connecci?? de SOCKETS
	struct addrinfo hints,*serverinfo,*p;
	int rvin,rvout;
	
	memset(&hints,0,sizeof(hints));
	
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	char s[INET6_ADDRSTRLEN];
	rvout = getaddrinfo(IP,portout,&hints,&serverinfo);
	for(p=serverinfo; p!=NULL ;p=p->ai_next){
		inet_ntop(p->ai_family,p->ai_addr,s,sizeof(s));
		if(connect(sdout,p->ai_addr,p->ai_addrlen)	== -1){
			sprintf(buffer,"NO Connectat OUT\n");
			write(1,buffer,strlen(buffer));
			continue;
		}
		sprintf(buffer,"Connectat OUT\n");
		write(1,buffer,strlen(buffer));
		break;
	}
	rvin = getaddrinfo(IP,portin,&hints,&serverinfo);
	for(p=serverinfo; p!=NULL ;p=p->ai_next){
		inet_ntop(p->ai_family,p->ai_addr,s,sizeof(s));
		if(connect(sdin,p->ai_addr,p->ai_addrlen)	== -1){
			sprintf(buffer,"NO Connectat IN\n");
			write(1,buffer,strlen(buffer));
			continue;
		}
		sprintf(buffer,"Connectat IN\n");
		write(1,buffer,strlen(buffer));
		break;
	}
	freeaddrinfo(serverinfo);

	//Envio longitud del username
	sprintf(buffer,"%ld",strlen(username));
	write(sdout,buffer,strlen(buffer));
	usleep(50);
	//Envio username
	write(sdout,username,strlen(username));
	
	//Creaci?? thread llegir
	pthread_t id;
	pthread_create(&id,NULL,read_print,(void*)&sdin);
	sprintf(buffer,"thread llegir server creat\n");
	write(1,buffer,strlen(buffer));

	//Creaci?? thread escriure
	struct missatge *missatge;
	sprintf(buffer,"Entrant bucle escriure teclat\n");
	write(1,buffer,strlen(buffer));
	while(1){
		missatge=getmsg();
		sprintf(buffer,"%d",missatge->len);
		write(sdout,buffer,strlen(buffer));
		/*sprintf(buffer,"lenbuffer sdout=%ld\n",strlen(buffer)+1);
		write(1,buffer,strlen(buffer));*/
		usleep(50);
		write(sdout,missatge->txt,missatge->len);
		free(missatge);
	}
	return 0;
}
