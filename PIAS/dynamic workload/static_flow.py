import os
import sys
import string
import time

#This function is to print usage of this script
def usage():
	sys.stderr.write('This script is used to generate static flow workloads for PIAS project:\n')
	sys.stderr.write('static_flow.py [IP address] [port] [size(KB)] [iterations] [output]\n')

#main function
if len(sys.argv)==6:
	ip=sys.argv[1]
	port=sys.argv[2]
	size=sys.argv[3]
	iterations=int(sys.argv[4])
	filename=sys.argv[5]
	
	#Initialize FCT results
	fcts=[]
	
	#Start static flow workloads
	for i in range(iterations):
		line=os.popen('./client.o '+ip+' '+port+' '+size).read()
		result=line.split()
		if len(result)>=2:
			fct=int(result[len(result)-2])
			fcts.append(fct)
			print str(fct)+'\n'
	    #Sleep 1 second
		time.sleep(1)
	
	#Sort flow low to high
	fcts.sort()
	#Write FCT results into a file
	file=open(filename,'w')
	for fct in fcts:
		file.write(str(fct)+'\r\n')
	file.close()
	
	if len(fcts)>0:
		avg=sum(fcts)/len(fcts)
		tail=fcts[len(fcts)*99/100-1]
		print 'Avg FCT: '+str(avg)+' us\n'
		print '99th FCT: '+str(tail)+' us\n'
else:
	usage()