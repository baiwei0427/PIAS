import os
import sys
import string
import time
import random
import math

#http://preshing.com/20111007/how-to-generate-random-timings-for-a-poisson-process/
#Return next time interval (ms) 
def nextTime(rateParameter):
	#We use round function to get a integer
    return round(-math.log(1.0 - random.random()) / rateParameter)
	
#Given a flow distribution and a random number in (0,1), return a flow size (KB)
def flowsize(distribution, random):
	i=0
	for var in distribution:
		if var[0]>random:
			x1=distribution[i-1][0]
			x2=distribution[i][0]
			y1=distribution[i-1][1]
			y2=distribution[i][1]
			value=int(((y2-y1)/(x2-x1)*random+(x2*y1-x1*y2)/(x2-x1))*1448/1024)
			return value
		elif var[0]==random:
			return int(var[1]*1448/1024)
		else:
			i=i+1
	#By default, return 0
	return 0
	
#This function is to print usage of this script
def usage():
	sys.stderr.write('This script is used to generate dynamic flow workloads for PIAS project:\n')
	sys.stderr.write('static_flow.py [host file]  [traffic distribution file] [load] [link capacity (Mbps)] [flow number] [result file] \n')

#main function
if len(sys.argv)!=7:
	usage()
else:
	host_filename=sys.argv[1]
	traffic_filename=sys.argv[2]
	load=float(sys.argv[3])
	capacity=float(sys.argv[4])
	flow_num=int(sys.argv[5])
	result_filename=sys.argv[6]

	#Initialize hosts (list)
	hosts=[]
	#Get host line by line from input file 
	host_file=open(host_filename,'r')
	lines=host_file.readlines()
	for line in lines:
		hosts.append(line.strip('\n'))
	host_file.close()
	
	#Initialize flow distribution 
	distribution=[]
	#Get traffic distribution line by line from input file
	traffic_file=open(traffic_filename,'r')
	lines=traffic_file.readlines()
	i=0
	for line in lines:
		tmp=line.split()
		if len(tmp)>=2:
			distribution.append([float(tmp[1]),float(tmp[0])])
			i=i+1
	traffic_file.close()
	
	#Get average throughput
	throughput=load*capacity
	
	#Generate flow size
	flows=[]
	for i in range(flow_num):
		flows.append(flowsize(distribution,random.random()))
	
	#Get average flow size
	avg=sum(flows)/len(flows)
	num=throughput*1024*1024/(avg*1024*8) #number of requests per second 
	
	#Print information
	print 'Auto generate '+str(len(flows))+' flows:'
	print 'The average flow size: '+str(avg)+' KB'
	print 'The average request speed: '+str(num)+' requests/second'
	print 'Dynamic flow emulation will last for about '+str(len(flows)/num)+' seconds'
	rate=num/1000 #number of requests every 1000 ms
	
	#Start emulation
	raw_input('Press any key to continue')
	for i in range(len(flows)):
		#Get next time interval (sleep time)
		sleeptime=nextTime(rate) 
		#Get host id
		host=hosts[random.randint(0,len(hosts)-1)]
		time.sleep(sleeptime/1000)
		print 'Sleep '+str(sleeptime)+'ms. Get '+str(flows[i])+' KB data from '+host