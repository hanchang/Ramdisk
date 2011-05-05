/*
 * rd.h - header file with ioctl definitions
 * The declarations in this header file are shared
 * between the kernel module as well as user-space processes accessing
 * the kernels funcionality.
 */

#ifndef RAMDISK_IOCTL_H
#define RAMDISK_IOCTL_H

#include <linux/ioctl.h>

// Look in /usr/include/linux/major.h to see which major_nums are
// currently used
#define MAJOR_NUM 300

// Path to the device file
#define DEVICE_PATH "/proc/ramdisk_ioctl"

// Name of device
#define DEVICE_NAME "ramdisk"

// This is the structure we use to pass paremeters to the kernel
typedef struct {
    char* pathname;
    int pathnameLength;
    int fd;
    char* address;
    int addressLength;
    int num_bytes;
    int offset;
    int ret;
} ioctl_rd;


// These are the messages to the kernel
// _IOR = passing information from user process to kernel module
// _IOW = passing information from kernel module to user process
// _IORW = passing information both ways
#define IOCTL_RD_CREAT    _IOWR(MAJOR_NUM, 0, ioctl_rd)
#define IOCTL_RD_MKDIR    _IOWR(MAJOR_NUM, 1, ioctl_rd)
#define IOCTL_RD_OPEN     _IOWR(MAJOR_NUM, 2, ioctl_rd)
#define IOCTL_RD_CLOSE    _IOWR(MAJOR_NUM, 3, ioctl_rd)
#define IOCTL_RD_READ     _IOWR(MAJOR_NUM, 4, ioctl_rd)
#define IOCTL_RD_WRITE    _IOWR(MAJOR_NUM, 5, ioctl_rd)
#define IOCTL_RD_LSEEK    _IOWR(MAJOR_NUM, 6, ioctl_rd)
#define IOCTL_RD_UNLINK   _IOWR(MAJOR_NUM, 7, ioctl_rd)
#define IOCTL_RD_READDIR  _IOWR(MAJOR_NUM, 8, ioctl_rd)
#define IOCTL_RD_SYNC     _IOWR(MAJOR_NUM, 9, ioctl_rd)
#define IOCTL_RD_RESTORE  _IOWR(MAJOR_NUM, 10, ioctl_rd)

// Wrapper functions
int ioctl_rd_creat(int deviceFd, char* pathname);
int ioctl_rd_mkdir(int deviceFd, char* pathname);
int ioctl_rd_open(int deviceFd, char* pathname);
int ioctl_rd_close(int deviceFd, int fd);
int ioctl_rd_read(int deviceFd, int fd, char** address, int num_bytes);
int ioctl_rd_write(int deviceFd, int fd, char* address, int num_bytes);
int ioctl_rd_lseek(int deviceFd, int fd, int offset);
int ioctl_rd_unlink(int deviceFd, char* pathname);
int ioctl_rd_readdir(int deviceFd, int fd, char** address);
int ioctl_rd_sync(int deviceFd);
int ioctl_rd_restore(int deviceFd);

#endif
