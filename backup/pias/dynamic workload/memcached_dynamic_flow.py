import os
import sys
import string
import time
import random
import math
import threading

#This function is to print usage of this script
def usage():
	sys.stderr.write('This script is to generate memcached benchmark with backrgound traffic:\n')
	sys.stderr.write('dynamic_flow.py [host file] [backrgound trace file] [memcached query rate (number per second)] [result file] \n')

#Memcached query thread
class QueryThread(threading.Thread):
	def __init__(self):
		threading.Thread.__init__(self)
		
	def run(self):
		#By default, we have 15 memcached servers
		line=os.popen('./query.o 15').read()
		result=line.split()
		if len(result)>=2:
			#Get GCT (Group Completion Time) for this query and save it into global variable gcts
			gct=int(result[1])
			gcts.append(gct)
	
#Background flow thread	
class FlowThread(threading.Thread):
	def __init__(self, host, size, id):
		self.host=host
		self.size=size
		self.id=id
		threading.Thread.__init__(self)

	def run(self):
		#By default, we use port number of 5001. We don't need to get FCT for backrgound traffic
		line=os.popen('./client.o '+str(self.host)+' '+str(5001)+' '+str(self.size)).read()

#main function
if len(sys.argv)!=5:
	usage()
else:
	host_filename=sys.argv[1]
	trace_filename=sys.argv[2]
	rate=int(sys.argv[3])
	result_filename=sys.argv[4]
	
	#Get average time interval (ms) for query
	avg_interval=1000.0/rate;
	#Init number of queries in this emulation
	query_num=0
	#We maintain a currnet_interval for query. Once we find currnet_interval is larger than avg_interval, we will generate query. 
	current_interval=0
	
	#Get hosts from host file 
	hosts=[]
	host_file=open(host_filename,'r')
	lines=host_file.readlines()
	for line in lines:
		hosts.append(line.strip('\n'))
	host_file.close()
	
	#Maximum number of queries in an emulation
	max_query_num=1000
	#Group Completion Time
	global gcts
	gcts=[]
	
	#Sleep Interval
	times=[]
	#Flow Size
	flows=[]
	#Sender IP
	senders=[]
	
	#Get trace from trace file
	traces=[]
	trace_file=open(trace_filename,'r')
	traces=trace_file.readlines()
	
	for i in range(len(traces)):
		tmp=traces[i].split()
		if len(tmp)==3:
			times.append(float(tmp[0]))
			flows.append(int(tmp[1]))
			senders.append(hosts[int(tmp[2])])
		
	#Start emulation
	raw_input('Press any key to start emulation')
	
	for i in range(len(flows)):
		#Get next time interval (sleep time)
		sleeptime=times[i]
		#Get sender IP address
		host=senders[i]
		#Sleep
		time.sleep(sleeptime/1000)
		#Start thread to send traffic
		newthread=FlowThread(host,flows[i],i)
		newthread.start()
		
		if query_num<max_query_num:
			current_interval=current_interval+sleeptime
			#If current time interval is larger than average time interval for query
			if current_interval>avg_interval:
				#Generate query 
				query=QueryThread()
				query.start()
				#Update related variables for next query generatation
				query_num=query_num+1
				current_interval=current_interval-avg_interval
	
	#Stop emulation
	raw_input('Press any key to stop emulation')
	
	#Write GCT results into the file
	result_file=open(result_filename,'w')
	for gct in gcts:
		result_file.write(str(gct)+'\n')
	result_file.close()
	
	gcts.sort()
	if len(gcts)>0:
		print 'We generate '+str(len(gcts))+' queries in total \n'
		print 'The average GCT is '+str(sum(gcts)/len(gcts))+' us\n'
		print 'The 99th GCT is '+str(gcts[99*len(gcts)/100-1])+ 'us\n'
	
	

	