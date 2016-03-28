KDIR = /lib/modules/`uname -r`/build 

obj-m += globalfifo.o

build: kernel_modules

kernel_modules:
	make -C $(KDIR) M=$(CURDIR) modules
clean:
	make -C $(KDIR) M=$(CURDIR) clean

