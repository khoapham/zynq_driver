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
#include <pthread.h>

#include "vfdev.h"  
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

//#define VF_FIR 0
//#define VF_MM  1
 
static int databuf[500], fd;
void vf_data_input(int num_data);
void vf_conf_fir(int num_conf);

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

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

void vf_conf_mm(int num_conf) {
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

int vf_cnt(void) {
	int ret_val;
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
	return databuf[0];
}

//void *vf(void *ptr) {
void *fir() {
    int i, ret_val, stat, cnt;  //, j;
    int num_data, num_conf;
//    int *mode;

//	mode = (int *) ptr;

	num_data = 32;
	num_conf = 22;

	pthread_mutex_lock(&mutex1);
// Configure the FIR function for the vf
//	switch (*mode) {
//	case VF_FIR:
		vf_conf_fir(num_conf);
		vf_data_input(num_data);
//		break;
//	case VF_MM:
//		vf_conf_mm(num_conf);
//	        vf_data_input(num_data);
//		break;
//	default:
//		exit(-1);
//	}
// Read the counter
	cnt = vf_cnt();
	printf("cnt %d\n\r", cnt);
// Start the vf
        ret_val = ioctl(fd, VF_START, (long)num_data);
        if (ret_val < 0) {
                printf("ioctl to start VF is failed: %d\n", ret_val);
		exit(-1);
        }
// Check the status
	stat = vf_status();
	while(stat != 0x1) {
	        stat = vf_status();
	}
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
	pthread_mutex_unlock(&mutex1);
}

void *mm() {
    int i, ret_val, stat, cnt;  //, j;
    int num_data, num_conf;

        num_data = 32;
        num_conf = 22;

	pthread_mutex_lock(&mutex1); 
// Configure the FIR function for the vf
        vf_conf_mm(num_conf);
        vf_data_input(num_data);

// Read the counter
        cnt = vf_cnt();
        printf("cnt %d\n\r", cnt);
// Start the vf
        ret_val = ioctl(fd, VF_START, (long)num_data);
        if (ret_val < 0) {
                printf("ioctl to start VF is failed: %d\n", ret_val);
                exit(-1);
        }
// Check the status
        stat = vf_status();
        while(stat != 0x1) {
                stat = vf_status();
        }
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
	pthread_mutex_unlock(&mutex1);
}

int main() {
// Open the vf driver
	int rt1, rt2;
//	int *m1 = VF_FIR;
//	int *m2 = VF_MM;
//	int rt3, rt4;
        pthread_t t1, t2;
//        pthread_t t3, t4;

    if((fd = open("/dev/vf-driver", O_RDWR | O_SYNC)) == -1) FATAL;
    printf("/dev/vf-driver opened.\n");
    fflush(stdout);
while(1) {
// create 2 threads
        if( (rt1=pthread_create(&t1, NULL, &fir, NULL)) ) printf("Thread creation failed: %d\n", rt1);
        if( (rt2=pthread_create(&t2, NULL, &mm, NULL)) ) printf("Thread creation failed: %d\n", rt2);
        //if( (rt1=pthread_create(&t1, NULL, vf, (void *)m1)) ) printf("Thread creation failed: %d\n", rt1);
        //if( (rt2=pthread_create(&t2, NULL, vf, (void *)m2)) ) printf("Thread creation failed: %d\n", rt2);
//        if( (rt3=pthread_create(&t3, NULL, &fir, NULL)) ) printf("Thread creation failed: %d\n", rt1);
//        if( (rt4=pthread_create(&t4, NULL, &mm, NULL)) ) printf("Thread creation failed: %d\n", rt2);
//while (1) {
// wait for both threads to finish
        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
//        pthread_join(t3, NULL);
//        pthread_join(t4, NULL);
}
    close(fd);
    return 0;
}

