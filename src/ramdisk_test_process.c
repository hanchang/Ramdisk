#define CREAT 0
#define MKDIR 1
#define UNCREAT 2
#define UNMKDIR 3

#include <stdio.h>
#include <stdlib.h> 
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "ramdisk_ioctl.h"                                                                                        

int testBig(int deviceFd, int num, int type) {
    int i, ret;
    char buffer[15];
    for (i = 0; i < num; i++) {
        if (type == CREAT) {
            sprintf(buffer, "/reg_file%d", i);
            ret = ioctl_rd_creat(deviceFd, buffer);
            printf("testBig: rd_creat %s returned %d\n", buffer, ret);
        }
        if (type == MKDIR) {
            sprintf(buffer, "/dir%d", i);
            ret = ioctl_rd_mkdir(deviceFd, buffer);
            printf("testBig: rd_mkdir %s returned %d\n", buffer, ret);
        }
        if (type == UNCREAT) {
            sprintf(buffer, "/reg_file%d", i);
            ret = ioctl_rd_unlink(deviceFd, buffer);
            printf("testBig: unlink (reg) %s returned %d\n", buffer, ret);
        }
        if (type == UNMKDIR) {
            sprintf(buffer, "/dir%d", i);
            ret = ioctl_rd_unlink(deviceFd, buffer);
            printf("testBig: unlink (dir) %s returned %d\n", buffer, ret);
        }
  
    }
    return i;
}

// Test functions.
void test_creat(int deviceFd) {
    int ret;

    printf("Begin test_creat() ----------\n\n");


    printf("A directory can have at most 1023 directory entries because \n");
    printf("there are 1023 inodes in the inode array and each directory \n");
    printf("entry requires one inode to represent it.\n");

    printf("A directory will start using single indirect pointers to blocks \n");
    printf("when it contains over 128 entries. \n");

    printf("\n");

    // Single indirects for directories
    printf("Testing directory indirect pointers by creating 1000 regular files in /:\n");
    testBig(deviceFd, 1000, CREAT);

    printf("\n");

    printf("Deleting the 1000 regular files created above:\n");
    testBig(deviceFd, 1000, UNCREAT);

	printf("Create a file called README in /.\n");
	ret = ioctl_rd_creat(deviceFd, "/README");
    printf("return: %d\n", ret);
	ioctl_rd_unlink(deviceFd, "/README");

	printf("Create a file called TEST in /home.\n");
	ret = ioctl_rd_mkdir(deviceFd, "/home");
    printf("return: %d\n", ret);

	ret = ioctl_rd_creat(deviceFd, "/home/TEST");
    printf("return: %d\n", ret);
	ioctl_rd_unlink(deviceFd, "/home/TEST");
	ioctl_rd_unlink(deviceFd, "/home");

	printf("Create a file called FAIL in /doesnotexist.\n");
	ret = ioctl_rd_creat(deviceFd, "/doesnotexist/FAIL");
    printf("return: %d\n", ret);

	printf("\n");

	printf("End test_creat() -----------\n\n");
}


void test_mkdir(int deviceFd) {
    int ret; 

    printf("Begin test_mkdir() ----------\n\n");


    printf("A directory can have at most 1023 directory entries because \n");
    printf("there are 1023 inodes in the inode array and each directory \n");
    printf("entry requires one inode to represent it.\n");

    printf("A directory will start using single indirect pointers to blocks \n");
    printf("when it contains over 128 entries. \n");

    printf("\n");

    // Single indirects for directories
    printf("Testing directory indirect pointers by creating 1000 regular files in /:\n");
    testBig(deviceFd, 1000, MKDIR);

    printf("\n");

    printf("Deleting the 1000 regular files created above:\n");
    testBig(deviceFd, 1000, UNMKDIR);

    printf("Create a directory called /test.\n");
    ret = ioctl_rd_mkdir(deviceFd, "/test");
    printf("return: %d\n", ret);
	ioctl_rd_unlink(deviceFd, "/test");

    printf("Create a directory called /home.\n");
    ret = ioctl_rd_mkdir(deviceFd, "/home");
    printf("return: %d\n", ret);

	printf("Create a directory called /home/user.\n");
    ret = ioctl_rd_mkdir(deviceFd, "/home/user");
    printf("return: %d\n", ret);
	
	printf("Create a directory called /home/user/test.\n");
	ret = ioctl_rd_mkdir(deviceFd, "/home/user/test");
    printf("return: %d\n", ret);
	ioctl_rd_unlink(deviceFd, "/home/user/test");
	ioctl_rd_unlink(deviceFd, "/home/user");
	ioctl_rd_unlink(deviceFd, "/home");

    printf("Create a file called FAIL in /doesnotexist.\n");
    ret = ioctl_rd_creat(deviceFd, "/doesnotexist/FAIL");
    printf("return: %d\n", ret);
	printf("End test_mkdir() ----------\n\n");	
	printf("\n");
}

void test_open(int deviceFd) {
    int ret;
	printf("Begin test_open() -----------\n\n");

	printf("Creating a test directory /test:\n");
	ret = ioctl_rd_mkdir(deviceFd, "/test");
    printf("return: %d\n", ret);
	printf("\n");

	printf("Creating a test file /TEST:\n");
	ret = ioctl_rd_creat(deviceFd, "/TEST");
    printf("return: %d\n", ret);
	printf("\n");

	printf("Testing ioctl_rd_open on the directory /test:\n");
	int fd1 = ioctl_rd_open(deviceFd, "/test");
    printf("return: %d\n", fd1);
	printf("\n");

	printf("Testing ioctl_rd_open on the file /TEST:\n");
	int fd2 = ioctl_rd_open(deviceFd, "/TEST");
    printf("return: %d\n", fd2);
	printf("\n");

	printf("Testing ioctl_rd_open a non-existent directory /fake:\n");
	ret = ioctl_rd_open(deviceFd, "/fake");
    printf("return: %d\n", ret);
	printf("\n");

	printf("Testing ioctl_rd_open on a non-existent file /FAKE:\n");
	ret = ioctl_rd_open(deviceFd, "/FAKE");
    printf("return: %d\n", ret);
	printf("\n");

	printf("Testing ioctl_rd_open on a non-existent path /fake/FILE:\n");
	ret = ioctl_rd_open(deviceFd, "/fake/FILE");
    printf("return: %d\n", ret);
	printf("\n");

	ioctl_rd_close(deviceFd, fd1);
	ioctl_rd_close(deviceFd, fd2);
	ioctl_rd_unlink(deviceFd, "/test");
	ioctl_rd_unlink(deviceFd, "/TEST");
	
	
	printf("End test_open() ----------\n\n");
}

void test_close(int deviceFd) {
    int ret;
	printf("Begin test_close() -----------\n\n");

    printf("Creating a test directory /test:\n");
    ret = ioctl_rd_mkdir(deviceFd, "/test");
    printf("return: %d\n", ret);

    printf("\n");

    printf("Creating a test file /TEST:\n");
    ret = ioctl_rd_creat(deviceFd, "/TEST");
    printf("return: %d\n", ret);
    printf("\n");

    printf("Testing ioctl_rd_open on the directory /test:\n");
    int test_dir = ioctl_rd_open(deviceFd, "/test");
    printf("return: %d\n", test_dir);
    printf("\n");

    printf("Testing ioctl_rd_open on the file /TEST:\n");
    int test_reg = ioctl_rd_open(deviceFd, "/TEST");
    printf("return: %d\n", test_reg);
    printf("\n");

	printf("Testing ioctl_rd_close on the directory /test:\n");
	ret = ioctl_rd_close(deviceFd, test_dir);
    printf("return: %d\n", ret);
	printf("\n");

	printf("Testing ioctl_rd_close on the file /TEST:\n");
	ret = ioctl_rd_close(deviceFd, test_reg);
    printf("return: %d\n", ret);
	printf("\n");

	printf("Testing ioctl_rd_close on non-existent file with fake file descriptor:\n");
	int fake_fd = 20;
	ret = ioctl_rd_close(deviceFd, fake_fd);	
    printf("return: %d\n", ret);

	printf("\n");

	ioctl_rd_unlink(deviceFd, "/test");
	ioctl_rd_unlink(deviceFd, "/TEST");

	printf("End test_close() -----------\n\n");
}

void test_write(int deviceFd) {
    int ret;
	printf("Begin test_write() -----------\n\n");

    printf("A file can be 18432 bytes and not use double indirect block pointers;\n");
    printf("once it is 18432 bytes, it will require the use of double indirect block pointers.\n");

    printf("\n");

    printf("Testing double indirect pointers and \n");
	printf("max file size by writing 256 characters \n"); 
	printf("of the alphabet into a file called \n");
	printf("LARGE of size 1067008 bytes in /. \n");

    printf("ioctl_rd_creat: Creating /LARGE: \n");
    ret = ioctl_rd_creat(deviceFd, "/LARGE");
    printf("return: %d\n", ret);
	printf("ioctl_rd_open: Opening /LARGE: \n");
    int fd = ioctl_rd_open(deviceFd, "/LARGE");

    // Writing to a file     
	printf("ioctl_rd_write: Writing 1067008 bytes to /LARGE: \n");
    int i;
    char buffer[] = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuv"; // 16 letters of the alphabet
	// 1067008 divided by 16 is 66688.
    for (i = 0; i < 4168; i++) { 
        ret = ioctl_rd_write(deviceFd, fd, buffer, 256);
		if (ret != 256) {
			printf("bytes written: %d\n", ret);
		}
    }
	printf("/LARGE written.\n");

	printf("Close /LARGE: \n");
	ret = ioctl_rd_close(deviceFd, fd);
    printf("return: %d\n", ret);

	printf("Testing ioctl_rd_write into an invalid file with a fake file descriptor:\n");
	int fake_fd = 20;
	ret = ioctl_rd_write(deviceFd, fake_fd, buffer, 16);
    printf("return: %d\n", ret);

	printf("Testing ioctl_rd_write into a directory file /home:\n");
	ret = ioctl_rd_creat(deviceFd, "/home");
	printf("creating /home return: %d\n", ret);

	int home_fd = ioctl_rd_open(deviceFd, "/home");
	printf("/home created with fd: %d, \n", home_fd);
	ret = ioctl_rd_write(deviceFd, home_fd, buffer, 16);
	ioctl_rd_close(deviceFd, home_fd);
    printf("return: %d\n", ret);

	ioctl_rd_unlink(deviceFd, "/LARGE");
	ioctl_rd_unlink(deviceFd, "/home");
	printf("End test_write() ----------\n\n");
}

void test_read(int deviceFd) {
    int ret;
    printf("Begin test_read()----------\n\n");

	
    printf("ioctl_rd_creat: Creating /home: \n");
    ret = ioctl_rd_mkdir(deviceFd, "/home");
    printf("return: %d\n", ret);
    printf("ioctl_rd_creat: Creating /home/abcs: \n");
    ret = ioctl_rd_creat(deviceFd, "/home/abcs");
    printf("return: %d\n", ret);

	printf("\n");

    printf("ioctl_rd_open: Opening /home/abcs: \n");
    int fd = ioctl_rd_open(deviceFd, "/home/abcs");

    // Writing to a file     
    printf("ioctl_rd_write: Writing 26000 bytes to /home/abcs: \n");
    int i;
    char buffer[] = "abcdefghijklmnopqrstuvwxyz";
    for (i = 0; i < 1000; i++) {
        ret = ioctl_rd_write(deviceFd, fd, buffer, 26);
        if (ret != 26) {
            printf("bytes written: %d\n", ret);
        }
    }
    printf("/home/abcs written.\n");

	printf("Resetting fileposition of /home/abcs to 0\n");
	ioctl_rd_lseek(deviceFd, fd, 0);

	printf("Having written the alphabet into /home/abcs, test ioctl_rd_read:\n");
	printf("Read 100 bytes from /home/abcs:\n");
	char* out = NULL;
	ret = ioctl_rd_read(deviceFd, fd, &out, 100);
    printf("return: %d\n", ret);
	out[100] = '\0';
	printf("Here is the data read: %s\n", out);
	ret = ioctl_rd_close(deviceFd, fd);
    printf("return: %d\n", ret);
	free(out);

	printf("Testing ioctl_rd_read into an invalid file with a fake file descriptor:\n");
	ret = ioctl_rd_read(deviceFd, 20, &out, 100);
    printf("return: %d\n", ret);

	out = NULL;
	printf("Testing ioctl_rd_read into a directory file /home:\n");
	int home_fd = ioctl_rd_open(deviceFd, "/home");
	ret = ioctl_rd_read(deviceFd, home_fd, &out, 10);
	out[10] = '\0';
    printf("return: %d\n", ret);
	ioctl_rd_close(deviceFd, home_fd);
	free(out);
	
	ioctl_rd_unlink(deviceFd, "/home/abcs");
	ioctl_rd_unlink(deviceFd, "/home");

	printf("End test_read() ----------\n\n");
}

void test_lseek(int deviceFd) {
    int ret;
	printf("Begin test_lseek()----------\n\n");

    printf("ioctl_rd_creat: Creating /home: \n");
    ret = ioctl_rd_mkdir(deviceFd, "/home");
    printf("return: %d\n", ret);
    printf("ioctl_rd_creat: Creating /home/abcs: \n");
    ret = ioctl_rd_creat(deviceFd, "/home/abcs");
    printf("return: %d\n", ret);

    printf("\n");

    printf("ioctl_rd_open: Opening /home/abcs: \n");
    int fd = ioctl_rd_open(deviceFd, "/home/abcs");

    // Writing to a file     
    printf("ioctl_rd_write: Writing 2600 bytes to /home/abcs: \n");
    int i;
    char buffer[] = "abcdefghijklmnopqrstuvwxyz";
    for (i = 0; i < 100; i++) {
        ret = ioctl_rd_write(deviceFd, fd, buffer, 26);
        if (ret != 26) {
            printf("bytes written: %d\n", ret);
        }
    }
    printf("/home/abcs written.\n");
	printf("\n");
	
	printf("Having written the alphabet into /home/abcs, test ioctl_rd_lseek:\n");

	printf("Seek to byte number 77 in /home/abcs:\n");
	ret = ioctl_rd_lseek(deviceFd, fd, 77);
    printf("return: %d\n", ret);

	printf("Read the next 20 bytes from /home/abcs:\n");
	char* out = NULL; 
	ret = ioctl_rd_read(deviceFd, fd, &out, 20);
    printf("return: %d\n", ret);
	out[20] = '\0';
    printf("Here is the data read: %s\n", out);
	ioctl_rd_close(deviceFd, fd);

	printf("Testing ioctl_rd_lseek() with a negative offset of -10:\n");
	ret = ioctl_rd_lseek(deviceFd, fd, -10);
    printf("return: %d\n", ret);

	printf("Testing ioctl_rd_lseek() with offset 0 on the directory /home:\n");
	int home_fd = ioctl_rd_open(deviceFd, "/home");
	ret = ioctl_rd_lseek(deviceFd, home_fd, 0);
	ioctl_rd_close(deviceFd, home_fd);
    printf("return: %d\n", ret);

	printf("Testing ioctl_rd_lseek() with a fake file descriptor:\n");
	ret = ioctl_rd_lseek(deviceFd, 20, 0);
    printf("return: %d\n", ret);
	
	ioctl_rd_unlink(deviceFd, "/home/abcs");
	ioctl_rd_unlink(deviceFd, "/home");

	printf("End test_lseek() -----------\n\n");
}

void test_unlink(int deviceFd) {
    int ret;
	printf("Begin test_unlink() ----------\n");

	printf("Testing ioctl_rd_unlink on 1023 directory files:\n");
	testBig(deviceFd, 1023, MKDIR);
	testBig(deviceFd, 1023, UNMKDIR);

	printf("Testing ioctl_rd_unlink on 1023 regular files:\n");
	testBig(deviceFd, 1023, CREAT);
	testBig(deviceFd, 1023, UNCREAT);

	printf("Testing ioctl_rd_unlink on a non-existent file /FAKE:\n");
	ret = ioctl_rd_unlink(deviceFd, "/FAKE");
	printf("return: %d\n", ret);

    printf("Testing ioctl_rd_unlink on an opened file /OPENED:\n");
	printf("First create /OPENED:\n");
	ret = ioctl_rd_creat(deviceFd, "/OPENED");
    printf("return: %d\n", ret);
	printf("Next, open /OPENED: \n");
	int fd = ioctl_rd_open(deviceFd, "/OPENED");
    printf("Opened /OPENED with fd = %d\n", fd);
	printf("Lastly, try to unlink /OPENED: \n");
	ret = ioctl_rd_unlink(deviceFd, "/OPENED");
    printf("return: %d\n", ret);
	ioctl_rd_close(deviceFd, fd);
	ioctl_rd_unlink(deviceFd, "/OPENED");

	printf("Testing ioctl_rd_unlink on an invalid path /home/invalid/FAKE:\n");
	ret = ioctl_rd_mkdir(deviceFd, "/home");
	ret = ioctl_rd_unlink(deviceFd, "/home/invalid/FAKE");
    printf("return: %d\n", ret);

	printf("Testing ioctl_rd_unlink on an non-empty directory /home:\n");
	printf("First creating directory /home/valid:\n");
	ret = ioctl_rd_mkdir(deviceFd, "/home/valid");
    printf("return: %d\n", ret);
	printf("Then creating file /home/FILE:\n");
	ret = ioctl_rd_creat(deviceFd, "/home/FILE");
    printf("return: %d\n", ret);
	printf("Now /home has two entries; attempting to unlink /home:\n");
	ret = ioctl_rd_unlink(deviceFd, "/home");
    printf("return: %d\n", ret);

	printf("Testing ioctl_rd_unlink on root:\n");
	ret = ioctl_rd_unlink(deviceFd, "/");	
    printf("return: %d\n", ret);

	ioctl_rd_unlink(deviceFd, "/home/valid");
	ioctl_rd_unlink(deviceFd, "/home/FILE");
	ioctl_rd_unlink(deviceFd, "/home");
	
	printf("End ioctl_rd_unlink() -----------\n\n");
}

void test_readdir(int deviceFd) {
    int ret;
	printf("Begin test_readdir() -----------\n");

	printf("Creating directory /home:\n");
	ret = ioctl_rd_mkdir(deviceFd, "/home");
    printf("return: %d\n", ret);
//    printf("\n");

	printf("Creating directory /home/user:\n");
	ret = ioctl_rd_mkdir(deviceFd, "/home/user");
    printf("return: %d\n", ret);
//    printf("\n");

	printf("Creating directory /home/cs552:\n");
	ret = ioctl_rd_mkdir(deviceFd, "/home/cs552");
    printf("return: %d\n", ret);
//    printf("\n");

	printf("Creating regular file /home/file:\n");
	ret = ioctl_rd_creat(deviceFd, "/home/file");
    printf("return: %d\n", ret);
//    printf("\n");

	printf("Open /home in preparation for reading:\n");
	int fd = ioctl_rd_open(deviceFd, "/home");
    printf("return: %d\n", fd);

//    printf("\n");

	printf("Reading /home:\n");
	char* address = NULL;
	char fileName[15];
    char inodeNumberStr[3];
    int inodeNumber;    

	ret = ioctl_rd_readdir(deviceFd, fd, &address);
    printf("return: %d\n", ret);
	while (ret != 0) {
        memcpy(fileName, address, 14);
        fileName[14] = '\0';
        
        inodeNumberStr[0] = address[14];
        inodeNumberStr[1] = address[15];
        inodeNumberStr[2] = '\0';
        inodeNumber = atoi(inodeNumberStr);

		printf("filename: %s, inode_number: %d\n", fileName, inodeNumber);	

		free(address);
		ret = ioctl_rd_readdir(deviceFd, fd, &address);
        printf("return: %d\n", ret);
    	printf("\n");
	}

	ioctl_rd_close(deviceFd, fd);
	ioctl_rd_unlink(deviceFd, "/home/file");
	ioctl_rd_unlink(deviceFd, "/home/user");
	ioctl_rd_unlink(deviceFd, "/home/cs552");
	ioctl_rd_unlink(deviceFd, "/home");

	printf("End test_readdir() -----------\n\n\n");
}

void test_sync(int deviceFd) {
    printf("Begin test_sync() -----------\n\n");

	testBig(deviceFd, 100, MKDIR);
	printf("after testbig\n");
	ioctl_rd_sync(deviceFd);
	printf("End test_sync() ---------------\n\n");
}


void test_restore(int deviceFd) {
    char* address = NULL;
    char fileName[15];
    char inodeNumberStr[3];
    int inodeNumber;

	printf("Begin test_restore() -----------\n\n");
	ioctl_rd_restore(deviceFd);

	printf("Using ioctl_rd_readdir to check integrity of restoration\n");
	int fd = ioctl_rd_open(deviceFd, "/");
	
    int ret;
   	ret = ioctl_rd_readdir(deviceFd, fd, &address);
    printf("return: %d\n", ret);
    while (ret != 0) {
        memcpy(fileName, address, 14);
        fileName[14] = '\0';

        inodeNumberStr[0] = address[14];
        inodeNumberStr[1] = address[15];
        inodeNumberStr[2] = '\0';
        inodeNumber = atoi(inodeNumberStr);

        printf("filename: %s, inode_number: %d\n", fileName, inodeNumber);

        free(address);
        ret = ioctl_rd_readdir(deviceFd, fd, &address);
        printf("return: %d\n", ret);
        printf("\n");
    }

    free(address);

	printf("End test_restore() -----------\n\n\n");	
}

int main() {
	int fd;
	int choice = 999;

	fd = open(DEVICE_PATH, O_RDONLY);
	if (fd < 0) {
		printf("Failed to open ramdisk.\n");
		return -1;
	}

	while (choice > 0) {
	printf("Welcome to the Ramdisk Tester!\n");
	printf("(0) Exit Ramdisk Tester\n");
	printf("(1) rd_creat    (2) rd_mkdir    (3) rd_open \n");
	printf("(4) rd_close    (5) rd_write    (6) rd_read \n");
	printf("(7) rd_lseek    (8) rd_unlink   (9) rd_readdir \n");
	printf("Extra credit    (10) rd_sync    (11) rd_restore \n");
	printf("Choose test: ");
	scanf("%d", &choice); 
	while (choice < 0 || choice > 11) {
		printf("Choose test: ");
		scanf("%d", &choice);
	}

	if (choice == 0)
		break;
	if (choice == 1)
		test_creat(fd);
	if (choice == 2)
		test_mkdir(fd);
	if (choice == 3)
		test_open(fd);
	if (choice == 4)
		test_close(fd);
	if (choice == 5)
		test_write(fd);
	if (choice == 6)
		test_read(fd);
	if (choice == 7)
		test_lseek(fd);
	if (choice == 8)
		test_unlink(fd);
	if (choice == 9)
		test_readdir(fd);
/*
	if (choice == 10)
		test_sync(fd);
	if (choice == 11)
		test_restore(fd);
*/
	}

    return 0;
}

