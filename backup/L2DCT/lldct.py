import os
import sys
import string

#This function is to print usage of this script
def usage():
	sys.stderr.write('lldct.py [nodes] [data] [enable LLDCT]\n')

#This function is to calculate FCT for a TCP flow
def fct(file):
	fp = open(file)
	start=''
	end=''
	while True:
		line=fp.readline()
		if not line:
			break
		if len(start)==0 and not '0.00000' in line and 'cwnd' in line:
			start=line
		if len(start)>0:
			end=line
	fp.close()
	starttime=start.split()[0]
	endtime=end.split()[0]
	return float(endtime)-float(starttime)
	
if len(sys.argv)==4: 
	ns_path='/home/wei/ns-allinone-2.34/ns-2.34/ns'
	tcl_path='/home/wei/lldct/lldct-test.tcl'
	nodes=sys.argv[1]
	data=sys.argv[2]
	enable=sys.argv[3]
	os.system('rm ./tcp_flow -r\n')
	os.system('mkdir tcp_flow')
	cmd=ns_path+' '+tcl_path+' '+nodes+' '+data+' '+enable
	os.system(cmd)
	
	fcts=0.0
	flows=0
	#Get FCT results
	for file in os.listdir("./tcp_flow"):
		fcts=fcts+fct('./tcp_flow/'+file)
		flows=flows+1
	print fcts/flows
	
else:
	usage()
