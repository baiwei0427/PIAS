#1 What is PIAS ?

PIAS, <strong>P</strong>ractical <strong>I</strong>nformation-<strong>A</strong>gnostic flow <strong>S</strong>cheduling, targets at minimizing flow completion times (FCT) for commodity data centers. As its heart, PIAS leverages multiple priority queues available in existing switches to perform Multiple Level Feedback Queue. A PIAS flow is gradually demoted from higher-priority queues to lower-priority queues based on the bytes it has sent. Therefore, short flows are likely to finish in the first few high-priority queues and prioritized over large flows in general. Given the traffic volatility in data centers, we also employ Explicit Congestion Notification (ECN) to keep PIAS's good performance in such a highly dynamic environment. 

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

In addition to in-network switch queueing delay, sender NIC also introduces latency because it is actually the first contention point of the network fabric. To solve this problem, I also develop a Linux queueing discipline (`qdisc`) kernel module to perform PIAS packet scheduling and ECN marking at end hosts. An unstable version of this qdisc module is in `sch_pias` directory.   

###2.2.1 Compiling and Installing
I have only tested this qdisc module in Linux kernel 3.18.9. If you want to port this ti older kernels, you may need to modify source codes. Note that `sch_pias` is a drop-in replacement for `tbf`, you will have to remove `tbf` from a running kernel before you use `sch_pias`. 

<pre><code>$ rmmod sch_tbf<br/>
$ cd sch_pias<br/>
$ make
$ insmod sch_pias.ko<br/>
$ tc qdisc add dev eth1 root tbf rate 995mbit limit 1000k burst 1000k mtu 66000 peakrate 1000mbit<br/>
$ dmesg|tail<br/>
sch_pias: start working<br/>
sch_pias: rate 995 Mbps<br/>
</code></pre>

Note that you can only use `tc` interface to control rate parameter for `sch_pias` (995mbit in above example). For the other parameters (see `params.h` and `params.c` for their definitions), you can use sysctl to configure them. For example, to set per-port ECN marking threshold to 30KB:

<pre><code>$ sysctl sch_pias.PIAS_QDISC_ECN_THRESH_BYTES=30720
</code></pre>

To remove this module
<pre><code>$ tc qdisc del dev eth1 root<br/>
$ rmmod sch_pias
$ dmesg|tail<br/><br/>
sch_pias: stop working<br/>
</code></pre>

###2.2.2 Usage
After installing `sch_pias`, start iperf and measure goodput. In my testbed, iperf can achieve 942 Mbps goodput at most. With `sch_pias`, it can achieve 936 Mbps goodput (~99.3% link utilization). We trade a little bandwidth to avoid queueing delay in NIC hardware.  

With iperf background traffic, you can send ping packets to measure RTT. If the ping packets have the same priority as iperf traffic, the RTT is around 379 us. If the ping packets have higher priority than iperf traffic (use `ping -Q` to set ToS value), the RTT can be reduced to 203 us.   

##2.3 Rate Control
For our NSDI experiments, we used the open source DCTCP patch for Linux 2.6.38.3 kernel provided by DCTCP authors (<http://simula.stanford.edu/~alizade/Site/DCTCP.html>). We also tested Linux 3.18.9 which was released recently. Our DCTCP setting of 2.6.38.3 kernel is shown as follows:
<pre><code>$ sysctl -w net.ipv4.tcp_congestion_control=reno<br/>   
$ sysctl -w net.ipv4.tcp_dctcp_enable=1<br/>            
$ sysctl -w net.ipv4.tcp_ecn=1<br/>                     
$ sysctl -w net.ipv4.tcp_delayed_ack=0<br/>             
$ sysctl -w net.ipv4.tcp_dctcp_shift_g=4 <br/>  
</code></pre>

In our experiments, we observed an undesirable interaction between the open-source DCTCP implementation and our switch. The DCTCP implementation does not set the ECN-capable (ECT) codepoint on TCP SYN packets and retransmitted packets, following the ECN standard. However, our switch drops any non-ECT packets from ECN-enabled queues, when the instant queue length is larger than the ECN marking threshold. This problem has been confirmed by many papers (<a href="http://research.microsoft.com/pubs/175520/conext20-wu.pdf">ECN*</a> and <a href="https://www.usenix.org/conference/nsdi15/technical-sessions/presentation/judd">Morgan Stanley paper</a>). Our solution is to set ECT on every TCP packet at the packet tagging module.

#3 NSDI 2015 Experiments
Under construction
