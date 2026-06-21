obj-m += hello_world.o

all:
	make -C /lib/modules/6.18.26-1-MANJARO/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
