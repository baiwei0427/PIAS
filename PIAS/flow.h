#ifndef	FLOW_H
#define	FLOW_H

//Define the structure of information of a TCP flow
struct Information 
{
	//the last update (incoming/outgoing data) for this flow
	unsigned int last_update_time; 
	//the total size of data sent by local side
	unsigned int send_data;
}; 

//Define the structure of for a TCP flow
struct Flow
{	
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