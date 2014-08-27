import os
import sys
import string

#This function is to print usage of this script
def usage():
	sys.stderr.write('This script is used to analyze FCT of dynamic flow workloads for PIAS project:\n')
	sys.stderr.write('result.py [result file]\n')

#main function
if len(sys.argv)!=2:
	usage()
else:
	#Open file
	filename=sys.argv[1]
	file=open(filename,'r')
	lines=file.readlines()
	
	#Initialize variables
	fcts=[]	
	tiny_fcts=[]	#0-10KB
	short_fcts=[]	#10KB-100KB
	median_fcts=[]	#100KB-10MB
	long_fcts=[]			#10MB-
	
	for line in lines:
		result=line.split()
		if len(result)==4:
			size=int(result[0])
			fct=int(result[2])
			if fct>0:
				fcts.append(fct)
				if size<=10:
					tiny_fcts.append(fct)
				elif size<=100:
					short_fcts.append(fct)
				elif size<=10000:
					median_fcts.append(fct)
				else:
					long_fcts.append(fct)
				
	#Close file
	file.close()
	tiny_fcts.sort()
	short_fcts.sort()
	
	if len(tiny_fcts)>0:
		print 'There are '+str(len(tiny_fcts))+' flows in (0,10KB). Their average FCT is '+str(sum(tiny_fcts)/len(tiny_fcts))+' us. Their 99th FCT is '+str(tiny_fcts[int(0.99*len(tiny_fcts)-1)])+' us'
	if len(short_fcts)>0:
		print 'There are '+str(len(short_fcts))+' flows in (10KB,100KB). Their average FCT is '+str(sum(short_fcts)/len(short_fcts))+' us. Their 99th FCT is '+str(short_fcts[int(0.99*len(short_fcts)-1)])+' us'
	if len(median_fcts)>0:
		print 'There are '+str(len(median_fcts))+' flows in (100KB,10MB). Their average FCT is '+str(sum(median_fcts)/len(median_fcts))+' us'
	if len(long_fcts)>0:
		print 'There are '+str(len(long_fcts))+' flows in (10MB,). Their average FCT is '+str(sum(long_fcts)/len(long_fcts))+' us'
	if len(fcts)>0:
		print 'There are '+str(len(fcts))+' flows in total. Their average FCT is '+str(sum(fcts)/len(fcts))+' us'