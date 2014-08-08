#ifndef FLOW_H
#define FLOW_H

//We define the status of TCP here (see TCP/IP Illustrated Volume 1: The Protocols for more details)
#define SYN_SENT			1
#define SYN_RCVD 		2
#define ESTABLISHED 	3
#define FIN_WAIT_1 		4
#define CLOSE_WAIT 	5
#define FIN_WAIT_2 		6
#define LAST_ACK 			7
#define TIME_WAIT 		8
#define CLOSE 				9

//Define the structure of information of a TCP flow
//We use receive_data/(latest_receive_time-start_time) to calculate FCT at receiver side
struct Information 
{
	//start time for this flow (us)
	unsigned int start_time; 
	//the latest time when the local side receives some payload data (us)
	unsigned int latest_receive_time; 
	//the smooth averaged RTT value
	unsigned int srtt;
	//the latest sequence number
	unsigned int seq;
	//the latest acknowledgement number
	unsigned int ack;
	//the total size of data received from remote side 
	unsigned int receive_data;
	//the total size of data sent by local side
	unsigned int send_data;
	//status of TCP 
	unsigned int stauts;
}; 

//Define the structure of for a TCP flow
struct Flow
{
	//If this connection is actively open by the remote side, the direction is 0
	//If this connection is actively open by the local side, the direction is 1
	unsigned int direction; 
	
	//Remote IP address
	unsigned int remote_ip;
	//Local IP address
	unsigned int local_ip;
	//Remote TCP port
	unsigned int remote_port;
	//Local TCP port
	unsigned int local_port;
	
	//Structure of information  for this connection
	struct Information info; 
};

#endif