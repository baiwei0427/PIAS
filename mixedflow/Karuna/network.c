#include <linux/ip.h>
#include <net/dsfield.h>

#include "network.h"
#include "params.h"

void Karuna_enable_ecn_dscp(struct sk_buff *skb, u8 dscp)
{
	struct iphdr *iph =ip_hdr(skb);
	if(likely(iph!=NULL))
	{
		if(skb_make_writable(skb,sizeof(struct iphdr))
		{
			ipv4_change_dsfield(iph, 0xff, (dscp<<2)|INET_ECN_ECT_0);
		}
	}
}

//Maximum unsigned 32-bit integer value: 4294967295
//Function: determine whether seq1 is larger than seq2
//If Yes, return 1. Else, return 0.
//We use a simple heuristic to handle wrapped TCP sequence number 
inline bool Karuna_is_seq_larger(u32 seq1, u32 seq2)
{
	if(likely(seq1>seq2&&seq1-seq2<=4294900000))
	{
		return 1;
	}
	else if(seq1<seq2&&seq2-seq1>4294900000)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
