#ifndef FLOW_H
#define FLOW_H

#define INCOMING	0
#define OUTGOING	1

//Define the structure of information of a TCP flow
//We use receive_data/(latest_receive_time-start_time) to calculate FCT at receiver side
struct Information 
{
	//start time for this flow: when we observed an outgoing request (us) 
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
	//If this connection is actively open by the remote side, the direction is 0  (INCOMING) 
	//If this connection is actively open by the local side, the direction is 1	 	(OUTGOING)       
	unsigned int direction; 
	
	//Remote IP address
	unsigned int remote_ip;
	//Local IP address
	unsigned int local_ip;
	//Remote TCP port
	unsigned short int remote_port;
	//Local TCP port
	unsigned short int local_port;
	
	//Structure of information  for this connection
	struct Information info; 
};

#endif