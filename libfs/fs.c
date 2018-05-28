#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xFFFF

/* TODO: Phase 1 */

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

struct superblock SUPERBLOCK;
struct rootDirectory ROOTDIR[128];
struct FileAllocTable *FAT = 0;
static bool open_disk = false;
static int open_count = 0; //Checks against FS_OPEN_MAX_COUNT

void mem_clear(void)
{
  for(int i = 1; i < SUPERBLOCK.num_fat_blocks*BLOCK_SIZE/2; i++)
  {
    FAT -> fat[i].index = 0;
  }
  SUPERBLOCK.signature[0] = '\0';
  SUPERBLOCK.num_blocks_vdisk = 0;
  SUPERBLOCK.rootDir_block_index = 0;
  SUPERBLOCK.datablock_start_index = 0;
  SUPERBLOCK.num_data_blocks = 0;
  SUPERBLOCK.num_fat_blocks = 0;
}

int fs_mount(const char *diskname)
{
  if (block_disk_open(diskname) == -1) { return -1; }
   
  block_read(0, (void*)&SUPERBLOCK);
  
  FAT = malloc(sizeof(struct FileAllocTable));
	
  //Read FAT block
  for(int i = 0; i < SUPERBLOCK.num_fat_blocks; i++)
  {
    block_read(i+1, (void*)&FAT -> fat+(4096/2)*i);
  }
/*
    printf("total_blk_count in mount=%d\n", SUPERBLOCK.num_blocks_vdisk);
	printf("fat_blk_count in mount=%d\n", SUPERBLOCK.num_fat_blocks);
	printf("rdir_blk in mount=%d\n", SUPERBLOCK.rootDir_block_index);
	printf("data_blk in mount=%d\n", SUPERBLOCK.datablock_start_index);
	printf("data_blk_count in mount=%d\n", SUPERBLOCK.num_data_blocks);
*/
  //Read Root block, set disk to open
  block_read(SUPERBLOCK.rootDir_block_index, ROOTDIR);
  
  open_disk = true;
  
  return 0;
}

int fs_umount(void)
{
  //If the open file descriptor count is not 0 or no virtual disk files opened
  //Return error
  if(open_disk == false || open_count > 0){ return -1; }

  block_write(0, (void*)&SUPERBLOCK);

  //Write FAT block
  for(int i = 0; i < SUPERBLOCK.num_fat_blocks; i++)
  {
    block_write(i+1, (void*)&FAT+(4096/2)*i);
  }

  //Write Root block
  block_write(SUPERBLOCK.rootDir_block_index, ROOTDIR);

  //If the block disk can't be closed
  //Return error
  if(block_disk_close() == -1){ return -1; }

  //Clear memory and set disk to open
  mem_clear();
  open_disk = false;
  
  return 0;
}

  char signature[8];
  uint16_t num_blocks_vdisk;
  uint16_t rootDir_block_index;
  uint16_t datablock_start_index;
  uint16_t num_data_blocks;
  uint8_t num_fat_blocks;
  uint8_t padding[4079];

int fs_info(void)
{
  //If there is no virtual disk file opened
  //Return error
  if(open_disk == false){ return -1; }
  
  //Calculate free fat blocks
  uint16_t freeFatBlocks = 0;
  for(int i = 0; i < SUPERBLOCK.num_data_blocks; i++)
  {
    if(FAT -> fat[i].index == 0)
    {
        freeFatBlocks++;
    }
  }
  
  //Calculate free roots
  uint8_t freeRoots = 0;
  for(int i = 0; i < FS_FILE_MAX_COUNT; i++)
  {
    if(ROOTDIR[i].filename[0] == '\0')
    {
        freeRoots++;
    }        
  }

  printf("FS Info:\n");
  printf("total_blk_count=%d\n", SUPERBLOCK.num_blocks_vdisk);
  printf("fat_blk_count=%d\n", SUPERBLOCK.num_fat_blocks);
  printf("rdir_blk=%d\n", SUPERBLOCK.rootDir_block_index);
  printf("data_blk=%d\n", SUPERBLOCK.datablock_start_index);
  printf("data_blk_count=%d\n", SUPERBLOCK.num_data_blocks);
  printf("fat_free_ratio=%d/%d\n", freeFatBlocks, SUPERBLOCK.num_data_blocks);
  printf("rdir_free_ratio=%d/%d\n", freeRoots, FS_FILE_MAX_COUNT);

  return 0;
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

