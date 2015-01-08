PIAS
==========

Data Center Flow Scheduling 

Keywords
==========

Linux Kernel, Netfilter, ECN, Scheduling 

People
==========

Wei Bai (baiwei0427@gmail.com)

Department of Computer Science and Engineering

Hong Kong University of Science and Technology

Papers
==========
PIAS in HotNets 2014  

http://conferences.sigcomm.org/hotnets/2014/papers/hotnets-XIII-final91.pdf

PIAS technical report 

http://sing.cse.ust.hk/~wei/pias-tr.pdf

How to use
==========

PIAS is implemented as a Linux Netfilter kernel module to maintain flow states and mark packets at end hosts. Please visit https://github.com/baiwei0427/PIAS/tree/master/PIAS to download source codes of PIAS kernel module. The key idea of PIAS kernel module is quite simple: maintaing flow states for active flows in a hash table and modifying DSCP field of IP header based on the bytes sent information of the flow and priority settings. In addition, PIAS also requires configuraing strict priority queueing and per-port ECN on switches. 

Currently, I am still improving PIAS in following two aspects:

1.Implement probe approach by Netfiler kernel module or hacking TCP stack. Maybe there are some other ways to deal with startvation problem (e.g. use weighted fair queueing rather than strict priority queueing for PIAS).

2.Implement PIAS qdisc in Linux tc or hardware (e.g. NetFPGA) 





