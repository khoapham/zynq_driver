#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "vfdev.h"  
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
 
static int databuf[500], fd;
void vf_data_input(int num_data);
void vf_conf_fir(int num_conf);

void vf_conf_fir(int num_conf) {
	int ret_val;

      databuf[0]=0x07b5a000;
      databuf[1]=0x00000005;
      databuf[2]=0x00000000;
      databuf[3]=0x07b5a000;
      databuf[4]=0x00000007;
      databuf[5]=0x00000000;
      databuf[6]=0x07b5e000;
      databuf[7]=0x00000005;
      databuf[8]=0x00000000;
      databuf[9]=0x07b52000;
      databuf[10]=0x00000007;
      databuf[11]=0x00000000;

      databuf[12]=0xffffffff;
      databuf[13]=0x00678000;
      databuf[14]=0xffffffff;
      databuf[15]=0x00678000;
      databuf[16]=0x00000345;
      databuf[17]=0xffffffff;
      databuf[18]=0x00678000;
      databuf[19]=0xffffffff;
      databuf[20]=0x00678000;
      databuf[21]=0xffffffff;

	ret_val = write(fd, (char *)databuf, num_conf*4);
	if (ret_val < 0) {
		printf("fail to write configuration data to vf\n\r");
		exit(-1);
	}

	ret_val = ioctl(fd, VF_CONF_FIR, (long)num_conf);
        if (ret_val < 0) {
                printf("ioctl to configure FIR is failed: %d\n", ret_val);
                exit(-1);
        }
}

void vf_data_input(int num_data) {
	int i, ret_val;

      for(i=0;i<num_data;i++) databuf[i]=i+0x01;

        ret_val = write(fd, (char *)databuf, num_data*4);
        if (ret_val < 0) {
                printf("fail to write input data to vf\n\r");
                exit(-1);
        }

        ret_val = ioctl(fd, VF_DATA_INPUT, (long)num_data);
        if (ret_val < 0) {
                printf("ioctl to input data is failed: %d\n", ret_val);
                exit(-1);
        }
}

int vf_status(void) {
	int ret_val;
        ret_val = ioctl(fd, VF_GET_STAT, 0);
        if (ret_val < 0) {
                printf("ioctl to read status is failed: %d\n", ret_val);
                exit(-1);
        }

        ret_val = read(fd, (char *)databuf, 4);
        if (ret_val < 0) {
                printf("read status is failed: %d\n", ret_val);
                exit(-1);
        }
	return databuf[0];
}

int main(int argc, char **argv) {
    int i, ret_val, stat, j;
    int num_data, num_conf;

	num_data = 32;
	num_conf = 22;
// Open the vf driver
    
    if((fd = open("/dev/vf-driver", O_RDWR | O_SYNC)) == -1) FATAL;
    printf("/dev/vf-driver opened.\n"); 
    fflush(stdout);
    
//for(j=0;j<20;j++) {
while(1) {
// Configure the FIR function for the vf
	vf_conf_fir(num_conf);
	vf_data_input(num_data);

// Read the counter

        ret_val = ioctl(fd, VF_GET_CNT, 0);
        if (ret_val < 0) {
                printf("ioctl to read counter is failed: %d\n", ret_val);
                exit(-1);
        }

	ret_val = read(fd, (char *)databuf, 4);
        if (ret_val < 0) {
                printf("read counter is failed: %d\n", ret_val);
                exit(-1);
        }

	printf("cnt %d\n\r", databuf[0]);
// Start the vf
	
        ret_val = ioctl(fd, VF_START, (long)num_data);
        if (ret_val < 0) {
                printf("ioctl to start VF is failed: %d\n", ret_val);
		exit(-1);
        }

// Read the counter

//        ret_val = ioctl(fd, VF_GET_CNT, 0);
//        if (ret_val < 0) {
//                printf("ioctl to read counter is failed: %d\n", ret_val);
//                exit(-1);
//        }
//
//        ret_val = read(fd, (char *)databuf, 4);
//        if (ret_val < 0) {
//                printf("read counter is failed: %d\n", ret_val);
//                exit(-1);
//        }
//
//        printf("cnt %d\n\r", databuf[0]);

// Read the status
//	ret_val = ioctl(fd, VF_GET_STAT, (long)num_data);
//        if (ret_val < 0) {
//                printf("ioctl to read status is failed: %d\n", ret_val);
//                exit(-1);
//        }
//	
//	ret_val = read(fd, (char *)databuf, 4);
//        if (ret_val < 0) {
//                printf("read status is failed: %d\n", ret_val);
//                exit(-1);
//        }
	stat = vf_status();
//	while(stat[0] != 0x0) printf("vf haven't started yet\n\r");
	while(stat != 0x1) {
//		printf("vf haven't finished yet\n\r");
//		ret_val = ioctl(fd, VF_GET_STAT, (long)num_data);
//        	if (ret_val < 0) {
//                	printf("ioctl to read status is failed: %d\n", ret_val);
//	                exit(-1);
//        	}
//
//		ret_val = read(fd, (char *)databuf, 4);
//        	if (ret_val < 0) {
//	                printf("read status is failed: %d\n", ret_val);
//                	exit(-1);
//        	}
	        stat = vf_status();
	}
//	printf("stat is %x\n\r", stat);
	
// Read the result

	ret_val = ioctl(fd, VF_GET_RESULT, (long)num_data);
        if (ret_val < 0) {
                printf("ioctl to read result is failed: %d\n", ret_val);
		exit(-1);
        }

	ret_val = read(fd, (char *)databuf, num_data*4);
	if (ret_val < 0) {
                printf("read result is failed: %d\n", ret_val);
		exit(-1);
        }

//	for(i=0;i<num_data;i++) printf("[%d]: %x\n\r", i, databuf[i]);
}
    close(fd);
    return 0;
}

