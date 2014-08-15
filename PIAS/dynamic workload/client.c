#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h> 
#include <sys/time.h>
#include <pthread.h>

//Print usage information
void usage();

int main(int argc, char **argv)
{	
	if(argc!=4)
	{
		usage();
		return 0;
	}
	char server[16];							//IP address (e.g. 192.168.101.101) 
	int port;											//TCP port number
	int data_size;								//Request data size (KB)
	struct timeval tv_start;				//Start time (after three way handshake)
	struct timeval tv_end;				//End time
	int sockfd;										//Socket
	struct sockaddr_in servaddr;	//Server address (IP:Port)
	int len;											//Read length
	char buf[BUFSIZ];						//Receive buffer
	unsigned long fct;						//Flow Completion Time
	
	//Initialize ‘server’
	memset(server,'\0',16);
	//Get server address
	strncpy(server,argv[1],strlen(argv[1]));
	//Get TCP port: char* to int
	port=atoi(argv[2]);
	//Get data_size: char* to int
	data_size=atoi(argv[3]);
	
	//Init sockaddr_in
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	//IP address
	servaddr.sin_addr.s_addr=inet_addr(server);
	//Port number
	servaddr.sin_port=htons(port);

	//Init socket
	if((sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{
		perror("socket error\n");  
		return;  
	}
	//Establish connection
	if(connect(sockfd,(struct sockaddr *)&servaddr,sizeof(struct sockaddr))<0)
	{
		printf("Can not connect to %s\n",server);
		return 0;
	}
	
	//Get start time after connection establishment
	gettimeofday(&tv_start,NULL);
	
	//Send request
	len=send(sockfd,argv[3],strlen(argv[3]),0);
	//Receive data
	while(1)
	{
		len=recv(sockfd,buf,BUFSIZ,0);
		if(len<=0)
			break;
	}
	//Get end time after receiving all of the data 
	gettimeofday(&tv_end,NULL);
	//Close connection
	close(sockfd);
	//Calculate time interval (unit: microsecond)
	fct=(tv_end.tv_sec-tv_start.tv_sec)*1000000+(tv_end.tv_usec-tv_start.tv_usec);
	printf("From %s: %d KB %lu us\n", server,data_size,fct);
	return 0;
}


void usage()
{
	printf("./client.o [server IP address] [server port] [request data size(KB)]\n");
}