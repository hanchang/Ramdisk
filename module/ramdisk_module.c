/* ramdisk_module.c
 * Inserts the ramdisk as a module into the kernel.
 * Kevin Hu (khu19), Han Chang (halo)
 */

#include <linux/vmalloc.h>
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

#include "ramdisk.h"	
#include "../src/ramdisk_ioctl.h"    // Includes definitions for our ioctl commands

// -- Global Variable Declarations ---------------------------------------------
// -----------------------------------------------------------------------------
static char* ramdisk;
static superblock* sb;
static inode* inodeArray;
static fileDescriptorNode* fileDescriptorProcessList;
static struct file_operations ramdiskOperations;
static struct proc_dir_entry *proc_entry;
static struct proc_dir_entry *proc_backup;
static char procfs_buffer[RAMDISK_SIZE];


int getpid(void) {
    return current->pid;
}
//===========================================================================//

// -- Bit Manipulation Functions -----------------------------------------------
// -----------------------------------------------------------------------------

// Index refers to the power of 2
// Therefore, index 0 = 2^0 and 1 = 2^1...etc
unsigned int bitPosition(unsigned int index) {
    return (0x01 << index);
}


unsigned int getBit(unsigned int* value, int bitPosition) {
    return (*value & bitPosition);
}


void setBit(unsigned int* value, int bitPosition) {
    *value |= bitPosition;
}


void clearBit(unsigned int* value, int bitPosition) {
    *value &= ~(bitPosition);
}


int getMin(int a, int b) {
	if (a <= b) return a;
	else 		return b;
}


// -- Bitmap Functions -----------------------------------------------------------
// -------------------------------------------------------------------------------

void initBlockBitmap(void) {
    char* blockBitmapIter;
    int i;
    blockBitmapIter = sb->blockBitmapStart;
    for (i = 0; i < BLOCK_BITMAP_SIZE; i++) {
        *blockBitmapIter = FREE;
        blockBitmapIter++;
    }
}


// Get the next vfree block and update the block bitmap
char* getFreeBlock(void) {
    int bitNumber;
    int byteNumber;
    int blockNumber;
    char* blockAddress;

    // Iterate all the bits in the block bitmap
    for (byteNumber = 0; byteNumber < BLOCK_BITMAP_SIZE; byteNumber++) {
        // Iterate the specific bits in a single byte
        for (bitNumber = 0; bitNumber < 8; bitNumber++) {
            // Find the next vfree block
            if (getBit((unsigned int *)(byteNumber + sb->blockBitmapStart), bitPosition(bitNumber)) == FREE) {
                sb->freeBlocks--;
                setBit((unsigned int *)(sb->blockBitmapStart + byteNumber), bitPosition(bitNumber));
                // Calculate the vfree block addr given the bit num and byte num
                blockNumber = (byteNumber * 8) + bitNumber;
                blockAddress = sb->freeBlockStart + (RD_BLOCK_SIZE * blockNumber);  
                memset(blockAddress, 0, RD_BLOCK_SIZE);
                return blockAddress;         
            }             
        }
    }
    // Means no more vfree blocks - the caller should check
    // this before even calling this function
    return NULL;
}


void printBlockBitmap(void) {
    int byteNumber;
    int bitNumber;
    int bitValue;

    // Iterate all the bits in the block bitmap
    for (byteNumber = 0; byteNumber < BLOCK_BITMAP_SIZE; byteNumber++) {
        // Iterate the specific bits in a single byte
		//printk("byte: %d   ", byteNumber);
        for (bitNumber = 0; bitNumber < 8; bitNumber++) {
            // Find the next vfree block
            bitValue = getBit((unsigned int *)(byteNumber + sb->blockBitmapStart), bitPosition(bitNumber));
			if (bitValue != FREE) {
          //      printk("%d ", 1);
			}
			else {
		//		printk("%d ", 0);
			}
        }
	//	printk("\n");
    }
}
      

// -- Inode Array Functions ----------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

void initInodeArray(void) {
    int i, j;
    for (i = 0; i < INODE_COUNT; i++) {
        inodeArray[i].inodeNumber = i;
        inodeArray[i].status = FREE;
        strcpy(inodeArray[i].type, "nul");
        inodeArray[i].size = 0;
        for (j = 0; j < INODE_BLOCK_POINTERS; j++) {     
            inodeArray[i].location[j] = NULL;
        }
        inodeArray[i].locationCount = 0;
    }
}


// -- Ramdisk Maintenance Functions --------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

int initRamdisk(void) {
    char* superblockStart;
    char* inodeArrayStart;
    char* blockBitmapStart;
    char* freeBlockStart;
    int freeBlocks, freeInodes;

    ramdisk = (void*) vmalloc(RAMDISK_SIZE);
    if (!ramdisk) {
        printk("Could not allocate memory for ramdisk.\n");
        return -1;
    }
    superblockStart = ramdisk;
    inodeArrayStart = superblockStart + SUPERBLOCK_SIZE; 
    blockBitmapStart = inodeArrayStart + INODE_SIZE;
    freeBlockStart = blockBitmapStart + BLOCK_BITMAP_SIZE;

    // Calculate the number of initial vfree blocks and inodes
    freeBlocks = (RAMDISK_SIZE - SUPERBLOCK_SIZE - INODE_SIZE - BLOCK_BITMAP_SIZE) / RD_BLOCK_SIZE;
    freeInodes = INODE_SIZE / INODE_STRUCTURE_SIZE;
    
    // Starting addr of the superblock struct is the starting address of the superblock
    // NOTE: no need to vmalloc memory for sb since it is pointing to a 2mb block of malloc'ed memory
    sb = (superblock*)superblockStart;
    sb->freeBlocks = freeBlocks;
    sb->freeInodes = freeInodes - 1;
    sb->blockBitmapStart = blockBitmapStart;
    sb->freeBlockStart = freeBlockStart;
    printk("sb->freeBlocks: %d\n", sb->freeBlocks);
    // Set starting address of inode array
    inodeArray = (inode*)inodeArrayStart;

    // Set all bits in the block bitmap to zero
    initBlockBitmap();
    
    // Initialize the inodes
    initInodeArray();

    // Initialize an inode for root
    inodeArray[0].inodeNumber = 0;
    inodeArray[0].status = ALLOCATED;
    strcpy(inodeArray[0].type, "dir");
    inodeArray[0].size = 0;
    inodeArray[0].location[0] = getFreeBlock();
    inodeArray[0].locationCount++;
    return 1;
}


void destroyRamdisk(void) {
    if (ramdisk) {
        vfree(ramdisk);
    }
    if (fileDescriptorProcessList) {
        vfree(fileDescriptorProcessList);
    }
    ramdisk = NULL;
    sb = NULL;
    inodeArray = NULL;
}



// -- File Descriptor Table Functions -------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

void initFileDescriptorTable(void) {
    // Initialize global file descriptor linked list.
    fileDescriptorNode* dummyHead;
    dummyHead = (fileDescriptorNode*) vmalloc(sizeof(fileDescriptorNode));
    dummyHead->pid = 0;
    dummyHead->next = NULL;
    fileDescriptorProcessList = dummyHead;
}

// Checks to see if the pid is in the linked list.
fileDescriptorNode* findFileDescriptor(fileDescriptorNode* trav, int pid) {
    // Dont want to call getpid here, b/c the kernel will have to pass in
    // the pid of the calling process
    while (trav) {
        if (trav->pid == pid) {
            return trav;
		}
        trav = trav->next;
    }
    return NULL;
}

// Finds the first empty entry in the process's file descriptor table.
int getFileDescriptorIndex(fileDescriptorNode* pointer) {
    int i;
    for (i = 0; i < MAX_FILES_OPEN; i++) {
        // Found an empty entry in the table.
        if (pointer->fileDescriptorTable[i].filePosition == -1)
            return i;
    }
    return -1;
}


// Makes a new entry in the process's file descriptor table.
int createFileDescriptor(int pid, int inodeNumber) {
    // Check if the pid is in the linked list.
    fileDescriptorNode* check;
    int fd, i;
    check = findFileDescriptor(fileDescriptorProcessList, getpid());

    if (check != NULL) {
        // Found the pid; insert into the array table.
        if (check->pid == pid) {
            // Find an empty table entry and insert
            fd = getFileDescriptorIndex(check);
            check->fileDescriptorTable[fd].inodePointer = &inodeArray[inodeNumber];
            check->fileDescriptorTable[fd].filePosition = 0;

            return fd;
        }
    }
    else {
        // The pid was not found in the linked list, need to add to list.
        fileDescriptorNode* newEntry = (fileDescriptorNode*) vmalloc(sizeof(fileDescriptorNode));


        // Want to initialize his fileDescriptorEntry table
        for (i = 0; i < MAX_FILES_OPEN; i++) {
            newEntry->fileDescriptorTable[i].filePosition = -1;
            newEntry->fileDescriptorTable[i].inodePointer = NULL;
        }

        newEntry->pid = pid;
        // Insert to the front of the linked list.
        newEntry->next = fileDescriptorProcessList;
        fileDescriptorProcessList = newEntry;

        fd = getFileDescriptorIndex(newEntry);

        newEntry->fileDescriptorTable[fd].inodePointer = &inodeArray[inodeNumber];
        newEntry->fileDescriptorTable[fd].filePosition = 0;

        // Print inode info for debug
        return fd;
    }
    return -1;
}



// -- Helper Functions ----------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

// Given a pathname, fills parents with path of parents
// Fill fileName with the name of the filename you are creating
void parse(char* pathname, char** parents, char** fileName) {
    // Create a copy of the pathname string
    char* copyPathname;
    char* lastSlash;
    int indexOfLastSlash;
    char* temp;
    int length;

    length = strlen(pathname);
    copyPathname = (char*) vmalloc(sizeof(char) * (length + 1));
    strcpy(copyPathname, pathname);
    // Return ptr to last occurrence of a '/'
    lastSlash = strrchr(copyPathname, '/');
    indexOfLastSlash = lastSlash - copyPathname;

    // Extract the parent path names
    *parents = (char*) vmalloc(sizeof(char) * indexOfLastSlash + 2);
    memcpy(*parents, copyPathname, indexOfLastSlash + 1);
	temp = *parents;
	temp += (indexOfLastSlash + 1);
	*temp = '\0';

    // Extract the name of the dir we are trying to make
    *fileName = (char*) vmalloc(sizeof(char) * (length - indexOfLastSlash));
    strcpy(*fileName, lastSlash + 1);
    
}


void setBitmap(char* blockPointer) {
    // Find the block number by subtracting memory addresses.
    int blockNumber;
    int bitmapByteIndex;
    int bitmapBitIndex;
    char* positionInBitmap;


    blockNumber = ((blockPointer - sb->freeBlockStart) / RD_BLOCK_SIZE);

    bitmapByteIndex = blockNumber / 8;
    bitmapBitIndex  = blockNumber % 8;

    positionInBitmap = sb->blockBitmapStart + bitmapByteIndex;
	sb->freeBlocks++;

    clearBit((unsigned int*)positionInBitmap, bitPosition(bitmapBitIndex));
}


// Given an inode pointer, allocate another block for this file.
// First attempts to create a direct point, if all 8 are used, then it
// tries to create an indirect pointer, etc...
char* allocateBlock(inode* node) {
	// Get total number of location pointers used
	int locationCount;
    int iter, iter1,iter2;
    singleIndirectLevel* level;
    doubleIndirectLevel* doubleLevel;

    level = NULL;
    doubleLevel = NULL;
    locationCount = node->locationCount;
    // Still have room to make single direct pointer
    if (locationCount < 8) {
        node->location[locationCount] = getFreeBlock();
        node->locationCount++;
        return node->location[locationCount];
    }
    // Must make the first single indirect pointer
    else if (locationCount == 8) {
        node->location[8] = getFreeBlock(); // stores 64 ptrs
        level = (singleIndirectLevel*) node->location[8];
        level->pointers[0] = getFreeBlock();
        node->locationCount++;
        return level->pointers[0];
    }
    // Iterate single indirect pointers looking for vfree spot
    else if (locationCount == 9) {
        level = (singleIndirectLevel*) node->location[8];
        
        // Search for a vfree spot to get new block
        for (iter = 0; iter < 64; iter++) {
            if (level->pointers[iter] == NULL) {
                level->pointers[iter] = getFreeBlock();
                return level->pointers[iter];
            }
        }
        // Need to create double indirect level
        node->location[9] = getFreeBlock(); // second level
        doubleLevel = (doubleIndirectLevel*) node->location[9];
        doubleLevel->pointers[0] = (singleIndirectLevel*) getFreeBlock();
        doubleLevel->pointers[0]->pointers[0] = getFreeBlock();
        node->locationCount++;
        return doubleLevel->pointers[0]->pointers[0];
    }
    // Iterate level two pointers
    else if (locationCount == 10) {
        doubleLevel = (doubleIndirectLevel*) node->location[9];
        for (iter1 = 0; iter1 < 64; iter1++) {
            if (doubleLevel->pointers[iter1] == NULL) {
                doubleLevel->pointers[iter1] = (singleIndirectLevel*) getFreeBlock();
                doubleLevel->pointers[iter1]->pointers[0] = getFreeBlock();
                return doubleLevel->pointers[iter1]->pointers[0];
            }
            level = doubleLevel->pointers[iter1];
            for (iter2 = 0; iter2 < 64; iter2++) {
                if (level->pointers[iter2] == NULL) {
                    level->pointers[iter2] = getFreeBlock();
                    return level->pointers[iter2];
                }
            }
        }
    }

    return NULL;
}


// Check if a file already exists.  This means that the file is a directory
// entry for another directory.  iNodenumber is the directory to search in.
// fileName is the name of the file to search for and type is specifies
// whether we are looking for a regular or directory file
// Returns -1 if the file does not exist in the directory specified by 
// the inodeNumber
// Otherwise it returns the inodeNumber of file we searched for
int existsInBlock(char* blockAddress, char* fileName, char* type) {
    dirEntry* dirTraverser;
    char* dirEntryFileName;
    int dirEntryInodeNumber;
    char* dirEntryType;
    int i;
    char* dirEntryIter;
    int entryCount;

    dirEntryIter = blockAddress;
    entryCount = RD_BLOCK_SIZE / DIR_ENTRY_STRUCTURE_SIZE;
    // Traverse all directory entries
    for (i = 0; i < entryCount; i++) {
        dirTraverser = (dirEntry*) dirEntryIter;
        dirEntryFileName = dirTraverser->fileName;
        dirEntryInodeNumber = dirTraverser->inodeNumber;
        dirEntryType = inodeArray[dirEntryInodeNumber].type;
    
        // Cut the search short, stop when you find an empty slot
        if (strcmp(dirEntryFileName, "/") != 0 && dirEntryInodeNumber == 0) {
            return -1;
        }
        // If the names and type are the same then it exists
        // So return the inode number of the matched file
        if (strcmp(dirEntryFileName, fileName) == 0) {
		    if ((strcmp("ign", type) == 0) || (strcmp(dirEntryType, type) == 0)) {
//                printk("existsInBlock: found a match, returning dirEntryInodeNumber = %d\n", dirEntryInodeNumber);
                return dirEntryInodeNumber;
            }
		}	
        
        // Jump to the next directory entry in the directory
        dirEntryIter += DIR_ENTRY_STRUCTURE_SIZE;
    }
    return -1;
}


// lastEntry is a pointer to the last entry in a block
// If the block is empty, then return -1
// The caller should call setBitmap to release this block and decrement
// the location count of the calling inode
int getLastEntry(char* blockAddress, char** lastEntry) {
    dirEntry* dirTraverser;
    char* dirEntryIterPrev;
    char* dirEntryIter;
    int i;
    int entryCount;
    char* dirEntryFileName;
    int dirEntryInodeNumber;

    entryCount = RD_BLOCK_SIZE / DIR_ENTRY_STRUCTURE_SIZE;
    dirEntryIter = dirEntryIterPrev = blockAddress;
    // Traverse all directory entries
    for (i = 0; i < entryCount; i++) {
        dirTraverser = (dirEntry*) dirEntryIter;
        dirEntryFileName = dirTraverser->fileName;
        dirEntryInodeNumber = dirTraverser->inodeNumber;
	//	printk("getLastEntry:  dirEntryFileName = %s  dirEntryInodeNumber = %d\n", dirEntryFileName, dirEntryInodeNumber);

        // Cut the search short, stop when you find an empty slot
        if (strcmp(dirEntryFileName, "/") != 0 && dirEntryInodeNumber == 0) {
			if (dirEntryIter == dirEntryIterPrev) {
				*lastEntry = NULL;
				return -1;
			}
            *lastEntry = dirEntryIterPrev;
			return 1;
        }
        
        // Jump to the next directory entry in the directory
        dirEntryIterPrev = dirEntryIter;
        dirEntryIter += DIR_ENTRY_STRUCTURE_SIZE;
    }
	
	*lastEntry = dirEntryIterPrev;
    return 1;
}


char* scanBlockForFreeSlot(char* blockAddress) {
    dirEntry* dirTraverser;
    char* dirEntryIter = blockAddress;
    int i;
    char* dirEntryFileName;
    int dirEntryInodeNumber;
    int entryCount;
    entryCount = RD_BLOCK_SIZE / DIR_ENTRY_STRUCTURE_SIZE;
    // Traverse all directory entries
    for (i = 0; i < entryCount; i++) {
        dirTraverser = (dirEntry*) dirEntryIter;
        dirEntryFileName = dirTraverser->fileName;
        dirEntryInodeNumber = dirTraverser->inodeNumber;
        if (strcmp(dirEntryFileName, "/") != 0 && dirEntryInodeNumber == 0) {
            return dirEntryIter;
        }
        // Jump to the next directory entry in the directory
        dirEntryIter += DIR_ENTRY_STRUCTURE_SIZE;
    }
    return NULL;
}

char* findLastEntry(int inodeNumber) {
    int locationCount;
    char* blockAddress;
    singleIndirectLevel* indirectBlock;
    doubleIndirectLevel* doubleIndirectBlock;
    char* dirEntryIter;
    int i, j, count;
    char* lastEntry;
    int ret;

    locationCount = inodeArray[inodeNumber].locationCount;
    
   
    if (locationCount == 10) {
        // Get base address of double indirect block
        doubleIndirectBlock = (doubleIndirectLevel*) inodeArray[inodeNumber].location[9];

        // Iterate all indirect pointers
        for (i = 63; i >= 0; i--) {
            // Get indirect block
            indirectBlock = doubleIndirectBlock->pointers[i];

            if (!indirectBlock) {
                continue;
            }

            // Iterate all 64 block pointers
            for (j = 63; j >= 0; j--) {
                // Get block pointed to by indirect pointer
                dirEntryIter = indirectBlock->pointers[j];

                if (!dirEntryIter) {
                    continue;
                }
                // Check if this file exists in the block
                ret = getLastEntry(dirEntryIter, &lastEntry);
                // If the files inode number is not -1, then return this inode
                if (ret == 1) {
                    return lastEntry;
                }
				else if (ret == -1) {
					setBitmap(dirEntryIter);
					indirectBlock->pointers[j] = NULL;
   		           	if (j == 0) {
                   		setBitmap((char*)indirectBlock);
						doubleIndirectBlock->pointers[i] = NULL;
                	}
				}
            }
        }
		setBitmap(inodeArray[inodeNumber].location[9]);
		inodeArray[inodeNumber].location[9] = NULL;
		inodeArray[inodeNumber].locationCount--;
    }
	locationCount = inodeArray[inodeNumber].locationCount;
    if (locationCount > 8) {
        // Get the base address of the block holding indirect pointers
        indirectBlock = (singleIndirectLevel*) inodeArray[inodeNumber].location[8];

        // Iterate all 64 block pointers
        for (i = 63; i >= 0; i--) {
            // Get block pointed to by indirect pointer
            dirEntryIter = indirectBlock->pointers[i];

            // If this single indirect block has no more pointers to blocks...
            if (!dirEntryIter) {
                continue;
            }

            // Check if this file exists in the block
            ret = getLastEntry(dirEntryIter, &lastEntry);
            // If the files inode number is not -1, then return this inode
            if (ret == 1) {
            	return lastEntry;
            }
            else if (ret == -1) {
                setBitmap(dirEntryIter);
				indirectBlock->pointers[i] = NULL;
				if (i == 0) {
					setBitmap(inodeArray[inodeNumber].location[8]);
					inodeArray[inodeNumber].location[8] = NULL;
					inodeArray[inodeNumber].locationCount--;
				}
            }
        }
    }

	locationCount = inodeArray[inodeNumber].locationCount;
        count = locationCount - 1;
        while (count >= 0) {
            // Get base address of the block in question
            blockAddress = inodeArray[inodeNumber].location[count];
 			
            // Check if this file exists in the block
            ret = getLastEntry(blockAddress, &lastEntry);
            // If the files inode number is not -1, then return this inode
            if (ret == 1) {
                return lastEntry;
            }
            else if (ret == -1) {
                //if (count == 0) {
                //    memset(blockAddress, 0, BLOCK_SIZE);
                 //   printk("not deleting last block\n");
                //}
                //else {
                    setBitmap(blockAddress);
                    inodeArray[inodeNumber].locationCount--;
                //}
            }
		
            // Otherwise check the next block
            count--;
        }                
  //  }
	printk("didnt get valid last entry\n");
    return NULL;
}



// Scan the block pointed to by blockAddress looking for fileName
// If it is found, delete its entry
// Otherwise return -1
// Move the last directory entry to the newly opened slot
int deleteFromBlock(char* blockAddress, char* fileName, char* type, int parentInodeNumber) {
    dirEntry* dirTraverser;
    char* dirEntryIter = blockAddress;
    int i;
    int entryCount;
    char* dirEntryFileName;
    int dirEntryInodeNumber;
    char* dirEntryType;
    char* lastEntry;

    entryCount = RD_BLOCK_SIZE / DIR_ENTRY_STRUCTURE_SIZE;
    // Traverse all directory entries
    for (i = 0; i < entryCount; i++) {
        dirTraverser = (dirEntry*) dirEntryIter;
        dirEntryFileName = dirTraverser->fileName;
        dirEntryInodeNumber = dirTraverser->inodeNumber;
        dirEntryType = inodeArray[dirEntryInodeNumber].type;

        // Cut the search short, stop when you find an empty slot
        if (strcmp(dirEntryFileName, "/") != 0 && dirEntryInodeNumber == 0) {
            return -1;
        }

        // If the names and type are the same then it exists
        // So return the inode number of the matched file
        if (strcmp(dirEntryFileName, fileName) == 0) {
            if ((strcmp("ign", type) == 0) || (strcmp(dirEntryType, type) == 0)) {
				
                // Move the last entry here
                // Delete the last entry
                lastEntry = findLastEntry(parentInodeNumber);

                // Copy guy at lastEntry into slot dirEntryIter
                memcpy(dirEntryIter, lastEntry, DIR_ENTRY_STRUCTURE_SIZE);

                // vfree that other guy
                memset(lastEntry, 0, DIR_ENTRY_STRUCTURE_SIZE);

                return dirEntryInodeNumber;
            }
        }

        // Jump to the next directory entry in the directory
        dirEntryIter += DIR_ENTRY_STRUCTURE_SIZE;
    }
    return -1;
}



// Gets the inode number of the directory entry.
int isDirEntry(int inodeNumber, char* fileName, char* type) {
    int locationCount;
    int directCount;
    int dirEntryInodeNumber;
    char* blockAddress;
    singleIndirectLevel* indirectBlock;
    doubleIndirectLevel* doubleIndirectBlock;
    char* dirEntryIter;
    int i, pointerCount, count;
    
    directCount = count = 0;
    pointerCount = 64;
    locationCount = inodeArray[inodeNumber].locationCount;
    if (locationCount > 8) {
        directCount = 8;
    }
    else {
        directCount = locationCount;
    }
 
   
    // Iterate the direct pointers first
    while (count < directCount) {
        // Get base address of the block in question
        blockAddress = inodeArray[inodeNumber].location[count];
        
        // Check if this file exists in the block
        dirEntryInodeNumber = existsInBlock(blockAddress, fileName, type);
    	
        // If the files inode number is not -1, then return this inode
        if (dirEntryInodeNumber != -1) {
            return dirEntryInodeNumber;
        }
        
        // Otherwise check the next block
        count++;                
    }
    // Iterate single indirect pointers
    if (locationCount > 8) {
        // Get the base address of the block holding indirect pointers
        indirectBlock = (singleIndirectLevel*) inodeArray[inodeNumber].location[8];
        
        // Iterate all 64 block pointers
        for (i = 0; i < pointerCount; i++) {
            // Get block pointed to by indirect pointer
            dirEntryIter = indirectBlock->pointers[i];

            // If this single indirect block has no more pointers to blocks...    
            if (!dirEntryIter) {
                break;
            }

            // Check if this file exists in the block
            dirEntryInodeNumber = existsInBlock(dirEntryIter, fileName, type);
            // If the files inode number is not -1, then return this inode
            if (dirEntryInodeNumber != -1) {
                return dirEntryInodeNumber;
            }
        }
    }
    // Check double indirect pointers
    if (locationCount == 10) {
        // Get base address of double indirect block
        doubleIndirectBlock = (doubleIndirectLevel*) inodeArray[inodeNumber].location[9];
        
        // Iterate all indirect pointers
        for (i = 0; i < pointerCount; i++) {
            // Get indirect block
            indirectBlock = doubleIndirectBlock->pointers[i];
    
            if (!indirectBlock) {
                break;
            }

            // Iterate all 64 block pointers
            for (i = 0; i < pointerCount; i++) {
                // Get block pointed to by indirect pointer
                dirEntryIter = indirectBlock->pointers[i];

                if (!dirEntryIter) {
                    break;
                }

                // Check if this file exists in the block
                dirEntryInodeNumber = existsInBlock(dirEntryIter, fileName, type);

                // If the files inode number is not -1, then return this inode
                if (dirEntryInodeNumber != -1) {
                    return dirEntryInodeNumber;
                }
            }
        }    
    }
    return -1;
}

int unlinkHelper(int inodeNumber, char* fileName, char* type) {
    int locationCount;
    int directCount;
    int dirEntryInodeNumber;
    char* blockAddress;
    singleIndirectLevel* indirectBlock;
    doubleIndirectLevel* doubleIndirectBlock;
    char* dirEntryIter;
    int i, pointerCount, count;

    directCount = count = 0;
    pointerCount = 64;
    locationCount = inodeArray[inodeNumber].locationCount;

    if (locationCount > 8) {
        directCount = 8;
    }
    else {
        directCount = locationCount;
    }


    // Iterate the direct pointers first
    while (count < directCount) {
        // Get base address of the block in question
        blockAddress = inodeArray[inodeNumber].location[count];

        // Check if this file exists in the block
        dirEntryInodeNumber = deleteFromBlock(blockAddress, fileName, type, inodeNumber);

        // If the files inode number is not -1, then return this inode
        if (dirEntryInodeNumber != -1) {
            return dirEntryInodeNumber;
        }

        // Otherwise check the next block
        count++;
    }
    // Iterate single indirect pointers
    if (locationCount > 8) {
        // Get the base address of the block holding indirect pointers
        indirectBlock = (singleIndirectLevel*) inodeArray[inodeNumber].location[8];

        // Iterate all 64 block pointers
        for (i = 0; i < pointerCount; i++) {
            // Get block pointed to by indirect pointer
            dirEntryIter = indirectBlock->pointers[i];

            // If this single indirect block has no more pointers to blocks...
            if (!dirEntryIter) {
                break;
            }

            // Check if this file exists in the block
            dirEntryInodeNumber = deleteFromBlock(dirEntryIter, fileName, type, inodeNumber);
            // If the files inode number is not -1, then return this inode
            if (dirEntryInodeNumber != -1) {
                return dirEntryInodeNumber;
            }
        }
    }
    // Check double indirect pointers
    if (locationCount == 10) {
        // Get base address of double indirect block
        doubleIndirectBlock = (doubleIndirectLevel*) inodeArray[inodeNumber].location[9];

        // Iterate all indirect pointers
        for (i = 0; i < pointerCount; i++) {
            // Get indirect block
            indirectBlock = doubleIndirectBlock->pointers[i];

            if (!indirectBlock) {
                break;
            }

            // Iterate all 64 block pointers
            for (i = 0; i < pointerCount; i++) {
                // Get block pointed to by indirect pointer
                dirEntryIter = indirectBlock->pointers[i];

                if (!dirEntryIter) {
                    break;
                }
                // Check if this file exists in the block
                dirEntryInodeNumber = deleteFromBlock(dirEntryIter, fileName, type, inodeNumber);

                // If the files inode number is not -1, then return this inode
                if (dirEntryInodeNumber != -1) {
                    return dirEntryInodeNumber;
                }
            }
        }
    }
    return -1;
}

// Get the inode number of a directory  
int getDirInodeNumber(char* pathname) {
    char* dirName;
    int inodeNumber;
    int dirInodeNum;
    // Get the first "dir"
    dirName = strsep(&pathname, "/");
    dirName = strsep(&pathname, "/");
    // Initialize search from root
    inodeNumber = 0;

    // Search through all parent directories
    while (dirName && strlen(dirName) > 0) {
        // See if dirName exists in its parent
        dirInodeNum = isDirEntry(inodeNumber, dirName, "dir");
        // If so, then continue search into next folder 
        if (dirInodeNum > -1) {
            inodeNumber = dirInodeNum;
        }
        else {
            return -1;
        }
        // Get the next parent directory
        dirName = strsep(&pathname, "/");
        printk("dirName: %s\n", dirName);
    }
    return inodeNumber;
}       


// Check to see that the directory specified by pathname is a valid request.
// All parent directories must exist, and the directory must not already exist.
// Returns the inode number of the parent directory where the new directory
// is going to be created.  If this request is not valid, it returns -1
int validateFile(char* pathname, char* type) {
    char* fileName;
	char* parents; 
    int parentInodeNum;
    int isDir;

//    printk("validateFile: pathname = %s  type = %s\n", pathname, type);
    fileName = parents = NULL;
	parse(pathname, &parents, &fileName);
    
//    printk("validateFile: parents = %s  fileName = %s\n", parents, fileName);

    // Get the inode number of the immediate parent "dir"
    parentInodeNum = getDirInodeNumber(parents);
    printk("validateFile: parentInodeNum = %d\n", parentInodeNum);
	vfree(parents);
	parents = NULL;

    // Parent doesn't exist
    if (parentInodeNum == -1) {
        vfree(fileName);
		fileName = NULL;
        return -1;
    }


//   printk("validateFile: parents exists\n");
    // Parents do exist so check if this directory already exists
    isDir = isDirEntry(parentInodeNum, fileName, type);
//    printk("validateFile: isDirEntry returned %d\n", isDir);
    vfree(fileName);
	fileName = NULL;

    // Directory doesn't exist, so rd_mkdir is allowed to create it.
    // Return the inode number of the parent directory
    if (isDir == -1) {
        return parentInodeNum;
    }
    else {
        // Directory already exists, so don't make it, so tell rd_mkdir -1 
        return -1;
    }
}


// Given an inode number, find the position in memory where the next directory
// can be created.  
dirEntry* getFreeDirEntry(int inodeNumber) {
    int locationCount;
    char* freeSlot;
    char* blockAddress;
    singleIndirectLevel* indirectBlock;
    doubleIndirectLevel* doubleIndirectBlock;
    char* dirEntryIter;
    int i, pointerCount, count;

    pointerCount = 64;
    locationCount = inodeArray[inodeNumber].locationCount;
    count = 0;

    // Iterate the direct pointers first
    if (locationCount <= 8) {
        //while (count < locationCount) {
            // Get base address of the block in question
            blockAddress = inodeArray[inodeNumber].location[locationCount - 1];

            // See if this block has a vfree slot
            freeSlot = scanBlockForFreeSlot(blockAddress);

            if (freeSlot) {
                return ((dirEntry*) freeSlot);
            }

            // Otherwise check the next block
          //  count++;
      //  }  
    }
    // Iterate single indirect pointers
    else if (locationCount == 9) {
        // Get the base address of the block holding indirect pointers
        indirectBlock = (singleIndirectLevel*) inodeArray[inodeNumber].location[8];

        // Iterate all 64 block pointers
        for (i = 0; i < pointerCount; i++) {
            // Get block pointed to by indirect pointer
            dirEntryIter = indirectBlock->pointers[i];

            if (!dirEntryIter) {
                break;
            }
            // See if this block has a vfree slot
            freeSlot = scanBlockForFreeSlot(dirEntryIter);
            if (freeSlot) {
                return ((dirEntry*) freeSlot);
            }
        }
    }
    // Check double indirect pointers
    else if (locationCount == 10) {
        // Get base address of double indirect block
        doubleIndirectBlock = (doubleIndirectLevel*) inodeArray[inodeNumber].location[9];

        // Iterate all indirect pointers
        for (i = 0; i < pointerCount; i++) {
            // Get indirect block
            indirectBlock = doubleIndirectBlock->pointers[i];

            if (!indirectBlock) {
                break;
            }

            // Iterate all 64 block pointers
            for (i = 0; i < pointerCount; i++) {
                // Get block pointed to by indirect pointer
                dirEntryIter = indirectBlock->pointers[i];

                if (!dirEntryIter) {
                    break;
                }

                // See if this block has a vfree slot
                freeSlot = scanBlockForFreeSlot(dirEntryIter);

                if (freeSlot) {
                    return ((dirEntry*) freeSlot);
                }
            }
        }
    }
    
    // If we're here, then the inode has no more vfree slots and needs a new block
    inode* inodePointer = &inodeArray[inodeNumber];
    dirEntry* freeEntry = (dirEntry*) allocateBlock(inodePointer);
    return freeEntry;
}


// Given a pointer to an inode, and a file position, return a pointer to the
// address in memory for this file position
int mapFilepositionToMemAddr(inode* pointer, int filePosition, char** filePositionAddress) {
    int locationNumber;
    int shiftWithinBlock;
    char* filePositionTemp;
    char* blockPtr;
    int indirectShift;

    // Exist in a direct block, so just shift accordingly
    if (filePosition < DIRECT_LIMIT) {
        filePositionTemp = NULL;
        locationNumber = filePosition / RD_BLOCK_SIZE;

        // Point to the first address of the block
        filePositionTemp = pointer->location[locationNumber];

        shiftWithinBlock = filePosition % RD_BLOCK_SIZE;
        filePositionTemp += shiftWithinBlock;
        *filePositionAddress = filePositionTemp;
    }
    // Exists in single indirect, so filePositionTemp points to a block of pointers to blocks
    else if (filePosition < SINGLE_INDIRECT_LIMIT) {
        singleIndirectLevel* firstLevel = (singleIndirectLevel*) pointer->location[8];
        filePosition -= DIRECT_SIZE;

        indirectShift = filePosition / RD_BLOCK_SIZE;
        blockPtr = firstLevel->pointers[indirectShift];

        shiftWithinBlock = filePosition % RD_BLOCK_SIZE;
        blockPtr += shiftWithinBlock;
        *filePositionAddress = blockPtr;
    }
    else if (filePosition < DOUBLE_INDIRECT_LIMIT) {
        blockPtr = NULL;
        doubleIndirectLevel* doubleLevel = (doubleIndirectLevel*) pointer->location[9];
        singleIndirectLevel* singleLevel;

        // Subtract size of direct blocks and single indirect blocks
        filePosition -= SINGLE_INDIRECT_SIZE;
        filePosition -= DIRECT_SIZE;

        // Divide by size of single indirect block to determine offset
        // withing double indirect block
        indirectShift = filePosition / SINGLE_INDIRECT_SIZE;

        singleLevel = doubleLevel->pointers[indirectShift];

        // Decrement filePosition by the number of (single indirect * increment)
        filePosition -= (indirectShift * SINGLE_INDIRECT_SIZE);

        // Divide by block size to determine increment in single indirect block
        indirectShift = filePosition / RD_BLOCK_SIZE;
        blockPtr = singleLevel->pointers[indirectShift];

        // Find the shift within the block containing data
        shiftWithinBlock = filePosition % RD_BLOCK_SIZE;
        blockPtr += shiftWithinBlock;
        *filePositionAddress = blockPtr;
    }
    else {
        return -1;
    }
    return RD_BLOCK_SIZE - shiftWithinBlock;
}


// Returns the file descriptor index of pathname given a pointer to the
// file descriptor node containing a file descriptor table.
int findFileDescriptorIndexByPathname(fileDescriptorNode* pointer, char* pathname) {
    int i;
    int parentInodeNumber;
    int fileInodeNumber;
    char* fileName;
    char* parents;
    char* inodePointer;
    inode inodePtr;
    
    fileName = parents = inodePointer = NULL;
    // Find pathname's inode number or ptr to inode.
    parse(pathname, &parents, &fileName);
    // Look for filename in parent's directory entries.
    parentInodeNumber = getDirInodeNumber(parents);
    fileInodeNumber = isDirEntry(parentInodeNumber, fileName, "reg");
    inodePtr = inodeArray[fileInodeNumber];
    inodePointer = (char*) &inodePtr;
    for (i = 0; i < MAX_FILES_OPEN; i++) {
        // Found an empty entry in the table.
        //if (pointer->fileDescriptorTable[i].filePosition == 1) {
            // pathname's inode pointer == i node ptr of a file in fdt
            if ((char*) pointer->fileDescriptorTable[i].inodePointer == inodePointer)
                return i;
        //}
    }
    return -1;
}





// Looks to see if a file denoted by fd is in the fileDescriptorProcessList.
int isFileInFDProcessList(char* inodePointer) {
    // Change inodePointer to an inodeNumber.

    int i;
    fileDescriptorNode* trav = fileDescriptorProcessList;
//  printk("isFileInFDProcessList\n");
    while(trav) {
//      printk("isFileInFDProcessList: in while\n");
        for(i = 0; i < MAX_FILES_OPEN; i++) {
//          printk("isFileInFDProcessList: in for %d\n", i);
            if (trav->fileDescriptorTable[i].filePosition > -1) {// != NULL) {
//              printk("Non null\n");
                if ((char*) trav->fileDescriptorTable[i].inodePointer == inodePointer) {
                    // Found a process with it open!
                    return -1;
                }
            }
            else {
//              printk("Null\n");
                // Don't bother looking thru the rest.
                break;
            }
        }
        trav = trav->next;
    }

//  printk("Returning 0.\n");
    return 0;
}



///////////////////////////////////////////////////////////////////////////////
// File operations
//////////////////////////////////////////////////////////////////////////////

// Make a new file (both regular and directories)
// INPUT: path to a file, type of the file
// NOTE: pathname CANNOT have a trailing slash
// DESC: Given an absolute pathname, it performs two checks before creating
// a the file: 1) All the parent directories must exist and  2) The
// file itself does not already exist
// If the pathname is a, then it finds a vfree inode and
// populates its fields.  It also updates the parent directory by increasing
// the size field of the inode as well as adding a directory entry
// into the parents block.
int create(char* pathname, char* type) {
    int parentInodeNum;
    char* fileName;
    int i;
    int freeInodeNum;
    
    dirEntry* freeDirEntry;
    freeInodeNum = 0;
    printk("start create: freeaBlocks: %d\n", sb->freeBlocks);

    if (sb->freeInodes <= 0) {
        printk("ERROR ramdisk full: no free inodes available!\n");
        return -1;
    }

	if (sb->freeBlocks <= 0) {
        printk("ERROR ramdisk full: no free blocks available!\n");
        return -1;
    }

    // Check if we can make this file
    // 1) All parent dirs must exist
    // 2) This file has not already been created
    parentInodeNum = validateFile(pathname, type);
//    printk("rd_creat: parentInodeNum = %d\n", parentInodeNum);
    // Extract the file name we are attempting to create
    fileName = strrchr(pathname, '/');
    fileName++;

    // Check if it meets the two requirements
    if (parentInodeNum > -1) {
        // Find vfree inode
        for (i = 1; i < INODE_COUNT; i++) {
            if (inodeArray[i].status == FREE) {
                freeInodeNum = i;
                break;
            }
        }
//        printk("rd_creat: found free inode, creating\n");
        // Update the vfree inode with the new directory
		sb->freeInodes--;
        inodeArray[freeInodeNum].status = ALLOCATED;
        strcpy(inodeArray[freeInodeNum].type, type);
        inodeArray[freeInodeNum].size = 0;
        inodeArray[freeInodeNum].location[0] = getFreeBlock();
        inodeArray[freeInodeNum].locationCount = 1;

        // Update parent by updating its inode dirEntry size and dir entries
        freeDirEntry = (dirEntry*)getFreeDirEntry(parentInodeNum);
        inodeArray[parentInodeNum].size += DIR_ENTRY_STRUCTURE_SIZE;
        strcpy(freeDirEntry->fileName, fileName);

		if (type == "reg") {
        	printk("rd_creat: created regular file: %s\n", freeDirEntry->fileName);
		}
		if (type == "dir") {
			printk("rd_mkdir: created directory: %s\n", freeDirEntry->fileName);
		}

        freeDirEntry->inodeNumber = freeInodeNum;
        printk("created %s\n", pathname);
        return 1;
    }

	if (type == "reg") {
		printk("rd_creat: ERROR creating file\n");
	}
	else if (type == "dir") {
		printk("rd_mkdir: ERROR creating directory\n");
	}
	else {
		printk("create: ERROR filetype unknown\n");
	}
    return -1;
}

// Make a new regular file with the given pathname
int rd_creat(char* pathname) {
    return create(pathname, "reg");
}


// Make a new directory with the given pathname
int rd_mkdir(char* pathname) {
    return create(pathname, "dir");
}


// Open a file (reg or dir) and return its fd.
int rd_open(char* pathname) {
    char* parents;
    char* fileName;
    int parentInodeNum;
    int fileInodeNum;
    int fd;
    int pid = getpid();

    // Check if pathname is root.
    if (strcmp(pathname, "/") == 0) {
        fd = createFileDescriptor(pid, 0);  // 0 is root's inode number.
        printk("rd_open: opened root; fd = %d\n", fd);
        return fd;
    }

    // Parse pathname into parent and file name
    parse(pathname, &parents, &fileName);

    // Get our parents inode number
    parentInodeNum = getDirInodeNumber(parents);

    if (parentInodeNum == -1) {
		printk("rd_open: ERROR invalid pathname\n");
        vfree(parents);
        vfree(fileName);
        return -1;
    }

    // Our parent does exist, so find our inode number by iterating the parents
    // directory entries
    fileInodeNum = isDirEntry(parentInodeNum, fileName, "ign");

    // Check if we exist
    if (fileInodeNum == -1) {
		printk("rd_open: ERROR file not found\n");
        vfree(parents);
        vfree(fileName);
        return -1;
    }

    // So we do exist and we now have our inode number
    // Create an entry in the file descriptor table with our pid

    fd = createFileDescriptor(pid, fileInodeNum);
	printk("rd_open: opened file %s with path %s\n", fileName, pathname);
    return fd;
}



// Close a file and remove its fd; return 0 on success and -1 on failure.
int rd_close(int fd) {


    if (fd < 0) {
        printk("rd_close: ERROR invalid file descriptor %d\n", fd);
        return -1;
    }

    fileDescriptorNode* fdClose = findFileDescriptor(fileDescriptorProcessList, getpid());

    if (fdClose == NULL) {
		printk("rd_close: ERROR process does not have any open files\n");
        return -1;
    }

    if (fdClose->fileDescriptorTable[fd].inodePointer == NULL) {
        printk("rd_close: ERROR attempted to close a file not opened by the process\n");
        return -1;
    }

    fdClose->fileDescriptorTable[fd].filePosition = -1;
    fdClose->fileDescriptorTable[fd].inodePointer = NULL;

    printk("rd_close: closed file\n");

    return 0;
}



// Read num_bytes from a file designated by fd and store contents in address.
int rd_read(int fd, char* address, int num_bytes) {
    // Initialize vars
    char* filePositionAddress;
    int fileposition, readableBytes, bytesToRead, totalBytesRead;
    fileDescriptorNode* fdRead;
    inode* inodePointer;

	
    if (fd < 0) {
        printk("rd_read: ERROR invalid file descriptor %d\n", fd);
        return -1;
    }

    filePositionAddress = NULL;
    fdRead = findFileDescriptor(fileDescriptorProcessList, getpid());
    fileposition = readableBytes = bytesToRead = totalBytesRead = 0; 
    // File doesn't exist.
    if (fdRead == NULL) {
		printk("rd_read: ERROR process does not have any open files\n");
        return -1;
    }

    // Check to see if the calling process has opened the file in question.
    if (fdRead->fileDescriptorTable[fd].inodePointer == NULL) {
		printk("rd_read: ERROR attempted to read a file not opened by the process\n");
        return -1;
    }


    // Make sure the file read is a regular file.
    if (strcmp(fdRead->fileDescriptorTable[fd].inodePointer->type, "reg") != 0) {
		printk("rd_read: ERROR attempted to read a non-regular file\n");
        return -1;
    }

    // Get the inode
    inodePointer = fdRead->fileDescriptorTable[fd].inodePointer;


    // Make sure we don't read more than exists
    if (num_bytes > inodePointer->size) {
        num_bytes = inodePointer->size;
    }

    // Keep reading until no more bytes to read or EOF
    while (num_bytes > 0) {
        fileposition = fdRead->fileDescriptorTable[fd].filePosition;
        readableBytes = mapFilepositionToMemAddr(inodePointer, fileposition, &filePositionAddress);
        bytesToRead = getMin(readableBytes, num_bytes);

        if (filePositionAddress == NULL) {
            printk("rd_read: ERROR mapping file position to memory address\n");
            return -1;
        }

        // Read our bytes
        memcpy(address, filePositionAddress, bytesToRead);
        printk("rd_read: read %d bytes\n", bytesToRead);
        num_bytes -= bytesToRead;
        totalBytesRead += bytesToRead;

        // Update file position.
        fdRead->fileDescriptorTable[fd].filePosition += bytesToRead;
        filePositionAddress = NULL;
    }
	printk("rd_read: read file\n");
    return totalBytesRead;
}


// Write num_bytes into a file designated by fd from address.
int rd_write(int fd, char *address, int num_bytes) {
    char* filePositionAddress;
    int totalBytesWritten;
    fileDescriptorNode* fdWrite;
    int fileposition;
    int writeableBytes;
    char* ret;
    int bytesToWrite;
    inode* inodePointer;


	if (fd < 0) {
		printk("rd_write: ERROR invalid file descriptor: %d", fd);
		return -1;
	}

    fdWrite = findFileDescriptor(fileDescriptorProcessList, getpid());
    filePositionAddress = NULL;
    totalBytesWritten = 0;

    if (fdWrite == NULL) {
		printk("rd_write: ERROR process does not have any open files\n");
        return -1;
    }

    // Check to see if the calling process has opened the file in question.
    if (fdWrite->fileDescriptorTable[fd].inodePointer == NULL) {
		printk("rd_write: ERROR attempted to write to a file not opened by the process\n");
        return -1;
    }


    inodePointer = fdWrite->fileDescriptorTable[fd].inodePointer;

    // Cannot write to directory files
    if (strcmp(inodePointer->type, "reg") != 0) {
		printk("rd_write: ERROR attempted to write to a directory file\n");
        return -1;
    }

    // Error checking for memory.
    if (num_bytes > MAX_FILE_SIZE) {
        printk("rd_write: ERROR file exceeds maximum file size\n");
        return -1;
    }

    while (num_bytes > 0) {
        fileposition = fdWrite->fileDescriptorTable[fd].filePosition;
        // If we are going to extend the file, then add another block
        if ((fileposition % RD_BLOCK_SIZE == 0) && 
			(fileposition == inodePointer->size) && 
			(inodePointer->size != 0)) 
		{
			if (fileposition == MAX_FILE_SIZE) {
				printk("rd_write: max file size reached\n");
				return -1;
			}
            printk("allocating new block fileposition %d  freeblocks %d\n", fileposition, sb->freeBlocks);
            ret = allocateBlock(inodePointer);
            if (!ret) {
                printk("rd_write: WARNING - ramdisk full!\n");
                // now what?
                return -1;
            }
        }

        writeableBytes = mapFilepositionToMemAddr(inodePointer, fileposition, &filePositionAddress);
        bytesToWrite = min(writeableBytes, num_bytes);

        if (filePositionAddress == NULL) {
            printk("rd_write: ERROR mapping file position to memory address\n");
            return -1;
        }
        // Write our bytes
        memcpy(filePositionAddress, address, bytesToWrite);
        printk("rd_write: wrote %d bytes\n", bytesToWrite);
        num_bytes -= bytesToWrite;
        totalBytesWritten += bytesToWrite;

        // Update file position.
        fdWrite->fileDescriptorTable[fd].filePosition += bytesToWrite;
        inodePointer->size += bytesToWrite;

        filePositionAddress = NULL;
    }
    return totalBytesWritten;
}


// seeks to the offset in a file designated by fd.
int rd_lseek(int fd, int offset) {
    // Check to see if file descriptor node exists
    fileDescriptorNode* fdSeek;
    int fileSize;

	if (fd < 0) {
		printk("rd_lseek: ERROR invalid file descriptor %d\n", fd);
		return -1;
	}

    fdSeek = findFileDescriptor(fileDescriptorProcessList, getpid());
    if (fdSeek == NULL) {
		printk("rd_lseek: ERROR process does not have any open files\n");
        return -1;
    }

    // Check to see if the calling process has opened the file in question.
    if (fdSeek->fileDescriptorTable[fd].inodePointer == NULL) {
        printk("rd_lseek: ERROR attempted to read a file not opened by the process\n");
        return -1;
    }

    // Cannot run lseek on directory files
    if (strcmp(fdSeek->fileDescriptorTable[fd].inodePointer->type, "dir") == 0) {
		printk("rd_lseek: ERROR attempted to seek in a directory file\n");
        return -1;
    }

    // Determine if the offset is valid
    fileSize = fdSeek->fileDescriptorTable[fd].inodePointer->size;

    if (offset < 0) {
        // Can't have a negative offset.
        offset = 0;
    }
    if (offset > fileSize) {
        fdSeek->fileDescriptorTable[fd].filePosition = fileSize;
    }
    else {
        fdSeek->fileDescriptorTable[fd].filePosition = offset;
    }
    printk("rd_lseek: new file position: %d\n", fdSeek->fileDescriptorTable[fd].filePosition);

    return 0;
}


// removes a file referred to by pathname.
int rd_unlink(char* pathname) {
    char* parents;
    char* fileName;
    int parentInodeNum;
    int fileInodeNum;
    char* inodePointer;
    char* fileType;
    int deletedInodeNum;
    int locationCount, directCount, i;
	singleIndirectLevel* singleIndirectBlock;
	char* unlinkEntryIter;
	doubleIndirectLevel* doubleIndirectBlock;

    // Check if pathname is root.
    if (strcmp(pathname, "/") == 0) {
        printk("rd_unlink: ERROR cannot unlink root directory!\n");
        return -1;
    }

    // Check to see if file descriptor node exists
    // fileDescriptorNode* fdUnlink = findFileDescriptor(fileDescriptorProcessList, getpid());

    // Parse pathname into parent and file name
    parse(pathname, &parents, &fileName);

    // Get our parents inode number
    parentInodeNum = getDirInodeNumber(parents);

    // Parents don't exists so this is not a valid pathname
    if (parentInodeNum == -1) {
        printk("rd_unlink: ERROR cannot open invalid path\n");
        vfree(parents);
        vfree(fileName);
        return -1;
    }

    // Our parent does exist, so find our inode number by iterating the parents
    // directory entries
    fileInodeNum = isDirEntry(parentInodeNum, fileName, "ign");
    // Check if we exist
    if (fileInodeNum == -1) {
        printk("rd_unlink: ERROR cannot open non-existent file\n");
        vfree(parents);
        vfree(fileName);
        return -1;
    }

    // The file does exist, so we want to find if the file is open by any processes
    inodePointer = (char*) &inodeArray[fileInodeNum];
    if (isFileInFDProcessList(inodePointer) != 0) {
        printk("rd_unlink: ERROR cannot unlink file opened by a process\n");
        vfree(parents);
        vfree(fileName);
        return -1;
    }

    // Trying to unlink non-empty directory
    fileType = inodeArray[fileInodeNum].type;
    if (strcmp(fileType, "dir") == 0 && inodeArray[fileInodeNum].size != 0) {
        printk("rd_unlink: ERROR cannot unlink non-empty directory\n");
        return -1;
    }

    // Delete our entry from our parent
    deletedInodeNum = unlinkHelper(parentInodeNum, fileName, inodeArray[fileInodeNum].type);
        
    // Logistics
    inodeArray[parentInodeNum].size -= DIR_ENTRY_STRUCTURE_SIZE;
    sb->freeInodes += 1;
    
    inodeArray[deletedInodeNum].inodeNumber = deletedInodeNum;
    inodeArray[deletedInodeNum].status = FREE;
    inodeArray[deletedInodeNum].size = 0;
    strcpy(inodeArray[deletedInodeNum].type, "nil");

	// Get all blocks by traversing location[] array for block pointers.
	locationCount = inodeArray[deletedInodeNum].locationCount;
	if (locationCount > 8)
		directCount = 8;
	else
		directCount = locationCount;

	for(i = 0; i < directCount; i++) {
		if (inodeArray[deletedInodeNum].location[i] == NULL)
			break;

		// Obtain a block from a direct pointer and write 256 (BLOCK_SIZE) number of bytes of 0s to it.
		memset(inodeArray[deletedInodeNum].location[i], 0, RD_BLOCK_SIZE);
		setBitmap(inodeArray[deletedInodeNum].location[i]);
	}

	// Do single indirects.
    if (locationCount > 8) {
        // Get the base address of the block holding indirect pointers
        singleIndirectBlock = (singleIndirectLevel*) inodeArray[deletedInodeNum].location[8];

        // Iterate all 64 block pointers
        for (i = 0; i < 64; i++) {
            // Get block pointed to by indirect pointer
            unlinkEntryIter = singleIndirectBlock->pointers[i];

            // If this single indirect block has no more pointers to blocks...
            if (!unlinkEntryIter) {
                break;
            }

			memset(unlinkEntryIter, 0, RD_BLOCK_SIZE);
			setBitmap(unlinkEntryIter);
        }
    }

    // Check double indirect pointers
    if (locationCount == 10) {
        // Get base address of double indirect block
        doubleIndirectBlock = (doubleIndirectLevel*) inodeArray[deletedInodeNum].location[9];

        // Iterate all indirect pointers
        for (i = 0; i < 64; i++) {
            // Get indirect block
            singleIndirectBlock = doubleIndirectBlock->pointers[i];

            if (!singleIndirectBlock) {
                break;
            }

            // Iterate all 64 block pointers
            for (i = 0; i < 64; i++) {
                // Get block pointed to by indirect pointer
                unlinkEntryIter = singleIndirectBlock->pointers[i];

                if (!unlinkEntryIter) {
                    break;
                }

				memset(unlinkEntryIter, 0, RD_BLOCK_SIZE);	
				setBitmap(unlinkEntryIter);
				// Mark the block bitmap.
            }
        }
    }
    
    inodeArray[deletedInodeNum].locationCount = 0;

	printk("rd_unlink: %s unlinked\n", pathname);
	return 0;
}


// reads a directory entry and stores it into address from a directory designated by fd.
int rd_readdir(int fd, char* address) {
//	printk("Starting readdir: \n");
	char* filePositionMemAddress;
    int ret;
    fileDescriptorNode* fdReadDir;
    fileDescriptorEntry fd_entry;
    int filePosition;
    inode* inodePointer;

	
    if (fd < 0) {
        printk("rd_readdir: ERROR invalid file descriptor %d\n", fd);
        return -1;
    }

    fdReadDir = findFileDescriptor(fileDescriptorProcessList, getpid());
    fd_entry = fdReadDir->fileDescriptorTable[fd];
	filePosition = fd_entry.filePosition;
//	printk("filePosition: %d\n", filePosition);

    // File doesn't exist
    if (fdReadDir == NULL) {
		printk("rd_readdir: ERROR process does not have open files\n");
        return -1;
    }

    // Make sure the file read is a regular file.
    if (strcmp(fd_entry.inodePointer->type, "dir") != 0) {
		printk("rd_readdir: ERROR attempted to read a regular file\n");
        return -1;
    }

    // Get the inode pointer.
    inodePointer = fd_entry.inodePointer;

	// -- Error checking... -------------------------------------
	// Make sure the directory is not empty.
	if (inodePointer->size == 0) {
		printk("rd_readdir: ERROR directory empty\n");
		return 0;
	}
	// Make sure the directory is not corrupt.  
	if (inodePointer->size < 0) {
		printk("rd_readdir: ERROR directory corrupted\n");
		return -1;
	}
	// Make sure the directory has a legal fileposition
	// pointing at the start of a directory entry.
	if (filePosition % 16 != 0) {
		printk("rd_readdir: ERROR directory fileposition pointer corrupted\n");
		return -1;
	}
	// Make sure the directory is not at the EOF.
	if (filePosition == inodePointer->size) {
		printk("rd_readdir: ERROR directory is at EOF\n");
		return 0;
	}
	// -- Error checking complete. ------------------------------

	// Set address to the directory entry.
	ret = mapFilepositionToMemAddr(inodePointer, filePosition, &filePositionMemAddress);
	if (filePositionMemAddress == NULL) {
		printk("rd_readdir: ERROR translation from file position to memory address failed\n");
		return -1;
	}
	if (ret < 0) {
		printk("rd_readdir: ERROR translation from file position to memory address failed\n");
		return -1;
	}

	// Copy the contents of file
	memcpy(address, filePositionMemAddress, DIR_ENTRY_STRUCTURE_SIZE);
	// Increment fileposition.
	fdReadDir->fileDescriptorTable[fd].filePosition += DIR_ENTRY_STRUCTURE_SIZE;

	printk("rd_readdir: one directory entry read\n");
    return 1;
}

int rd_sync() {
	int ret;
	ret = copy_from_user(procfs_buffer, ramdisk, RAMDISK_SIZE);
	if (ret != 0) {
		printk("rd_sync: ERROR failed\n");
		return -1;
	}
	printk("rd_sync: ramdisk backup created\n");
	return 0;
}

int rd_restore() {
	memcpy(ramdisk, procfs_buffer, RAMDISK_SIZE);
	printk("rd_restore: ramdisk backup restored\n");
    return 0;
}



// =====================================================================
// =====================================================================
// IOCTL FUNCTIONS
// Entry point for the ioctl
static int ramdisk_ioctl(struct inode *inode, struct file *file, 
                         unsigned int cmd, unsigned long arg) {
    
    // Stores our params
    ioctl_rd params;
    char* path;
    char* kernelAddress;
    int size;
    int ret;
    path = NULL;

    // Create a copy of the params object
    copy_from_user(&params, (ioctl_rd *)arg, sizeof(ioctl_rd));

    // Determine which command was issued
    switch (cmd) {
        case IOCTL_RD_CREAT:
            // Create a copy of pathname from user space
            size = sizeof(char) * (params.pathnameLength + 1);
            path = (char*)vmalloc(size);
            copy_from_user(path, params.pathname, size);
    
			ret = rd_creat(path);
            vfree(path);
            return ret;
            break;
   
        case IOCTL_RD_MKDIR:
            // Create a copy of pathname from user space
            size = sizeof(char) * (params.pathnameLength + 1);
            path = (char*)vmalloc(size);
            copy_from_user(path, params.pathname, size);
			
            ret = rd_mkdir(path);
            vfree(path);
            return ret;
            break;
    
        case IOCTL_RD_OPEN:
            // Create a copy of pathname from user space
            size = sizeof(char) * (params.pathnameLength + 1);
            path = (char*)vmalloc(size);
            copy_from_user(path, params.pathname, size);
			ret = rd_open(path);
            vfree(path);
            return ret;
            break;
        
        case IOCTL_RD_CLOSE:
			ret = rd_close(params.fd);
            return ret;
            break;

        case IOCTL_RD_READ:
			kernelAddress = (char*) vmalloc(params.num_bytes);
			ret = rd_read(params.fd, kernelAddress, params.num_bytes);
			if (ret != -1) {
            	copy_to_user(params.address, kernelAddress, ret);
            }
			vfree(kernelAddress);
            return ret;
            break;
    
        case IOCTL_RD_WRITE:
			kernelAddress = (char*) vmalloc(params.num_bytes);
            copy_from_user(kernelAddress, params.address, (unsigned long) params.num_bytes);
			ret = rd_write(params.fd, kernelAddress, params.num_bytes);
            vfree(kernelAddress);
            return ret;
            break;
    
        case IOCTL_RD_LSEEK:
			ret = rd_lseek(params.fd, params.offset);
            return ret;
            break;

        case IOCTL_RD_UNLINK:
            // Create a copy of pathname from user space
            size = sizeof(char) * (params.pathnameLength + 1);
            path = (char*)vmalloc(size);
            copy_from_user(path, params.pathname, size);

			ret = rd_unlink(path);
            vfree(path);
            return ret;
            break;

        case IOCTL_RD_READDIR:
            kernelAddress = (char*) vmalloc(16);
            ret = rd_readdir(params.fd, kernelAddress);
            copy_to_user(params.address, kernelAddress, 16);
            vfree(kernelAddress);
            return ret;
            break;

		case IOCTL_RD_SYNC:
            // Create a copy of pathname from user space
            size = sizeof(char) * (params.pathnameLength + 1);
            path = (char*)vmalloc(size);
            copy_from_user(path, params.pathname, size);

			ret = rd_sync(path);
			vfree(path);
			return ret;
			break;

		case IOCTL_RD_RESTORE:
            // Create a copy of pathname from user space
            size = sizeof(char) * (params.pathnameLength + 1);
            path = (char*)vmalloc(size);
            copy_from_user(path, params.pathname, size);

            ret = rd_restore(path);
            vfree(path);
            return ret;
			break;
  
        default:
            return -EINVAL;
            break;
  }
  return 0;
}


/* Initialization point for the module */
static int __init init_routine(void) {
    int ret;
    printk("Loading ramdisk module...\n");

    // Set the entry point for the ramdisk_ioctl
    ramdiskOperations.ioctl = ramdisk_ioctl;

    // Start create proc entry
    proc_entry = create_proc_entry("ramdisk_ioctl", 0444, &proc_root);
	proc_backup = create_proc_entry("ramdisk_backup", 0644, &proc_root);
    if (!proc_entry || !proc_backup) {
        printk("rd: Error creating /proc entry.\n");
        return 1;
    }

    proc_entry->owner = THIS_MODULE;
    proc_entry->proc_fops = &ramdiskOperations;

    // Initialize ramdisk
    ret = initRamdisk();
    printk("Ramdisk module loaded!\n");

    return ret;
}


/* Exit point for the module */
static void __exit exit_routine(void) {
    printk("Removing ramdisk module...\n");

    // Destroy our ramdisk
    destroyRamdisk();

    // Remove the entry from /proc
    remove_proc_entry("ramdisk_ioctl", &proc_root);
	remove_proc_entry("ramdisk_backup", &proc_root);

    printk("Ramdisk module removed!\n");
    return;
}


module_init(init_routine);
module_exit(exit_routine);

