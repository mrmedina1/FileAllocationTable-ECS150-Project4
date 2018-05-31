```Authors: Sahana Melkris, Matt Medina```

# Project 4

### Introduction

In this project we have implemented a File Allocation Table (FAT) file system 
that supports up to 128 files in a single root directory.  The file system has a 
signature of '**ECS150-FS**'. It is implemented on top of a virtual disk which 
uses two layers consisting of:

1. The Block API, which is used to open/close a virtual disk & read/write blocks

2. The FS Layer, which allows for mounting the virtual disk, listing the files 
in the disk, adding/deleting new files, reading/writing files.

### Report

##### Background Information

Executables used: 

	1. fs_make.x
	2. fs_ref.x
	3. test_fs.x

Disk creation can be done using **fs_make.x** with the following arguments:
	fs_make.x <*diskname*> <*data block count*>
	
The *diskname* is created with the file extension *.fs*.

**fs_ref.x** and **test_fs.x** contain the following commands which can be used 
with the created disk:

	1. info <diskname>				(lists disks information)
	2. ls <diskname> <filename>			(lists the file system)
	3. add <diskname> <host filename>		(adds a file to the disk)
	4. rm <diskname> <filename>			(removes a file from the disk)
	5. cat <diskname> <filename>			(lists the contents of a file)
	6. stat <diskname> <filename>			(lists the size of a file)

For our working FAT file system, we have implemented the following structs to 
manage the super block, root directory, file allocation table, and open files.

The specifications for each struct is as follows:

```c
	//Superblock is the very first block of the disk and contains information about 
	//the file system
	struct superblock
	{
	  char signature[8];
	  uint16_t num_blocks_vdisk;
	  uint16_t rootDir_block_index;
	  uint16_t datablock_start_index;
	  uint16_t num_data_blocks;
	  uint8_t num_fat_blocks;
	  uint8_t padding[4079];  
	}__attribute__((packed));

	//Root directory is 128 entries stored in the block following the FAT
	struct rootDirectory
	{
	  char filename[16];
	  uint32_t file_size;
	  uint16_t file_index;
	  uint8_t padding[10];
	}__attribute__((packed));

	//Data blocks are used by the content of the files
	struct dataBlock
	{
	  uint16_t index;
	}__attribute__((packed));

	//File Allocation table
	struct FileAllocTable
	{
	  struct dataBlock fat[8192];
	}__attribute__((packed));

	struct fd_open
	{
	  char filename[16];
	  uint8_t index;
	  uint16_t fileDescriptor;
	  uint16_t blockOffset;
	  uint16_t fileOffset;
	}__attribute__((packed));

	struct fd_open FDTable[32];
	static int fd_count = 0; //Checks against FS_OPEN_MAX_COUNT
	struct superblock SUPERBLOCK;
	struct rootDirectory ROOTDIR[128];
	struct FileAllocTable *FAT = 0;
	static bool open_disk = false;
	static int file_count = 0;
```

##### fs_mount(diskname), fs_umount(void), fs_info(void)

* The *fs_mount* function is used to mount a disk onto the drive.  It checks to 
make sure the disk is open, and then reads the data from the disk into the super 
block, as well as the fat block, and the root block.

* *fs_umount* checks that there is open file descriptors and that the disk is open 
before writing to the virtual disk block starting at block 0.  It then writes 
the fat block and the root block before checking if the disk can be closed.  If 
the disk can be closed then the superblock memory is cleared using the 
*mem_clear* function.

* *fs_info* is used to print super block values to the screen, which provides 
useful information about the disk to the user.  The disk is first checked to be 
open, and then free fat blocks and free roots are calculated using an iterative 
process.  Super block infomation then gets printed to the screen.

##### fs_create(filename), fs_delete(filename), fs_ls(void)

* The *fs_create* function creates an empty file named *filename* in the root 
directory of the file system.  The function error checks on a closed disk, or if 
the filename is longer than the maximum filename characters allowed, or if the
file count exceeds the maximum file count.  There is then a for loop which 
checks each filename for a match.  If there is a match then there is an error 
returned.  The root directory is then traversed through until an empty file slot 
is found.  Once the file slot is found then the filename is copied into the root 
directory and file size and file index is updated, and file count is incremented.

* *fs_delete* deletes a specified *filename* from the root directory of the file 
system.  The function first error checks on a closed disk, whether the 
*filename* is empty, or if the filename is longer than the maximum filename 
characters allowed.  A for loop then checks all the files in the root directory 
for a match, and if there is a match then the index is stored.  If there is no 
match then an error is sent back as a -1.  The file descriptor is then error 
checked to see if it is still open.  The index of the root directory is then 
updated null and the file count is decremented, and the fat blocks are cleared 
out.

* *fs_ls* is used to list information about files located in the root directory.  
The function error checks on a closed disk, traverses the root directory and 
prints file information for each file in the root directory.  Amongst 
information is the files name, size, and data block index.

##### fs_open(*filename), fs_close(fd), fs_stat(fd), fs_lseek(fd, offset)

* The *fs_open* function opens a file for writing and/or reading and returns the 
file descriptor of that file.  There is an error check on closed disk, file name 
length, or if there are too many open files.  The root directory is then 
traversed to find the index of the root.  The ofile struct is then initialized 
on the first available file descriptor in the file descriptor table, and then 
the file descriptor is returned.

* *fs_close* is used to close a file descriptor.  It error checks on a closed disk 
or if the file descriptor is greater than the max count, or if there is an 
invalid file descriptor, or if the file descriptor has no filename.  The 
filename of the file descriptor is then set to null and the file descriptor 
count is decremented.

* *fs_stat* is used to get the current size of the file in the file descriptor.  
It error checks on a closed disk or if the file descriptor is greater than the 
max count, or if there is an invalid file descriptor, or if the file descriptor 
has no filename.  It then checks the files in the root directory to find a match 
and returns the file size.

* *fs_lseek* is used to set the file offset associated with a particular file 
within a file descriptor.  This is used for read and write operations.  It error 
checks on a closed disk or if the file descriptor is greater than the max count, 
or if there is an invalid file descriptor, or if the file descriptor has no 
filename, or if the offset surpasses the end of a file.  The block offset is set 
to start index of data block and the open files offset attribute is then set.  
The blocks offset is then updated.

##### fs_write(fd, *buf, count), fs_read(fd, *buf, count)

* *fs_write* is used to write a certain amount of data into a file.  It error 
checks on a closed disk or if the file descriptor is greater than the max count, 
or if there is an invalid file descriptor, or if the file descriptor has no 
filename.  If the file index is at the end of chain then the file is extended to 
hold the additional data.  While there is space in the buffer, the block is read 
and the number of spanned blocks is calculated and checked on whether the number 
of blocks surpasses the set BLOCK_SIZE.  If the spanned blocks surpass the 
BLOCK_SIZE then the block is written to the BLOCK_SIZE and then 
the block is extended so that the data can be written to.  If it doesn't surpass 
the set BLOCK_SIZE then the spanned blocks are simply written to the block.  The 
function then returns the number of written bytes.

* *fs_read* is used to read a certain amount of data from a file.  It error 
checks on a closed disk or if the file descriptor is greater than the max count, 
or if there is an invalid file descriptor, or if the file descriptor has no 
filename.  While there is space in the buffer, the data is read from each block 
starting at the data blocks start index in addition to the blocks offset passed 
in.  If the number of spanned blocks surpassed the set BLOCK_SIZE, then the 
spanned block is read to the BLOCK_SIZE and the blocks offset is updated.  If 
the number of spanned blocks does not surpass the set BLOCK_SIZE, then the 
spanned block is simply read and stored.  The number of bytes read is returned.

##### test_fs_student.sh

##### Makefile

* Our Makefile generates a static library archive named **'libfs.a'** when 
*'make'* is called from the test directory.

It compiles and links all files from the libfs directory into the library 
and generates executables in the test directory that are from all of the .c 
files within the test directory.

Use *'make clean'* to return the test and libfs directory to original 
state.  This includes removing all .o, .x, and .a files.

* Executables produced by Make:
	1. **test_fs.x** (used to test the FAT file system)

# Limitations

* Using the **rm** command with **test_fs.x** to remove a file from the 
directory causes a hangup.  This is most likely because there is an issue with 
our delete function.

* Using the **ls** command with **test_fs.x** to list the file structure causes 
a mounting issue when trying to run **fs_ref.x** after.  This is most likely 
due to an issue with *fs_umount* in *fs.c*

* This has only been compiled through GCC

### Sources
* Used for memcpy() information

http://www.geeksforgeeks.org/write-memcpy/