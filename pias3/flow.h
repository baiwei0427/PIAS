#ifndef __FLOW_H__
#define __FLOW_H__

#include <linux/types.h>
#include <linux/ktime.h>

/*Define structure of information for a TCP flow
  *latest_update_time: the last time when we observe an outgoing packet (from local side to remote side) 
  *latest_timeout_seq: the sequence number for last TCP timeout
  *latest_seq: the largest (latest) sequence number for outoging traffic 
  *latest_ack: the largest (latest) ACK number for outoging traffic 
  *bytes_sent: bytes sent of outgoing traffic
  *timeouts: the number of consecutive timeouts experienced by outgoing traffic  
  */
  
struct PIAS_Flow_Info
{
	ktime_t latest_update_time; 	
	u32 latest_timeout_seq;
	u32 latest_seq;
	u32 latest_ack;
	u32 bytes_sent;
	u16 timeouts;
};

//Define structure of a TCP flow
//A TCP Flow is defined by 4-tuple <local_ip,remote_ip,local_port,remote_port> and its related information
struct PIAS_Flow
{
	u32 local_ip;	//Local IP address
	u32 remote_ip;	//Remote IP address
	u16 local_port;	//Local TCP port
	u16 remote_port;	//Remote TCP port
	struct PIAS_Flow_Info info;	//Information for this flow
};

//Link Node of Flow
struct PIAS_Flow_Node
{
	struct PIAS_Flow f;	//structure of Flow
	struct PIAS_Flow_Node* next;	//pointer to next node 
};

//Link List of Flows
struct PIAS_Flow_List
{
	struct PIAS_Flow_Node* head;	//pointer to head node of this link list
	unsigned int len;	//current length of this list 
};

//Hash Table of Flows
struct PIAS_Flow_Table
{
	struct PIAS_Flow_List* table;	//many FlowList 
	unsigned int size;	//total number of nodes in this table
	spinlock_t tableLock;
};

//Print functions
void PIAS_Print_Flow(struct PIAS_Flow* f, int type);
void PIAS_Print_Node(struct PIAS_Flow_Node* fn);
void PIAS_Print_List(struct PIAS_Flow_List* fl);
void PIAS_Print_Table(struct PIAS_Flow_Table* ft);

inline unsigned int PIAS_Hash(struct PIAS_Flow* f);
inline bool PIAS_Equal(struct PIAS_Flow* f1,struct PIAS_Flow* f2);

//Initialization functions
void PIAS_Init_Info(struct PIAS_Flow_Info* info);
void PIAS_Init_Flow(struct PIAS_Flow* f);
void PIAS_Init_Node(struct PIAS_Flow_Node* fn);
void PIAS_Init_List(struct PIAS_Flow_List* fl);
void PIAS_Init_Table(struct PIAS_Flow_Table* ft);

//Insert functions
unsigned int PIAS_Insert_List(struct PIAS_Flow_List* fl, struct PIAS_Flow* f, int flags);
unsigned int PIAS_Insert_Table(struct PIAS_Flow_Table* ft,struct PIAS_Flow* f, int flags);

//Search functions
struct PIAS_Flow_Info* PIAS_Search_List(struct PIAS_Flow_List* fl, struct PIAS_Flow* f);
struct PIAS_Flow_Info* PIAS_Search_Table(struct PIAS_Flow_Table* ft, struct PIAS_Flow* f);

//Delete functions
u32 PIAS_Delete_List(struct PIAS_Flow_List* fl, struct PIAS_Flow* f);
u32 PIAS_Delete_Table(struct PIAS_Flow_Table* ft,struct PIAS_Flow* f);

//Empty functions
void PIAS_Empty_List(struct PIAS_Flow_List* fl);
void PIAS_Empty_Table(struct PIAS_Flow_Table* ft);

#endif