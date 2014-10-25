PIAS
==========

Practical Information-Agnostic Flow Scheduling for Datacenter Networks

Keywords
==========

Linux Kernel, Netfilter, ECN, Scheduling 

People
==========

Wei Bai (baiwei0427@gmail.com)

Department of Computer Science and Engineering

Hong Kong University of Science and Technology

How to use
==========

PIAS is implemented as a Linux Netfilter kernel module to maintain flow states and mark packets at end hosts. Please visit https://github.com/baiwei0427/PIAS/tree/master/PIAS to download source codes of PIAS kernel module. The key idea of PIAS kernel module is quite simple: maintaing flow states for active flows in a hash table and modify DSCP field of IP header based on the bytes sent information of the flow and priority settings.

Currently, I am still improving PIAS in following aspects:

1. Implement probe approach by Netfiler kernel module or hacking TCP stack. Maybe there are some other ways to deal with startvation problem (e.g. use weighted fair queueing rather than strict priority queueing for PIAS).

2. Implement PIAS qdisc in Linux tc or hardware (e.g. NetFPGA) 





