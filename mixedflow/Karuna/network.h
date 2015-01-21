#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <linux/skbuff.h>
#include <linux/types.h>

//Based on the flow type (ddl/non-ddl with flow size/non-ddl w/o flow size) and size (bytes_total/bytes_sent), return DSCP value
u8 Karuna_priority(u8 type, u32 size);
//mark DSCP and enable ECN
inline void Karuna_enable_ecn_dscp(struct sk_buff *skb, u8 dscp);
//If seq1 is larger than seq2
inline bool Karuna_is_seq_larger(u32 seq1, u32 seq2);
inline u32 Karuna_seq_gap(u32 larger, u32 smaller);

#endif

