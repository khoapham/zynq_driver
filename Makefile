#obj-m += vf_driver.o
#obj-m += test.o
obj-m += simple.o
#KVERSION = $(shell uname -r)

all:
#	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
#	make -C /home/khoa/Downloads/linux-xlnx M=$(PWD) modules
	make -C /home/khoa/Downloads/linux-digilent M=$(PWD) modules

clean:
#	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
#	make -C /home/khoa/Downloads/linux-xlnx M=$(PWD) clean
	make -C /home/khoa/Downloads/linux-digilent M=$(PWD) clean
