#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <net/netlink.h>
#include <linux/pkt_sched.h>
#include <net/sch_generic.h>
#include <net/pkt_sched.h>

#define PIAS_MAX_PRIO 2

#define PIAS_MAX_LEN_BYTES 1048576 //1MB should be enough

#define PIAS_RATE_BPS 980000000

#define PIAS_MAX_PACKET_BYTES 65536
#define PIAS_MIN_PACKET_BYTES 64


struct pias_sched_data 
{
/* Parameters */
	struct Qdisc **queues; /* Priority queues */
	u64 rate_bps;	/* Rate in bps */

/* Variables */
	u64 tokens;	/* Tokens in bytes */ 
	u32 max_len; 	/* Maximum length of all queues in bytes */
	spinlock_t spinlock;
	struct Qdisc *sch; 
	s64	time_us;	/* Time check-point */ 
	struct hrtimer timer;	/* Watchdog timer */
}

/*
 * Borrow somethings from skb_gso_mac_seglen in sch_tbf.c 
 * We use this function to account for the true number of bytes sent on wire.
 */
static inline int skb_size(struct sk_buff *skb)
{
	return max(skb->len,PIAS_MIN_PACKET_BYTES);
}

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
	
	spin_lock_init(&q->spinlock);
	q->time_us = ktime_to_ns(ktime_get());
	q->rate_bps = PIAS_RATE_BPS;
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
	.attach = pias_qdisc_attach,
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