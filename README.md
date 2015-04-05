#1 What is PIAS ?

PIAS, Practical Information-Agnostic flow Scheduling, targets at minimizing flow completion times (FCT) for commodity data centers. As its heart, PIAS leverages multiple priority queues available in existing switches to perform Multiple Level Feedback Queue. A PIAS flow is gradually demoted from higher-priority queues to lower-priority queues based on the bytes it has sent. Therefore, short flows are likely to finish in the first few high-priority queues and prioritized over large flows in general. Given the traffic volatility in data centers, we also employ Explicit Congestion Notification (ECN) to keep PIAS's good performance in such a highly dynamic environment. 

For more information about this project, please visit <http://sing.cse.ust.hk/projects/PIAS>.

#2 How To Use

The PIAS prototype has three components: packet tagging, switch configuration and rate control.

##2.1 Packet Tagging 

###2.1.1 Compiling
The stable version of packet tagging is in `pias3` directory. I have tested this with Linux kernel 2.6.38.3 and 3.18.9. It is a Linux kernel module that maintains per-flow state (`flow.h` and `flow.c`) and mark packets with priority (`network.h` and `network.c`). So you need the kernel headers to compile it:  

<pre><code>$ cd pias3<br/>
$ make</code></pre>

Then you can get a kernel module called `pias.ko`. Note that the anti-starvation mechanism (`DANTI_STARVATION` in Makefile) is enabled by default. Under such setting, for a flow experiencing several consecutive TCP timeouts, we will reset its bytes sent information back to 0. You can also modify Makefile to disable it. 

###2.1.2 Installing 
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

###2.1.3 Usage
PIAS packet tagging module exports two types of configurations interfaces: a sysfs file to control flow table and several sysctl interfaces to configure priority parameters (see `params.h` for their definitions).

To print the information of all flows in current flow table:
<pre><code>$ echo -n print > /sys/module/pias/parameters/param_table_operation<br/>
$ dmesg|tail<br/>
PIAS: current flow table<br/>
PIAS: flowlist 136<br/>
PIAS: flow record from 192.168.101.11:60410 to 192.168.101.12:5001, bytes_sent=481926280, seq=2365093177, ACK=2364897698<br/>
PIAS: flowlist 160<br/>
PIAS: flow record from 192.168.101.11:60411 to 192.168.101.12:5001, bytes_sent=578661368, seq=3795198569, ACK=3794930690<br/>
PIAS: flowlist 184<br/>
PIAS: flow record from 192.168.101.11:60412 to 192.168.101.12:5001, bytes_sent=385470656, seq=3374872543, ACK=3374742224<br/>
PIAS: flowlist 208<br/>
PIAS: flow record from 192.168.101.11:60413 to 192.168.101.12:5001, bytes_sent=433638376, seq=215506294, ACK=215310815<br/>
PIAS: there are 4 flows in total<br/>
</code></pre>

To clear all the information in current flow table:
<pre><code>$ echo -n clear > /sys/module/pias/parameters/param_table_operation<br/>
</code></pre>

To show the DSCP value of highest priority:
<pre><code>$ sysctl pias.PIAS_PRIO_DSCP_1<br/>
pias.PIAS_PRIO_DSCP_1 = 7
</code></pre>

To set the first demoting threshold to 50KB:
<pre><code>$ sysctl -w pias.PIAS_PRIO_THRESH_1=51200
</code></pre>

##2.2 Switch Configuration

PIAS enforces strict priority queueing and ECN on switches. We classify packets based on the DSCP field. We observe that some of today’s commodity switching chips offer multiple ways (e.g. per-queue and per-port) to configure ECN marking when configured to use multiple queues per port. We recommend to use per-port ECN. In our experiments, we use Pica8 P-3295 Gigabit switches (<http://www.pica8.com/documents/pica8-datasheet-48x1gbe-p3290-p3295.pdf>). If you also use Pica8 Gigabit switches, just contact me and I will share you with my configuration.

In addition to in-network switch queueing delay, sender NIC also introduces latency because it is actually the first contention point of the network fabric. To solve this problem, I also develop a Linux queueing discipline (`qdisc`) kernel module to perform PIAS packet scheduling and ECN marking at end hosts. The source codes of this qdisc module are in `sch_pias` directory.   

###2.2.1 Compiling and Installing
I have only tested this qdisc module in Linux kernel 3.18.9. If you want to port this ti older kernels, you may need to modify source codes. Note that `sch_pias` is a drop-in replacement for `tbf`, you will have to remove `tbf` from a running kernel before you use `sch_pias`. 

<pre><code>$ rmmod sch_tbf<br/>
$ cd sch_pias<br/>
$ insmod sch_pias.ko<br/>
$ tc qdisc add dev eth1 root tbf rate 995mbit limit 1000k burst 1000k mtu 66000 peakrate 1000mbit<br/>
</code></pre>


##2.3 Rate Control
