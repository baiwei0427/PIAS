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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BAI Wei baiwei0427@gmail.com");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Linux kernel module for MCP");

char *param_dev=NULL;
MODULE_PARM_DESC(param_dev, "Interface to operate MCP");
module_param(param_dev, charp, 0);

static struct MCP_Flow_Table ft;

//The hook for outgoing packets at POSTROUTING
static struct nf_hook_ops MCP_nf_hook_out;
//The hook for incoming packets at PREROUTING
static struct nf_hook_ops MCP_nf_hook_in;

//POSTROUTING for outgoing packets
static unsigned int MCP_hook_func_out(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct iphdr *iph=NULL;	//IP  header structure
	struct tcphdr *tcph=NULL;	//TCP header structure
	struct MCP_Flow f;
	unsigned long flags;
	struct MCP_Flow_Info* infoPtr=NULL;
	u32 window=0;
	u32 deadline=0;
	u32 delta_in_us=0;
	ktime_t now=ktime_get();
	
	if(!out)
		return NF_ACCEPT;
        
	if(strncmp(out->name,param_dev,IFNAMSIZ)!=0)
		return NF_ACCEPT;
	
	iph=(struct iphdr *)skb_network_header(skb);
	
	//The packet is not ip packet (e.g. ARP or others)
	if (unlikely(iph==NULL))
		return NF_ACCEPT;
	
	//MCP packets
	if(iph->protocol==IPPROTO_TCP&&skb->mark>0) 
	{
		tcph=(struct tcphdr *)((__u32 *)iph+iph->ihl);
		f.local_ip=iph->saddr;
		f.remote_ip=iph->daddr;
		f.local_port=ntohs(tcph->source);
		f.remote_port=ntohs(tcph->dest);
		MCP_Init_Info(&(f.info));
		
		if(tcph->syn)
		{
			//Get data size (KB to B)
			f.info.bytes_total=((skb->mark)&0x000fffff)<<10;
			f.info.last_update=now;
			//Get deadline time (ms to ns)
			deadline=(skb->mark)&0xfff00000;
			f.info.deadline.tv64=now.tv64+deadline;
			f.info.scale=(1<<tcp_get_scale(skb));
			f.info.window=MCP_INIT_WIN;
			f.info.srtt= MCP_INIT_RTT;
			//For test
			//printk(KERN_INFO "Flow size is %u bytes. Deadline is %u ms. Window scale is %hu.\n",f.info.bytes_total,deadline>>20,f.info.scale);
			//Insert MCP flow entry
			spin_lock_irqsave(&(ft.tableLock),flags);
			if(MCP_Insert_Table(&ft,&f,GFP_ATOMIC)==0)
			{
				printk(KERN_INFO "Insert fails\n");
			}
			spin_unlock_irqrestore(&(ft.tableLock),flags);
			tcp_modify_outgoing(skb,f.info.window*MCP_TCP_MSS, get_tsval());
		}
		else
		{
			infoPtr=MCP_Search_Table(&ft,&f);
			if(infoPtr!=NULL)
			{
				spin_lock_irqsave(&(ft.tableLock),flags);
				infoPtr->last_update=now;
				delta_in_us=ktime_us_delta(infoPtr->deadline,now);
				if(delta_in_us>0)
				{
					//Calculate window needed to meet flow ddl
					window=(infoPtr->bytes_total-infoPtr->bytes_received)*infoPtr->srtt/delta_in_us*MCP_ETHERNET_PKT_LEN/ MCP_TCP_PAYLOAD_LEN;
					if(window>infoPtr->window*MCP_TCP_MSS)
					{
						infoPtr->window+=1;
					}
					else if(window<=(infoPtr->window-1)*MCP_TCP_MSS)
					{
						infoPtr->window=max(infoPtr->window-1,MCP_INIT_WIN);
					}
				}
				//Deadline has been missed (now>ddl)
				else
				{
					//We should set minimal window to this flow
					infoPtr->window=max(infoPtr->window-1,MCP_INIT_WIN);
				}
				spin_unlock_irqrestore(&(ft.tableLock),flags);
				//printk(KERN_INFO "Window is %u MSS\n",infoPtr->window);
				tcp_modify_outgoing(skb,infoPtr->window*MCP_TCP_MSS/infoPtr->scale+1, get_tsval());
			}
		}
	}
	
	return NF_ACCEPT;
}

//PREROUTING for incoming packets
static unsigned int MCP_hook_func_in(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct iphdr *iph=NULL;	//IP  header structure
	struct tcphdr *tcph=NULL;	//TCP header structure
	struct MCP_Flow f;
	unsigned long flags;
	u16 result=0;
	u32 rtt=0;	//Sample RTT value
	u32 seq=0;
	u16 payloadLen=0;	//TCP payload length
	struct MCP_Flow_Info* infoPtr=NULL;
	//ktime_t now=ktime_get();
	
	if(!in)
		return NF_ACCEPT;
        
	if(strncmp(in->name,param_dev,IFNAMSIZ)!=0)
		return NF_ACCEPT;
	
	iph=(struct iphdr *)skb_network_header(skb);
	
	//The packet is not ip packet (e.g. ARP or others)
	if (unlikely(iph==NULL))
		return NF_ACCEPT;
		
	if(likely(iph->protocol==IPPROTO_TCP)) 
	{
		tcph=(struct tcphdr *)((__u32 *)iph+ iph->ihl);
		f.local_ip=iph->daddr;
		f.remote_ip=iph->saddr;
		f.local_port=ntohs(tcph->dest);
		f.remote_port=ntohs(tcph->source);
		MCP_Init_Info(&(f.info));
		
		if(tcph->fin||tcph->rst)
		{
			spin_lock_irqsave(&(ft.tableLock),flags);
			result=MCP_Delete_Table(&ft,&f);
			spin_unlock_irqrestore(&(ft.tableLock),flags);
			if(result>0)
			{
				tcp_modify_incoming(skb,(__u32)(jiffies));
				clear_ecn(skb);
			}
		}
		else
		{
			infoPtr=MCP_Search_Table(&ft,&f);
			if(infoPtr!=NULL)
			{
				//printk(KERN_INFO "Flow entry exists\n");
				//Get sample RTT 
				rtt=max(tcp_modify_incoming(skb,(__u32)(jiffies)),MCP_INIT_RTT);
				//printk(KERN_INFO "RTT is %u us\n",rtt);
				payloadLen=ntohs(iph->tot_len)-(iph->ihl<<2)-(tcph->doff<<2);
				seq=ntohl(tcph->seq);
				if(payloadLen>=1)
					seq=seq+payloadLen-1;
				
				spin_lock_irqsave(&(ft.tableLock),flags);
				//Update smooth RTT
				infoPtr->srtt=(MCP_RTT_SMOOTH*infoPtr->srtt+(1000-MCP_RTT_SMOOTH)*rtt)/1000;
				//Update received data and sequence number 
				if(is_seq_larger(seq,infoPtr->latest_seq)>0)
				{
					infoPtr->latest_seq=seq;
					infoPtr->bytes_received=min(infoPtr->bytes_received+payloadLen,infoPtr->bytes_total);
				}
				spin_unlock_irqrestore(&(ft.tableLock),flags);
				//Clear ECN marking for MCP flows
				clear_ecn(skb);
			}
		}
	}
	
	return NF_ACCEPT;
}

//Called when module loaded using 'insmod'
int init_module()
{
	int i=0;
	//Get interface
	if(param_dev==NULL) 
	{
		printk(KERN_INFO "MCP: not specify network interface\n");
		param_dev = "eth1\0";
	}
	else
	{
		// trim 
		for(i = 0; i < 32 && param_dev[i] != '\0'; i++) 
		{
			if(param_dev[i] == '\n') 
			{
				param_dev[i] = '\0';
				break;
			}
		}
	}	
	//Initialize FlowTable
	MCP_Init_Table(&ft);
	
	//Register POSTROUTING hook
	MCP_nf_hook_out.hook=MCP_hook_func_out;                                    
	MCP_nf_hook_out.hooknum=NF_INET_POST_ROUTING;       
	MCP_nf_hook_out.pf=PF_INET;                                                       
	MCP_nf_hook_out.priority=NF_IP_PRI_FIRST;                            
	nf_register_hook(&MCP_nf_hook_out);                                         
        
	//Register PREROUTING hook
	MCP_nf_hook_in.hook=MCP_hook_func_in;					                  
	MCP_nf_hook_in.hooknum=NF_INET_PRE_ROUTING;			
	MCP_nf_hook_in.pf=PF_INET;							                          
	MCP_nf_hook_in.priority=NF_IP_PRI_FIRST;			              
	nf_register_hook(&MCP_nf_hook_in);		                   
	
	printk(KERN_INFO "Start MCP kernel module on %s\n", param_dev);

	return 0;
}

//Called when module unloaded using 'rmmod'
void cleanup_module()
{
	//Unregister two hooks
	nf_unregister_hook(&MCP_nf_hook_out);  
	nf_unregister_hook(&MCP_nf_hook_in);
	
	//Clear flow table
	//Print_Table(&ft);
	MCP_Empty_Table(&ft);
	
	printk(KERN_INFO "Stop MCP kernel module\n");
}