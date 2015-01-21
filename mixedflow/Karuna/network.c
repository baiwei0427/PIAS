#include <linux/ip.h>
#include <net/dsfield.h>

#include "network.h"
#include "params.h"

//type: deadline flow(0), non-deadline flow with priori knowledge of size(1), non-deadline flow without priori knowledge of size(2)
//size: 0 for deadline flow, bytes_total/bytes_sent for non-deadline flow 
u8 Karuna_priority(u8 type, u32 size)
{
	if(type==1)
	{
		if(size<=Karuna_AWARE_THRESHOLD_1)
			return PRIORITY_DSCP_1;
		else if(size<=Karuna_AWARE_THRESHOLD_2)
			return PRIORITY_DSCP_2;
		else if(size<=Karuna_AWARE_THRESHOLD_3)
			return PRIORITY_DSCP_3;
		else if(size<=Karuna_AWARE_THRESHOLD_4)
			return PRIORITY_DSCP_4;
		else if(size<=Karuna_AWARE_THRESHOLD_5)
			return PRIORITY_DSCP_5;
		else if(size<=Karuna_AWARE_THRESHOLD_6)
			return PRIORITY_DSCP_6;
		else if(size<=Karuna_AWARE_THRESHOLD_7)
			return PRIORITY_DSCP_7;
		else
			return PRIORITY_DSCP_8;
	}
	else if(type==2)
	{
		if(size<=Karuna_AGNOSTIC_THRESHOLD_1)
			return PRIORITY_DSCP_1;
		else if(size<=Karuna_AGNOSTIC_THRESHOLD_2)
			return PRIORITY_DSCP_2;
		else if(size<=Karuna_AGNOSTIC_THRESHOLD_3)
			return PRIORITY_DSCP_3;
		else if(size<=Karuna_AGNOSTIC_THRESHOLD_4)
			return PRIORITY_DSCP_4;
		else if(size<=Karuna_AGNOSTIC_THRESHOLD_5)
			return PRIORITY_DSCP_5;
		else if(size<=Karuna_AGNOSTIC_THRESHOLD_6)
			return PRIORITY_DSCP_6;
		else if(size<=Karuna_AGNOSTIC_THRESHOLD_7)
			return PRIORITY_DSCP_7;
		else
			return PRIORITY_DSCP_8;
	}
	else//type 0 or thers
	{
		return PRIORITY_DSCP_1;
	}
}

//mark DSCP and enable ECN
inline void Karuna_enable_ecn_dscp(struct sk_buff *skb, u8 dscp)
{
	if(skb_make_writable(skb,sizeof(struct iphdr))
	{
		ipv4_change_dsfield(ip_hdr(skb), 0xff, (dscp<<2)|INET_ECN_ECT_0);
	}
}

//Maximum unsigned 32-bit integer value: 4294967295
//Function: determine whether seq1 is larger than seq2
//If Yes, return 1. Else, return 0.
//We use a simple heuristic to handle wrapped TCP sequence number 
inline bool Karuna_is_seq_larger(u32 seq1, u32 seq2)
{
	if(likely(seq1>seq2&&seq1-seq2<=4294900000))
		return 1;
	else if(seq1<seq2&&seq2-seq1>4294900000)
		return 1;
	else
		return 0;
}

inline u32 Karuna_seq_gap(u32 larger, u32 smaller)
{
	if(likely(larger>=smaller))
		return larger-smaller;
	else
		return 4294967295-(smaller-larger); 
}
