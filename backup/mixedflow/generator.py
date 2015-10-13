import os
import sys
import string
import time
import random
import math

#This function is to print usage of this script
def usage():
	sys.stderr.write('This script is used to generate dynamic flow workloads for mixedflow project:\n')
	sys.stderr.write('generator.py [percentage1] [percentage2] [percentage3] [CDF file 1] [CDF file 2] [CDF file 3] [load] [link capacity (Mbps)] [flow number] [host number] [output file]\n')

def check_input(p1,p2,p3):
	if p1>=0 and p2>=0 and p3>=0 and (p1+p2+p3)==1:
		return True
	else:
		return False
	
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

#main function
if len(sys.argv)!=12:
	usage()
else:
	p1=float(sys.argv[1])
	p2=float(sys.argv[2])
	p3=float(sys.argv[3])
	
	if check_input(p1,p2,p3)==False:
		print 'Illegal percentages'
		sys.exit()
		
	cdf_filename1=sys.argv[4]
	cdf_filename2=sys.argv[5]
	cdf_filename3=sys.argv[6]
	
	load=float(sys.argv[7])
	capacity=float(sys.argv[8])
	flow_num=int(sys.argv[9])
	host_num=int(sys.argv[10])
	output_filename=sys.argv[11]
	
	#Initialize flow distribution 
	#Type 1 flows
	distribution1=[]
	#Type 2 flows
	distribution2=[]
	#Type 3 flows
	distribution3=[]
	
	#Get traffic distribution line by line from 3 input files 
	#Type 1
	cdf_file1=open(cdf_filename1,'r')
	lines=cdf_file1.readlines()
	i=0
	for line in lines:
		tmp=line.split()
		if len(tmp)>=2:
			distribution1.append([float(tmp[1]),float(tmp[0])])
			i=i+1
	cdf_file1.close()
	#Type 2
	cdf_file2=open(cdf_filename2,'r')
	lines=cdf_file2.readlines()
	i=0
	for line in lines:
		tmp=line.split()
		if len(tmp)>=2:
			distribution2.append([float(tmp[1]),float(tmp[0])])
			i=i+1
	cdf_file2.close()
	#Type 3
	cdf_file3=open(cdf_filename3,'r')
	lines=cdf_file3.readlines()
	i=0
	for line in lines:
		tmp=line.split()
		if len(tmp)>=2:
			distribution3.append([float(tmp[1]),float(tmp[0])])
			i=i+1
	cdf_file3.close()
	
	#Generate flow size
	flows=[]
	for i in range(flow_num):
		number=random.random()
		#Type 1 flows
		if number<=p1:
			flows.append([flowsize(distribution1,random.random()),1])
		#Type 2 flows
		elif number<=p1+p2:
			flows.append([flowsize(distribution2,random.random()),2])
		#Type 3 flows
		else:
			flows.append([flowsize(distribution2,random.random()),3])
	
	#Get average throughput
	throughput=load*capacity
	#Get average flow size
	total_size=0
	for flow in flows:
		total_size+=flow[0]
	avg=total_size/len(flows)
	
	#Get average number of requests per second 
	num=throughput*1024*1024/(avg*1024*8) 
	#Get average request rate (number of requests every 1 ms)
	rate=num/1000 
	
	#Generate time interval
	times=[]
	for i in range(flow_num):
		#Get time interval (sleep time) 
		times.append(nextTime(rate)) 
	
	#Write trace results into output file
	#Trace format: <time_interval, flow_size(KB), flow_type, deadline(ms), host_id> 
	output_file=open(output_filename,'w')
	start=random.randint(0,host_num-1)
	for i in range(flow_num):
		flow_size=flows[i][0]
		flow_type=flows[i][1]
		deadline=0
		#Type 1 flows
		if flow_type==1:
			#Calculate ideal FCT (ms)
			ideal_fct=0.2+flow_size*8*1024/(capacity*1024*1024)*1000
			#Deadline is assigned to 
			deadline=int(math.ceil(2*ideal_fct))
		output_file.write(str(times[i])+' '+str(flow_size)+' '+str(flow_type)+' '+str(deadline)+' '+str(start)+'\n')
		start=(start+1)%host_num
	output_file.close()
	
	print 'Auto generate '+str(len(flows))+' flows:'
	print 'The average flow size: '+str(avg)+' KB'
	print 'The average request speed: '+str(num)+' requests/second'
	print 'Dynamic flow emulation will last for about '+str(len(flows)/num)+' seconds'
	print 'Done'
	
