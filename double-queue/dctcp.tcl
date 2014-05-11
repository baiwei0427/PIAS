set nodenum [lindex $argv 0]
set ftp_sentsize  [lindex $argv 1]
set buffersize_limit [lindex $argv 2]
set buffersize 10000
set RTT 0.0001
set threshhold 20
set tracedir tcp_flow

set simulationTime 1.0
set switchAlg RED
set lineRate 1Gb

set DCTCP_g_ 0.5
set ackRatio 1
set packetSize 1460
set traceSamplingInterval 0.0001
set enableNAM 0

set ns [new Simulator]

Agent/TCP set syn_ false
Agent/TCP set ecn_ 1
Agent/TCP set old_ecn_ 1
Agent/TCP set packetSize_ $packetSize
Agent/TCP set windows_ 125
Agent/TCP set slow_start_restart_ false
Agent/TCP set tcpTick_ 0.01
Agent/TCP set minrto_ 0.2
Agent/TCP set windowOption_ 0
Agent/TCP set dctcp_ true
Agent/TCP set dctcp_g_ $DCTCP_g_
Agent/TCP/FullTcp set segsize_ $packetSize
Agent/TCP/FullTcp set segsperack_ 1
Agent/TCP/FullTcp set spa_thresh_ 3000
Agent/TCP/FullTcp set interval_ 0
Agent/TCP/FullTcp set ecn_syn_ true

Queue set limit_ 100
Queue/RED set bytes_ false
Queue/RED set queue_in_bytes_ true
Queue/RED set mean_pktsize_ $packetSize
Queue/RED set setbit_ true
Queue/RED set gentle_ false
Queue/RED set q_weight_ 1.0
Queue/RED set mark_p_ 1.0
Queue/RED set thresh_ [expr $threshhold]
Queue/RED set maxthresh_ [expr $threshhold]

DelayLink set avoidReordering_ true
set mytracefile [open mytracefile.tr w]
$ns trace-all $mytracefile

if {$enableNAM != 0} {
	set namfile [open out.nam w]
	$ns namtrace-all $namfile
}

proc finish {} {
	global ns enableNAM namfile mytracefile
	$ns flush-trace
    close $mytracefile
    if {$enableNAM != 0} {
        close $namfile
        exec nam out.nam &
    }
    exit 0
}
for {set i 0} {$i < $nodenum} {incr i} {
	set n($i) [$ns node]
}
set nqueue [$ns node]
set nclient [$ns node]
$nqueue color red
$nqueue shape box
$nclient color blue

for {set i 0} { $i < $nodenum} {incr i} {
	$ns duplex-link $n($i) $nqueue $lineRate [expr $RTT/4] DropTail
	$ns duplex-link-op $n($i) $nqueue queuePos 0.25
}


$ns simplex-link $nqueue $nclient $lineRate [expr $RTT/4] $switchAlg
$ns simplex-link $nclient $nqueue $lineRate [expr $RTT/4] DropTail
$ns queue-limit $nqueue $nclient $buffersize_limit
$ns queue-limit $nclient $nqueue $buffersize

for {set i 0} { $i < $nodenum} {incr i} {
	$ns queue-limit $n($i) $nqueue $buffersize
	$ns queue-limit $nqueue $n($i) $buffersize
}

for {set i 0} {$i < 5 } {incr i} {
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
	$ns at 0.1 "$ftp($i) start"
	$ns at 1.1 "$ftp($i) end"
}

for {set i 5} {$i < $nodenum} {incr i} {
    #set tcp($i) [new Agent/TCP/Newreno]
    set tcp($i) [new Agent/TCP/FullTcp/Sack]
    #set sink($i) [new Agent/TCPSink]
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
	set randsendsize [expr rand() * $ftp_sentsize]
    $ns at [expr 0.1+0.001*$i] "$ftp($i) send $ftp_sentsize"
}
$ns at $simulationTime "finish"
$ns run
