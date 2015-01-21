#ifndef __FLOW_H__
#define __FLOW_H__

#include <linux/types.h>
#include <linux/ktime.h>

/*Define structure of information for a TCP flow
  *latest_update_time: the last time when we observe an outgoing packet (from local side to remote side) 
  *latest_timeout_time: the last time when we observe an TCP timeout
  *latest_timeout_seq: the sequence number for last TCP timeout
  *latest_seq: the largest (latest) sequence number for outoging traffic 
  *latest_ack: the largest (latest) ACK number for outoging traffic 
  *bytes_sent: bytes sent of outgoing traffic
  *bytes_total: total size of outgoing traffic
  *timeouts: the number of consecutive timeouts experienced by outgoing traffic  
  *is_size_known: shall we know size of this flow?
  *is_deadline_known: shall we know deadline of this flow?
  */
  
struct Karuna_Flow_Info
{
	ktime_t latest_update_time; 	
	ktime_t latest_timeout_time;
	u32 latest_timeout_seq;
	u32 latest_seq;
	u32 latest_ack;
	u32 bytes_sent;
	u32 bytes_total;
	u16 timeouts;
	bool is_size_known;	//If is_size_known is 1, it is type 2
	bool is_deadline_known;	//If is_deadline_known is 1, it's type 1 
};

//Define structure of a TCP flow
//A TCP Flow is defined by 4-tuple <local_ip,remote_ip,local_port,remote_port> and its related information
struct Karuna_Flow
{
	u32 local_ip;	//Local IP address
	u32 remote_ip;	//Remote IP address
	u16 local_port;	//Local TCP port
	u16 remote_port;	//Remote TCP port
	struct Karuna_Flow_Info info;	//Information for this flow
};

//Link Node of Flow
struct Karuna_Flow_Node
{
	struct Karuna_Flow f;	//structure of Flow
	struct Karuna_Flow_Node* next; //pointer to next node 
};

//Link List of Flows
struct Karuna_Flow_List
{
	struct Karuna_Flow_Node* head; //pointer to head node of this link list
	unsigned int len;	//current length of this list (max: QUEUE_SIZE)
};

//Hash Table of Flows
struct Karuna_Flow_Table
{
	struct Karuna_Flow_List* table;	//many FlowList (HASH_RANGE)
	unsigned int size;	//total number of nodes in this table
	spinlock_t tableLock;
};

//Print functions
void Karuna_Print_Flow(struct Karuna_Flow* f, int type);
void Karuna_Print_Node(struct Karuna_Flow_Node* fn);
void Karuna_Print_List(struct Karuna_Flow_List* fl);
void Karuna_Print_Table(struct Karuna_Flow_Table* ft);

static inline unsigned int Karuna_Hash(struct Karuna_Flow* f);
static inline bool Karuna_Equal(struct Karuna_Flow* f1,struct Karuna_Flow* f2);

//Initialization functions
void Karuna_Init_Info(struct Karuna_Flow_Info* info);
void Karuna_Init_Flow(struct Karuna_Flow* f);
void Karuna_Init_Node(struct Karuna_Flow_Node* fn);
void Karuna_Init_List(struct Karuna_Flow_List* fl);
void Karuna_Init_Table(struct Karuna_Flow_Table* ft);

//Insert functions
unsigned int Karuna_Insert_List(struct Karuna_Flow_List* fl, struct Karuna_Flow* f, int flags);
unsigned int Karuna_Insert_Table(struct Karuna_Flow_Table* ft,struct Karuna_Flow* f, int flags);

//Search functions
struct Karuna_Flow_Info* Karuna_Search_List(struct Karuna_Flow_List* fl, struct Karuna_Flow* f);
struct Karuna_Flow_Info* Karuna_Search_Table(struct Karuna_Flow_Table* ft, struct Karuna_Flow* f);

//Delete functions
u32 Karuna_Delete_List(struct Karuna_Flow_List* fl, struct Karuna_Flow* f, u8* type);
u32 Karuna_Delete_Table(struct Karuna_Flow_Table* ft,struct Karuna_Flow* f, u8* type);

//Empty functions
void Karuna_Empty_List(struct Karuna_Flow_List* fl);
void Karuna_Empty_Table(struct Karuna_Flow_Table* ft);

#endif