#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>

#define SEND_BUFSIZ 65536	//size of send buffer

//Start TCP server
void start_server(int port);
//Function to deal with an incoming connection
void* server_thread_func(void* client_sockfd_ptr);
//Print usage information
void usage(char *program);

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		usage(argv[0]);
		return 0;
	}

	start_server(atoi(argv[1]));
	return 0;
}

void usage(char *program)
{
	printf("%s [port]\n", program);
}

void start_server(int port)
{
	//Socket for server
	int server_sockfd;
	//Socket pointer for client
	int* client_sockfd_ptr = NULL;
	//A thread to deal with client_sockfd
	pthread_t server_thread;
	//Server address
	struct sockaddr_in server_addr;
	//Client address
	struct sockaddr_in client_addr;
	int sock_opt = 1;
	socklen_t sin_size = sizeof(struct sockaddr_in);

	//Initialize server address (0.0.0.0:port)
	memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

	//Init socket
	if ((server_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket error");
		return;
	}

	//Set socket options
	if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt)) < 0)
	{
		perror("Error: set SO_REUSEADDR option");
		return;
	}
	if (setsockopt(server_sockfd, IPPROTO_TCP, TCP_NODELAY, &sock_opt, sizeof(sock_opt)) < 0)
	{
		perror("ERROR: set TCP_NODELAY option");
		return;
	}

	//Bind socket on IP:Port
	if (bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
	{
		perror("bind error");
		return;
	}

	//Start listen
	listen(server_sockfd, SOMAXCONN);

	while (1)
	{
		client_sockfd_ptr = (int*)malloc(sizeof(int));
		if (!client_sockfd_ptr)
		{
			perror("malloc error");
			return;
		}

		int value = accept(server_sockfd, (struct sockaddr *)&client_addr, &sin_size);
		if (value < 0)
		{
			perror("accept error");
			free(client_sockfd_ptr);
			return;
		}

		*client_sockfd_ptr = value;
		if (pthread_create(&server_thread, NULL , server_thread_func, (void*)client_sockfd_ptr) < 0)
		{
			perror("could not create thread");
			return;
		}
	}
}

void* server_thread_func(void* client_sockfd_ptr)
{
	int sock = *(int*)client_sockfd_ptr;
	char write_message[SEND_BUFSIZ] = {1};
	char read_message[64] = {'\0'};
	int len;
	int data_size;

	free(client_sockfd_ptr);

	len = recv(sock, read_message, 64, 0);
	if (len <= 0)
	{
		close(sock);
		return((void *)0);
	}

	//Get data volume (Unit:KB)
	data_size = atoi(read_message) * 1024;
	while (data_size > 0)
	{
		len = (SEND_BUFSIZ < data_size)? SEND_BUFSIZ : data_size;
		data_size -= send(sock, write_message, len, 0);
	}

	close(sock);
	return((void *)0);
}
