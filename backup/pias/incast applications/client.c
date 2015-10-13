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


//Host list. In our experiment, we have 15 physical machines as senders. 
#define max_hosts 15
char* hosts[max_hosts]={
	"192.168.101.2",
	"192.168.101.3",
	"192.168.101.4",
	"192.168.101.5",
	"192.168.101.6",
	"192.168.101.7",
	"192.168.101.8",
	"192.168.101.9",
	"192.168.101.10",
	"192.168.101.11",
	"192.168.101.12",
	"192.168.101.13",
	"192.168.101.14",
	"192.168.101.15",
	"192.168.101.16"
	};
//Mutex for printf of multi-threads
pthread_mutex_t mutex;  

//A struct for a connection
struct connection
{
	int id;
	int hostid;
	int port;
	int size;
};

//Function to init a connection to transmit data
void* client_thread_func(void* connection_ptr);

//Set send window
void set_send_window(int sockfd, int window);

//Set receive window
void set_recv_window(int sockfd, int window);

//Print usage information
void usage();

int main(int argc, char **argv)
{
	int i=0;
	int port=5001;
	int data_size=0;
	int connections=0;
	//Array of struct connection
	struct connection* incast_connections=NULL;
	//Array of pthread_t
	pthread_t* client_threads=NULL;
	//Total start time
	struct timeval tv_start_total;
	//Total end time
	struct timeval tv_end_total;
	
	if(argc!=3)
	{
		usage();
		return 0;
	}
	
	//Get connections: char* to int
	connections=atoi(argv[1]);
	//Get data_size: char* to int
	data_size=atoi(argv[2]);
	//Initialize 
	incast_connections=(struct connection*)malloc(connections*sizeof(struct connection));
	client_threads=(pthread_t*)malloc(connections*sizeof(pthread_t));
	
	gettimeofday(&tv_start_total,NULL);
	for(i=0;i<connections;i++)
	{
		incast_connections[i].port=port;
		incast_connections[i].id=i+1;
		incast_connections[i].hostid=i%max_hosts;
		incast_connections[i].size=data_size;
		
		if(pthread_create(&client_threads[i], NULL , client_thread_func , (void*)&incast_connections[i]) < 0)
		{
			perror("could not create client thread");
		}	
	}
	
	for(i=0;i<connections;i++)
	{
		pthread_join(client_threads[i], NULL);  
	}
	gettimeofday(&tv_end_total,NULL);
	//Time interval (unit: microsecond)
	unsigned long interval=(tv_end_total.tv_sec-tv_start_total.tv_sec)*1000000+(tv_end_total.tv_usec-tv_start_total.tv_usec);
	//KB->bit 1024*8
	float throughput=data_size*connections*1024*8.0/interval;
	printf("[Total] 0-%lu ms, %d KB, %.1f Mbps\n",interval/1000,data_size*connections,throughput);
	free(incast_connections);
	free(client_threads);
	return 0;
}

void* client_thread_func(void* connection_ptr)
{
	struct connection incast_connection=*(struct connection*)connection_ptr;
	//Get ID
	int id=incast_connection.id;
	//Get IP address from hostid
	char* IPaddress=hosts[incast_connection.hostid];
	//Get port
	int port=incast_connection.port;
	//Get traffic size
	int size=incast_connection.size;
	int sockfd;
	struct sockaddr_in servaddr;
	int len;
	char data_size[6]={0};
	char buf[BUFSIZ];
	struct timeval tv_start;
	struct timeval tv_end;
	
	//Init sockaddr_in
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	//IP address
	servaddr.sin_addr.s_addr=inet_addr(IPaddress);
	//Port number
	servaddr.sin_port=htons(port);
	
	//Convert int to char*
	sprintf(data_size,"%d",size);
	
	//Init socket
	if((sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{
		perror("socket error\n");  
		return;  
	}
	
	//set_recv_window(sockfd, 512000);
	//set_send_window(sockfd, 32000);
		
	//Establish connection
	if(connect(sockfd,(struct sockaddr *)&servaddr,sizeof(struct sockaddr))<0)
	{
		//Print throughput information
		pthread_mutex_lock(&mutex); 
		printf("Can not connect to %s\n",IPaddress);
		pthread_mutex_unlock(&mutex); 
		return((void *)0);
	}
	
	//Get start time
	gettimeofday(&tv_start,NULL);
	
	//Send Request
	len=send(sockfd,data_size,strlen(data_size),0);
	
	//Recv Data from Server
	int total=0;
	while(1)
	{
		len=recv(sockfd,buf,BUFSIZ,0);
		if(len<=0)
			break;
	}
	
	//Get end time
	gettimeofday(&tv_end,NULL);
	close(sockfd);
	
	//Time interval (unit: microsecond)
	unsigned long interval=(tv_end.tv_sec-tv_start.tv_sec)*1000000+(tv_end.tv_usec-tv_start.tv_usec);
	//KB->bit 1024*8
	float throughput=size*1024*8.0/interval;
	
	//Print throughput information
	pthread_mutex_lock(&mutex); 
	printf("[%d] From %s 0-%lu ms, %d KB, %.1f Mbps\n",id,IPaddress,interval/1000,size,throughput);
	pthread_mutex_unlock(&mutex); 
	return((void *)0);
}

void set_recv_window(int sockfd, int rcvbuf)
{
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&rcvbuf, sizeof(rcvbuf));
}

void set_send_window(int sockfd, int sndbuf)
{
	setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&sndbuf, sizeof(sndbuf));
}

void usage()
{
	printf("./client.o [connections] [data_size]\n");
}