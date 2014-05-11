set node [lindex $argv 0]
set data_size  [lindex $argv 1]

#Base RTT-100us
set RTT 0.0001 
#ECN marking threshold=20 packets
set threshhold 20

#Directory to save each TCP flow's information
set tracedir tcp_flow
set simulationTime 3.0
set lineRate 1Gb



