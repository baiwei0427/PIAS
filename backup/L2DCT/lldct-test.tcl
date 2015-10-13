set node [lindex $argv 0]
set data_size [lindex $argv 1]
set enable_lldct [lindex $argv 2] 

set RTT 0.0001 
set DCTCP_K 65
#RED or MLFQ
set switchAlg MLFQ

set tracedir tcp_flow
set simulationTime 1.0
set lineRate 10Gb

set DCTCP_g 0.0625
set ackRatio 1
set pktSize 1460

set ns [new Simulator]

################# Transport Options ####################
Agent/TCP set ecn_ 1
Agent/TCP set old_ecn_ 1
Agent/TCP set packetSize_ $pktSize
Agent/TCP set window_ 10000
Agent/TCP set windowInit_ 10
Agent/TCP set slow_start_restart_ false
Agent/TCP set windowOption_ 0
Agent/TCP set tcpTick_ 0.001
Agent/TCP set minrto_ 0.001
Agent/TCP set maxrto_ 2

Agent/TCP/FullTcp set nodelay_ true; #Disable Nagle
Agent/TCP/FullTcp set segsperack_ $ackRatio
Agent/TCP/FullTcp set segsize_ $pktSize
Agent/TCP/FullTcp set spa_thresh_ 0

if {$ackRatio > 2} {
    Agent/TCP/FullTcp set spa_thresh_ [expr ($ackRatio - 1) * $pktSize]
}

################ DCTCP Options #########################
Agent/TCP set ecnhat_ true
Agent/TCPSink set ecnhat_ true
Agent/TCP set ecnhat_g_ $DCTCP_g

################ LLDCT Options #########################
Agent/TCP set lldct_ $enable_lldct; #Enable LLDCT
Agent/TCP set lldct_w_min_ 0.125
Agent/TCP set lldct_w_max_ 2.5
Agent/TCP set lldct_w_c_ 2.5

################ Queue Options #########################
Queue set limit_ 10000

################ Random Early Detection ################
Queue/RED set bytes_ false
Queue/RED set queue_in_bytes_ true
Queue/RED set mean_pktsize_ $pktSize
Queue/RED set setbit_ true
Queue/RED set gentle_ false
Queue/RED set q_weight_ 1.0
Queue/RED set mark_p_ 1.0
Queue/RED set thresh_ $DCTCP_K
Queue/RED set maxthresh_ $DCTCP_K

############# Multi-level Feedback Queue ##############
Queue/MLFQ set queue_num_ 1
Queue/MLFQ set thresh_ $DCTCP_K
Queue/MLFQ set dequeue_marking_ 0
Queue/MLFQ set mean_pktsize_ $pktSize

#################### NS2 Trace ########################
set mytracefile [open mytracefile.tr w]
$ns trace-all $mytracefile
proc finish {} {
	global ns mytracefile
	$ns flush-trace
    close $mytracefile
    exit 0
}

##################### Topology #########################
for {set i 0} {$i < $node} {incr i} {
	set n($i) [$ns node]
}
set nqueue [$ns node]
set nclient [$ns node]

for {set i 0} { $i < $node} {incr i} {
	$ns duplex-link $n($i) $nqueue $lineRate [expr $RTT/4] DropTail
	$ns duplex-link-op $n($i) $nqueue queuePos 0.25
	$ns queue-limit $n($i) $nqueue 10000; #End Host buffer size is very large
	$ns queue-limit $nqueue $n($i) 1000
}

$ns simplex-link $nqueue $nclient $lineRate [expr $RTT/4] $switchAlg
$ns simplex-link $nclient $nqueue $lineRate [expr $RTT/4] DropTail
$ns queue-limit $nqueue $nclient 1000
$ns queue-limit $nclient $nqueue 1000

################## Long-live Flows #######################
for {set i 0} {$i < 2 } {incr i} {
    set tcp($i) [new Agent/TCP/FullTcp/Sack]
    set sink($i) [new Agent/TCP/FullTcp/Sack]
    $sink($i) listen

    $ns attach-agent $n($i) $tcp($i)
    $ns attach-agent $nclient $sink($i)

	$tcp($i) set fid_ [expr $i]
    $sink($i) set fid_ [expr $i]
	
    $ns connect $tcp($i) $sink($i)
    set ftp($i) [new Application/FTP]
    $ftp($i) attach-agent $tcp($i)
    $ftp($i) set type_ FTP
	$ns at 0.1 "$ftp($i) start"
	$ns at $simulationTime "$ftp($i) stop"
}

################## Short Flows #######################
for {set i 2} {$i < $node} {incr i} {
    set tcp($i) [new Agent/TCP/FullTcp/Sack]
    set sink($i) [new Agent/TCP/FullTcp/Sack]
    $sink($i) listen

    $ns attach-agent $n($i) $tcp($i)
    $ns attach-agent $nclient $sink($i)

    #Record TCP logs
    $tcp($i) attach [open ./$tracedir/$i.tr w]
    $tcp($i) set bugFix_ false
    $tcp($i) trace cwnd_
    $tcp($i) trace ack_
    $tcp($i) trace ssthresh_
    $tcp($i) trace nrexmit_
    $tcp($i) trace nrexmitpack_
    $tcp($i) trace nrexmitbytes_
    $tcp($i) trace ncwndcuts_
    $tcp($i) trace ncwndcuts1_
    $tcp($i) trace dupacks_

	$tcp($i) set fid_ [expr $i]
    $sink($i) set fid_ [expr $i]
	
    $ns connect $tcp($i) $sink($i)
    set ftp($i) [new Application/FTP]
    $ftp($i) attach-agent $tcp($i)
    $ftp($i) set type_ FTP
    $ns at [expr 0.2+0.002*$i] "$ftp($i) send $data_size"
}

$ns at $simulationTime "finish"
$ns run



