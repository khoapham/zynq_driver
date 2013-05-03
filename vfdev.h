#ifndef VFDEV_H
#define VFDEV_H

#include <linux/ioctl.h>

#define MAJOR_NUM 202

// Configure the FIR function for the VF

#define VF_CONF_FIR _IOR(MAJOR_NUM, 2, char *)
// _IOR means that we're creating an ioctl command number for passing information from a user process to the kernel module

#define VF_START _IOR(MAJOR_NUM, 0, char *)

#define VF_GET_RESULT _IOR(MAJOR_NUM, 1, char *)

#endif
