#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <net/tcp.h>
#include <linux/types.h>
#include <linux/ktime.h>
#include <net/checksum.h>
#include <linux/netfilter_ipv4.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>

#include "flow.h"
#include "network.h"
#include "params.h"

/* FlowTable */
static struct PIAS_Flow_Table ft;
/* The hook for outgoing packets */
static struct nf_hook_ops pias_nf_hook_out;
/* The hook for incoming packets */
static struct nf_hook_ops pias_nf_hook_in;

/*
 * The following two functions are related to param_table_operation
 * To clear flow table: echo -n clear > /sys/module/pias/parameters/param_table_operation
 * To print flow table: echo -n print > /sys/module/pias/parameters/param_table_operation
 */
static int pias_set_operation(const char *val, struct kernel_param *kp);
static int pias_noget(const char *val, struct kernel_param *kp);
module_param_call(param_table_operation, pias_set_operation, pias_noget, NULL, S_IWUSR); //Write permission by owner

/* param_dev: NIC to operate PIAS */
static char *param_dev = NULL;
MODULE_PARM_DESC(param_dev, "Interface to operate PIAS");
module_param(param_dev, charp, 0);

static int pias_set_operation(const char *val, struct kernel_param *kp)
{
	//For debug
	printk(KERN_INFO "PIAS: param_table_operation is set\n");
	//Clear flow table
	if (strncmp(val, "clear", 5) == 0)
		PIAS_Clear_Table(&ft);
	//Print flow table
	else if (strncmp(val, "print", 5) == 0)
		PIAS_Print_Table(&ft);
	else
		printk(KERN_INFO "PIAS: unrecognized flow table operation\n");

	return 0;
}

static int pias_noget(const char *val, struct kernel_param *kp)
{
	return 0;
}

/* Hook function for outgoing packets */
static unsigned int pias_hook_func_out(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct iphdr *iph = NULL;  //IP  header structure
	struct tcphdr *tcph = NULL;    //TCP header structure
	struct PIAS_Flow f;    //PIAS flow structure
    struct PIAS_Flow *ptr = NULL;   //pointer to PIAS flow structure
	unsigned long flags;   //variable for save current states of irq
	u8 dscp = 0;   //DSCP value
	u16 payload_len = 0;	//TCP payload length
	u32 seq = 0;	//TCP sequence number
	u32 result = 0;	//Delete_Table return result
    s64 idle_time = 0;
	ktime_t now = ktime_get();	//get current time

	if (!out)
		return NF_ACCEPT;

	if (strncmp(out->name, param_dev, IFNAMSIZ) != 0)
		return NF_ACCEPT;

	//Get IP header
	iph = ip_hdr(skb);
	//The packet is not an IP packet (e.g. ARP or others), return NF_ACCEPT
	if (unlikely(!iph))
		return NF_ACCEPT;

	if (iph->protocol == IPPROTO_TCP)
	{
		tcph = (struct tcphdr *)((__u32 *)iph + iph->ihl);
		PIAS_Init_Flow(&f);
		f.local_ip = iph->saddr;
		f.remote_ip = iph->daddr;
		f.local_port = (u16)ntohs(tcph->source);
		f.remote_port = (u16)ntohs(tcph->dest);

		//TCP SYN packet, a new  connection
		if(tcph->syn)
		{
			f.info.last_seq = ntohl(tcph->seq);
			f.info.last_update_time = now;
			//A new Flow entry should be inserted into FlowTable
			if (!PIAS_Insert_Table(&ft,&f,GFP_ATOMIC))
				printk(KERN_INFO "PIAS: insert fail\n");

			dscp = pias_priority(0);
		}
		//TCP FIN/RST packets, connection will be closed
		else if (tcph->fin || tcph->rst)
		{
			result = PIAS_Delete_Table(&ft, &f);
			if (result == 0)
				printk(KERN_INFO "PIAS: delete fail\n");

			dscp = pias_priority(result);
		}
		else
		{
			//TCP payload length=Total IP length - IP header length-TCP header length
			payload_len = ntohs(iph->tot_len) - (iph->ihl << 2) - (tcph->doff << 2);
			seq = (u32)ntohl(tcph->seq);
			//Get the sequence number of the last payload byte
			if (payload_len >= 1)
				seq = seq + payload_len - 1;

			//Update existing Flow entry's information
			ptr = PIAS_Search_Table(&ft, &f);
			if (ptr)
			{
                spin_lock_irqsave(&(ptr->lock), flags);
                idle_time = ktime_us_delta(now, ptr->info.last_update_time);
				//A new TCP packet
				if (pias_is_seq_larger(seq, ptr->info.last_seq))
				{
					//Update sequence number
					ptr->info.last_seq = seq;
					//Update bytes sent
                    if (idle_time >= PIAS_IDLE_TIME)
                    {
                        if (PIAS_DEBUG_MODE == 1)
                            printk(KERN_INFO "A new message is dected after %lld us idle time\n", idle_time);
                        ptr->info.bytes_sent = 0;
                    }
                    else
				        ptr->info.bytes_sent += payload_len;
				}
				//TCP timeout?
				else if (idle_time >= PIAS_RTO_MIN && pias_is_seq_larger(ptr->info.last_seq, ptr->info.last_ack))
				{
                    ptr->info.last_timeout_seq = seq;
					//A consecutive' TCP timeout
					if (PIAS_TIMEOUT_THRESH == 1 || (PIAS_TIMEOUT_THRESH >= 2 && (pias_seq_gap(seq, ptr->info.last_timeout_seq) <= PIAS_TIMEOUT_SEQ_GAP)))
					{
                        ptr->info.timeouts++;
						//If the number of consecutive TCP timeouts is larger than the threshold
						if (ptr->info.timeouts >= PIAS_TIMEOUT_THRESH)
						{
                            ptr->info.timeouts = 0;
							//Reset bytes sent to zero (highest priority) when aging is enabled
                            if (PIAS_ENABLE_AGING == 1)
                                ptr->info.bytes_sent = 0;
                            if (PIAS_DEBUG_MODE == 1)
                                printk(KERN_INFO "%d consecutive TCP timeouts are detected!\n", PIAS_TIMEOUT_THRESH);
						}
					}
                    //Not a consecutive TCP timeout
                    else
                        ptr->info.timeouts = 1;
				}
				//Update last_update_time of this flow
				ptr->info.last_update_time = now;
                spin_unlock_irqrestore(&(ptr->lock), flags);
				//Calculate priority of this flow based on bytes sent
				dscp = pias_priority(ptr->info.bytes_sent);
			}
			//No such Flow entry. Maybe last few packets of the flow. We need to accelerate flow completion.
			else
				dscp = pias_priority(0);
		}
		//Modify DSCP and make the packet ECT
		pias_enable_ecn_dscp(skb, dscp);
	}
	return NF_ACCEPT;
}

/* Hook function for incoming packets */
static unsigned int pias_hook_func_in(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct iphdr *iph = NULL;  //IP  header structure
	struct tcphdr *tcph = NULL;    //TCP header structure
	struct PIAS_Flow f;    //Flow structure
	struct PIAS_Flow* ptr = NULL;  //Pointer to structure Information
	u32 ack;   //TCP ACK number
    u16 payload_len = 0;    //TCP payload length
	unsigned long flags;   //variable for save current states of irq

	if (!in)
		return NF_ACCEPT;

	if (strncmp(in->name, param_dev, IFNAMSIZ) != 0)
		return NF_ACCEPT;

	//Get IP header
	iph = ip_hdr(skb);
	//The packet is not an IP packet (e.g. ARP or others), return NF_ACCEPT
	if (unlikely(!iph))
		return NF_ACCEPT;

	if (iph->protocol == IPPROTO_TCP)
	{
		tcph = (struct tcphdr *)((__u32 *)iph + iph->ihl);
		if (tcph->ack)
		{
            //TCP payload length=Total IP length - IP header length-TCP header length
            payload_len = ntohs(iph->tot_len) - (iph->ihl << 2) - (tcph->doff << 2);

			PIAS_Init_Flow(&f);
			f.local_ip = iph->daddr;
			f.remote_ip = iph->saddr;
			f.local_port = (u16)ntohs(tcph->dest);
			f.remote_port = (u16)ntohs(tcph->source);
			//Update existing Flow entry's information
			ptr = PIAS_Search_Table(&ft, &f);
			if (ptr)
			{
                spin_lock_irqsave(&(ptr->lock), flags);
				ack = (u32)ntohl(tcph->ack_seq);
				if (pias_is_seq_larger(ack, ptr->info.last_ack))
                    ptr->info.last_ack = ack;
                spin_unlock_irqrestore(&(ptr->lock), flags);
			}
		}
	}
	return NF_ACCEPT;
}

static int pias_module_init(void)
{
	int i = 0;
    //Get interface
    if (param_dev==NULL)
    {
        param_dev = "eth1\0";
        printk(KERN_INFO "PIAS: not specify network interface (%s by default)\n", param_dev);
	}
    // trim
	for (i = 0; i < 32 && param_dev[i] != '\0'; i++)
    {
		if(param_dev[i] == '\n')
        {
			param_dev[i] = '\0';
			break;
		}
	}

	if (PIAS_Params_Init() < 0)
		return -1;

	//Initialize FlowTable
	PIAS_Init_Table(&ft);

	//Register outgoing hook
	pias_nf_hook_out.hook = pias_hook_func_out;
	pias_nf_hook_out.hooknum = NF_INET_POST_ROUTING;
	pias_nf_hook_out.pf = PF_INET;
	pias_nf_hook_out.priority = NF_IP_PRI_FIRST;
	nf_register_hook(&pias_nf_hook_out);

	//Register incoming hook
	pias_nf_hook_in.hook = pias_hook_func_in;
	pias_nf_hook_in.hooknum = NF_INET_PRE_ROUTING;
	pias_nf_hook_in.pf = PF_INET;
	pias_nf_hook_in.priority = NF_IP_PRI_FIRST;
	nf_register_hook(&pias_nf_hook_in);

	printk(KERN_INFO "PIAS: start on %s\n", param_dev);
	return 0;
}

static void pias_module_exit(void)
{
	//Unregister two hooks
	nf_unregister_hook(&pias_nf_hook_out);
	nf_unregister_hook(&pias_nf_hook_in);

	//Clear flow table
	PIAS_Exit_Table(&ft);
	PIAS_Params_Exit();

	printk(KERN_INFO "PIAS: stop working\n");
}

module_init(pias_module_init);
module_exit(pias_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BAI Wei baiwei0427@gmail.com");
MODULE_VERSION("1.3");
MODULE_DESCRIPTION("Linux kernel module for PIAS (Practical Information-Agnostic flow Scheduling)");
