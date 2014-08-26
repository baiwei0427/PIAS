import os
import sys
import string

#This function is to print usage of this script
def usage():
	sys.stderr.write('This script is used to analyze dynamic flow trace:\n')
	sys.stderr.write('dynamic_flow.py [trace file] [percentage] \n')

#main function
if len(sys.argv)!=3:
	usage()
else:
	trace_filename=sys.argv[1]
	percentage=float(sys.argv[2])
	#Open trace file
	trace_file=open(trace_filename,'r')
	lines=trace_file.readlines()
	flows=[]
	for line in lines:
		tmp=line.split()
		if len(tmp)>=3:
			flows.append(int(tmp[1]))
	trace_file.close()
	flows.sort()
	sum=sum(flows)
	size=0
	for i in range(int(len(flows)*percentage)):
		size=size+flows[i]
	print 'The '+str(int(percentage*100))+'th flow in our trace is '+str(flows[int(len(flows)*percentage)-1])+'KB'
	print float(size)/sum
	print (float(size)+flows[int(len(flows)*percentage)-1]*(len(flows)-int(len(flows)*percentage)))/sum