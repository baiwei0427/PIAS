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

#include "hash.h" 
#include "prio.h"	
#include "network.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BAI Wei wbaiab@cse.ust.hk");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Kernel module of PIAS (Practical Information-Agnostic Flow Scheduling)");

//FlowTable
static struct FlowTable ft;
//Global Lock (For FlowTable)
static spinlock_t globalLock;
 
//Hook for outgoing packets at LOCAL_OUT 
static struct nf_hook_ops nfho_outgoing;
//Hook for outgoing packets at LOCAL_IN
static struct nf_hook_ops nfho_incoming;

//Deal with outgoing packets
static unsigned int hook_func_out(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct iphdr *ip_header=NULL;					//IP  header structure
	struct tcphdr *tcp_header=NULL;				//TCP header structure
	struct Flow f;													//Flow structure
	struct Information* info_pointer=NULL;	//Pointer to structure Information
	unsigned short int src_port;							//TCP source port
	unsigned short int dst_port;						//TCP destination port
	unsigned long flags;         	 							//variable for save current states of irq
	unsigned int dscp;											//DSCP value
	unsigned int payload_len;								//TCP payload length
	unsigned int result;										//Delete_Table return result
	
	//Get IP header
	ip_header=(struct iphdr *)skb_network_header(skb);
	//The packet is not an IP packet (e.g. ARP or others), return NF_ACCEPT
	if (!ip_header)
	{
		return NF_ACCEPT;
	}
	
	//TCP
	if(ip_header->protocol==IPPROTO_TCP) 
	{
		tcp_header = (struct tcphdr *)((__u32 *)ip_header+ ip_header->ihl);
		src_port=ntohs(tcp_header->source);
		dst_port=ntohs(tcp_header->dest);
		//We only deal with our experiment traffic: TCP source port or destination port = 5001
		if(src_port==5001||dst_port==5001)
		{
			Init_Flow(&f);
			f.local_ip=ip_header->saddr;
			f.remote_ip=ip_header->daddr;
			f.local_port=src_port;
			f.remote_port=dst_port;
			
			if(tcp_header->syn) //TCP SYN packet, a new  initialized outgoing connection
			{
				//A new Flow entry should be inserted into FlowTable
				spin_lock_irqsave(&globalLock,flags);
				if(Insert_Table(&ft,&f)==0)
				{
					printk(KERN_INFO "Insert fail\n");
				}
				spin_unlock_irqrestore(&globalLock,flags);
				//Give this packet highest priority because bytes sent=0 when the flow is initialized
				dscp=priority(0);
				modify_dscp(skb,dscp);
			}
			else if(tcp_header->fin||tcp_header->rst)  //TCP FIN/RST packets, connection will be closed 
			{
				//An existing Flow entry should be deleted from FlowTable. 
				spin_lock_irqsave(&globalLock,flags);
				//Result=bytes sent of this flow
				result=Delete_Table(&ft,&f);
				if(result==0)
				{
					printk(KERN_INFO "Delete fail\n");
				}
				/*else
				{
					printk(KERN_INFO "Delete succeed\n");
				}*/
				spin_unlock_irqrestore(&globalLock,flags);
				dscp=priority(result);
				modify_dscp(skb,dscp);
			}
			else
			{
				//Update existing Flow entry's information
				spin_lock_irqsave(&globalLock,flags);
				info_pointer=Search_Table(&ft,&f);
				spin_unlock_irqrestore(&globalLock,flags);
				if(info_pointer!=NULL)
				{
					//TCP payload length=Total length - IP header length-TCP header length
					payload_len=skb->len-(ip_header->ihl<<2)-(unsigned int)(tcp_header->doff*4);
					//payload length>0 and info_pointer->send_data will not exceed the maximum value of unsigned int  (4,294,967,295)
					if(payload_len>0 && payload_len+info_pointer->send_data<4294967295)
					{
						info_pointer->send_data+=payload_len;
						info_pointer->last_update_time=get_tsval();
					}
					dscp=priority(info_pointer->send_data);
					modify_dscp(skb,dscp);
				}
				//No such Flow entry, last few packets. We need to accelerate flow completion.
				else
				{
					dscp=priority(0);
					modify_dscp(skb,dscp);
				}
			}
		}
	}
	return NF_ACCEPT;
}

//Deal with incoming packets
/*static unsigned int hook_func_in(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	
	struct iphdr *ip_header=NULL;					//IP  header structure
	struct tcphdr *tcp_header=NULL;				//TCP header structure
	struct Flow f;													//Flow structure
	struct Information* info_pointer=NULL;	//Pointer to structure Information
	unsigned short int src_port;							//TCP source port
	unsigned short int dst_port;						//TCP destination port
	unsigned long flags;         	 							//variable for save current states of irq
	unsigned int payload_len;								//TCP payload length
	
	//Get IP header
	ip_header=(struct iphdr *)skb_network_header(skb);
	//The packet is not an IP packet (e.g. ARP or others), return NF_ACCEPT
	if (!ip_header)
	{
		return NF_ACCEPT;
	}
	
	//TCP
	if(ip_header->protocol==IPPROTO_TCP) 
	{
		tcp_header = (struct tcphdr *)((__u32 *)ip_header+ ip_header->ihl);
		src_port=ntohs(tcp_header->source);
		dst_port=ntohs(tcp_header->dest);
		//We only deal with our experiment traffic: TCP source port or destination port = 5001
		if(src_port==5001||dst_port==5001)
		{
			//Update existing Flow entry's information. Note that direction has been changed.
			Init_Flow(&f);
			f.local_ip=ip_header->daddr;
			f.remote_ip=ip_header->saddr;
			f.local_port=dst_port;
			f.remote_port=src_port;
			//TCP SYN packet, a new  initialized outgoing connection
			if(tcp_header->syn&&!tcp_header->ack) 
			{
				//A new Flow entry should be inserted into FlowTable
				spin_lock_irqsave(&globalLock,flags);
				if(Insert_Table(&ft,&f)==0)
				{
					printk(KERN_INFO "Insert fail\n");
				}
				else
				{
					printk(KERN_INFO "Insert succeed\n");
				}
				spin_unlock_irqrestore(&globalLock,flags);
			}
			else
			{
				//spin_lock_irqsave(&globalLock,flags);
				info_pointer=Search_Table(&ft,&f);
				//spin_unlock_irqrestore(&globalLock,flags);
				if(info_pointer!=NULL)
				{
					//TCP payload length=Total length - IP header length-TCP header length
					payload_len=skb->len-(ip_header->ihl<<2)-(unsigned int)(tcp_header->doff*4);
					if(payload_len>0)
						info_pointer->last_update_time=get_tsval();
				}
			}
		}
	}
	return NF_ACCEPT;
}*/

//Called when module loaded using 'insmod'
int init_module()
{
	//Initialize Global Lock
	spin_lock_init(&globalLock);
	
	//Initialize FlowTable
	Init_Table(&ft);
		
	//NF_LOCAL_IN Hook
	//nfho_incoming.hook = hook_func_in;								//function to call when conditions below met
	//nfho_incoming.hooknum =  NF_INET_LOCAL_IN;			//called in NF_IP_LOCAL_IN
	//nfho_incoming.pf = PF_INET;												//IPv4 packets
	//nfho_incoming.priority = NF_IP_PRI_FIRST;					//set to highest priority over all other hook functions
	//nf_register_hook(&nfho_incoming);									//register hook*/
	
	//NF_LOCAL_OUT Hook
	nfho_outgoing.hook = hook_func_out;								//function to call when conditions below met
	nfho_outgoing.hooknum =  NF_INET_LOCAL_OUT;		//called in NF_IP_LOCAL_OUT
	nfho_outgoing.pf = PF_INET;												//IPv4 packets
	nfho_outgoing.priority = NF_IP_PRI_FIRST;						//set to highest priority over all other hook functions
	nf_register_hook(&nfho_outgoing);									//register hook
		
	printk(KERN_INFO "Start PIAS kernel module\n");
	return 0;
}

//Called when module unloaded using 'rmmod'
void cleanup_module()
{
	//Unregister two hooks
	nf_unregister_hook(&nfho_outgoing);  
	//nf_unregister_hook(&nfho_incoming);
	
	//Clear table
	 Empty_Table(&ft);

	printk(KERN_INFO "Stop PIAS kernel module\n");
}
