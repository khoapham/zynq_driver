obj-m += vf_driver.o

all:
	make -C /home/khoa/Downloads/linux-xlnx M=$(PWD) modules

clean:
	make -C /home/khoa/Downloads/linux-xlnx M=$(PWD) clean
