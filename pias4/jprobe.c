#include <linux/ktime.h>
#include <linux/tcp.h>
#include <net/tcp.h>
#include <linux/kprobes.h>

#include "jprobe.h"
#include "flow.h"
#include "params.h"

/* Flow Table */
extern struct PIAS_Flow_Table ft;
/* TCP port */
extern int param_port;

/* Hook inserted to be called before each socket call.
 * Note: arguments must match tcp_sendmsg()! */
static int jtcp_sendmsg(struct kiocb *iocb, struct sock *sk, struct msghdr *msg, size_t size)
{
    struct PIAS_Flow f;	//PIAS flow structure
    struct PIAS_Flow *ptr = NULL;	//pointer to PIAS flow structure
    ktime_t now = ktime_get();	//get current time
    const struct tcp_sock *tp = tcp_sk(sk);
    const struct inet_sock *inet = inet_sk(sk);
    s64 idle_time = 0;	//idle time in us
    unsigned long flags;

	f.local_ip = inet->inet_saddr;
	f.remote_ip = inet->inet_daddr;
	f.local_port = (u16)ntohs(inet->inet_sport);
	f.remote_port = (u16)ntohs(inet->inet_dport);

	if (param_port == 0 || f.local_port == param_port || f.remote_port == param_port)
	{
		ptr = PIAS_Search_Table(&ft, &f);
		if (ptr)
		{
            spin_lock_irqsave(&(ptr->lock), flags);
			//First message in this connections
			if (ptr->info.last_copy_time.tv64 == 0)
			{
				ptr->info.messages++;
				if (PIAS_DEBUG_MODE)
					printk(KERN_INFO "Meesage %hu is detected on TCP connection %pI4:%hu to %pI4:%hu\n", ptr->info.messages, &(f.local_ip), f.local_port, &(f.remote_ip), f.remote_port);
			}
			else if(tp->snd_nxt == tp->write_seq)
			{
				idle_time = ktime_us_delta(now, ptr->info.last_copy_time);
				if (idle_time > PIAS_IDLE_TIME)
				{
					ptr->info.bytes_sent = 0;
					ptr->info.messages++;
					if (PIAS_DEBUG_MODE)
						printk(KERN_INFO "Message %hu is detected on TCP connection %pI4:%hu to %pI4:%hu after %lld us idle time\n", ptr->info.messages, &(f.local_ip), f.local_port, &(f.remote_ip), f.remote_port, idle_time);
				}
			}
			ptr->info.last_copy_time = now;
			spin_unlock_irqrestore(&(ptr->lock), flags);
		}
	}

	jprobe_return();
	return 0;
}

static struct jprobe pias_tcp_sendmsg =
{
	.kp = { .symbol_name = "tcp_sendmsg",},
	.entry = jtcp_sendmsg,
};

bool PIAS_JProbe_Init(void)
{
    pias_tcp_sendmsg.kp.symbol_name = "tcp_sendmsg";
    pias_tcp_sendmsg.entry = jtcp_sendmsg;

    //Register jprobe hook
    BUILD_BUG_ON(__same_type(tcp_sendmsg, jtcp_sendmsg) == 0);
    if (register_jprobe(&pias_tcp_sendmsg))
    {
        printk(KERN_INFO "Cannot register the jprobe hook for tcp_sendmsg\n");
        return false;
    }
    else
        return true;
}

void PIAS_JProbe_Exit(void)
{
    //Unregister jprobe hook
    unregister_jprobe(&pias_tcp_sendmsg);
}
