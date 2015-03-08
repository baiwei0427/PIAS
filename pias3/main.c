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
MODULE_VERSION("1.2");
MODULE_DESCRIPTION("Linux kernel module for PIAS (Practical Information-Agnostic flow Scheduling)");

//FlowTable
static struct PIAS_Flow_Table ft;
//The hook for outgoing packets 
static struct nf_hook_ops pias_nf_hook_out;
//The hook for incoming packets 
static struct nf_hook_ops pias_nf_hook_in;

//The following two functions are related to param_table_operation
//To clear flow table: echo -n clear > /sys/module/pias/parameters/param_table_operation
//To print flow table: echo -n print > /sys/module/pias/parameters/param_table_operation
static int pias_set_operation(const char *val, struct kernel_param *kp);
static int pias_noget(const char *val, struct kernel_param *kp);
//static char *param_table_operation=NULL;
//MODULE_PARM_DESC(param_table_operation, "Operation to flow table (Print or Clear)");
module_param_call(param_table_operation, pias_set_operation, pias_noget, NULL, S_IWUSR); //Write permission by owner

static char *param_dev=NULL;
MODULE_PARM_DESC(param_dev, "Interface to operate PIAS");
module_param(param_dev, charp, 0);

static int pias_set_operation(const char *val, struct kernel_param *kp)
{
	unsigned long flags;	//variable for save current states of irq
		
	//For debug
	printk(KERN_INFO "PIAS: param_table_operation is set\n");
	//Clear flow table
	if(strncmp(val,"clear\0",5)==0)
	{
		spin_lock_irqsave(&(ft.tableLock),flags);
		PIAS_Clear_Table(&ft);
		spin_unlock_irqrestore(&(ft.tableLock),flags);
	}
	//Print flow table
	else if(strncmp(val,"print\0",5)==0)
	{
		//spin_lock_irqsave(&(ft.tableLock),flags);
		PIAS_Print_Table(&ft);
		//spin_unlock_irqrestore(&(ft.tableLock),flags);
	}
	else
	{
		printk(KERN_INFO "PIAS: unrecognized flow table operation\n");
	}
	return 0;
}

static int pias_noget(const char *val, struct kernel_param *kp)
{
	return 0;
}

//Hook function for outgoing packets
static unsigned int pias_hook_func_out(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct iphdr *iph=NULL;	//IP  header structure
	struct tcphdr *tcph=NULL;	//TCP header structure
	struct PIAS_Flow f;	//PIAS flow structure
	struct PIAS_Flow_Info* infoPtr=NULL;	//Pointer to structure Information
	unsigned long flags;	//variable for save current states of irq
	u8 dscp=0;	//DSCP value
	u16 payload_len=0;	//TCP payload length
	u32 seq=0;	//TCP sequence number
	u32 result=0;	//Delete_Table return result
	ktime_t now=ktime_get();	//get current time
	
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
		PIAS_Init_Flow(&f);
		f.local_ip=iph->saddr;
		f.remote_ip=iph->daddr;
		f.local_port=(u16)ntohs(tcph->source);
		f.remote_port=(u16)ntohs(tcph->dest);
		
		//TCP SYN packet, a new  connection
		if(tcph->syn) 
		{
			f.info.latest_seq=ntohl(tcph->seq);	
			f.info.latest_update_time=now;
			//A new Flow entry should be inserted into FlowTable
			spin_lock_irqsave(&(ft.tableLock),flags);
			if(PIAS_Insert_Table(&ft,&f,GFP_ATOMIC)==0)
			{
				printk(KERN_INFO "PIAS: insert fail\n");
			}
			spin_unlock_irqrestore(&(ft.tableLock),flags);
			dscp=PIAS_priority(0);
		}
		//TCP FIN/RST packets, connection will be closed 
		else if(tcph->fin||tcph->rst)  
		{
			//An existing Flow entry should be deleted from FlowTable. 
			spin_lock_irqsave(&(ft.tableLock),flags);
			result=PIAS_Delete_Table(&ft,&f);
			//PIAS_Delete_Table returns bytes sent information of this flow
			if(result==0)
			{
				printk(KERN_INFO "PIAS: delete fail\n");
			}
			spin_unlock_irqrestore(&(ft.tableLock),flags);
			dscp=PIAS_priority(result);
		}
		else
		{
			//Update existing Flow entry's information
			spin_lock_irqsave(&(ft.tableLock),flags);
			infoPtr=PIAS_Search_Table(&ft,&f);
			if(infoPtr!=NULL)
			{
				//TCP payload length=Total IP length - IP header length-TCP header length
				payload_len=ntohs(iph->tot_len)-(iph->ihl<<2)-(tcph->doff<<2);      
				seq=ntohl(tcph->seq);
				//Get the sequence number of the last payload byte 
				if(payload_len>=1)
				{
					seq=seq+payload_len-1;
				}
				//A new TCP packet 
				if(PIAS_is_seq_larger(seq,infoPtr->latest_seq)==1)
				{
					//Update sequence number 
					infoPtr->latest_seq=seq;
					//Update bytes sent 
					if(payload_len+infoPtr->bytes_sent<PIAS_MAX_FLOW_SIZE)
					{
						infoPtr->bytes_sent+=payload_len;
					}
				}
#ifdef ANTI_STARVATION
				//A retransmitted TCP packet. Packet drops happen!
				else
				{
					//TCP timeout
					if(ktime_us_delta(now,infoPtr->latest_update_time)>=PIAS_RTO_MIN&&PIAS_is_seq_larger(infoPtr->latest_seq,infoPtr->latest_ack)==1)
					{
						//printk(KERN_INFO "PIAS: TCP timeout is detected with RTO = %u\n",(unsigned int)ktime_us_delta(now,infoPtr->latest_update_time)); 
						//It's a 'consecutive' TCP timeout?  
						if(PIAS_TIMEOUT_THRESH==1||(PIAS_TIMEOUT_THRESH>=2&&(PIAS_seq_gap(seq,infoPtr->latest_timeout_seq)<=PIAS_SEQ_GAP_THRESH)))
						{
							infoPtr->timeouts++;
							//If the number of consecutive TCP timeouts is larger than the threshold
							if(infoPtr->timeouts>=PIAS_TIMEOUT_THRESH)
							{
								//printk(KERN_INFO "%u consecutive TCP timeouts are detected!\n", pias_TIMEOUT_THRESH);
								infoPtr->timeouts=0;
								//Reset bytes sent to zero (highest priority)
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
#endif
				//Update latest_update_time of this flow
				infoPtr->latest_update_time=now;
				//Calculate priority of this flow based on bytes sent
				dscp=PIAS_priority(infoPtr->bytes_sent);
			}
			//No such Flow entry. Maybe last few packets of the flow. We need to accelerate flow completion.
			else
			{
				dscp=PIAS_priority(0);
			}
			spin_unlock_irqrestore(&(ft.tableLock),flags);
		}
		//Modify DSCP and make the packet ECT
		PIAS_enable_ecn_dscp(skb,dscp);
	}
	return NF_ACCEPT;	
}

//Hook function for incoming packets
static unsigned int pias_hook_func_in(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	struct iphdr *iph=NULL;	//IP  header structure
	struct tcphdr *tcph=NULL;	//TCP header structure
	struct PIAS_Flow f;	//Flow structure
	struct PIAS_Flow_Info* infoPtr=NULL;	//Pointer to structure Information
	u32 ack;	//TCP ACK number
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
		PIAS_Init_Flow(&f);
		f.local_ip=iph->daddr;
		f.remote_ip=iph->saddr;
		f.local_port=(u16)ntohs(tcph->dest);
		f.remote_port=(u16)ntohs(tcph->source);
		//Update existing Flow entry's information
		spin_lock_irqsave(&(ft.tableLock),flags);
		infoPtr=PIAS_Search_Table(&ft,&f);
		if(infoPtr!=NULL)
		{
			ack=(u32)ntohl(tcph->ack);
			if(PIAS_is_seq_larger(ack,infoPtr->latest_ack)==1)
			{
				infoPtr->latest_ack=ack;
			}
		}
		spin_unlock_irqrestore(&(ft.tableLock),flags);
	}
	return NF_ACCEPT;
}

static int pias_module_init(void)
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
	
	if(PIAS_params_init()<0)
		return -1;
		
	//Initialize FlowTable
	PIAS_Init_Table(&ft);
	
	//Register outgoing hook
	pias_nf_hook_out.hook=pias_hook_func_out;                                    
	pias_nf_hook_out.hooknum=NF_INET_LOCAL_OUT;       
	pias_nf_hook_out.pf=PF_INET;                                                       
	pias_nf_hook_out.priority=NF_IP_PRI_FIRST;                            
	nf_register_hook(&pias_nf_hook_out);              
	
#ifdef ANTI_STARVATION
	//Register incoming hook
	pias_nf_hook_in.hook=pias_hook_func_in;					                  
	pias_nf_hook_in.hooknum=NF_INET_LOCAL_IN;			
	pias_nf_hook_in.pf=PF_INET;							                          
	pias_nf_hook_in.priority=NF_IP_PRI_FIRST;			              
	nf_register_hook(&pias_nf_hook_in);		                   
#endif 
	printk(KERN_INFO "PIAS: start on %s\n", param_dev);
#ifdef ANTI_STARVATION
	printk(KERN_INFO "PIAS: anti-starvation mechanism is enabled\n");
#endif
	return 0;
}

static void pias_module_exit(void)
{
	//Unregister two hooks
	nf_unregister_hook(&pias_nf_hook_out);  
#ifdef ANTI_STARVATION
	nf_unregister_hook(&pias_nf_hook_in);
#endif
	//Clear flow table
	PIAS_Exit_Table(&ft);
	PIAS_params_exit();
	printk(KERN_INFO "PIAS: stop working\n");
}

module_init(pias_module_init);
module_exit(pias_module_exit);

