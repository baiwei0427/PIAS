#ifndef __FLOW_H__
#define __FLOW_H__

#include <linux/types.h>
#include <linux/ktime.h>

/*Define structure of information for a TCP flow
 *	latest_seq: the latest sequence number of this flow
 *	bytes_received: the number of bytes that have been received (should exclude retranmission)
 * bytes_total: the total number of bytes of this flow
 *	window: the window size (MSS) of this flow
 * scale: TCP window scale
 *	srtt: the smoothed RTT value (us)
 *	bytes_received_rtt: the number of bytes that have been received in this RTT
 * bytes_received_ecn_rtt: he number of bytes that have been received and marked with ECN in this RTT 
 *	deadline: deadline time
 *	last_update: last update time of this flow
 */
struct MCP_Flow_Info
{
	u32 latest_seq;
	u32 bytes_received;
	u32 bytes_total;
	u16	window;
	u16 scale;
	u32 srtt;
	u32 bytes_rtt_received;
	u32 bytes_rtt_received_ecn;
	ktime_t deadline;
	ktime_t last_update;
};

//Define structure of a TCP flow
//A TCP Flow is defined by 4-tuple <local_ip,remote_ip,local_port,remote_port> and its related information
struct MCP_Flow
{
	u32 local_ip;	//Local IP address
	u32 remote_ip;	//Remote IP address
	u16 local_port;	//Local TCP port
	u16 remote_port;	//Remote TCP port
	struct MCP_Flow_Info info;	//Information for this flow
};

//Link Node of Flow
struct MCP_Flow_Node
{
	struct MCP_Flow f;	//structure of Flow
	struct MCP_Flow_Node* next; //pointer to next node 
};

//Link List of Flows
struct MCP_Flow_List
{
	struct MCP_Flow_Node* head;	//pointer to head node of this link list
	unsigned int len;	//current length of this list (max: QUEUE_SIZE)
};

//Hash Table of Flows
struct MCP_Flow_Table
{
	struct MCP_Flow_List* table;	//many FlowList (HASH_RANGE)
	unsigned int size;	//total number of nodes in this table
	spinlock_t tableLock;
};

//Print functions
void MCP_Print_Flow(struct MCP_Flow* f, int type);
void MCP_Print_Node(struct MCP_Flow_Node* fn);
void MCP_Print_List(struct MCP_Flow_List* fl);
void MCP_Print_Table(struct MCP_Flow_Table* ft);

unsigned int MCP_Hash(struct MCP_Flow* f);
unsigned int MCP_Equal(struct MCP_Flow* f1,struct MCP_Flow* f2);

//Initialization functions
void MCP_Init_Info(struct MCP_Flow_Info* info);
void MCP_Init_Flow(struct MCP_Flow* f);
void MCP_Init_Node(struct MCP_Flow_Node* fn);
void MCP_Init_List(struct MCP_Flow_List* fl);
void MCP_Init_Table(struct MCP_Flow_Table* ft);

//Insert functions
unsigned int MCP_Insert_List(struct MCP_Flow_List* fl, struct MCP_Flow* f, int flags);
unsigned int MCP_Insert_Table(struct MCP_Flow_Table* ft,struct MCP_Flow* f, int flags);

//Search functions
struct MCP_Flow_Info* MCP_Search_List(struct MCP_Flow_List* fl, struct MCP_Flow* f);
struct MCP_Flow_Info* MCP_Search_Table(struct MCP_Flow_Table* ft, struct MCP_Flow* f);

//Delete functions
u32 MCP_Delete_List(struct MCP_Flow_List* fl, struct MCP_Flow* f);
u32 MCP_Delete_Table(struct MCP_Flow_Table* ft,struct MCP_Flow* f);

//Empty functions
void MCP_Empty_List(struct MCP_Flow_List* fl);
void MCP_Empty_Table(struct MCP_Flow_Table* ft);

#endif