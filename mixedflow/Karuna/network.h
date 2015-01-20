#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <linux/skbuff.h>
#include <linux/types.h>

void Karuna_enable_ecn_dscp(struct sk_buff *skb, u8 dscp);
inline bool Karuna_is_seq_larger(u32 seq1, u32 seq2);

#endif

