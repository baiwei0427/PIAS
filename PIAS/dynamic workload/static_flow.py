import os
import sys
import string
import time
import threading

#Static flow workloads:
#Sender 1 and Sender 2 to the Receiver: large flows of 10MB following dependent/sequential pattern
#Sender 3 to the Receiver: small flows of 20KB following dependent/sequential pattern 

#This function is to print usage of this script
def usage():
	sys.stderr.write('This script is used to generate static flow workloads for PIAS project:\n')
	sys.stderr.write('static_flow.py [iterations] [output]\n')

class FlowThread(threading.Thread):
	def __init__(self, host, size, iterations,output):
		self.host=host
		self.size=size
		self.iterations=iterations
		self.output=output
		threading.Thread.__init__(self)
	
	def run(self):
		fcts=[]
		for i in range(self.iterations):
			#If the flow size is smaller than 1MB, we believe this is  a short flow and sleep for 10ms
			if self.size<1024*1024:
				time.sleep(10/1000)
			#By default, we use port number of 5001
			line=os.popen('./client.o '+str(self.host)+' '+str(5001)+' '+str(self.size)).read()
			#We need to get FCT results for this thread
			if len(self.output)>0:
				result=line.split()
				if len(result)>=2:
					#Get FCT for this flow and save it into global variable fcts
					fct=int(result[len(result)-2])
					fcts.append(fct)
		#Get final results
		if len(fcts)>0:
			#Sort FCT from low to high
			fcts.sort()
			#Write FCT results into the file
			result_file=open(self.output,'w')
			for i in range(len(fcts)):
				result_file.write(str(fcts[i])+'\n')
			result_file.close()
			avg=sum(fcts)/len(fcts)
			tail=fcts[len(fcts)*99/100-1]
			print 'Avg FCT: '+str(avg)+' us\n'
			print '99th FCT: '+str(tail)+' us\n'
			
#main function
if len(sys.argv)==3:
	iterations=int(sys.argv[1])
	filename=sys.argv[2]
	
	#20KB 
	small_flow=20
	#10MB
	large_flow=10*1024
	
	#Senders
	sender1='192.168.101.2'
	sender2='192.168.101.3'
	sender3='192.168.101.4'
	
	#Init three threads for emulation
	thread1=FlowThread(sender1,large_flow,iterations,'')
	thread2=FlowThread(sender2,large_flow,iterations,'')
	thread3=FlowThread(sender3,small_flow,iterations,filename)
	
	#Start three threads
	thread1.start()
	thread2.start()
	thread3.start()
	
	#Wait for three threads
	thread3.join()
	thread2.join()
	thread1.join()
	
else:
	usage()