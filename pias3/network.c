#include <linux/ip.h>
#include <net/dsfield.h>
#include <net/inet_ecn.h>

#include "network.h"
#include "params.h"

//Based on size, return DSCP value
//We have 8 priorities at most.
u8 PIAS_priority(u32 size)
{
	if(size<=PIAS_THRESHOLD_1)
		return PIAS_PRIORITY_DSCP_1;
	else if(size<=PIAS_THRESHOLD_2)
		return PIAS_PRIORITY_DSCP_2;
	else if(size<=PIAS_THRESHOLD_3)
		return PIAS_PRIORITY_DSCP_3;
	else if(size<=PIAS_THRESHOLD_4)
		return PIAS_PRIORITY_DSCP_4;
	else if(size<=PIAS_THRESHOLD_5)
		return PIAS_PRIORITY_DSCP_5;
	else if(size<=PIAS_THRESHOLD_6)
		return PIAS_PRIORITY_DSCP_6;
	else if(size<=PIAS_THRESHOLD_7)
		return PIAS_PRIORITY_DSCP_7;
	else
		return PIAS_PRIORITY_DSCP_8;
}

//mark DSCP and enable ECN
inline void PIAS_enable_ecn_dscp(struct sk_buff *skb, u8 dscp)
{
	if(skb_make_writable(skb,sizeof(struct iphdr)))
	{
		ipv4_change_dsfield(ip_hdr(skb), 0xff, (dscp<<2)|INET_ECN_ECT_0);
	}
}

//Maximum unsigned 32-bit integer value: 4294967295
//Function: determine whether seq1 is larger than seq2
//If Yes, return 1. Else, return 0.
//We use a simple heuristic to handle wrapped TCP sequence number 
inline bool PIAS_is_seq_larger(u32 seq1, u32 seq2)
{
	if(likely(seq1>seq2&&seq1-seq2<=4294900000))
		return 1;
	else if(seq1<seq2&&seq2-seq1>4294900000)
		return 1;
	else
		return 0;
}

u32 PIAS_seq_gap(u32 seq1, u32 seq2)
{
	//seq1 is larger seq2
	if(PIAS_is_seq_larger(seq1,seq2)==1)
	{
		if(likely(seq1>seq2))
			return seq1-seq2;
		else
			return 4294967295-(seq2-seq1);
	}
	else
	{
		if(likely(seq2>seq1))
			return seq2-seq1;
		else
			return 4294967295-(seq1-seq2);
	}
}
