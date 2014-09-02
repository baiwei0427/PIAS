#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/init.h>
#include <linux/types.h>
#include <linux/netfilter.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/inet.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <net/icmp.h>
#include <linux/netfilter_ipv4.h>
#include <linux/string.h>
#include <linux/time.h>  
#include <linux/fs.h>
#include <linux/random.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h> /* copy_from/to_user */
#include <asm/byteorder.h>
#include <linux/netlink.h>
#include <net/netlink.h>
#include <net/net_namespace.h>

#include "queue.h" 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BAI Wei wbaiab@cse.ust.hk");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Kernel module of Strict Priority Queuing (SP) with ECN");

//ECN marking threshold (bytes): 150KB in 10G network (we need to take TSO into consideration)
#define ECN_thresh 150000
//1 denotes we use per-port ECN while 0 denotes we use per-queue ECN
#define Port_ECN 1 
//Number of priority queues (levels)
#define Prio_level 2 
//Maximum number of packets in a priority queue
#define Queue_size 256
//10Gbps link=9412 Mbps goodput for iperf with LSO enabled
#define LINK_SPEED	9412	
//microsecond to nanosecond
#define US_TO_NS(x)	(x * 1E3L)
//millisecond to nanosecond
#define MS_TO_NS(x)	(x * 1E6L)

//Link rate 1Mbps=0.125MBit per second=125 KBit per second
static	unsigned long rate=LINK_SPEED*125000;
//Delay: we set 50us in 10G network because TSO sized (64KB) packet takes 51.2us to transmit at 10G network 
static unsigned long delay_in_us = 50L;
//Bucket size: 360KB in 10G
static unsigned long bucket=360000;
//Tokens in bucket
static unsigned long tokens=0;
//Global Lock (For queues)
static spinlock_t globalLock;
//High resolution timer
static struct hrtimer hr_timer;
//Old time value
static struct timeval tv_old;
//PacketQueue pointer
static struct PacketQueue **q=NULL;
//Hook for outgoing packets at LOCAL_OUT 
static struct nf_hook_ops nfho_outgoing;

//Function to calculate timer interval (microsecond)
static unsigned long time_of_interval(struct timeval tv_new,struct timeval tv_old)
{
	return (tv_new.tv_sec-tv_old.tv_sec)*1000000+(tv_new.tv_usec-tv_old.tv_usec);
}

//Get the sum of packets whose priorities are higher or equal to prio
static unsigned int queue_sum_size(struct PacketQueue **queues, unsigned int prio)
{
	unsigned int size=0;
	int i=0;
	for(i=0;i<=prio;i++)
	{
		size+=queues[i]->size;
	}
	return size;
}

//Get the sum of bytes whose priorities are higher or equal to prio
static unsigned int queue_sum_bytes(struct PacketQueue **queues, unsigned int prio)
{
	unsigned int bytes=0;
	int i=0;
	for(i=0;i<=prio;i++)
	{
		bytes+=queues[i]->bytes;
	}
	return bytes;
}

//Do ECN marking
//If succeed, return 1. Else, return 0
static unsigned int ECN_mark(struct sk_buff *skb)
{
	struct iphdr *ip_header=NULL;	//IP header structure
	unsigned int dscp_value=0;	//DSCP value
	unsigned int ecn_value=0;	//ECN value
	unsigned int tos_value=0;		//TOS value (Note that TOS is 4*DSCP+ECN)
	unsigned char* tmp=NULL;  //temporary variable

	//Get IP header
	ip_header=(struct iphdr *)skb_network_header(skb);
	
	//The packet is not a IP packet (e.g. ARP or others)
	if (!ip_header)
	{
		return 0;
	}
	
	//Make this packet writable
	if(!skb_make_writable(skb,sizeof(*ip_header)))
	{
		printk(KERN_INFO "Not writable\n");
		return 0;
	}
	//Get new IP header
	ip_header=(struct iphdr *)skb_network_header(skb);
	
	//Get DSCP and ECN value from ToS field of IP header
	dscp_value=(unsigned int)ip_header->tos>>2;
	ecn_value=(unsigned int)(ip_header->tos&0x03);
	//If the packet is non-ECT (00) or CE (11), no need to modify this packet
	if(ecn_value==0||ecn_value==3)
	{
		return 1;
	}

	tos_value=4*dscp_value+3;
	//printk(KERN_INFO "New ToS value should be %u\n", tos_value);
	tmp=(unsigned char*)&tos_value;
	//Modify TOS of IP header
	ip_header->tos=*tmp;
	//Recalculate IP checksum
	ip_header->check=0;
	ip_header->check=ip_fast_csum(ip_header,ip_header->ihl);      
	return 1;
}

//Input: DSCP value
//Output: priority queue index (0 has the highest priority)
static unsigned int classify(unsigned int dscp)
{
	if(dscp==1)
		return 0;
	else
		return 1;
}  

//Deal with outgoing packets
static unsigned int hook_func_out(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct iphdr *ip_header=NULL;					//IP  header structure
	struct tcphdr *tcp_header=NULL;				//TCP header structure
	unsigned short int src_port=0;					//TCP source port
	unsigned short int dst_port=0;					//TCP destination port
	unsigned long flags;         	 							//variable for save current states of irq
	unsigned int priority;										//Priority for this packet (0 is the highest priority)
	int result;
	
	//Get IP header
	ip_header=(struct iphdr *)skb_network_header(skb);
	//The packet is not an IP packet (e.g. ARP or others), return NF_ACCEPT
	if (!ip_header)
	{
		return NF_ACCEPT;
	}
	
	//TCP or ICMP
	if(ip_header->protocol==IPPROTO_TCP||ip_header->protocol==IPPROTO_ICMP) 
	{
		//Get TCP header information
		if(ip_header->protocol==IPPROTO_TCP)
		{
			tcp_header = (struct tcphdr *)((__u32 *)ip_header+ ip_header->ihl);
			src_port=ntohs(tcp_header->source);
			dst_port=ntohs(tcp_header->dest);
		}
		//We only deal with our experiment traffic: TCP source port or destination port = 5001 / ICMP ping packets
		if(src_port==5001||dst_port==5001||ip_header->protocol==IPPROTO_ICMP)
		{
			priority=classify((unsigned int)ip_header->tos>>2);
			
			//For test
			if(ip_header->protocol==IPPROTO_ICMP)
				printk(KERN_INFO "Priority %u\n", priority);
				
			//If no packet in current queues has higher priority than this incoming packet and there are enough tokens  
			if(queue_sum_size(q,priority)==0&&tokens>=skb->len)
			{
				spin_lock_irqsave(&globalLock,flags);
				//In our testbed, 52 bytes= IP header (20 bytes)+ TCP header (20 bytes)+ TCP options (12 bytes)
				if(skb->len>52)
				{	
					tokens=tokens-(skb->len-52);
				}
				else	 //May be ICMP
				{
					tokens=tokens-skb->len;
				}
				spin_unlock_irqrestore(&globalLock,flags);
				return NF_ACCEPT;
			}
			else 
			{
				spin_lock_irqsave(&globalLock,flags);
				//Per-port ECN marking
				if(Port_ECN==1&&queue_sum_bytes(q,Prio_level-1)>ECN_thresh)
				{
					printk(KERN_INFO "ECN marking\n");
					ECN_mark(skb);
				}
				//Per-queue ECN marking
				else if(Port_ECN==0&&q[priority]->bytes>ECN_thresh)
				{
					printk(KERN_INFO "ECN marking\n");
					ECN_mark(skb);
				}
				result=Enqueue_PacketQueue(q[priority],skb,okfn);
				spin_unlock_irqrestore(&globalLock,flags);
				
				//Enqueue succeeds
				if(result==1)
				{
					return NF_STOLEN;
				}
				else
				{
					return NF_DROP;
				}
			}
		}
	}
	return NF_ACCEPT;
}

static enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer )
{
	struct timeval tv;	//timeval structure used by do_gettimeofday
	unsigned long time_interval; //time interval (microsecond)
	ktime_t interval,now;  
	unsigned long flags;	//variable for save current states of irq
	unsigned int len;
	int i;

	//Get current time
	do_gettimeofday(&tv);
	//Calculate interval	
	time_interval=time_of_interval(tv,tv_old);
	//Reset tv_old
	tv_old=tv;
	//Update tokens	
	tokens=tokens+(time_interval*rate)/1000000;
	
	spin_lock_irqsave(&globalLock,flags);
	//Schedule packets from multiple queues based on priority 
	for(i=0;i<Prio_level;i++)
	{
		while(1)
		{
			//There are still some packets in queues[i]
			if(q[i]->size>0)
			{	
				//In our testbed, 52 bytes= IP header (20 bytes)+ TCP header (20 bytes)+ TCP options (12 bytes)
				if(q[i]->packets[q[i]->head].skb->len>52)
					len=q[i]->packets[q[i]->head].skb->len-52;
				else
					len=q[i]->packets[q[i]->head].skb->len;
				if(len<=tokens)
				{
					//Reduce tokens
					tokens=tokens-len;
					//Dequeue packets
					Dequeue_PacketQueue(q[i]);
				}
				//No enough tokens
				else
				{
					break;
				}
			}
			//No packet in this queue
			else
			{
				break;
			}
		}
	}
	
	//Tokens no larger then bucket size if there are no packets to transmit
	if(tokens>=bucket&&queue_sum_size(q,Prio_level-1)==0)
		tokens=bucket;
	spin_unlock_irqrestore(&globalLock,flags);

	interval = ktime_set(0, US_TO_NS(delay_in_us));
	now = ktime_get();
	hrtimer_forward(timer,now,interval);
	return HRTIMER_RESTART;
}


//Called when module loaded using 'insmod'
int init_module()
{
	int i=0;
	ktime_t ktime;
	
	//Initialize multiple priority queues q[0] has the highest priority level by default
	q=vmalloc(Prio_level*sizeof(struct PacketQueue *));
	for(i=0;i<Prio_level;i++)
	{
		q[i]=vmalloc(sizeof(struct PacketQueue));
		//Vmalloc error
		if(Init_PacketQueue(q[i],Queue_size)==0)
		{
			printk(KERN_INFO "PacketQueue initialization error\n");
			return 0;
		}
	}
	//Initialize Global Lock
	spin_lock_init(&globalLock);
	
	//NF_LOCAL_OUT Hook
	nfho_outgoing.hook = hook_func_out;								//function to call when conditions below met
	nfho_outgoing.hooknum = NF_INET_POST_ROUTING;//called in NF_IP_POST_ROUTING
	nfho_outgoing.pf = PF_INET;												//IPv4 packets
	nfho_outgoing.priority = NF_IP_PRI_FIRST;						//set to highest priority over all other hook functions
	nf_register_hook(&nfho_outgoing);									//register hook
	
	//Initialize tokens
	tokens=bucket;
	
	//Initialize timer
	do_gettimeofday(&tv_old);
	ktime = ktime_set( 0, US_TO_NS(delay_in_us) );
	hrtimer_init( &hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	hr_timer.function = &my_hrtimer_callback;
	hrtimer_start( &hr_timer, ktime, HRTIMER_MODE_REL );
	
	printk(KERN_INFO "Start SP kernel module\n");
	return 0;
}

//Called when module unloaded using 'rmmod'
void cleanup_module()
{
	int ret, i;
	ret = hrtimer_cancel( &hr_timer );
	if (ret) 
	{
		printk("The timer is still in use...\n");
	}
	
	//Unregister the hook
	nf_unregister_hook(&nfho_outgoing);   
	
	for(i=0;i<Prio_level;i++)
	{
		//Vfree packets in PacketQueue q[i]
		Free_PacketQueue(q[i]);
		//Vfree q[i]
		vfree(q[i]);
	}
	vfree(q);
	
	printk(KERN_INFO "Stop SP kernel module\n");
}
