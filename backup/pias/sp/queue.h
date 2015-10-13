#ifndef QUEUE_H
#define QUEUE_H

#include <linux/vmalloc.h>

struct Packet{

    int (*okfn)(struct sk_buff *);  	//function pointer to reinject packets
    struct sk_buff *skb;           			//socket buffer descriptor to packet               
}; 

struct PacketQueue{
    struct Packet *packets;		//Array to store packets
    unsigned int head;				//Head packet
	unsigned int size;				//Current queue size (packets)
	unsigned int bytes;				//Current queue size (bytes)
	unsigned int max_length;	//Maximum length for this packet queue
};

//If initialization succeeds, return 1
//Else, return 0
unsigned int Init_PacketQueue(struct PacketQueue* q, unsigned int length)
{
	struct Packet *buf=vmalloc(length*sizeof(struct Packet));
	if(buf!=NULL)
	{
		q->packets=buf;
		q->head=0;
		q->size=0;
		q->max_length=length=length;
		return 1;
	}
	else
	{
		printk(KERN_INFO "Vmalloc error\n");
		return 0;
	}
}

void Free_PacketQueue(struct PacketQueue* q)
{
	vfree(q->packets);
}

int Enqueue_PacketQueue(struct PacketQueue* q,struct sk_buff *skb,int (*okfn)(struct sk_buff *))
{
	//There is capacity to contain new packets
	if(q->size<q->max_length) {

		//Index for new insert packet
		int queueIndex=(q->head+q->size)%q->max_length;
		q->packets[queueIndex].skb=skb;
		q->packets[queueIndex].okfn=okfn;
		q->size++;
		q->bytes=q->bytes+skb->len;
		return 1;

	} else {

		return 0;
	}
}

int Dequeue_PacketQueue(struct PacketQueue* q)
{
	if(q->size>0) {
		//Reinject head packet of current queue
		(q->packets[q->head].okfn)(q->packets[q->head].skb);
		q->size--;
		q->bytes=q->bytes-q->packets[q->head].skb->len;
		q->head=(q->head+1)%q->max_length;
		return 1;

	} else {
	
		return 0;
	}
}

#endif