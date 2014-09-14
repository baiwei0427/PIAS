import memcache

def populate(sender,port):
	#1KB-value: 11111....1111
	value_array=[]
	for i in range(1024):
		value_array.append('1')
	value=''.join(value_array)
	
	#Generate 1024 key-value pairs
	s = memcache.Client([sender+':'+str(port)])	
	for i in range(1024):
		s.set(str(i),value)
	
#This script is to pre-populate key-value pairs into following memcache servers
senders=[\
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
	'192.168.101.15',\
	'192.168.101.16'
	]

for sender in senders:
	print sender+'\n'
	populate(sender,11211)
	


