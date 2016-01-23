#include <linux/ip.h>
#include <net/dsfield.h>
#include <net/inet_ecn.h>

#include "network.h"
#include "params.h"

/* Based on size, return DSCP value of corresponding priority. */
int pias_priority(u32 size)
{
    int i = 0;

    for (i = 0; i < PIAS_PRIO_NUM - 1; i++)
    {
        if (size <= PIAS_PRIO_THRESH[i])
            return PIAS_PRIO_DSCP[i];
    }

    //By default, return DSCP of the lowest priority
    return PIAS_PRIO_DSCP[PIAS_PRIO_NUM - 1];
}

/* mark DSCP and enable ECN */
inline void pias_enable_ecn_dscp(struct sk_buff *skb, u8 dscp)
{
	if (likely(skb && skb_make_writable(skb, sizeof(struct iphdr))))
		ipv4_change_dsfield(ip_hdr(skb), 0x00, (dscp << 2)|INET_ECN_ECT_0);
}

/* Determine whether seq1 is larger than seq2 */
inline bool pias_is_seq_larger(u32 seq1, u32 seq2)
{
    if (likely(seq1 > seq2 && seq1 - seq2 <= 4294900000))
        return true;
    else if (seq1 < seq2 && seq2 - seq1 > 4294900000)
        return true;
    else
        return false;
}

/* Calculate gap between seq1 and seq2 */
u32 pias_seq_gap(u32 seq1, u32 seq2)
{
	//seq1 is larger seq2
	if (pias_is_seq_larger(seq1, seq2))
	{
		if (likely(seq1 > seq2))
			return seq1-seq2;
		else
			return 4294967295- (seq2 - seq1);
	}
	else
	{
		if (likely(seq2 > seq1))
			return seq2 - seq1;
		else
			return 4294967295 - (seq1 - seq2);
	}
}
