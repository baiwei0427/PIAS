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

#define PIAS_RATE_BPS 980000000

#define PIAS_MAX_PACKET_BYTES 65536
#define PIAS_MIN_PACKET_BYTES 64


struct pias_sched_data 
{
/* Parameters */
	struct Qdisc **queues; /* Priority queues where queues[0] has the highest priority*/
	u64 rate_bps;	/* Rate in bps */

/* Variables */
	u64 tokens;	/* Tokens in bytes */ 
	u32 sum_len;	/* The sum of lengh og all queues in bytes */
	u32 max_len; 	/* Maximum value of sum_len*/
	//spinlock_t spinlock;
	struct Qdisc *sch; 
	s64	time_us;	/* Time check-point */ 
	struct hrtimer timer;	/* Watchdog timer */
}

/* We use this function to account for the true number of bytes sent on wire.*/
static inline unsigned int skb_size(struct sk_buff *skb)
{
	return max(skb->len,PIAS_MIN_PACKET_BYTES);
}

static inline void pias_qdisc_ecn(struct sk_buff *skb)
{
	if(skb_make_writable(skb,sizeof(struct iphdr))&&ip_hdr(skb))
		IP_ECN_set_ce(ip_hdr(skb));
}

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
static inline void pias_qdisc_watchdog(struct pias_sched_data *q, u64 dt_ns)
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
	return HRTIMER_NORESTART;
}

static int pias_qdisc_enqueue(struct sk_buff *skb, struct Qdisc *sch)
{
	struct Qdisc *qdisc;
	unsigned int len=skb_size(skb);
	int ret;
	struct pias_sched_data *q = qdisc_priv(sch);	
	
	qdisc=pias_qdisc_classify(skb,sch);
	
	//No appropriate priority queue or queue limit is reached
	if(qdisc==NULL||sum_len+len>q->max_len)
	{
		qdisc_qstats_drop(sch);
		//kfree_skb(skb);
		return NET_XMIT_DROP;
	}
	else
	{
		//ECN marking
		if(sum_len+len>PIAS_ECN_THRESH_BYTES)
			pias_qdisc_ecn(skb);
		
		ret=qdisc_enqueue(skb, qdisc);
		if(ret == NET_XMIT_SUCCESS) 
		{
			sch->q.qlen++;
			sum_len+=len;
			return NET_XMIT_SUCCESS;
		}
		else
		{
			if (net_xmit_drop_count(ret))
				qdisc_qstats_drop(sch);
			return ret;
		}
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
	hrtimer_cancel(&q->timer);
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
	if(q->queuess == NULL)
		return -ENOMEM;
	
	//spin_lock_init(&q->spinlock);
	q->time_us = ktime_to_ns(ktime_get());
	q->rate_bps = PIAS_RATE_BPS;
	q->sum_len=0;
	q->max_len = PIAS_MAX_LEN_BYTES;	
	q->sch = sch;
	hrtimer_init(&q->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	q->timer.function = pias_qdisc_timeout;
	
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