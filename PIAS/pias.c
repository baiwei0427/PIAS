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
MODULE_DESCRIPTION("Driver module of PIAS (Practical Information-Agnostic Flow Scheduling)");

//FlowTable
static struct FlowTable ft;
//Global Lock (For FlowTable)
static spinlock_t globalLock;

//Kernel socket (Netlink)
static struct sock *sk;
//Message Type
#define NLMSG_OUTPUT 0x11
//Max Payload Length 
#define MAX_PAYLOAD 1024
 
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
	struct Infomation* info_pointer=NULL;	//Pointer to structure Information
	
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
		//We only deal with our experiment traffic: TCP source port or destination port = 5001
		if(htons((unsigned short int) tcp_header->dest)==5001||htons((unsigned short int) tcp_header->source)==5001)
		{
			//TCP SYN packets, a new  initialized outgoing connection
			if(tcp_header->syn&&!tcp_header->ack)
			{
				
			}
			//TCP SYN/ACK packets, a new initialized incoming connection
			else if(tcp_header->syn&&tcp_header->ack)
			{
				
			}
			//TCP FIN/RST packets, connection will be closed 
			else if(tcp_header->fin||tcp_header->rst)
			{
				
			}
			else
			{
				
			}
		}
	}
	return NF_ACCEPT;
}

//Deal with incoming packets
static unsigned int hook_func_in(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	
}

//Called when module loaded using 'insmod'
int init_module()
{
	//Initialize FlowTable
	Init_Table(&ft);
	
	//Initialize Global Lock
	spin_lock_init(&globalLock);
	
	//NF_LOCAL_IN Hook
	nfho_incoming.hook = hook_func_in;								//function to call when conditions below met
	nfho_incoming.hooknum = NF_INET_LOCAL_IN;			//called in NF_IP_LOCAL_IN
	nfho_incoming.pf = PF_INET;												//IPv4 packets
	nfho_incoming.priority = NF_IP_PRI_FIRST;					//set to highest priority over all other hook functions
	nf_register_hook(&nfho_incoming);									//register hook
	
	//NF_LOCAL_OUT Hook
	nfho_outgoing.hook = hook_func_out;								//function to call when conditions below met
	nfho_outgoing.hooknum = NF_INET_LOCAL_OUT;		//called in NF_IP_LOCAL_OUT
	nfho_outgoing.pf = PF_INET;												//IPv4 packets
	nfho_outgoing.priority = NF_IP_PRI_FIRST;						//set to highest priority over all other hook functions
	nf_register_hook(&nfho_outgoing);									//register hook
	
	//Register Netlink 
    sk = netlink_kernel_create(&init_net,NETLINK_TEST,0,nl_custom_data_ready,NULL,THIS_MODULE);
	if (!sk) 
	{
        printk(KERN_INFO "Netlink create error!\n");
    }
	
	printk(KERN_INFO "Start PIAS kernel module\n");
	return 0;
}

//Called when module unloaded using 'rmmod'
void cleanup_module()
{
	netlink_kernel_release(sk);
	printk(KERN_INFO "Stop PIAS kernel module\n");
}
