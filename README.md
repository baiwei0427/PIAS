# PIAS

PIAS, Practical Information-Agnostic flow Scheduling, targets at minimizing flow completion times (FCT) for commodity data centers. As its heart, PIAS leverages multiple priority queues available in existing switches to perform Multiple Level Feedback Queue. A PIAS flow is gradually demoted from higher-priority queues to lower-priority queues based on the bytes it has sent. Therefore, short flows are likely to finish in the first few high-priority queues and prioritized over large flows in general. Given the traffic volatility in data centers, we also employ Explicit Congestion Notification (ECN) to keep PIAS's good performance in such a highly dynamic environment. 

For more information about this project, please visit <http://sing.cse.ust.hk/projects/PIAS>.

## How To Use

The PIAS prototype has three components: packet tagging, switch configuration and rate control.

### Packet Tagging

#### Compiling
The stable version of packet tagging is in `pias3` directory. It is a Linux kernel module that maintains per-flow state (`flow.h` and `flow.c`) and mark packets with priority (`network.h` and `network.c`). So you need the kernel headers to compile it:  

<pre><code>$ cd pias3<br/>
$ make</code></pre>

Then you can get a kernel module called `pias.ko`. Note that the anti-starvation mechanism (`DANTI_STARVATION` in Makefile) is enabled by default. Under such setting, for a flow experiencing several consecutive TCP timeouts, we will reset its bytes sent information back to 0. You can also modify Makefile to disable it. 

#### Installing 
The packet tagging module hooks into the data path using `Netfilter` hooks. To install it:
<pre><code>$ insmod pias.ko param_dev=eth2<br/>
$ dmesg|tail<br/>
PIAS: start on eth2<br/>
PIAS: anti-starvation mechanism is enabled
</code></pre>

`param_dev` is the name of NIC that `pias.ko` works on. It is `eth1` by default.

To remove the packet tagging module:
<pre><code>$ rmmod pias<br/>
$ dmesg|tail<br/>
PIAS: stop working
</code></pre>

#### Usage
