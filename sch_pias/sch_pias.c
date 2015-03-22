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

#define PIAS_MAX_PRIO 2

#define PIAS_MAX_LEN_BYTES 1048576	//1MB should be enough

#define PIAS_ECN_THRESH_BYTES 30720	//Per-port ECN marking threshold 

#define PIAS_RATE_BPS 940000000 //940Mbps

#define PIAS_BUCKET_NS 4000000 //4ms	

#define PIAS_MAX_PACKET_BYTES 65536	//64KB
#define PIAS_MIN_PACKET_BYTES 64	

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
	u64 bucket;	/* Token bucket depth/rate in nanoseconds */
	
/* Variables */
	u64 tokens;	/* Tokens in nanoseconds */ 
	u32 sum_len;	/* The sum of lengh og all queues in bytes */
	u32 max_len; 	/* Maximum value of sum_len*/
	struct Qdisc *sch; 
	s64	time_ns;	/* Time check-point */ 
	struct qdisc_watchdog watchdog;	/* Watchdog timer */
};

/* We use this function to account for the true number of bytes sent on wire.*/
static inline unsigned int skb_size(struct sk_buff *skb)
{
	return max_t(unsigned int, skb->len,PIAS_MIN_PACKET_BYTES);
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


/* Length to Time, convert length in bytes to time in ns
 * to determinate the expiry time of watchdog timer
 */
/*static u64 pias_qdisc_l2t(u64 rate_in_bps, u64 len_in_bytes)
{
	return len_in_bytes*8*NSEC_PER_SEC/rate_in_bps;
}*/

/* Time to Length, convert time in ns to length in bytes
 * to determinate how many bytes can be sent in given time.
 */
/*static u64 pias_qdisc_t2l(u64 rate_in_bps, u64 time_in_ns)
{
	u64 result=rate_in_bps/8*time_in_ns/NSEC_PER_SEC;
	//printk(KERN_INFO "sch_pias: time interval is %llu us, new tokens is %llu bytes\n", time_in_ns/1000,result);
	return result;
}*/
			
static struct Qdisc * pias_qdisc_classify(struct sk_buff *skb, struct Qdisc *sch)
{
	struct pias_sched_data *q = qdisc_priv(sch);	
	int i;
	
	if(q->queues==NULL)
		return NULL;
	
	/* Return the highest priority queue */
	for(i=0;i<PIAS_MAX_PRIO && q->queues[i]; i++)
		return q->queues[i];
	
	return NULL;
}

/* Start watchdog timer when Qdisc is throttled */
/*static inline void pias_qdisc_watchdog(struct pias_sched_data *q, u64 dt_ns)
{
	if(!hrtimer_active(&q->timer)) 
	{
		hrtimer_start(&q->timer, ktime_set(0, dt_ns),HRTIMER_MODE_REL);
	}
}

enum hrtimer_restart pias_qdisc_timeout(struct hrtimer *timer) 
{
	struct pias_sched_data *q = container_of(timer, struct pias_sched_data, timer);
	qdisc_unthrottled(q->sch);
	printk(KERN_INFO "sch_pias: rescheduled by hrtimer\n");
	//__netif_schedule(qdisc_root(q->sch));
	return HRTIMER_NORESTART;
}*/

static struct sk_buff* pias_qdisc_dequeue_peeked(struct Qdisc *sch)
{
	struct pias_sched_data *q = qdisc_priv(sch);
	struct Qdisc *qdisc;
	struct sk_buff *skb;
	int i;

	for(i=0;i<PIAS_MAX_PRIO && q->queues[i]; i++)
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

	for(i=0;i<PIAS_MAX_PRIO && q->queues[i]; i++)
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
	unsigned int len;
	u64 now=ktime_to_ns(ktime_get());
	s64 toks;
	
	skb = pias_qdisc_peek(sch);
	if(skb)
	{
		/*skb = pias_qdisc_dequeue_peeked(sch);
		if (unlikely(!skb))
				return NULL;
		q->sum_len-=skb_size(skb);
		printk(KERN_INFO "sch_pias: queue length = %u\n",q->sum_len);
		//qdisc_unthrottled(sch);
		return skb;*/
		toks=min_t(s64, now-q->time_ns, PIAS_BUCKET_NS);
		len=skb_size(skb);
		toks+=q->tokens;
		//Bucket 
		if(toks>PIAS_BUCKET_NS)
			toks=PIAS_BUCKET_NS;

		toks-=(s64)l2t_ns(&q->rate, len);
		//If we have enough tokens to release this packet
		if(toks>=0)
		{
			skb = pias_qdisc_dequeue_peeked(sch);		
			if (unlikely(!skb))
				return NULL;
			q->time_ns = now;
			q->tokens = toks;
			q->sum_len-=len;
			sch->q.qlen--;
			printk(KERN_INFO "sch_pias: dequeue a packet with len=%u\n",len);
			//printk(KERN_INFO "sch_pias: sum_len2 = %u\n", q->sum_len);
			//sch->q.qlen--;			
			qdisc_unthrottled(sch);
			qdisc_bstats_update(sch, skb);
			return skb;
		}
		else
		{
			//We use now+t due to absolute mode of hrtimer ((HRTIMER_MODE_ABS) ) 
			qdisc_watchdog_schedule_ns(&q->watchdog,now+(u64)(-toks),true);
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
		printk(KERN_INFO "sch_pias: packet drop\n");
		qdisc_qstats_drop(sch);
		kfree_skb(skb);
		return NET_XMIT_DROP;
	}
	else
	{
		//ECN marking
		if(q->sum_len+len>PIAS_ECN_THRESH_BYTES)
		{
			printk(KERN_INFO "sch_pias: ECN marking\n");
			pias_qdisc_ecn(skb);
		}
		
		ret=qdisc_enqueue(skb, qdisc);
		if(ret == NET_XMIT_SUCCESS) 
		{
			sch->q.qlen++;
			q->sum_len+=len;
			printk(KERN_INFO "sch_pias: queue length = %u\n",q->sum_len);
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

/* We don't use TC netlink interfaces */
static int pias_qdisc_change(struct Qdisc *sch, struct nlattr *opt)
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
		for(i = 0; i < PIAS_MAX_PRIO && q->queues[i]; i++)
			qdisc_destroy(q->queues[i]);

		kfree(q->queues);
		q->queues = NULL;
	}
	qdisc_watchdog_cancel(&q->watchdog);
}

/* Initialize Qdisc */
static int pias_qdisc_init(struct Qdisc *sch, struct nlattr *opt)
{
	int i;
	struct pias_sched_data *q = qdisc_priv(sch);
	struct Qdisc *child;
	
	if(sch->parent != TC_H_ROOT)
		return -EOPNOTSUPP;
	
	q->queues=kcalloc(PIAS_MAX_PRIO, sizeof(struct Qdisc),GFP_KERNEL);
	if(q->queues == NULL)
		return -ENOMEM;
	
	q->tokens = 0;
	q->time_ns = ktime_to_ns(ktime_get());
	q->rate.rate_bps = PIAS_RATE_BPS;
	pias_qdisc_precompute_ratedata(&q->rate);
	q->sum_len=0;
	q->max_len = PIAS_MAX_LEN_BYTES;	
	q->sch = sch;
	qdisc_watchdog_init(&q->watchdog, sch);
	
	for(i=0;i<PIAS_MAX_PRIO;i++)
	{
		//bfifo is in bytes 
		child=fifo_create_dflt(sch, &bfifo_qdisc_ops, PIAS_MAX_LEN_BYTES);
		if(child!=NULL)
			q->queues[i]=child;
		else
			goto err;
	}
	return 0;
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
	printk(KERN_INFO "sch_pias: start working\n");
	return register_qdisc(&pias_qdisc_ops);
}

static void __exit pias_qdisc_module_exit(void)
{
	unregister_qdisc(&pias_qdisc_ops);
	printk(KERN_INFO "sch_pias: stop working\n");
}

module_init(pias_qdisc_module_init)
module_exit(pias_qdisc_module_exit)
MODULE_LICENSE("GPL");