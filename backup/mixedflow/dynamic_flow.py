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
	#host: sender IP address
	#size: flow size (KB)
	#id: flow id
	#type: flow type (1,2 or 3）
	#deadline: deadline time (ms) for type 1 flows
	def __init__(self, host, size, id, type, deadline):
		self.host=host
		self.size=size
		self.id=id
		self.type=type
		self.deadline=deadline
		threading.Thread.__init__(self)

	def run(self):
		is_agnostic=1
		#For Type 1 and 2 flows, we need to deliver flow information to kernel space 
		if self.type==1 or self.type==2:
			is_agnostic=0
		#By default, we use port number of 5001
		line=os.popen('./client.o '+str(self.host)+' '+str(5001)+' '+str(self.size)+' '+str(self.deadline)+' '+str(self.is_agnostic)).read()
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
	#Flow Type
	types=[]
	#Deadlines
	deadlines=[]
	#Sender IP
	senders=[]
	
	#Get trace from trace file
	traces=[]
	trace_file=open(trace_filename,'r')
	traces=trace_file.readlines()
	
	for i in range(len(traces)):
		tmp=traces[i].split()
		if len(tmp)==5:
			times.append(float(tmp[0]))
			flows.append(int(tmp[1]))
			types.append(int(tmp[2]))
			deadlines.append(int(tmp[3]))
			senders.append(hosts[int(tmp[4])])
			fcts.append(0)
		
	#Start emulation
	raw_input('Press any key to start emulation')
	
	for i in range(len(flows)):
		#Get next time interval (sleep time)
		sleeptime=times[i]
		#Get sender IP address
		host=senders[i]
		#Get flow type
		#Get deadline
		#Sleep
		time.sleep(sleeptime/1000)
		#Start thread to send traffic
		newthread=FlowThread(host,flows[i],i)
		newthread.start()
	
	#Stop emulation
	raw_input('Press any key to stop emulation')
	
	#Analyze FCT results
	short_fcts=[]
	medium_fcts=[]
	long_fcts=[]
	
	#Write FCT results into the file
	result_file=open(result_filename,'w')
	for i in range(len(flows)):
		#If flow size is smaller than 10KB
		if flows[i]<=100:
			tiny_fcts.append(fcts[i])
		#If flow size is in (10KB,100KB)
		elif flows[i]<=10000:
			medium_fcts.append(fcts[i])
		#If flow size is larger than 10MB
		else:
			long_fcts.append(fcts[i])
			
		result_file.write(str(flows[i])+' KB '+str(fcts[i])+' us\n')
	result_file.close()
	
	if len(tiny_fcts)>0:
		print 'There are '+str(len(tiny_fcts))+' flows in (0,10KB). Their average FCT is '+str(sum(tiny_fcts)/len(tiny_fcts))+' us'
	if len(short_fcts)>0:
		print 'There are '+str(len(short_fcts))+' flows in (10KB,100KB). Their average FCT is '+str(sum(short_fcts)/len(short_fcts))+' us'
	if len(long_fcts)>0:
		print 'There are '+str(len(long_fcts))+' flows in (10MB,). Their average FCT is '+str(sum(long_fcts)/len(long_fcts))+' us'
	