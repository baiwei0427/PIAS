import os
import sys
import string

#This function is to print usage of this script
def usage():
	sys.stderr.write('result.py [file]\n')

#Return a normalized time
#pkt: number of packets in this flow
#time: absolute time
def fct(pkt,time):
	opt=0.0
	if pkt<=12:
		opt=0.000082+pkt*0.0000012
	elif pkt<=36:
		opt=2*0.000082+(pkt-12)*0.0000012
	elif pkt<=84:
		opt=3*0.000082+(pkt-36)*0.0000012
	else:
		opt=3*0.000082+(pkt-84)*0.0000012
	return time/opt

#Get average FCT
def avg(flows):
	sum=0.0
	for f in flows:
		sum=sum+f
	return sum/len(flows)

#GET mean FCT
def mean(flows):
	flows.sort()
	return flows[50*len(flows)/100]
	
#GET 99% FCT
def max(flows):
	flows.sort()
	return flows[95*len(flows)/100-1]
	
	
if len(sys.argv)==2:
	file=sys.argv[1]

	#All the flows
	flows=[]
	
	#Mice flows (0,10KB)
	mice_flows=[]
	#Short flows (10KB,100KB)
	short_flows=[]
	#Large flows (10MB,)
	large_flows=[]
	#Median flows (100KB, 10MB)
	median_flows=[]
	#The number of total timeouts
	timeouts=0
	
	fp = open(file)
	#Get overall average normalized FCT
	while True:
		line=fp.readline()
		if not line:
			break
		pkt_size=int(float(line.split()[0]))
		byte_size=float(pkt_size)*1460
		time=float(line.split()[1])
		result=fct(pkt_size,time)
		if result<1:
			print str(pkt_size)+" "+str(result)
		flows.append(result)
		
		#If there are TCP timeouts
		if int(line.split()[2])>0:
			#print line.split()[2]
			#timeouts+=1
			timeouts+=int(line.split()[2])
		
		#If the flow is a mice flow (0, 10KB)
		if byte_size<=10*1000:
			mice_flows.append(result)
		#If the flow is a short flow (10KB,100KB)
		elif byte_size<=100*1000:
			short_flows.append(result)
		#If the flow is a median flow (100KB,10MB)
		elif byte_size<=10*1000*1000:
			median_flows.append(result)
		#If the flow is a large flow (10MB,)
		else:
			large_flows.append(result)
		
	fp.close()
	print "There are "+str(len(flows))+" flows in total"
	print "There are "+str(timeouts)+" TCP timeouts in total"
	print "Overall average normalized FCT: "+str(avg(flows))
	print "Average normalized FCT (0,10KB): "+str(len(mice_flows))+" flows "+str(avg(mice_flows))
	print "95th percentile normalized FCT (0,10KB): "+str(max(mice_flows))
	print "Average normalized FCT (10KB,100KB): "+str(len(short_flows))+" flows "+str(avg(short_flows))
	print "95th percentile normalized FCT (10KB, 100KB): "+str(max(short_flows))
	print "Average normalized FCT (100KB,10MB): "+str(len(median_flows))+" flows "+str(avg(median_flows))
	print "95th percentile normalized FCT (100KB, 10MB): "+str(max(median_flows))
	print "Average normalized FCT (10MB,): "+str(len(large_flows))+" flows "+str(avg(large_flows))
	print "95th percentile normalized FCT (10MB,): "+str(max(large_flows))
