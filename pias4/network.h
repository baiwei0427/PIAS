#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <linux/skbuff.h>
#include <linux/types.h>

#include "params.h"

/* Decide priority (DSCP) */
inline u8 pias_priority(u32 size);
/* Enable ECN and mark DSCP */
inline void pias_enable_ecn_dscp(struct sk_buff *skb, u8 dscp);
/* Determine whether seq1 is larger than seq2 */
inline bool pias_is_seq_larger(u32 seq1, u32 seq2);
/* Calculate gap between seq1 and seq2 */
inline u32 pias_seq_gap(u32 seq1, u32 seq2);

#endif
