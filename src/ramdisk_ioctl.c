// Accessing our kernel module ramdisk

#include "ramdisk_ioctl.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>  // for open()
#include <unistd.h> // for exit()
#include <sys/ioctl.h> // for ioctl()

int ioctl_rd_creat(int deviceFd, char* pathname) {
    int returnValue;
    
    // Object holds the params we are passing
    ioctl_rd params;

    params.pathname = pathname;
    params.pathnameLength = strlen(pathname);

    // Call our rd_creat function in the kernel
    returnValue = ioctl(deviceFd, IOCTL_RD_CREAT, &params);
    return returnValue;
}

int ioctl_rd_mkdir(int deviceFd, char* pathname) {
    int returnValue;

    // Object holds the params we are passing
    ioctl_rd params;

    params.pathname = pathname;
    params.pathnameLength = strlen(pathname);

    returnValue = ioctl(deviceFd, IOCTL_RD_MKDIR, &params);
    return returnValue;
}

int ioctl_rd_open(int deviceFd, char* pathname) {
    int returnValue;

    // Object holds the params we are passing
    ioctl_rd params;

    params.pathname = pathname;
    params.pathnameLength = strlen(pathname);

    returnValue = ioctl(deviceFd, IOCTL_RD_OPEN, &params);
    return returnValue;
}

int ioctl_rd_close(int deviceFd, int fd) {
    int returnValue;

    // Object holds the params we are passing
    ioctl_rd params;

    params.fd = fd;

    returnValue = ioctl(deviceFd, IOCTL_RD_CLOSE, &params);
    return returnValue;
}

int ioctl_rd_read(int deviceFd, int fd, char** address, int num_bytes) {
    int returnValue;

    // Object holds the params we are passing
    ioctl_rd params;

    *address = (char*) malloc(sizeof(char) * num_bytes);

    // Populate params
    params.address = *address;
    params.addressLength = num_bytes;
    params.fd = fd;
    params.num_bytes = num_bytes;

    returnValue = ioctl(deviceFd, IOCTL_RD_READ, &params);
    return returnValue;
}

int ioctl_rd_write(int deviceFd, int fd, char* address, int num_bytes) {
    int returnValue;

    // Object holds the params we are passing
    ioctl_rd params;

    // Populate params
    params.address = address;
    params.fd = fd;
    params.num_bytes = num_bytes;

    returnValue = ioctl(deviceFd, IOCTL_RD_WRITE, &params);
    return returnValue;
}


int ioctl_rd_lseek(int deviceFd, int fd, int offset) {
    int returnValue;

    // Object holds the params we are passing
    ioctl_rd params;

    // Populate params
    params.fd = fd;
    params.offset = offset;

    returnValue = ioctl(deviceFd, IOCTL_RD_LSEEK, &params);
    return returnValue;
}

int ioctl_rd_unlink(int deviceFd, char* pathname) {
    int returnValue;

    // Object holds the params we are passing
    ioctl_rd params;

    params.pathname = pathname;
    params.pathnameLength = strlen(pathname);

    // Call our rd_creat function in the kernel
    returnValue = ioctl(deviceFd, IOCTL_RD_UNLINK, &params);
    return returnValue;
}

int ioctl_rd_readdir(int deviceFd, int fd, char** address) {
    int returnValue;
                                                                                                 
    // Object holds the params we are passing
    ioctl_rd params;
                                                                                                 
    *address = (char*) malloc(16);
                                                                                                 
    // Populate params
    params.address = *address;
    params.addressLength = 16;
    params.fd = fd;
                                                                                                 
    returnValue = ioctl(deviceFd, IOCTL_RD_READDIR, &params);
    return returnValue;
}

int ioctl_rd_sync(int deviceFd) {
	int returnValue;

	ioctl_rd params;

	returnValue = ioctl(deviceFd, IOCTL_RD_SYNC, &params);
	return returnValue;
}

int ioctl_rd_restore(int deviceFd) {
    int returnValue;

    ioctl_rd params;

    returnValue = ioctl(deviceFd, IOCTL_RD_RESTORE, &params);
    return returnValue;
}


