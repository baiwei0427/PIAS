#include <linux/netfilter.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <net/tcp.h>
#include <linux/ktime.h>
#include <linux/netfilter_ipv4.h>

#include "netfilter.h"
#include "flow.h"
#include "network.h"
#include "params.h"

/* Flow Table */
extern struct PIAS_Flow_Table ft;
/* NIC device name */
extern char *param_dev;
/* TCP port */
extern int param_port;

/* The Netfilter hook for outgoing packets */
static struct nf_hook_ops pias_nf_hook_out;
/* The Netfilter hook for incoming packets */
static struct nf_hook_ops pias_nf_hook_in;

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

    if (param_dev && strncmp(out->name, param_dev, IFNAMSIZ) != 0)
        return NF_ACCEPT;

	//Get IP header
    iph = ip_hdr(skb);
	//The packet is not an IP packet (e.g. ARP or others), return NF_ACCEPT
	if (unlikely(!iph))
		return NF_ACCEPT;

	if (iph->protocol == IPPROTO_TCP)
	{
		tcph = (struct tcphdr *)((__u32 *)iph + iph->ihl);

		if (param_port != 0 && ntohs(tcph->source) != param_port && ntohs(tcph->dest) != param_port)
			return NF_ACCEPT;

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

	if (param_dev && strncmp(in->name, param_dev, IFNAMSIZ) != 0)
		return NF_ACCEPT;

	//Get IP header
	iph = ip_hdr(skb);
	//The packet is not an IP packet (e.g. ARP or others), return NF_ACCEPT
	if (unlikely(!iph))
		return NF_ACCEPT;

	if (iph->protocol == IPPROTO_TCP)
	{
		tcph = (struct tcphdr *)((__u32 *)iph + iph->ihl);

		if (param_port != 0 && ntohs(tcph->source) != param_port && ntohs(tcph->dest) != param_port)
			return NF_ACCEPT;

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

/* Install Netfilter hooks. Return true if it succeeds */
bool PIAS_Netfilter_Init(void)
{
    //Register outgoing Netfilter hook
    pias_nf_hook_out.hook = pias_hook_func_out;
    pias_nf_hook_out.hooknum = NF_INET_POST_ROUTING;
    pias_nf_hook_out.pf = PF_INET;
    pias_nf_hook_out.priority = NF_IP_PRI_FIRST;
    if (nf_register_hook(&pias_nf_hook_out))
    {
        printk(KERN_INFO "Cannot register Netfilter hook at NF_INET_POST_ROUTING\n");
        return false;
    }

    //Register incoming Netfilter hook
    pias_nf_hook_in.hook = pias_hook_func_in;
    pias_nf_hook_in.hooknum = NF_INET_PRE_ROUTING;
    pias_nf_hook_in.pf = PF_INET;
    pias_nf_hook_in.priority = NF_IP_PRI_FIRST;
    if (nf_register_hook(&pias_nf_hook_in))
    {
        printk(KERN_INFO "Cannot register Netfilter hook at NF_INET_PRE_ROUTING\n");
        return false;
    }

    return true;
}

/* Uninstall Netfilter hooks */
void PIAS_Netfilter_Exit(void)
{
    nf_unregister_hook(&pias_nf_hook_out);
    nf_unregister_hook(&pias_nf_hook_in);
}
