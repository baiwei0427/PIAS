#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <linux/skbuff.h>
#include <linux/types.h>

/* Based on size, return DSCP value */
u8 PIAS_priority(u32 size);

/* Mark DSCP and enable ECN */
inline void PIAS_enable_ecn_dscp(struct sk_buff *skb, u8 dscp);

/* Determine whether seq1 is larger than seq2 */
inline bool PIAS_is_seq_larger(u32 seq1, u32 seq2);

/* Calculate gap between seq1 and seq2 */
u32 PIAS_seq_gap(u32 larger, u32 smaller);

#endif

