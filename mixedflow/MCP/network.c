#include <linux/ip.h>
#include <linux/tcp.h>
#include <net/dsfield.h>
#include "network.h"

//Calculate microsecond-granularity TCP timestamp value 
inline u32 get_tsval(void)
{
	return (u32)(ktime_to_ns(ktime_get())>>10);
}

//Clear ECN marking
inline void clear_ecn(struct sk_buff *skb)
{
	struct iphdr *iph=ip_hdr(skb);
	if(likely(iph!=NULL))
	{
		if(skb_make_writable(skb, sizeof(struct iphdr)))
		{
			ipv4_change_dsfield(iph, 0xff, iph->tos & ~0x3);
		}
	}
}

//Function:  get TCP window scale shift count from SYN packets and return 0 by default
u8 tcp_get_scale(struct sk_buff *skb)
{
	struct iphdr *ip_header=NULL;	//IP  header structure
	struct tcphdr *tcp_header=NULL;	//TCP header structure
	unsigned int tcp_header_len=0;	//TCP header length
	u8 *tcp_opt=NULL;	//TCP option pointer
	u8 tcp_opt_value=0;	//TCP option pointer value
	
	//Get IP header
	ip_header=(struct iphdr *)skb_network_header(skb);
	//Get TCP header on the base of IP header
	tcp_header=(struct tcphdr *)((u32 *)ip_header+ ip_header->ihl);
	//Get TCP header length
	tcp_header_len=(unsigned int)(tcp_header->doff*4);
	
	//Minimum TCP header length=20(Raw TCP header)+10(TCP Timestamp option)+3(TCP window scale option) 
	if(tcp_header_len<33)
		return 1;
	
	//TCP option offset=IP header pointer+IP header length+TCP header length
	tcp_opt=(u8*)ip_header+ ip_header->ihl*4+20;
	
	while(1)
	{
		//If pointer has moved out off the range of TCP option, stop current loop
		if(tcp_opt-(u8*)tcp_header>=tcp_header_len)
			break;
		
		//Get value of current byte
		tcp_opt_value=*tcp_opt;
		
		if(tcp_opt_value==1)//No-Operation (NOP)
		{
			//Move to next byte
			tcp_opt++;
		}
		else if(tcp_opt_value==3) //TCP option kind: window scale (3)
		{
			//return window scale shift count
			return *(tcp_opt+2);
		}
		else //Other TCP options (e.g. MSS(2))
		{
			//Move to next byte to get length of this TCP option 
			tcp_opt++;
			//Get length of this TCP option
			tcp_opt_value=*tcp_opt;
			//Move to next TCP option
			tcp_opt=tcp_opt+1+(tcp_opt_value-2);
		}
	}
	//By default, shift count=0
	return 0;
}

//Modify incoming TCP packets and return RTT sample value
u32 tcp_modify_incoming(struct  sk_buff *skb)
{
	struct iphdr *ip_header=NULL;	//IP  header structure
	struct tcphdr *tcp_header=NULL;	//TCP header structure
	unsigned int tcp_header_len=0;	//TCP header length
	u8 *tcp_opt=NULL;	//TCP option pointer
	u32 *tsecr=NULL;	//TCP Timestamp echo reply pointer
	u32 tcplen=0;	//Length of TCP
	u8 tcp_opt_value=0;	//TCP option pointer value
	u32 rtt=0;	//Sample RTT
	
	//If we can not modify this packet, return 0
	if (skb_linearize(skb)!= 0) 
	{
		return 0;
	}
	
	//Get IP header
	ip_header=(struct iphdr *)skb_network_header(skb);
	//Get TCP header on the base of IP header
	tcp_header = (struct tcphdr *)((__u32 *)ip_header+ ip_header->ihl);
	//Get TCP header length
	tcp_header_len=(unsigned int)(tcp_header->doff*4);
	
	//Minimum TCP header length=20(Raw TCP header)+10(TCP Timestamp option)
	if(tcp_header_len<30)
	{
		return 0;
	}
	
	//TCP option offset=IP header pointer+IP header length+TCP header length
	tcp_opt=(u8*)ip_header+ ip_header->ihl*4+20;
	
	while(1)
	{
		//If pointer has moved out off the range of TCP option, stop current loop
		if(tcp_opt-(u8*)tcp_header>=tcp_header_len)
		{
			break;
		}
		//Get value of current byte
		tcp_opt_value=*tcp_opt;
		
		if(tcp_opt_value==1)//No-Operation (NOP)
		{
			//Move to next byte
			tcp_opt++;
		}
		else if(tcp_opt_value==8) //TCP option kind: Timestamp (8)
		{
			//Get pointer to Timestamp echo reply (TSecr)
			tsecr=(u32*)(tcp_opt+6);
			//Get one RTT sample
			rtt=get_tsval()-ntohl(*tsecr);
			//printk(KERN_INFO "RTT: %u\n",rtt);
			//Modify TCP TSecr back to jiffies
			//Don't disturb TCP. Wrong TCP timestamp echo reply may reset TCP connections
			*tsecr=htonl(jiffies);
			break;
		}
		else //Other TCP options (e.g. MSS(2))
		{
			//Move to next byte to get length of this TCP option 
			tcp_opt++;
			//Get length of this TCP option
			tcp_opt_value=*tcp_opt;
			//Move to next TCP option
			tcp_opt=tcp_opt+1+(tcp_opt_value-2);
		}
	}
	
	//TCP length=Total length - IP header length
	tcplen=(ip_header->tot_len)-(ip_header->ihl<<2);
	tcp_header->check=0;
	tcp_header->check = csum_tcpudp_magic(ip_header->saddr, ip_header->daddr,tcplen, ip_header->protocol,csum_partial((char *)tcp_header, tcplen, 0));
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	return rtt;
}

//Modify timestamp and receive window of outgoing TCP packets. Input: win (receive window value in bytes) and time (us)
//If it succeeds, return 1. Else, return 0.
u8 tcp_modify_outgoing(struct sk_buff *skb, u16 win, u32 time)
{
	struct iphdr *ip_header=NULL;	//IP  header structure
	struct tcphdr *tcp_header=NULL;	//TCP header structure
	unsigned int tcp_header_len=0;	//TCP header length
	u8 *tcp_opt=NULL;	//TCP option pointer
	u32 *tsval=NULL;	//TCP Timestamp value pointer
	u32 tcplen=0;	//Length of TCP
	u8 tcp_opt_value=0;	//TCP option pointer value
	
	if (skb_linearize(skb)!= 0) 
	{
		return 0;
	}
	
	//Get IP header
	ip_header=(struct iphdr *)skb_network_header(skb);
	//Get TCP header on the base of IP header
	tcp_header = (struct tcphdr *)((__u32 *)ip_header+ ip_header->ihl);
	//Get TCP header length
	tcp_header_len=(unsigned int)(tcp_header->doff*4);
	
	//Minimum TCP header length=20(Raw TCP header)+10(TCP Timestamp option)
	if(tcp_header_len<30)
	{
		return 0;
	}
	
	//Modify TCP window. Note that TCP received window should be in (0,65535].
    if(win<=65535&&win>0)
        tcp_header->window=htons(win);

	//TCP option offset=IP header pointer+IP header length+TCP header length
	tcp_opt=(u8*)ip_header+ ip_header->ihl*4+20;
	
	while(1)
	{
		//If pointer has moved out off the range of TCP option, stop current loop
		if(tcp_opt-(u8*)tcp_header>=tcp_header_len)
		{
			break;
		}
		
		//Get value of current byte
		tcp_opt_value=*tcp_opt;
		
		if(tcp_opt_value==1)//No-Operation (NOP)
		{
			//Move to next byte
			tcp_opt++;
		}
		else if(tcp_opt_value==8) //TCP option kind: Timestamp (8)
		{
			if(time>0) 
			{
				//Get pointer to Timestamp value 
				tsval=(u32*)(tcp_opt+2);
				//Modify TCP Timestamp value
				*tsval=htonl(time);
			}
			break;
		}
		else //Other TCP options (e.g. MSS(2))
		{
			//Move to next byte to get length of this TCP option 
			tcp_opt++;
			//Get length of this TCP option
			tcp_opt_value=*tcp_opt;
			//Move to next TCP option
			tcp_opt=tcp_opt+1+(tcp_opt_value-2);
		}
	}
	
	//TCP length=Total length - IP header length
	tcplen=(ip_header->tot_len)-(ip_header->ihl<<2);
	tcp_header->check=0;
	tcp_header->check=csum_tcpudp_magic(ip_header->saddr, ip_header->daddr,tcplen, ip_header->protocol,csum_partial((char *)tcp_header, tcplen, 0));					 
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	
	return 1;
}

//Maximum unsigned 32-bit integer value: 4294967295
//Function: determine whether seq1 is larger than seq2
//If Yes, return 1. Else, return 0.
//We use a simple heuristic to handle wrapped TCP sequence number 
inline u8 is_seq_larger(u32 seq1, u32 seq2)
{
    if(likely(seq1>seq2&&seq1-seq2<=4294900000))
	{
		return 1;
	}
	else if(seq1<seq2&&seq2-seq1>4294900000)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}