obj-m += ramdisk_module.o

all:
	make -C /lib/modules/2.6.5-1.358/build SUBDIRS=$(PWD) modules

clean:
	make -C /lib/modules/2.6.5-1.358/build SUBDIRS=$(PWD) clean
	
