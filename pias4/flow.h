#ifndef __FLOW_H__
#define __FLOW_H__

#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/list.h>

//Hash range (Number of flow lists)
#define PIAS_HASH_RANGE 256

/*
 * Define structure of information for a TCP flow (connections)
 * last_copy_time: last time when we observe data copy from user space to kernel TCP send buffer
 * last_update_time: last time when we observe an outgoing packet (from local side to remote side)
 * last_timeout_seq: sequence number for the lastest TCP timeout
 * last_seq: the latest (largest) sequence number for outoging traffic
 * last_ack: the latest (largest) ACK number for outoging traffic
 * bytes_sent: bytes sent of outgoing traffic
 * timeouts: the number of consecutive timeouts experienced by outgoing traffic
 * messages: the number of messages in this TCP connections
 */

struct PIAS_Flow_Info
{
    ktime_t last_copy_time;
    ktime_t last_update_time;
    u32 last_timeout_seq;
    u32 last_seq;
    u32 last_ack;
    u32 bytes_sent;
    u16 timeouts;
    u16 messages;
};

/* A TCP Flow is defined by 4-tuple <local_ip,remote_ip,local_port,remote_port> and its related information */
struct PIAS_Flow
{
    u32 local_ip;   //Local IP address
    u32 remote_ip;  //Remote IP address
    u16 local_port; //Local TCP port
    u16 remote_port;    //Remote TCP port
    struct PIAS_Flow_Info info; //Information for this flow
    spinlock_t lock;    //lock for this flow
    struct list_head list;	//linked list
};

/* Link List of Flows */
struct PIAS_Flow_List
{
    struct list_head head_node;    //head node of the flow list
    unsigned int len;  //total number of flows in the list
    spinlock_t lock;    //lock for this flow list
};

/* Hash Table of Flows */
struct PIAS_Flow_Table
{
    struct PIAS_Flow_List* flow_lists;  //array of linked lists to store per-flow information
    atomic_t size;
};

/* Print functions */
void PIAS_Print_Flow(struct PIAS_Flow* f, char* operation);
void PIAS_Print_List(struct PIAS_Flow_List* fl);
void PIAS_Print_Table(struct PIAS_Flow_Table* ft);

inline unsigned int PIAS_Hash_Flow(struct PIAS_Flow* f);
inline bool PIAS_Equal_Flow(struct PIAS_Flow* f1, struct PIAS_Flow* f2);

/* Initialization functions */
bool PIAS_Init_Info(struct PIAS_Flow_Info* info);
bool PIAS_Init_Flow(struct PIAS_Flow* f);
bool PIAS_Init_List(struct PIAS_Flow_List* fl);
bool PIAS_Init_Table(struct PIAS_Flow_Table* ft);

/* Search functions: search a flow entry from flow table/list */
struct PIAS_Flow* PIAS_Search_List(struct PIAS_Flow_List* fl, struct PIAS_Flow* f);
struct PIAS_Flow* PIAS_Search_Table(struct PIAS_Flow_Table* ft, struct PIAS_Flow* f);

/* Insert functions: insert a new flow entry to flow table/list */
bool PIAS_Insert_List(struct PIAS_Flow_List* fl, struct PIAS_Flow* f, int flags);
bool PIAS_Insert_Table(struct PIAS_Flow_Table* ft,struct PIAS_Flow* f, int flags);

/* Delete functions: delete a flow entry from flow table/list */
u32 PIAS_Delete_List(struct PIAS_Flow_List* fl, struct PIAS_Flow* f);
u32 PIAS_Delete_Table(struct PIAS_Flow_Table* ft, struct PIAS_Flow* f);

/* Clear functions: clear flow entries from flow table/list */
bool PIAS_Clear_List(struct PIAS_Flow_List* fl);
bool PIAS_Clear_Table(struct PIAS_Flow_Table* ft);

/* Exit functions: delete whole flow table */
bool PIAS_Exit_Table(struct PIAS_Flow_Table* ft);

#endif
