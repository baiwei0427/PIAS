import os
import sys
import string
import threading
import memcache
from datetime import datetime
from datetime import timedelta

class QueryThread(threading.Thread):
	def __init__(self, host, port,key):
		self.host=host
		self.port=port
		self.key=key
		threading.Thread.__init__(self)

	def run(self):
		s = memcache.Client([self.host+':'+str(self.port)])	
		result=s.get(self.key)

def usage():
	sys.stderr.write('This script is used to generate memcached GET queries \n')
	sys.stderr.write('query.py [connections] [key]\n')

#Global information
hosts=[\
	'192.168.101.2',\
	'192.168.101.3',\
	'192.168.101.4',\
	'192.168.101.5',\
	'192.168.101.6',\
	'192.168.101.7',\
	'192.168.101.8',\
	'192.168.101.9',\
	'192.168.101.10',\
	'192.168.101.11',\
	'192.168.101.12',\
	'192.168.101.13',\
	'192.168.101.14',\
	'192.168.101.15'
	]
port=11211

#main function
if len(sys.argv)!=3:
	usage()
else:
	connections=int(sys.argv[1])
	key=sys.argv[2]
	
	threads=[]

	#Init threads
	for i in range(connections):
		thread=QueryThread(hosts[i%len(hosts)],port,key)
		threads.append(thread)
	
	#Start all threads
	start=datetime.now()
	for thread in threads:
		thread.start()
		
	#Wait for all threads to be completed
	for thread in threads:
		thread.join()
	end=datetime.now()
	
	#Print query completion time
	gct=end-start
	print gct
	print str(gct.seconds*1000000+gct.microseconds)
	
		