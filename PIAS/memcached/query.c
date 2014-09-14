#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libmemcached/memcached.h>
#include <pthread.h>

#define max_hosts 15

//Constant key value for all memcached connections
char *key="1";
//Constant port number of all memcached servers
int port=11211;
//Mutex for printf of multi-threads
pthread_mutex_t mutex;  

//Host list
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
	"192.168.101.16"};

//A struct for a connection
struct connection
{
	int id;
	int hostid;
};

//Function to GET data from a memcached server 
void* client_thread_func(void* connection_ptr);

//Print usage information
void usage();

int main(int argc, char **argv)
{
	int i=0;
	int connections=0;
	//Array of structure connection
	struct connection* memcached_connections=NULL;
	//Key value
	char *key=NULL;
	//Array of pthread_t
	pthread_t* client_threads=NULL;
	//Total start time
	struct timeval tv_start_total;
	//Total end time
	struct timeval tv_end_total;
	
	if(argc!=2)
	{
		usage();
		return 0;
	}
	
	//Get connections: char* to int
	connections=atoi(argv[1]);
	
	//Initialize 
	memcached_connections=(struct connection*)malloc(connections*sizeof(struct connection));
	client_threads=(pthread_t*)malloc(connections*sizeof(pthread_t));
	
	//Get start time
	gettimeofday(&tv_start_total,NULL);
	
	//Start threads
	for(i=0;i<connections;i++)
	{
		memcached_connections[i].id=i+1;
		memcached_connections[i].hostid=i%max_hosts;
		
		if(pthread_create(&client_threads[i], NULL , client_thread_func , (void*)&memcached_connections[i]) < 0)
		{
			perror("could not create client thread");
		}	
	}
	
	//Wait for all threads to stop
	for(i=0;i<connections;i++)
	{
		pthread_join(client_threads[i], NULL);  
	}
	
	//Get stop time 
	gettimeofday(&tv_end_total,NULL);
	
	//Get time interval (unit: microsecond)
	unsigned long interval=(tv_end_total.tv_sec-tv_start_total.tv_sec)*1000000+(tv_end_total.tv_usec-tv_start_total.tv_usec);
	printf("[Total] %lu us\n",interval);
	
	//Free related array
	free(memcached_connections);
	free(client_threads);
	return 0;
}

void* client_thread_func(void* connection_ptr)
{
	struct connection memcached_connection=*(struct connection*)connection_ptr;
	//Get ID
	int id=memcached_connection.id;
	//Get server IP address based on hostid and hosts
	char* ip_addr=hosts[memcached_connection.hostid];
	
	memcached_server_st *servers = NULL;
	memcached_st *memc;
	memcached_return rc;
	memcached_return error;
	uint32_t flags;

	char *return_value;
	size_t return_value_length;

	memc= memcached_create(NULL);
	
	servers= memcached_server_list_append(servers, ip_addr, port, &rc);
	rc= memcached_server_push(memc, servers);

	if (rc != MEMCACHED_SUCCESS)
	{
		pthread_mutex_lock(&mutex); 
		fprintf(stderr,"[%d] Couldn't add server: %s\n",id,memcached_strerror(memc, rc));
		pthread_mutex_unlock(&mutex);
	}

	//Get data from the memcached server
	if(memcached_get(memc, key, strlen(key),&return_value_length,&flags,&error)==NULL)
	{
		pthread_mutex_lock(&mutex); 
		fprintf(stderr,"[%d] Cannot get value from %s\n",id,ip_addr);
		pthread_mutex_unlock(&mutex);
	}
	return((void *)0);
}


void usage()
{
	printf("./query.o [connections]\n");
}
