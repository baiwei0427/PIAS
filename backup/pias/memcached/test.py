import memcache

s = memcache.Client(["192.168.101.2:11211"])
print s.get('0')