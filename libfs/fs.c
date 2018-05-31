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

//an open file in the file descriptor table
struct fd_open
{
  char filename[16];
  uint8_t index;
  uint16_t fileDescriptor;
  uint16_t blockOffset;
  uint16_t fileOffset;
}__attribute__((packed));

struct fd_open FDTable[32];
static int fd_count = 0; //to make sure num open files are within limit
struct superblock SUPERBLOCK;
struct rootDirectory ROOTDIR[128];
struct FileAllocTable *FAT = 0;
static bool open_disk = false;
static int file_count = 0;

int num_blocks_spanning(uint32_t offset, size_t count, int written_bytes)
{
  int num_spanning_blocks = ((offset%BLOCK_SIZE) + (count - written_bytes));
  if (num_spanning_blocks  > BLOCK_SIZE)
  {
    return 1;
  }
  else if (num_spanning_blocks <= BLOCK_SIZE)
  {
    return 0;
  }
  return -1;
}

int num_blocks_spanning_read(uint32_t offset, size_t count, int read_bytes)
{
  int num_spanning_blocks = ((offset%BLOCK_SIZE) + (count - read_bytes));
  if (num_spanning_blocks >= BLOCK_SIZE)
  {
    return 1;
  }
  else if (num_spanning_blocks < BLOCK_SIZE)
  {
    return 0;
  }
  return -1;
}


int fs_mount(const char *diskname)
{
  if (block_disk_open(diskname) == -1) { return -1; }
  
  //read super block   
  block_read(0, (void*)&SUPERBLOCK);

  if (FAT == 0)
  {  
    FAT = malloc(sizeof(struct FileAllocTable));
	}
  //Read FAT block
  for(int i = 0; i < SUPERBLOCK.num_fat_blocks; i++)
  {
    block_read(i+1, (void*)&FAT -> fat+(4096/2)*i);
  }

  //Read Root block, set disk to open
  block_read(SUPERBLOCK.rootDir_block_index, ROOTDIR);
  
  open_disk = true;
  
  return 0;
}

int fs_umount(void)
{
  //If the open file descriptor count is not 0 or no virtual disk files opened
  //Return error
  if(open_disk == false || fd_count > 0){ return -1; }

  block_write(0, (void*)&SUPERBLOCK);

  //Write FAT block
  for(int i = 0; i < SUPERBLOCK.num_fat_blocks; i++)
  {
    if (block_write(i+1, (void*)&FAT+(4096)*i) == -1)
    {
      return -1;
    }
    
  }

  //Write Root block
  if (block_write(SUPERBLOCK.rootDir_block_index, ROOTDIR) == -1)
  {
    return -1;
  }

  //If the block disk can't be closed
  //Return error
  if(block_disk_close() == -1){ return -1; }

  //Clear memory and set disk to open
//  mem_clear();
  open_disk = false;
  
  return 0;
}

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
  //check that disk is open, filename not greater than limit and num files doesn't exceed max files
  if (open_disk == false || strlen(filename) > (FS_FILENAME_LEN - 1) || (file_count >= FS_FILE_MAX_COUNT-1))
  {
    return -1;
  } 
 
  //if filename already exists in root directory, then return 
  for (int i=0; i < FS_FILE_MAX_COUNT; i++)
  {
    if (strncmp(ROOTDIR[i].filename, filename, strlen(filename))==0)
    {
      return -1;
    }
  }
  
  //after finding empty space space in root, insert new file entry
  for (int j = 0; j < FS_FILE_MAX_COUNT; j++)
  {
    if (ROOTDIR[j].filename[0] == '\0')
    {
      strcpy(ROOTDIR[j].filename, filename);
      ROOTDIR[j].file_size = 0;
      ROOTDIR[j].file_index = FAT_EOC;
      break;
     }
  }
  file_count++;
  return 0;  
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
  int found_file = 0;
  int rootIndex = 0;
 
  //check if disk open and filename not ending with null and file name length less than limit 
  if (open_disk == false || filename[strlen(filename)] != '\0' || (strlen(filename) > FS_FILENAME_LEN-1))
  {
    return -1;
  }

  
  //searching for existence of matching file in root
  for(int i=0; i<FS_FILE_MAX_COUNT; i++)
  {
    if(strncmp(ROOTDIR[i].filename,filename,16)==0)
    {
      found_file = 1;
      rootIndex = i;
      break;
    }
  }
  //if matching file not found, return
  if (found_file == 0) {return -1;}
 
  //check if matching file is open in fd table
  for(int j=0; j<FS_OPEN_MAX_COUNT; j++)
  {
      if(strncmp(FDTable[j].filename,filename,16)==0)
      {
          return -1;
      }
  }
  //close the matching file at the root index stopped at while searching in root dir
  ROOTDIR[rootIndex].filename[0] = '\0';
  ROOTDIR[rootIndex].file_size = 0;
  ROOTDIR[rootIndex].file_index = 0; 
  
  //after closing the file in root dir, decrement the file count
  file_count--;
  
  int fat_index = ROOTDIR[rootIndex].file_index;
  int temp_index = 0;
  
  while(FAT->fat[fat_index].index!=FAT_EOC)
  {
      temp_index = fat_index;
      fat_index = FAT->fat[fat_index].index; 
      FAT->fat[temp_index].index = 0;
  }
  
  return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
  if (open_disk == false)
  {
    return -1;
  }
  
  //list all files in root directory for ls command
  for (int i = 0; i < FS_OPEN_MAX_COUNT; i++)
  {
    if (ROOTDIR[i].filename[0] != '\0')
    {
       printf("file: %s, size: %d, data_blk: %d\n", ROOTDIR[i].filename, ROOTDIR[i].file_size, ROOTDIR[i].file_index);
    }
  }
  return 0;  
}

int fs_open(const char *filename)
{
  //If disk isn't open
  //Return error
  if(open_disk == false) { return -1; } 

  //check if null terminated string,if filename length less than limit, and 
  //num open files less than max
  if(strlen(filename) > (FS_FILENAME_LEN-1) || fd_count > FS_OPEN_MAX_COUNT)
  {
    return -1;
  } 

  //search root dir for matching file 
  int file_found_status = 0;
  int rootindex = 0;
  int filedescriptor = 0;
  for(int i = 0; i < FS_FILE_MAX_COUNT; i++)
  {
      if(strncmp(ROOTDIR[i].filename,filename,strlen(filename)) == 0)
      {
        file_found_status = 1;
        rootindex = i;
        break;
      }
  }
  
  //if file not found in root dir then return
  if (file_found_status == 0)
  {
    return -1;
  }

  //finds first available space in fd table for file to open
  for(int j = 0; j < FS_OPEN_MAX_COUNT; j++)
  {
      if(FDTable[j].filename[0] == '\0')
		  {
          strncpy(FDTable[j].filename, filename, strlen(filename));
          FDTable[j].fileDescriptor = j;
          FDTable[j].index = rootindex;
          FDTable[j].blockOffset = ROOTDIR[rootindex].file_index;
          FDTable[j].fileOffset = 0;
          fd_count++;
          filedescriptor = j; 
       }
   }
  
   //return file descriptor of newly opened file
   return filedescriptor;
}

int fs_close(int fd)
{
  //If disk isn't open
  //Return error
  if(open_disk == false) { return -1; }
  
  //If fd is greater than max num of open files, out of bounds, or not open
  //Return error
  if(fd > (FS_OPEN_MAX_COUNT - 1) || fd < 0 || FDTable[fd].filename[0] == ('\0'))
  { return -1; }
  
  FDTable[fd].filename[0] = ('\0');
  fd_count--;
  
  return 0;
}

int fs_stat(int fd)
{
  //If disk isn't open
  //Return error
  if(open_disk == false) { return -1; }

  //If fd is greater than max num of open files, out of bounds, or not open
  //Return error
  if(fd > (FS_OPEN_MAX_COUNT - 1) || fd < 0 || FDTable[fd].filename[0] == '\0')
  { return -1; }

  //Checks files in root directory to find match
  for(int i = 0; i < FS_FILE_MAX_COUNT; i++)
  {
    if(strncmp(ROOTDIR[i].filename, FDTable[fd].filename, 16) == 0)
    {
      return ROOTDIR[i].file_size;
    }
  }
    
  return -1;
}

int fs_lseek(int fd, size_t offset)
{
  //If disk isn't open
  //Return error
  if(open_disk == false) { return -1; }
  
  //If fd is greater than max num of open files, out of bounds, or not open
  //Return error
  if(fd > (FS_OPEN_MAX_COUNT - 1) || fd < 0 || FDTable[fd].filename[0] == '\0')
  { return -1; }

  //If offset is beyond the end of file
  //Return error
  if(offset > ROOTDIR[FDTable[fd].index].file_size )
  {
    return -1;
  }

  //Set block offset to start index of data block, and set open files offset
  FDTable[fd].blockOffset = ROOTDIR[FDTable[fd].index].file_index;
  FDTable[fd].fileOffset = offset;

  //Update block offset
  for(int i = 0; i < offset/BLOCK_SIZE; i++)
  {
    FDTable[fd].blockOffset = FAT->fat[FDTable[fd].blockOffset].index;
  }

  return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	 /* TODO: Phase 4 */
   int next_fat_found = 0;
   if(open_disk == false) {return -1; }

    if(fd < 0 || count < 0 ||
        fd > FS_OPEN_MAX_COUNT-1 || FDTable[fd].filename[0] == '\0')
    {
        return -1;
    }
    
    //check if file index of given fd's index is at end
    //if so search for empty fat index and set the next index to EOC
    if(ROOTDIR[FDTable[fd].index].file_index == FAT_EOC)
    {
        for (int i = 0; i < SUPERBLOCK.num_data_blocks; i++)
        {
          if (FAT->fat[i].index == 0)
          {
            ROOTDIR[FDTable[fd].index].file_index = i;
            next_fat_found = 1;
            break;
          }
        }
        //return if extra fat space not found
        if(next_fat_found == 0) {return 0; }
        
        //set next fat index to EOC
        FAT->fat[ROOTDIR[FDTable[fd].index].file_index].index = FAT_EOC;
        //set block offset of new fd to corresponding file indexof fd in root
        FDTable[fd].blockOffset = ROOTDIR[FDTable[fd].index].file_index;
    }
    void *block_read_buffer[4096];
    uint16_t blockOffset = FDTable[fd].blockOffset;
    uint32_t fileOffset = FDTable[fd].fileOffset;
    uint32_t fileOffsetRem = (FDTable[fd].fileOffset % BLOCK_SIZE);
    uint16_t next_block;
    int written_bytes = 0;
    int next_fat_found_two;
    int is_space_in_buffer = 1;
    int num_spanning_blocks;
    int bytes_written_sofar = 0;
    
    //while there is still space in the buffer to write
    while(is_space_in_buffer)
    {
        //at new block offset, read into buffer
        block_read(blockOffset+SUPERBLOCK.datablock_start_index, block_read_buffer);
        //check whether the write spans more than one BLOCK_SIZE or just one
        //if num_spanning_blocks is 0, then only 1, but if 1 then multiple
        num_spanning_blocks = num_blocks_spanning(fileOffset, count, written_bytes);
        if (num_spanning_blocks == -1) {return -1;}
        //gets the remainder of fileOffset after one block
        fileOffsetRem = (FDTable[fd].fileOffset % BLOCK_SIZE);
        //if the write spans up to one block
        if(num_spanning_blocks == 0)
        {
            //copy to block end
            memcpy( block_read_buffer+fileOffsetRem, buf+written_bytes, (count - written_bytes));
            block_write(blockOffset+SUPERBLOCK.datablock_start_index, block_read_buffer);
            if(ROOTDIR[FDTable[fd].index].file_size < fileOffset+ (count - written_bytes))
            {
                ROOTDIR[FDTable[fd].index].file_size +=(count - written_bytes);
            }
            //update fileoffset of fd with what was written
            FDTable[fd].fileOffset += (count - written_bytes); 
            written_bytes += (count - written_bytes);  
            fileOffset = FDTable[fd].fileOffset;
        }
        //else if the write spanned multiple blocks
        else if(num_spanning_blocks == 1)
        {
            //copy to block end using memcpy
            memcpy(block_read_buffer+fileOffsetRem, buf+written_bytes,(BLOCK_SIZE-(fileOffsetRem))); 
            block_write(blockOffset+SUPERBLOCK.datablock_start_index, block_read_buffer);
            bytes_written_sofar = (BLOCK_SIZE-fileOffsetRem);
            if(ROOTDIR[FDTable[fd].index].file_size < (FDTable[fd].fileOffset+(BLOCK_SIZE-fileOffsetRem)))
            {
                ROOTDIR[FDTable[fd].index].file_size += bytes_written_sofar;
            }
            //update file offset of fd with the bytes written so far
            written_bytes += bytes_written_sofar; 
            fileOffset += bytes_written_sofar; 
            FDTable[fd].fileOffset = bytes_written_sofar;
            next_block = FAT->fat[FDTable[fd].blockOffset].index; 
            
            //if next fat block is EOC, find next available fat index
            next_fat_found_two = 0;
            if (next_block == FAT_EOC)
            {
                for (int k = 0; k < SUPERBLOCK.num_data_blocks; k++)
                {
                  if (FAT->fat[k].index == 0)
                  {
                    next_block = k;
                    next_fat_found_two = 1;
                    break;
                  }
                }
                if(next_fat_found_two == 0)
                {
                    FAT->fat[FDTable[fd].blockOffset].index = next_block;
                    return written_bytes;
                }
            }
            //update variables to next fat block
            FDTable[fd].blockOffset = next_block;
            blockOffset = next_block;
            FAT->fat[FDTable[fd].blockOffset].index = next_block;
        }
        //terminate loop if no space left in buffer
        if (written_bytes >= count) { is_space_in_buffer = 0; }
    }
    return written_bytes;
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
   if(open_disk == false) {return -1; }

    if(fd < 0 || count < 0 ||
        fd > FS_OPEN_MAX_COUNT-1 || FDTable[fd].filename[0] == '\0')
    {
        return -1;
    }

    void *block_read_buffer[4096];
    uint32_t fileSize = ROOTDIR[FDTable[fd].index].file_size;
    uint16_t blockOffset = FDTable[fd].blockOffset;
    uint32_t fileOffset = FDTable[fd].fileOffset;
    uint32_t fileOffsetRem = (FDTable[fd].fileOffset % BLOCK_SIZE);
    int read_bytes = 0;
    uint32_t max_count = count;
    int is_space_in_buffer = 1;
    int num_spanning_blocks;
    
    //makes sure to fit the file size    
    if(fileOffset + count > fileSize) {max_count = fileSize-fileOffset;}

    //while buffer has space, then read into it
    while(is_space_in_buffer)
    { 
        block_read(blockOffset+SUPERBLOCK.datablock_start_index, block_read_buffer);
        fileOffsetRem = (FDTable[fd].fileOffset % BLOCK_SIZE);
        //check to see how many blocks are being spanned, return 0 if 1 and 1 if many
        num_spanning_blocks = num_blocks_spanning_read(fileOffset, max_count, read_bytes);
        if (num_spanning_blocks == -1) {return -1;}
        //if only spanning 1 block
        if(num_spanning_blocks == 0)
        {
            //copy what was read to new location after buf
            memcpy(buf+read_bytes, block_read_buffer+(fileOffsetRem), (max_count - read_bytes));
            read_bytes += (max_count - read_bytes); 
            FDTable[fd].fileOffset += (max_count - read_bytes); 
            fileOffset = FDTable[fd].fileOffset;
        }
        //if spanning many blocks
        else if(num_spanning_blocks == 1)
        {
            //copy what was read to new location after buf
            memcpy(buf+read_bytes, block_read_buffer+(fileOffsetRem), (BLOCK_SIZE-(fileOffsetRem))); 
            //update bytes read so far into variables
            read_bytes += (BLOCK_SIZE-(fileOffsetRem)); 
            FDTable[fd].fileOffset += (BLOCK_SIZE - fileOffsetRem); 
            fileOffset =  FDTable[fd].fileOffset;
            //update to next block
            blockOffset = FAT->fat[FDTable[fd].blockOffset].index; 
            FDTable[fd].blockOffset = FAT->fat[FDTable[fd].blockOffset].index; 
        }

        if (read_bytes >= max_count) { is_space_in_buffer = 0;}
    }
    return read_bytes;
}

