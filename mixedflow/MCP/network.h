#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <linux/skbuff.h>
#include <linux/types.h>

inline u32 get_tsval(void);
inline void clear_ecn(struct sk_buff *skb);
u8 tcp_get_scale(struct sk_buff *skb);
u32 tcp_modify_incoming(struct  sk_buff *skb, u32 time);
u8 tcp_modify_outgoing(struct sk_buff *skb, u16 win, u32 time);
inline u8 is_seq_larger(u32 seq1, u32 seq2);


#endif

