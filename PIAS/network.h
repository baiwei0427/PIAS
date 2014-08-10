#ifndef NETWORK_H
#define NETWORK_H

#include <linux/skbuff.h>
#include <linux/time.h>  

//Function to calculate microsecond-granularity TCP timestamp value 
static unsigned int get_tsval(void)
{	
	return (unsigned int)(ktime_to_ns(ktime_get())>>10);
}

//Modify DSCP value and make the packet ECT, if success, return 1, else return 0
static unsigned int modify_dscp(struct sk_buff *skb, unsigned int dscp)
{
	struct iphdr *ip_header;   	//IP header structure
	unsigned int tos_value=0;		//TOS value (Note that TOS is 4*DSCP)
	unsigned char* tmp=NULL;  //temporary variable
	
	//Get IP header
	ip_header=(struct iphdr *)skb_network_header(skb);

	//The packet is not a IP packet (e.g. ARP or others)
	if (!ip_header)
	{
		return 0;
	}
	if(!skb_make_writable(skb,sizeof(*ip_header)))
	{
		printk(KERN_INFO "Not writable\n");
		return 0;
	}
	 //Get new IP header
	ip_header=(struct iphdr *)skb_network_header(skb);
	//Calculate ideal TOS value. Note that packets may be ECT (CE bits=10)
	tos_value=4*dscp+2;
	tmp=(unsigned char*)&tos_value;
	//Modify TOS of IP header
	ip_header->tos=*tmp;
	//Recalculate IP checksum
	ip_header->check=0;
	ip_header->check=ip_fast_csum(ip_header,ip_header->ihl);      
	return 1;
}

//Modify TCP packets' Timestamp option and get sample RTT value
//If time>0: modify outgoing TCP packets' Timestamp value
//Else: modify incoming TCP pakcets' Timestamp echo reply and return RTT
static unsigned int modify_timestamp(struct sk_buff *skb, unsigned int time)
{
	struct iphdr *ip_header=NULL;         	//IP  header structure
	struct tcphdr *tcp_header=NULL;      //TCP header structure
	unsigned int tcp_header_len=0;		  	//TCP header length
	unsigned char *tcp_opt=NULL;		  	//TCP option pointer
	unsigned int *tsecr=NULL;			  		//TCP Timestamp echo reply pointer
	unsigned int *tsval=NULL;			  		//TCP Timestamp value pointer
	int tcplen=0;						  						//Length of TCP
	unsigned char tcp_opt_value=0;		  	//TCP option pointer value
	unsigned int rtt=0;				 	  				//Sample RTT
	unsigned int option_len=0;			  		//TCP option length
	
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
	tcp_opt=(unsigned char*)ip_header+ ip_header->ihl*4+20;
	
	while(1)
	{
		//If pointer has moved out off the range of TCP option, stop current loop
		if(tcp_opt-(unsigned char*)tcp_header>=tcp_header_len)
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
			if(time>0) //Modify outgoing packets
			{
				//Get pointer to Timestamp value 
				tsval=(unsigned int*)(tcp_opt+2);
				//Modify TCP Timestamp value
				*tsval=htonl(time);
			}
			else //Modify incoming packets
			{
				//Get pointer to Timestamp echo reply (TSecr)
				tsecr=(unsigned int*)(tcp_opt+6);
				//Get one RTT sample
				rtt=get_tsval()-ntohl(*tsecr);
				printk(KERN_INFO "RTT: %u\n",rtt);
				//Modify TCP TSecr back to jiffies
				//Don't disturb TCP. Wrong TCP timestamp echo reply may reset TCP connections
				*tsecr=htonl(jiffies);
			}
			break;
		}
		else //Other TCP options (e.g. MSS(2))
		{
			//Move to next byte to get length of this TCP option 
			tcp_opt++;
			//Get length of this TCP option
			tcp_opt_value=*tcp_opt;
			option_len=(unsigned int)tcp_opt_value;
			
			//Move to next TCP option
			tcp_opt=tcp_opt+1+(option_len-2);
		}
	}
	
	//TCP length=Total length - IP header length
	tcplen=skb->len-(ip_header->ihl<<2);
	tcp_header->check=0;
			
	tcp_header->check = csum_tcpudp_magic(ip_header->saddr, ip_header->daddr,
                                  tcplen, ip_header->protocol,
                                  csum_partial((char *)tcp_header, tcplen, 0));
								  
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	return rtt;
} 

#endif
