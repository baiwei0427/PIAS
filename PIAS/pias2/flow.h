#ifndef	FLOW_H
#define	FLOW_H

//The structure of information of a TCP flow
//latest_update_time: the last time when we observe an outgoing packet (from local side to remote side) 
//latest_seq: the largest(latest) sequence number of outoging traffic 
//send_data: the total size of outgoing traffic  
//timeouts: the number of consecutive timeouts experienced by outgoing traffic
//priority: the latest priority of incoming traffic
struct Information 
{
	unsigned int latest_update_time; 	
	unsigned int latest_seq;
	unsigned int send_data;
	unsigned int timeouts;
	//0 is the highest priority 
	unsigned int priority;
}; 

//The structure of for a TCP flow
//A TCP flow is defined by 4-tuple <local_ip,remote_ip,local_port,remote_port> and its related information
struct Flow
{	
	unsigned int remote_ip;
	unsigned int local_ip;
	unsigned short int remote_port;
	unsigned short int local_port;
	struct Information info; 
};

#endif