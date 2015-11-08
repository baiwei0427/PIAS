#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define RECV_BUFSIZ 65536	//size of receive buffer

//Print usage information
void usage(char *program);

int main(int argc, char **argv)
{
	char server[16] = {'\0'};	//IP address (e.g. 192.168.101.101)
	int port;	//TCP port number
	int data_size;	//Request data size (KB)
	struct timeval tv_start;	//Start time (after three way handshake)
	struct timeval tv_end;	//End time
	int sockfd;	//Socket
	struct sockaddr_in servaddr;	//Server address (IP:Port)
	int len;	//Read length
	char buf[RECV_BUFSIZ];	//Receive buffer
	int write_count = 0;	//total number of bytes to write
	int read_count = 0;	//total number of bytes received
	unsigned long fct;	//Flow Completion Time

	if (argc != 4)
	{
		usage(argv[0]);
		return 0;
	}

	//Get server address
	strncpy(server, argv[1], 15);
	//Get TCP port: char* to int
	port = atoi(argv[2]);
	//Get data_size: char* to int
	data_size = atoi(argv[3]);

	//Init server address
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(server);
	servaddr.sin_port = htons(port);

	//Init socket
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Socket error\n");
		return 0;
	}

	//Establish connection
	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) < 0)
	{
		printf("Can not connect to %s\n", server);
		return 0;
	}

	//Get start time after connection establishment
	gettimeofday(&tv_start,NULL);

	//Send request
	write_count = strlen(argv[3]);
	while (write_count > 0)
		write_count -= send(sockfd, argv[3], write_count, 0);

	//Receive data
	while(1)
	{
		len = recv(sockfd, buf, RECV_BUFSIZ, 0);
		read_count += len;

		if(len <= 0)
			break;
	}

	//Get end time after receiving all of the data
	gettimeofday(&tv_end,NULL);
	//Close connection
	close(sockfd);

	//Calculate time interval (unit: microsecond)
	fct = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec;

	if (data_size * 1024 == read_count)
		printf("From %s: %d KB %lu us\n", server, data_size, fct);
	else
		printf("We receive %d (of %d) bytes.\n", read_count, data_size * 1024);

	return 0;
}


void usage(char *program)
{
	printf("%s [server IP] [server port] [request flow size(KB)]\n", program);
}
