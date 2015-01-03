#ifndef	FLOW_H
#define	FLOW_H

//The structure of information of a TCP flow
//latest_update_time: the last time when we observe an outgoing packet (from local side to remote side) 
//latest_timeout_time: the last time when we observe an TCP timeout
//latest_timeout_seq: the sequence number for last TCP timeout
//latest_seq: the largest (latest) sequence number for outoging traffic 
//latest_ack: the largest (latest) ACK number for outoging traffic 
//bytes_sent: the total payload size of outgoing traffic  
//timeouts: the number of consecutive timeouts experienced by outgoing traffic
struct Information 
{
	ktime_t latest_update_time; 	
	ktime_t latest_timeout_time;
	u32 latest_timeout_seq;
	u32 latest_seq;
	u32 latest_ack;
	u32 bytes_sent;
	u32 timeouts;
	//0 is the highest priority 
	//unsigned int priority;
}; 

//The structure of for a TCP flow
//A TCP flow is defined by 4-tuple <local_ip,remote_ip,local_port,remote_port> and its related information
struct Flow
{	
	u32 remote_ip;
	u32 local_ip;
	u16 remote_port;
	u16 local_port;
	struct Information info; 
};

#endif