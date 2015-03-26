#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <net/netlink.h>
#include <linux/pkt_sched.h>
#include <net/sch_generic.h>
#include <net/pkt_sched.h>
#include <linux/ip.h>
#include <net/dsfield.h>
#include <net/inet_ecn.h>

#include "params.h"

struct pias_rate_cfg 
{
	u64 rate_bps;	/* bit per second */ 
	u32 mult;
	u32 shift;
};

struct pias_sched_data 
{
/* Parameters */
	struct Qdisc **queues; /* Priority queues where queues[0] has the highest priority*/
	struct pias_rate_cfg rate;	
	
/* Variables */
	u64 tokens;	/* Tokens in nanoseconds */ 
	u32 sum_len;	/* The sum of lengh og all queues in bytes */
	u32 max_len; 	/* Maximum value of sum_len*/
	struct Qdisc *sch; 
	u64	time_ns;	/* Time check-point */ 
	struct qdisc_watchdog watchdog;	/* Watchdog timer */
};

/* 
 * We use this function to account for the true number of bytes sent on wire.
 * 90=frame check sequence(8B)+Frame check sequence(4B)+Interpacket gap(12B)
 * +Ethernet header(14B)+IP header (20B)+TCP header(20B)+TCP options(12B in our testbed)
 * 24=frame check sequence(8B)+Frame check sequence(4B)+Interpacket gap(12B)
 * 20=frame check sequence(8B)+Interpacket gap(12B)
 */
static inline unsigned int skb_size(struct sk_buff *skb)
{
	if(skb->len>PIAS_QDISC_MTU_BYTES)
		return skb->len+skb->len/PIAS_QDISC_MTU_BYTES*90+24;
	else
		return max_t(unsigned int, skb->len+4,PIAS_QDISC_MIN_PKT_BYTES)+20;
}

/* Borrow from ptb */
static inline void pias_qdisc_precompute_ratedata(struct pias_rate_cfg *r)
{
	r->shift = 0;
	r->mult = 1;

	if (r->rate_bps > 0) 
	{
		r->shift = 15;
		r->mult = div64_u64(8LLU * NSEC_PER_SEC * (1 << r->shift), r->rate_bps);
	}
}

/* Borrow from ptb */
static inline u64 l2t_ns(struct pias_rate_cfg *r, unsigned int len_bytes)
{
	return ((u64)len_bytes * r->mult) >> r->shift;
}

static inline void pias_qdisc_ecn(struct sk_buff *skb)
{
	if(skb_make_writable(skb,sizeof(struct iphdr))&&ip_hdr(skb))
		IP_ECN_set_ce(ip_hdr(skb));
}
			
static struct Qdisc * pias_qdisc_classify(struct sk_buff *skb, struct Qdisc *sch)
{
	struct pias_sched_data *q = qdisc_priv(sch);	
	struct iphdr* iph=ip_hdr(skb);
	int dscp;
	
	if(unlikely(q->queues==NULL))
		return NULL;
	
	/* Return the highest priority queue by default*/
	if(unlikely(iph==NULL))
		return q->queues[0];

	dscp=(const int)(iph->tos>>2);

	if(dscp==PIAS_QDISC_PRIO_DSCP_1)
		return q->queues[0];
	else if(dscp==PIAS_QDISC_PRIO_DSCP_2)
		return q->queues[1];
	else if(dscp==PIAS_QDISC_PRIO_DSCP_3)
		return q->queues[2];
	else if(dscp==PIAS_QDISC_PRIO_DSCP_4)
		return q->queues[3];
	else if(dscp==PIAS_QDISC_PRIO_DSCP_5)
		return q->queues[4];
	else if(dscp==PIAS_QDISC_PRIO_DSCP_6)
		return q->queues[5];
	else if(dscp==PIAS_QDISC_PRIO_DSCP_7)
		return q->queues[6];
	//By default, return the lowest priority queue
	else //if(dscp==PIAS_QDISC_PRIO_DSCP_8)
		return q->queues[7];
	return NULL;
}

static struct sk_buff* pias_qdisc_dequeue_peeked(struct Qdisc *sch)
{
	struct pias_sched_data *q = qdisc_priv(sch);
	struct Qdisc *qdisc;
	struct sk_buff *skb;
	int i;

	for(i=0;i<PIAS_QDISC_MAX_PRIO && q->queues[i]; i++)
	{
		qdisc = q->queues[i];
		skb = qdisc_dequeue_peeked(qdisc);
		if (skb)
			return skb;
	}
	return NULL;
}

static struct sk_buff*pias_qdisc_peek(struct Qdisc *sch)
{
	struct pias_sched_data *q = qdisc_priv(sch);
	struct Qdisc *qdisc;
	struct sk_buff *skb;
	int i;

	for(i=0;i<PIAS_QDISC_MAX_PRIO && q->queues[i]; i++)
	{
		qdisc = q->queues[i];
		skb = qdisc->ops->peek(qdisc);
		if (skb)
			return skb;
	}
	return NULL;
}

static struct sk_buff* pias_qdisc_dequeue(struct Qdisc *sch)
{
	struct pias_sched_data *q = qdisc_priv(sch);	
	struct sk_buff *skb;
	
	skb = pias_qdisc_peek(sch);
	if(skb)
	{
		u64 now=ktime_get_ns();
		u64 toks=min_t(u64, now-q->time_ns, PIAS_QDISC_BUCKET_NS)+q->tokens;
		unsigned int len=skb_size(skb);
		u64 pkt_ns=l2t_ns(&q->rate, len);
				
		//If we have enough tokens to release this packet
		if(toks>=pkt_ns)
		{
			skb = pias_qdisc_dequeue_peeked(sch);		
			if (unlikely(!skb))
				return NULL;
			
			q->time_ns=now;
			q->sum_len-=len;
			sch->q.qlen--;
			q->tokens=toks-pkt_ns;
			//Bucket. 
			if(q->tokens>PIAS_QDISC_BUCKET_NS)
				q->tokens=PIAS_QDISC_BUCKET_NS;
			
			qdisc_unthrottled(sch);
			qdisc_bstats_update(sch, skb);
			//printk(KERN_INFO "sch_pias: dequeue a packet with len=%u\n",len);
			return skb;
		}
		else
		{
			//We use now+t due to absolute mode of hrtimer ((HRTIMER_MODE_ABS) ) 
			qdisc_watchdog_schedule_ns(&q->watchdog,now+pkt_ns-toks,true);
			qdisc_qstats_overlimit(sch);
		}
	}
	return NULL;
}

static int pias_qdisc_enqueue(struct sk_buff *skb, struct Qdisc *sch)
{
	struct Qdisc *qdisc;
	unsigned int len=skb_size(skb);
	int ret;
	struct pias_sched_data *q = qdisc_priv(sch);	
	
	qdisc=pias_qdisc_classify(skb,sch);
	
	//No appropriate priority queue or queue limit is reached
	if(qdisc==NULL||q->sum_len+len>q->max_len)
	{
		//printk(KERN_INFO "sch_pias: packet drop\n");
		qdisc_qstats_drop(sch);
		kfree_skb(skb);
		return NET_XMIT_DROP;
	}
	else
	{
		//ECN marking
		if(q->sum_len+len>PIAS_QDISC_ECN_THRESH_BYTES)
		{
			//printk(KERN_INFO "sch_pias: ECN marking\n");
			pias_qdisc_ecn(skb);
		}
		
		ret=qdisc_enqueue(skb, qdisc);
		if(ret == NET_XMIT_SUCCESS) 
		{
			sch->q.qlen++;
			q->sum_len+=len;
			//printk(KERN_INFO "sch_pias: queue length = %u\n",q->sum_len);
		}
		else
		{
			if (net_xmit_drop_count(ret))
				qdisc_qstats_drop(sch);
		}
		return ret;
	}
}

/* We don't need this */
static unsigned int pias_qdisc_drop(struct Qdisc *sch)
{
	return 0;
}

/* We don't need this */
static int pias_qdisc_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	return 0;
}

/* Release Qdisc resources */
static void pias_qdisc_destroy(struct Qdisc *sch)
{
	struct pias_sched_data *q = qdisc_priv(sch);
	int i;

	if(q->queues!=NULL)
	{
		for(i = 0; i < PIAS_QDISC_MAX_PRIO && q->queues[i]; i++)
			qdisc_destroy(q->queues[i]);

		kfree(q->queues);
		q->queues = NULL;
	}
	qdisc_watchdog_cancel(&q->watchdog);
}

/*
	struct nla_policy {
		u16             type;
        u16             len;
	};
	
	struct tc_ratespec {
         unsigned char   cell_log;
         __u8            linklayer; 
         unsigned short  overhead;
         short           cell_align;
         unsigned short  mpu;
         __u32           rate;
	};
	
	struct tc_tbf_qopt {
         struct tc_ratespec rate;
         struct tc_ratespec peakrate;
         __u32           limit;
         __u32           buffer;
         __u32           mtu;
	};
*/
static const struct nla_policy pias_qdisc_policy[TCA_TBF_MAX + 1] = {
	[TCA_TBF_PARMS] = { .len = sizeof(struct tc_tbf_qopt) },
	[TCA_TBF_RTAB]	= { .type = NLA_BINARY, .len = TC_RTAB_SIZE },
	[TCA_TBF_PTAB]	= { .type = NLA_BINARY, .len = TC_RTAB_SIZE },
};

/* We only leverage TC netlink interface to configure rate */
static int pias_qdisc_change(struct Qdisc *sch, struct nlattr *opt)
{
	int err;
	struct pias_sched_data *q = qdisc_priv(sch);
	struct nlattr *tb[TCA_TBF_PTAB + 1];
	struct tc_tbf_qopt *qopt;
	__u32 rate;
	
	err = nla_parse_nested(tb, TCA_TBF_PTAB, opt, pias_qdisc_policy);
	if(err < 0)
		return err;
	
	err = -EINVAL;
	if (tb[TCA_TBF_PARMS] == NULL)
		goto done;
	
	qopt = nla_data(tb[TCA_TBF_PARMS]);
	rate = qopt->rate.rate;
	/* convert from bytes/s to b/s */
	q->rate.rate_bps=(u64)rate << 3;
	pias_qdisc_precompute_ratedata(&q->rate);
	err=0;
	printk(KERN_INFO "sch_pias: rate %llu Mbps\n", q->rate.rate_bps/1000000);
	
 done:
	return err;
}

/* Initialize Qdisc */
static int pias_qdisc_init(struct Qdisc *sch, struct nlattr *opt)
{
	int i;
	struct pias_sched_data *q = qdisc_priv(sch);
	struct Qdisc *child;
	
	if(sch->parent != TC_H_ROOT)
		return -EOPNOTSUPP;
	
	q->queues=kcalloc(PIAS_QDISC_MAX_PRIO, sizeof(struct Qdisc),GFP_KERNEL);
	if(q->queues == NULL)
		return -ENOMEM;
	
	q->tokens=0;
	q->time_ns=ktime_get_ns();
	//Mbps to bps
	//q->rate.rate_bps = PIAS_QDISC_RATE_MBPS*1000000;
	//pias_qdisc_precompute_ratedata(&q->rate);
	q->sum_len=0;
	q->max_len = PIAS_QDISC_MAX_LEN_BYTES;	
	q->sch = sch;
	qdisc_watchdog_init(&q->watchdog, sch);
	
	for(i=0;i<PIAS_QDISC_MAX_PRIO;i++)
	{
		//bfifo is in bytes 
		child=fifo_create_dflt(sch, &bfifo_qdisc_ops, PIAS_QDISC_MAX_LEN_BYTES);
		if(child!=NULL)
			q->queues[i]=child;
		else
			goto err;
	}
	return pias_qdisc_change(sch,opt);
err:
	pias_qdisc_destroy(sch);
	return -ENOMEM;
}

static struct Qdisc_ops pias_qdisc_ops __read_mostly = {
	.next = NULL,
	.cl_ops = NULL,
	.id = "tbf",
	.priv_size = sizeof(struct pias_sched_data ),
	.init = pias_qdisc_init,
	.destroy = pias_qdisc_destroy,
	.enqueue = pias_qdisc_enqueue,
	.dequeue = pias_qdisc_dequeue,
	.peek = pias_qdisc_peek,
	.drop = pias_qdisc_drop,
	.change = pias_qdisc_change,
	.dump = pias_qdisc_dump,
	.owner = THIS_MODULE,
};

static int __init pias_qdisc_module_init(void)
{
	if(pias_qdisc_params_init()<0)
		return -1;
	
	printk(KERN_INFO "sch_pias: start working\n");
	return register_qdisc(&pias_qdisc_ops);
}

static void __exit pias_qdisc_module_exit(void)
{
	pias_qdisc_params_exit();
	unregister_qdisc(&pias_qdisc_ops);
	printk(KERN_INFO "sch_pias: stop working\n");
}

module_init(pias_qdisc_module_init)
module_exit(pias_qdisc_module_exit)
MODULE_LICENSE("GPL");