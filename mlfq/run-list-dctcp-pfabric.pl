#!/usr/bin/perl -w
#$sim_end = 100000; #num of flows
$sim_end = 100000; #num of flows

$cap = 10;
$link_delay = 0.0000002;
$host_delay = 0.000020; 

@queueSize = (160);
@min_rto = (0.000200);

@load = (0.8);
$connections_per_pair = 1;
$meanFlowSize = 1138 * 1460;
@paretoShape = (1.05);
$enableMultiPath = 1;

@perflowMP = (0);  #Shuang!

@sourceAlg = ("DCTCP-Sack");
@ackratio = (1); #(1,12)
@slowstartrestart = ("true");
$DCTCP_g = 1.0/16.0;
#$switchAlg = "RED";
$switchAlg = "DropTail";
@drop_prio_ = ("true");
@deque_prio_ = ("true");
@prio_scheme_ = (2);
@prob_mode_ = (5);
@keep_order_ = ("true");

@enablePacer = (0);
@TBFsPerServer = (1);
@Pacer_qlength_factor = (3000); #used to be 750
$Pacer_rate_ave_factor = 0.125;
#$Pacer_rate_update_interval = 0.0000072;
@Pacer_rate_update_interval = (0.0000072);
@Pacer_assoc_prob = (1.0/8);
@Pacer_assoc_timeout = (0.001);

@PQ_mode = (0);
@DCTCP_K = (10000);
@enablePQ = (0);
@PQ_gamma = (0);
@PQ_thresh = (0);

@topology_spt = (16); #server per tor
@topology_tors = (9);
@topology_spines = (4);
@topology_x = (1);

$enableNAM = 0;

$numcores = 7;

$trial = 1;

###########################################
$user = "wei";
###########################################
$tcl_script = "spine_empirical_dctcp";
$top_dir = "/home/$user";
$work_dir = "/home/$user/mlfq";
###########################################
# ns source code server
$ns_source_server = "localhost";
# ns source code path
$ns_source_path = "/home/$user"; # we expect tree /ns-allinone-2.34 to be at this directory
                                       # for now, this also needs to be the same as $top_dir
###########################################
# worker server List
@server_list = ("localhost");
$server_loop_first = 1;  # First loop across servers, than cores
###########################################
# log destination server
$log_server = "localhost";
$log_dir = "$top_dir/log_test";
###########################################
# Prepare servers 
###########################################
#print "compressing ns...\n";
#`cd $ns_source_path && tar cvzf ns-latest.tar.gz ns-allinone-2.34 >> /dev/null`;

#print "backing up ns at parent folder...\n";
#`cp $ns_source_path/ns-latest.tar.gz ..`;
    
$ns_path ="$top_dir/ns-allinone-2.34/bin:$top_dir/ns-allinone-2.34/tcl8.4.18/unix:$top_dir/ns-allinone-2.34/tk8.4.18/unix:\$PATH";
$ld_lib_path = "$top_dir/ns-allinone-2.34/otcl-1.13:$top_dir/ns-allinone-2.34/lib";
$tcl_lib = "$top_dir/ns-allinone-2.34/tcl8.4.18/library";
$set_path_cmd = "export PATH=$ns_path && export LD_LIBRARY_PATH=$ld_lib_path && export TCL_LIBRARY=$tcl_lib";

foreach (@server_list) {
    $server = $_;
    if ($server ne $ns_source_server) {
	print "copying and decompressing ns at $server...\n"; 
	`cp ../ns-latest.tar.gz $user\@$server:$top_dir/.`;
	`cd $top_dir && tar xvzf ns-latest.tar.gz >> /dev/null`;
    } else {
	print "skipping copy/decompress ns for $server...\n";
    }
    `killall ns`;
    sleep(1);
    `rm -rf $work_dir/proc*`;       
}


#unless (-d "./logs/") {
#    `mkdir logs`;
#}

`mkdir $log_dir`;
`mkdir $log_dir/logs`;


$sleep_time = 10;
##########################################
# Run tests
##########################################
foreach (@load) {
    $cur_load = $_;
    foreach(@paretoShape) {
	$cur_paretoShape = $_;
      for ($i = 0; $i < @topology_spt; $i++) {
	$cur_topology_spt = @topology_spt[$i];
	$cur_topology_tors = @topology_tors[$i];
	$cur_topology_spines = @topology_spines[$i];
	$cur_topology_x = $topology_x[$i];
	foreach (@perflowMP) {
	    $cur_perflowMP = $_;
	    foreach (@queueSize) {
		$cur_queueSize = $_;
		foreach (@sourceAlg) {
		    $cur_sourceAlg = $_;
		    foreach(@enablePacer) {
			$cur_enablePacer = $_;					      
			foreach (@ackratio) {
			    $cur_ackratio = $_;			
			    foreach(@slowstartrestart) {
				$cur_slowstartrestart = $_;

			    foreach(@Pacer_rate_update_interval) {
				$cur_Pacer_rate_update_interval = $_;		      
				if (($cur_enablePacer == 0) && ($cur_Pacer_rate_update_interval != $Pacer_rate_update_interval[0])) {
						    next;
				}
				foreach(@Pacer_qlength_factor) {
				    $cur_Pacer_qlength_factor = $_;		      
				    if (($cur_enablePacer == 0) && ($cur_Pacer_qlength_factor != $Pacer_qlength_factor[0])) {
						    next;
				}
				    foreach(@Pacer_assoc_prob) {
					$cur_Pacer_assoc_prob = $_;		      
					if (($cur_enablePacer == 0) && ($cur_Pacer_assoc_prob != $Pacer_assoc_prob[0])) {
						    next;
				}
					foreach(@Pacer_assoc_timeout) {
					    $cur_Pacer_assoc_timeout = $_;
					    if (($cur_enablePacer == 0) && ($cur_Pacer_assoc_timeout != $Pacer_assoc_timeout[0])) {
						    next;
				}
					    foreach(@TBFsPerServer) {
						$cur_TBFsPerServer = $_;				    			      if (($cur_enablePacer == 0) && ($cur_TBFsPerServer != $TBFsPerServer[0])) {
						    next;
				}
						foreach(@min_rto) {
							$cur_rto = $_;

						
			    for ($i = 0; $i < @DCTCP_K; $i++) {
				$cur_PQ_mode = $PQ_mode[$i];
				$cur_DCTCP_K = $DCTCP_K[$i];
				$cur_enablePQ = $enablePQ[$i];
				$cur_PQ_gamma = $PQ_gamma[$i];
				$cur_PQ_thresh = $PQ_thresh[$i];
				
				#if (($cur_enablePQ == 0) && ($cur_enablePacer == 1 || $cur_slowstartrestart eq "true")) {
				#    next;
				#}

				#if ($cur_load == 0.05) {
				#    $sim_end = 60;
				#} elsif ($cur_load == 0.15) {
				#    $sim_end = 20;
				#} elsif ($cur_load == 0.25) {
				#    $sim_end = 12;
				#}
				foreach($dp = 0; $dp < @drop_prio_; $dp++)  {
					$cur_drop_prio = $drop_prio_[$dp];
					$cur_prio_scheme = $prio_scheme_[$dp];
					$cur_deque_scheme = $deque_prio_[$dp];

				foreach($prob = 0; $prob < @prob_mode_; $prob++) {
					$cur_prob_cap = $prob_mode_[$prob];
				
				foreach($ko = 0; $ko < @keep_order_; $ko++) {
					$cur_ko = $keep_order_[$ko];


				$arguments = "$sim_end $cap $link_delay $host_delay $cur_queueSize $cur_load $connections_per_pair $meanFlowSize $cur_paretoShape $enableMultiPath $cur_perflowMP $cur_sourceAlg $cur_ackratio $cur_slowstartrestart $DCTCP_g $switchAlg $cur_DCTCP_K $cur_enablePQ $cur_PQ_mode $cur_PQ_gamma $cur_PQ_thresh $cur_enablePacer $cur_TBFsPerServer $cur_Pacer_qlength_factor $Pacer_rate_ave_factor $cur_Pacer_rate_update_interval $cur_Pacer_assoc_prob $cur_Pacer_assoc_timeout $cur_topology_spt $cur_topology_tors $cur_topology_spines $cur_topology_x $enableNAM $cur_rto $cur_drop_prio $cur_prio_scheme $cur_deque_scheme $cur_prob_cap $cur_ko";
				
				if ($trial < 10) {
				    $strial = "00$trial";
				} elsif ($trial < 100) {
				    $strial = "0$trial";
				} else {
				    $strial = $trial;
				}

				$dir_name = "$strial-$tcl_script-s$cur_topology_spt-x$cur_topology_x-q$cur_queueSize-load$cur_load-avesize$meanFlowSize-shape$cur_paretoShape-mp$cur_perflowMP-$cur_sourceAlg-ar$cur_ackratio-SSR$cur_slowstartrestart-$switchAlg$cur_DCTCP_K-PQ$cur_enablePQ-mode$cur_PQ_mode-gamma$cur_PQ_gamma-thresh$cur_PQ_thresh-Pacer$cur_enablePacer-minrto$cur_rto-drop$cur_drop_prio-prio$cur_prio_scheme-dq$cur_deque_scheme-prob$cur_prob_cap-ko$cur_ko";
#p$cur_Pacer_assoc_prob-beta$cur_Pacer_qlength_factor-T$cur_Pacer_rate_update_interval-TBF$cur_TBFsPerServer";
			    
				print "args $arguments\n";
				print "dir_name $dir_name \n";
			    
				++$trial;
				
				$found = 0;
				$proc_index = 0;
				$server_index = 0;
				
				while (!$found) {
				    
				    $server = $server_list[$server_index];				                  
				    $target_dir = "$work_dir/proc$proc_index";
				    
				    #print "checking availability of $target_dir at $server\n";
				
				    `test -e $target_dir/logFile.tr`;
				    $rc = $? >> 8;
				
				    if ($rc) {		
					# target_dir/logFile.tr doesn't exist
					$found = 1;
				    
					`test -d $target_dir/`;
					$rc = $? >> 8;
				    
					if ($rc) {
					    # target_dir doesn't exist
					    `mkdir $target_dir`;
					} else {
					    wait();
					}
					`rm -f $target_dir/*`;
					last;
				    }

				    if ($server_loop_first == 1) {

					$server_index++;
					if ($server_index == @server_list) {
					    $server_index = 0;
					    $proc_index++;
					    if ($proc_index == $numcores) {
						$proc_index = 0;
						print "sleeping $sleep_time sec..\n";
						sleep($sleep_time);								  }
					}
				    } else {
					$proc_index++;
					if ($proc_index == $numcores) {
					    $proc_index = 0;		       	
					    $server_index++;
					    if ($server_index == @server_list) {
						$server_index = 0;
						print "sleeping $sleep_time sec..\n";
						sleep($sleep_time);
					    }
					}
				    }
				}
				
				print "starting $dir_name process on $server at $target_dir\n";
				
				`cp *.tcl $target_dir`;
				
				$pid = fork();
				die "unable to fork: $!" unless defined($pid);
				if (!$pid) {  # child
					print "start $pid\n";	
#`$set_path_cmd && cd $target_dir && nice ns $tcl_script.tcl $arguments > logFile.tr`;
				    `/home/wei/ns-allinone-2.34/ns-2.34/ns $tcl_script.tcl $arguments > logFile.tr`;
					`mkdir flow_$cur_load`;
					`mv *.tr flow_$cur_load`;
				    print "$dir_name (proc$proc_index) on $server finished. cleaning up...\n";
				    exit(0);
				} else {			
				    #print "started ns with index $proc_index and pid $pid\n";			
				    sleep(1);
				} }	   } } }
			    }
			    }
			    }
			    }			    
			    }
			    }
			}
		    }
		}
	    }
	}
    }
}
}
}

print "waiting for child procs to terminate.\n";

while (wait() != -1) {
    print "waiting for child procs to terminate.\n";
}


#######################################################
# Create log folder
#######################################################
($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst)  = localtime(time);
$mon++;


#`mv $log_dir/logs $log_dir/$mon$mday-$tcl_script`;

######################################################
# Cleanup servers
######################################################
foreach (@server_list) {
    $server = $_;
    print "cleaning up $server...\n";
#`rm -rf $work_dir/proc*`;

}

print "Done\n";
