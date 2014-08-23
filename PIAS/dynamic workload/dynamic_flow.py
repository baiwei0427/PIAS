import os
import sys
import string
import time
import random
import math
import threading

#This function is to print usage of this script
def usage():
	sys.stderr.write('This script is used for dynamic flow emulation:\n')
	sys.stderr.write('dynamic_flow.py [host file] [trace file] [result file] \n')
	
class FlowThread(threading.Thread):
	def __init__(self, host, size, id):
		self.host=host
		self.size=size
		self.id=id
		threading.Thread.__init__(self)

	def run(self):
		#By default, we use port number of 5001
		line=os.popen('./client.o '+str(self.host)+' '+str(5001)+' '+str(self.size)).read()
		result=line.split()
		if len(result)>=2:
			#Get FCT for this flow and save it into global variable fcts
			fct=int(result[len(result)-2])
			fcts[self.id]=fct
	
#main function
if len(sys.argv)!=4:
	usage()
else:
	host_filename=sys.argv[1]
	trace_filename=sys.argv[2]
	result_filename=sys.argv[3]
	
	#Get hosts from host file 
	hosts=[]
	host_file=open(host_filename,'r')
	lines=host_file.readlines()
	for line in lines:
		hosts.append(line.strip('\n'))
	host_file.close()
	
	#Flow Completion Time
	global fcts
	fcts=[]
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
			fcts.append(0)
		
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
	
	#Stop emulation
	raw_input('Press any key to stop emulation')
	
	#Analyze FCT results
	tiny_fcts=[]
	short_fcts=[]
	long_fcts=[]
	
	#Write FCT results into the file
	result_file=open(result_filename,'w')
	for i in range(len(flows)):
		#If flow size is smaller than 10KB
		if flows[i]<=10:
			tiny_fcts.append(fcts[i])
		#If flow size is in (10KB,100KB)
		elif flows[i]<=100:
			short_fcts.append(fcts[i])
		#If flow size is larger than 10MB
		elif flows[i]>=10000:
			long_fcts.append(fcts[i])
			
		result_file.write(str(flows[i])+' KB '+str(fcts[i])+' us\n')
	result_file.close()
	
	if len(tiny_fcts)>0:
		print 'There are '+str(len(tiny_fcts))+' flows in (0,10KB). Their average FCT is '+str(sum(tiny_fcts)/len(tiny_fcts))+' us'
	if len(short_fcts)>0:
		print 'There are '+str(len(short_fcts))+' flows in (10KB,100KB). Their average FCT is '+str(sum(short_fcts)/len(short_fcts))+' us'
	if len(long_fcts)>0:
		print 'There are '+str(len(long_fcts))+' flows in (10MB,). Their average FCT is '+str(sum(long_fcts)/len(long_fcts))+' us'
	