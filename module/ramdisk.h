#ifndef RAMDISK_H
#define RAMDISK_H

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/utsname.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/string.h>
#include <asm/unistd.h>
                                                                                                                
#define RAMDISK_SIZE 2097152
#define RD_BLOCK_SIZE 256
#define SUPERBLOCK_SIZE BLOCK_SIZE
#define INODE_SIZE 65536
#define BLOCK_BITMAP_SIZE 1024
                                                                                                                
#define FREE 0
#define ALLOCATED 1
                                                                                                                
#define INODE_STRUCTURE_SIZE 64
#define INODE_TYPE_SIZE 4
#define INODE_BLOCK_POINTERS 10
#define INODE_PADDING_SIZE 6
#define INODE_COUNT 1024
                                                                                                                
#define DIRECT_SIZE 2048
#define SINGLE_INDIRECT_SIZE 16384
#define DOUBLE_INDIRECT_SIZE 1048576
                                                                                                                
#define DIRECT_LIMIT 2048
#define SINGLE_INDIRECT_LIMIT 18432
#define DOUBLE_INDIRECT_LIMIT 1067008
                                                                                                                
#define DIR_ENTRY_FILENAME_SIZE 14
#define DIR_ENTRY_STRUCTURE_SIZE 16
                                                                                                                
#define MAX_FILE_SIZE 1067008
#define MAX_FILES_OPEN INODE_COUNT

#define TRUE 1
#define FALSE 0

typedef struct {
    unsigned int freeBlocks;
    unsigned int freeInodes;
    char* blockBitmapStart;
    char* freeBlockStart;
} superblock;
                                                                                                                
                                                                                                                
typedef struct {
    short int inodeNumber;
    int status;
    int size;
    char type[INODE_TYPE_SIZE];
    char* location[INODE_BLOCK_POINTERS];
    short int locationCount;
    char padding[INODE_PADDING_SIZE];
} inode;
                                                                                                                
                                                                                                                
typedef struct {
    char fileName[DIR_ENTRY_FILENAME_SIZE];
    short int inodeNumber;
} dirEntry;
                                                                                                                
                                                                                                                
typedef struct {
    int filePosition;
    inode* inodePointer;
} fileDescriptorEntry;
                                                                                                                
                                                                                                                
// Linked list node stored in fileDescriptorProcessList
typedef struct fileDescriptorNode_t {
    int pid;
    fileDescriptorEntry fileDescriptorTable[MAX_FILES_OPEN];
    struct fileDescriptorNode_t* next;
} fileDescriptorNode;
                                                                                                                
                                                                                                                
typedef struct {
    char* pointers[64];
} singleIndirectLevel;
                                                                                                                
                                                                                                                
typedef struct {
    singleIndirectLevel* pointers[64];
} doubleIndirectLevel;



unsigned int bitPosition(unsigned int index);
unsigned int getBit(unsigned int* value, int bitPosition);
void setBit(unsigned int* value, int bitPosition);
void clearBit(unsigned int* value, int bitPosition);
int getMin(int a, int b);
 
// Initialization
void initBlockBitmap(void);
void initInodeArray(void);
int initRamdisk(void);
void destroyRamdisk(void);
                                                                                                                
void initFileDescriptorTable(void);
fileDescriptorNode* findFileDescriptor(fileDescriptorNode* trav, int pid);
int getFileDescriptorIndex(fileDescriptorNode* pointer);
int createFileDescriptor(int pid, int inodeNumber);
                                                                                                                
void printBlockBitmap(void);
char* getFreeBlock(void);
void parse(char* pathname, char** parents, char** fileName);
void setBitmap(char* blockPointer);
char* allocateBlock(inode* node);
int existsInBlock(char* blockAddress, char* fileName, char* type);
int getLastEntry(char* blockAddress, char** lastEntry);
char* scanBlockForFreeSlot(char* blockAddress);
char* findLastEntry(int inodeNumber);
int deleteFromBlock(char* blockAddress, char* fileName, char* type, int parentInodeNumber);
int isDirEntry(int inodeNumber, char* fileName, char* type);
int unlinkHelper(int inodeNumber, char* fileName, char* type);
int getDirInodeNumber(char* pathname);
int validateFile(char* pathname, char* type);
dirEntry* getFreeDirEntry(int inodeNumber);
int mapFilepositionToMemAddr(inode* pointer, int filePosition, char** filePositionAddress);
int findFileDescriptorIndexByPathname(fileDescriptorNode* pointer, char* pathname);
int isFileInFDProcessList(char* inodePointer);
 
// File operations
int create(char* pathname, char* type);
int rd_creat(char* pathname);
int rd_mkdir(char* pathname);
int rd_open(char* pathname);
int rd_close(int fd);
int rd_read(int fd, char* address, int num_bytes);
int rd_write(int fd, char *address, int num_bytes);
int rd_lseek(int fd, int offset);
int rd_unlink(char* pathname);
int rd_readdir(int fd, char* address);
int rd_sync();
int rd_restore();

int getpid(void);
#endif
