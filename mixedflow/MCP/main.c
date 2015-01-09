#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/netfilter.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
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

//The hook for outgoing packets at POSTROUTING
static struct nf_hook_ops MCP_nf_hook_out;
//The hook for incoming packets at PREROUTING
static struct nf_hook_ops MCP_nf_hook_in;

//POSTROUTING for outgoing packets
static unsigned int MCP_hook_func_out(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
	
}

//PREROUTING for incoming packets
static unsigned int MCP_hook_func_in(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{

}

//Called when module loaded using 'insmod'
int init_module()
{
	
}

//Called when module unloaded using 'rmmod'
void cleanup_module()
{
	
}