```Authors: Sahana Melkris, Matt Medina```

# Project 4

### Introduction

In this project we have implemented a File Allocation Table (FAT) file system 
that supports up to 128 files in a single root directory.  Our file system is 
implemented on top of a virtual disk which uses two layers consisting of:

1. The Block API, which is used to open/close a virtual disk & read/write blocks

2. The FS Layer, which allows for mounting the virtual disk, listing the files 
in the disk, adding/deleting new files, reading/writing files.

### Report

For our working FAT file system, we have implemented the following structs to 
manage the super block, root directory, file allocation table, and open files.

The specifications for each struct is as follows:

	```c
	//PUT STRUCTS HERE
	```

##### fs_mount(diskname), fs_umount(void), fs_info(void)

The *fs_mount* function is used to mount a disk onto the drive.  It checks to 
make sure the disk is open, and then reads the data from the disk into the super 
block, as well as the fat block, and the root block.

*fs_umount* checks that there is open file descriptors and that the disk is open 
before writing to the virtual disk block starting at block 0.  It then writes 
the fat block and the root block before checking if the disk can be closed.  If 
the disk can be closed then the superblock memory is cleared using the 
*mem_clear* function.

*fs_info* is used to print super block values to the screen, which provides 
useful information about the disk to the user.  The disk is first checked to be 
open, and then free fat blocks and free roots are calculated using an iterative 
process.  Super block infomation then gets printed to the screen.

##### fs_create(filename), fs_delete(filename), fs_ls(void)
	
	```c
	//Insert Code Here
	```

##### fs_open(*filename), fs_close(fd), fs_stat(fd), fs_lseek(fd, offset)

##### fs_write(fd, *buf, count), fs_read(fd, *buf, count)

##### test_fs_student.sh

##### Makefile

### Limitations

* Has only been compiled through GCC.

### Sources
* Source1

http://google.com

* Source2

http://google.com