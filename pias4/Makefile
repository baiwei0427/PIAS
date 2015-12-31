obj-m += pias.o
pias-y := params.o netfilter.o jprobe.o flow.o network.o main.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
