#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/* TODO: Phase 1 */

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

struct rootDirectory
{
  char filename[16];
  uint32_t file_size;
  uint16_t file_index;
  uint8_t padding[10];
}__attribute__((packed));

struct dataBlock
{
  uint16_t index;
}__attribute__((packed));

struct FileAllocTable
{
  struct dataBlock fat[8192];
}__attribute__((packed));

struct superblock SUPERBLOCK;
struct rootDirectory ROOTDIR[128];
struct FileAllocTable FAT = 0;


int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */
  if (block_disk_open(diskname) == -1) { return -1; }
   
  block_read(0, (void*)&SUPERBLOCK);

  for (int i = 0; i < S    

  return 1;  
}

int fs_umount(void)
{
	/* TODO: Phase 1 */
  return 1;  
}

int fs_info(void)
{
	/* TODO: Phase 1 */
  return 1;  
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
  return 1;  
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
  return 1;  
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
  return 1;  
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
  return 1;  
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
  return 1;  
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
  return 1;  
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
  return 1;  
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
  return 1;  
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
  return 1;  
}

