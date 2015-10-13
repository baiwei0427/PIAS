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
MODULE_DESCRIPTION("Linux kernel module for Karuna");

static char *param_dev=NULL;
MODULE_PARM_DESC(param_dev, "Interface to operate Karuna");
module_param(param_dev, charp, 0);

//FlowTable
static struct Karuna_Flow_Table ft;

//The hook for outgoing packets 
static struct nf_hook_ops Karuna_nf_hook_out;
//The hook for incoming packets 
static struct nf_hook_ops Karuna_nf_hook_in;

//Hook function for outgoing packets
static unsigned int Karuna_hook_func_out(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct iphdr *iph=NULL;	//IP  header structure
	struct tcphdr *tcph=NULL;	//TCP header structure
	struct Karuna_Flow f;	//Flow structure
	struct Karuna_Flow_Info* infoPtr=NULL;	//Pointer to structure Information
	unsigned long flags;	//variable for save current states of irq
	u8 dscp=0;	//DSCP value
	u16 payload_len=0;	//TCP payload length
	u32 seq=0;	//TCP sequence number
	u32 result=0;	//Delete_Table return result
	u8 type=0;	//Flow type (0,1 and 2)
	u32 deadline=0;	//Deadline time 
	u32 flow_size_bytes=0;	//Flow size in bytes
	ktime_t now=ktime_get();	//current time
	
	if(!out)
		return NF_ACCEPT;
        
	if(strncmp(out->name,param_dev,IFNAMSIZ)!=0)
		return NF_ACCEPT;
	
	//Get IP header
	iph=ip_hdr(skb);
	//The packet is not an IP packet (e.g. ARP or others), return NF_ACCEPT
	if (unlikely(iph==NULL))
		return NF_ACCEPT;
		
	if(iph->protocol==IPPROTO_TCP) 
	{
		tcph=(struct tcphdr *)((__u32 *)iph+ iph->ihl);
		Karuna_Init_Flow(&f);
		f.local_ip=iph->saddr;
		f.remote_ip=iph->daddr;
		f.local_port=(u16)ntohs(tcph->source);
		f.remote_port=(u16)ntohs(tcph->dest);
		
		//Type 1 or 2 flows
		if(skb->mark>0)
		{
			//Get flow size information (bytes)
			flow_size_bytes=((skb->mark)&0x000fffff)<<10;
			//Get deadline information (ns)
			deadline=((skb->mark)&0xfff00000);
		}
			
		//TCP SYN packet, a new  connection
		if(tcph->syn) 
		{
			f.info.latest_seq=ntohl(tcph->seq);	
			f.info.latest_update_time=now;
			//Type 1 flows
			if(deadline>0)	
			{
				f.info.is_deadline_known=1;
				//This sentence has no use
				f.info.bytes_total=flow_size_bytes;
			}
			//Type 2 flows
			else if(flow_size_bytes>0)
			{
				f.info.is_size_known=1;
				f.info.bytes_total=flow_size_bytes;
			}
			//A new Flow entry should be inserted into FlowTable
			spin_lock_irqsave(&(ft.tableLock),flags);
			if(Karuna_Insert_Table(&ft,&f,GFP_ATOMIC)==0)
			{
				printk(KERN_INFO "Insert fail\n");
			}
			spin_unlock_irqrestore(&(ft.tableLock),flags);
			//Type 1 flows
			if(f.info.is_deadline_known==1)
				dscp=Karuna_priority(0,0);
			//Type 2 flows
			else if(f.info.is_size_known==1)
				dscp=Karuna_priority(1,f.info.bytes_total);
			//Type 3 flows
			else
				dscp=Karuna_priority(2,f.info.bytes_sent);
		}
		//TCP FIN/RST packets, connection will be closed 
		else if(tcph->fin||tcph->rst)  
		{
			//An existing Flow entry should be deleted from FlowTable. 
			spin_lock_irqsave(&(ft.tableLock),flags);
			result=Karuna_Delete_Table(&ft,&f, &type);
			if(result==0)
			{
				printk(KERN_INFO "Delete fail\n");
			}
			spin_unlock_irqrestore(&(ft.tableLock),flags);
			dscp=Karuna_priority(type,result);
		}
		else
		{
			//Update existing Flow entry's information
			spin_lock_irqsave(&(ft.tableLock),flags);
			infoPtr=Karuna_Search_Table(&ft,&f);
			if(infoPtr!=NULL)
			{
				if(skb->mark>0&&infoPtr->is_size_known==0&&infoPtr->is_deadline_known==0)
				{
					//Type1 1 flows
					if(deadline>0)	
					{
						infoPtr->is_deadline_known=1;
						infoPtr->bytes_total=flow_size_bytes;
					}
					//Type 2 flows
					else if(flow_size_bytes>0)
					{
						infoPtr->is_size_known=1;
						infoPtr->bytes_total=flow_size_bytes;
					}
				}
				//TCP payload length=Total IP length - IP header length-TCP header length
				payload_len=ntohs(iph->tot_len)-(iph->ihl<<2)-(tcph->doff<<2);      
				seq=ntohl(tcph->seq);
				if(payload_len>=1)
					seq=seq+payload_len-1;
				if(Karuna_is_seq_larger(seq,infoPtr->latest_seq)==1)
				{
					if(payload_len+infoPtr->bytes_sent<Karuna_MAX_FLOW_SIZE)
					{
						infoPtr->latest_seq=seq;
					}
					//For Type 3 flows, we should update bytes_sent
					if(infoPtr->is_size_known==0&&infoPtr->is_deadline_known==0)
					{
						infoPtr->bytes_sent+=payload_len;
					}
				}
				//Packet drop happens
				else
				{
					//TCP timeout
					if(ktime_us_delta(now,infoPtr->latest_update_time)>=Karuna_RTO_MIN&&Karuna_is_seq_larger(infoPtr->latest_seq,infoPtr->latest_ack)==1)
					{
						printk(KERN_INFO "Karuna: TCP timeout is detected with RTO = %u\n",(unsigned int)ktime_us_delta(now,infoPtr->latest_update_time)); 
						//It's a 'consecutive' TCP timeout? 
						if(Karuna_TIMEOUT_THRESH==1||(Karuna_TIMEOUT_THRESH>=2&&(Karuna_seq_gap(seq,infoPtr->latest_timeout_seq)<=5*1460||Karuna_seq_gap(infoPtr->latest_timeout_seq,seq)<=5*1460)))
						{
							infoPtr->timeouts++;
							//Fixed threshold of consecutive TCP timeouts
							if(infoPtr->timeouts>=Karuna_TIMEOUT_THRESH)
							{
								printk(KERN_INFO "%u consecutive TCP timeouts are detected!\n", Karuna_TIMEOUT_THRESH);
								infoPtr->timeouts=0;
								//TCP Aging to prevent starvation
								//Type 2 flows
								if(infoPtr->is_size_known==1&&infoPtr->is_deadline_known==0)
									//Reset bytes_total to change priority
									infoPtr->bytes_total=Karuna_smaller_threshold(infoPtr->bytes_total)-1;
								//Type 3 flows
								else if(infoPtr->is_size_known==0&&infoPtr->is_deadline_known==0)
									infoPtr->bytes_sent=0;
							}
						}
						//It is the first timeout
						else
						{
							infoPtr->timeouts=1;
						}
						infoPtr->latest_timeout_seq=seq;
					}
				}
				//Type 1 flows
				if(infoPtr->is_deadline_known==1)
					dscp=Karuna_priority(0,0);
				//Type 2 flows
				else if(infoPtr->is_size_known==1)
					dscp=Karuna_priority(1,infoPtr->bytes_total);
				//Type 3 flows
				else
					dscp=Karuna_priority(2,infoPtr->bytes_sent);
			}
			//No such Flow entry, last few packets. We need to accelerate flow completion.
			else
			{
				dscp=Karuna_priority(0,0);
			}
			spin_unlock_irqrestore(&(ft.tableLock),flags);
		}
		//Modify DSCP and make the packet ECT
		Karuna_enable_ecn_dscp(skb,dscp);
	}
	return NF_ACCEPT;	
}

//Hook function for incoming packets
static unsigned int Karuna_hook_func_in(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct iphdr *iph=NULL;	//IP  header structure
	struct tcphdr *tcph=NULL;	//TCP header structure
	struct Karuna_Flow f;	//Flow structure
	struct Karuna_Flow_Info* infoPtr=NULL;	//Pointer to structure Information
	u32 ack;
	unsigned long flags;	//variable for save current states of irq
	
	
	if(!in)
		return NF_ACCEPT;
        
	if(strncmp(in->name,param_dev,IFNAMSIZ)!=0)
		return NF_ACCEPT;
	
	//Get IP header
	iph=ip_hdr(skb);
	//The packet is not an IP packet (e.g. ARP or others), return NF_ACCEPT
	if (unlikely(iph==NULL))
		return NF_ACCEPT;
		
	if(iph->protocol==IPPROTO_TCP) 
	{
		tcph=(struct tcphdr *)((__u32 *)iph+ iph->ihl);
		Karuna_Init_Flow(&f);
		f.local_ip=iph->daddr;
		f.remote_ip=iph->saddr;
		f.local_port=(u16)ntohs(tcph->dest);
		f.remote_port=(u16)ntohs(tcph->source);
		//Update existing Flow entry's information
		spin_lock_irqsave(&(ft.tableLock),flags);
		infoPtr=Karuna_Search_Table(&ft,&f);
		if(infoPtr!=NULL)
		{
			ack=(u32)ntohl(tcph->ack);
			if(Karuna_is_seq_larger(ack,infoPtr->latest_ack)==1)
			{
				infoPtr->latest_ack=ack;
			}
		}
		spin_unlock_irqrestore(&(ft.tableLock),flags);
	}
	return NF_ACCEPT;
}

int init_module()
{
	int i=0;
    //Get interface
    if(param_dev==NULL) 
    {
        //printk(KERN_INFO "PIAS: not specify network interface.\n");
        param_dev = "eth1\0";
	}
    // trim 
	for(i = 0; i < 32 && param_dev[i] != '\0'; i++) 
    {
		if(param_dev[i] == '\n') 
        {
			param_dev[i] = '\0';
			break;
		}
	}
		
	//Initialize FlowTable
	Karuna_Init_Table(&ft);
	
	//Register POSTROUTING hook
	Karuna_nf_hook_out.hook=Karuna_hook_func_out;                                    
	Karuna_nf_hook_out.hooknum=NF_INET_POST_ROUTING;       
	Karuna_nf_hook_out.pf=PF_INET;                                                       
	Karuna_nf_hook_out.priority=NF_IP_PRI_FIRST;                            
	nf_register_hook(&Karuna_nf_hook_out);              
	
	//Register PREROUTING hook
	Karuna_nf_hook_in.hook=Karuna_hook_func_in;					                  
	Karuna_nf_hook_in.hooknum=NF_INET_PRE_ROUTING;			
	Karuna_nf_hook_in.pf=PF_INET;							                          
	Karuna_nf_hook_in.priority=NF_IP_PRI_FIRST;			              
	nf_register_hook(&Karuna_nf_hook_in);		                   
	
	printk(KERN_INFO "Start Karuna kernel module on %s\n", param_dev);
	return 0;
}

void cleanup_module()
{
	//Unregister two hooks
	nf_unregister_hook(&Karuna_nf_hook_out);  
	nf_unregister_hook(&Karuna_nf_hook_in);
	
	//Clear table
	Karuna_Empty_Table(&ft);

	printk(KERN_INFO "Stop Karuna kernel module\n");
}


