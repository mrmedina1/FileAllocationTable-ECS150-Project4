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

struct open_file
{
  char filename[16];
  uint8_t rootIndex;
  uint16_t fileDescriptor;
  uint16_t blockOffset;
  uint16_t fileOffset;
}__attribute__((packed));

struct open_file OFILE[32];
struct superblock SUPERBLOCK;
struct rootDirectory ROOTDIR[128];
struct FileAllocTable *FAT = 0;
static bool open_disk = false;
static int open_count = 0; //Checks against FS_OPEN_MAX_COUNT
static int file_count = 0;

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

uint16_t find_free_fat(void) // finds the first available block
{
    for(uint16_t i = 0; i < SUPERBLOCK.num_data_blocks; i++)
    {
        if(FAT->fat[i].index == 0)
  {
            return i;
        }

    }
    return FAT_EOC;
}

uint16_t find_next_fat(uint16_t index)//finds next block that file is mapped to
{
    return FAT->fat[index].index;
}

//Gets the next fat block
uint16_t GetNextFatBlock(uint16_t i)
{
    return FAT -> fat[i].index;
}

int num_blocks_spanning(uint32_t offset, size_t count, int written_bytes)
{
  if (((offset%BLOCK_SIZE) + (count - written_bytes)) > BLOCK_SIZE)
  {
    return 1;
  }
  else if (((offset%BLOCK_SIZE) + (count - written_bytes)) <= BLOCK_SIZE)
  {
    return 0;
  }
  return -1;
}


int fs_mount(const char *diskname)
{
  if (block_disk_open(diskname) == -1) { return -1; }
   
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
  if (open_disk == false || strlen(filename) > (FS_FILENAME_LEN - 1) || (file_count >= FS_FILE_MAX_COUNT-1))
  {
    printf("before first error\n");
    return -1;
  } 
  
  printf("passed first error\n");

  for (int i=0; i < FS_FILE_MAX_COUNT; i++)
  {
    if (strncmp(ROOTDIR[i].filename, filename, strlen(filename))==0)
    {
      printf("before 2nd error\n");
      return -1;
    }
  }
  printf("passed 2nd error\n");
  
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
  int index = 0;
  
  if (open_disk == false || filename[strlen(filename)] != '\0' || (strlen(filename) > FS_FILENAME_LEN-1))
  {
    return -1;
  }
  
   for(int i=0; i<FS_FILE_MAX_COUNT; i++) //parses through all files in ROOT
   {
     if(strncmp(ROOTDIR[i].filename,filename,16)==0)//if we found the file
     {

       for(int j=0; j<FS_OPEN_MAX_COUNT; j++)//check open file array
       {
         if(strncmp(OFILE[j].filename,filename,16)==0)//fd is still open
         {
           return -1;
         }
        }
        flag = 1;
        index = i;
        break;
      }
    }

    if(flag)
    {
      ROOTDIR[index].filename[0] = '\0';
      ROOTDIR[index].file_size = 0;
      ROOTDIR[index].file_index = 0; //not sure about this
      file_count--;
      int k = ROOTDIR[index].file_index;
      while(FAT->fat[k].index!=FAT_EOC)
      {
      //clears out FAT indexes when file is deleted
        int temp = k;
        k = FAT->fat[k].index; 
        FAT->fat[temp].index = 0;
      }
      return 0;
    } 

    return -1;  //if we did not find the file in our root directory
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
  if (open_disk == false)
  {
    return -1;
  }

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

  //check if null terminated string,if string length appropriate, 
  //or if there are already %FS_OPEN_MAX_COUNT files currently open
  //if(filename[strlen(filename)] != '\0'
  
  if(strlen(filename) > (FS_FILENAME_LEN-1) || open_count > FS_OPEN_MAX_COUNT)
  {
    return -1;
  } 

  
  int file_found_status = 0;
  int rootindex = 0;
  int filedescriptor = 0;
    /*parses through all files in root directory to find a match*/
  for(int i = 0; i < FS_FILE_MAX_COUNT; i++)
  {
      if(strncmp(ROOTDIR[i].filename,filename,strlen(filename)) == 0)
      {
	    /*if we found the file to open*/
        file_found_status = 1;
        rootindex = i;
        break;
      }
  }
  
  if (file_found_status == 0)
  {
    return -1;
  }

  for(int j = 0; j < FS_OPEN_MAX_COUNT; j++)
  {
      if(OFILE[j].filename[0] == '\0')
		  {
		  /*find first available entry and initialize the ofile struct*/
          strncpy(OFILE[j].filename, filename, strlen(filename));
          OFILE[j].fileDescriptor = j;
          OFILE[j].rootIndex = rootindex;
          OFILE[j].blockOffset = ROOTDIR[rootindex].file_index;
          OFILE[j].fileOffset = 0;
          open_count++;
          filedescriptor = j; //return the file descriptor
       }
   }
  
   return filedescriptor;
	//If there isnt a file named @filename to open
	//Return error
  // return -1;
}

int fs_close(int fd)
{
  //If disk isn't open
  //Return error
  if(open_disk == false) { return -1; }
  
  //If fd is greater than max num of open files, out of bounds, or not open
  //Return error
  if(fd > (FS_OPEN_MAX_COUNT - 1) || fd < 0 || OFILE[fd].filename[0] == ('\0'))
  { return -1; }
  
  OFILE[fd].filename[0] = ('\0');
  open_count--;
  
  return 0;
}

int fs_stat(int fd)
{
  //If disk isn't open
  //Return error
  if(open_disk == false) { return -1; }

  //If fd is greater than max num of open files, out of bounds, or not open
  //Return error
  if(fd > (FS_OPEN_MAX_COUNT - 1) || fd < 0 || OFILE[fd].filename[0] == '\0')
  { return -1; }

  //int rootDirIndex;

  //Checks files in root directory to find match
  //if found store the root directory index and break
  for(int i = 0; i < FS_FILE_MAX_COUNT; i++)
  {
    if(strncmp(ROOTDIR[i].filename, OFILE[fd].filename, 16) == 0)
    {
      return ROOTDIR[i].file_size;
    }
  }
    
  return -1;
  
  //Return current size of the file
  //return ROOTDIR[rootDirIndex].file_size;
}

int fs_lseek(int fd, size_t offset)
{
  //If disk isn't open
  //Return error
  if(open_disk == false) { return -1; }
  
  //If fd is greater than max num of open files, out of bounds, or not open
  //Return error
  if(fd > (FS_OPEN_MAX_COUNT - 1) || fd < 0 || OFILE[fd].filename[0] == '\0')
  { return -1; }

  //If offset is beyond the end of file
  //Return error
  if(offset > ROOTDIR[OFILE[fd].rootIndex].file_size )
  {
    return -1;
  }

  //Set block offset to start index of data block, and set open files offset
  OFILE[fd].blockOffset = ROOTDIR[OFILE[fd].rootIndex].file_index;
  OFILE[fd].fileOffset = offset;

  //Update block offset
  for(int i = 0; i < offset/BLOCK_SIZE; i++)
  {
    OFILE[fd].blockOffset = FAT->fat[OFILE[fd].blockOffset].index;
  }

  return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
  int next_fat_found = 0;
	/* TODO: Phase 4 */
   if(open_disk == false) {return -1; }

    if(fd < 0 || count < 0 ||
        fd > FS_OPEN_MAX_COUNT-1 || OFILE[fd].filename[0] == '\0')
    {
        return -1;
    }

    if(ROOTDIR[OFILE[fd].rootIndex].file_index == FAT_EOC)
    {
        for (int i = 0; i < SUPERBLOCK.num_data_blocks; i++)
        {
          if (FAT->fat[i].index == 0)
          {
            ROOTDIR[OFILE[fd].rootIndex].file_index = i;
            next_fat_found = 1;
            break;
          }
        }
       // ROOTDIR[OFILE[fd].rootIndex].file_index = find_free_fat();
        if(next_fat_found == 0) {return 0; }
        
        FAT->fat[ROOTDIR[OFILE[fd].rootIndex].file_index].index = FAT_EOC;
        OFILE[fd].blockOffset = ROOTDIR[OFILE[fd].rootIndex].file_index;
    }
    void *block_read_buffer[4096];
    uint16_t FATOffset = OFILE[fd].blockOffset;
    uint32_t fileOffset = OFILE[fd].fileOffset;
    uint32_t fileOffsetRem = (OFILE[fd].fileOffset % BLOCK_SIZE);
    uint16_t temp_blockOffset;
    int written_bytes = 0;
    int next_fat_found_two;
    int is_space_in_buffer = 1;
    int num_spanning_blocks;
    /*do this while there is still space to write into the buffer*/
    while(is_space_in_buffer)
    {
        block_read(FATOffset+SUPERBLOCK.datablock_start_index, block_read_buffer);//reads in one block at a time
        num_spanning_blocks = num_blocks_spanning(fileOffset, count, written_bytes);
        if (num_spanning_blocks == -1) {return -1;}
      /*the write spans multiple blocks*/
        if(num_spanning_blocks == 1)
        {
            memcpy(block_read_buffer+fileOffsetRem, buf+written_bytes,(BLOCK_SIZE-(fileOffsetRem))); // copy to the end of the block
            block_write(FATOffset+SUPERBLOCK.datablock_start_index, block_read_buffer);
            if(ROOTDIR[OFILE[fd].rootIndex].file_size < (OFILE[fd].fileOffset+(BLOCK_SIZE-fileOffsetRem)))
            {
                ROOTDIR[OFILE[fd].rootIndex].file_size += (BLOCK_SIZE-fileOffsetRem);
            }
            written_bytes += (BLOCK_SIZE-fileOffsetRem); //increment the written_bytes with what was able to be write
            fileOffset += (BLOCK_SIZE-fileOffsetRem); // increment the offset with what was write
            OFILE[fd].fileOffset = (BLOCK_SIZE-fileOffsetRem);
            temp_blockOffset = FAT->fat[OFILE[fd].blockOffset].index; // write to the next block

            next_fat_found_two = 0;
            if (temp_blockOffset == FAT_EOC)
            {
                for (int k = 0; k < SUPERBLOCK.num_data_blocks; k++)
                {
                  if (FAT->fat[k].index == 0)
                  {
                    temp_blockOffset = k;
                    next_fat_found_two = 1;
                    break;
                  }
                }
                if(next_fat_found_two == 0)
                {
                    FAT->fat[OFILE[fd].blockOffset].index = temp_blockOffset;
                    return written_bytes;
                }
            }
            OFILE[fd].blockOffset = temp_blockOffset;
            FATOffset = temp_blockOffset;
            FAT->fat[OFILE[fd].blockOffset].index = temp_blockOffset;
        }

        else if(num_spanning_blocks == 0)//the write only spans 1 block
        {
            memcpy( block_read_buffer+fileOffsetRem, buf+written_bytes, (count - written_bytes));
            block_write(FATOffset+SUPERBLOCK.datablock_start_index, block_read_buffer);
            if(ROOTDIR[OFILE[fd].rootIndex].file_size < fileOffset+ (count - written_bytes))
            {
                ROOTDIR[OFILE[fd].rootIndex].file_size +=(count - written_bytes);
            }
            OFILE[fd].fileOffset += (count - written_bytes); 
            written_bytes+=(count - written_bytes); //increment the written_bytes with what was able to be write
            fileOffset =  OFILE[fd].fileOffset;
        }
        if (written_bytes >= count) { is_space_in_buffer = 0; }
    }
    return written_bytes;

//  return 1;  
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
   if(open_disk == false||count < 0||fd < 0
  ||fd > FS_OPEN_MAX_COUNT-1||OFILE[fd].filename[0] == '\0')
    {
        return -1;
    }

    int bytes_read = 0;
    void *buffer[4096];
    uint32_t filesize = ROOTDIR[OFILE[fd].rootIndex].file_size;
    uint16_t FATOffset = OFILE[fd].blockOffset;
    uint32_t fileOffset = OFILE[fd].fileOffset;
    uint32_t temp_count = 0;

    /*adjusts the size of the count to fit into the file size*/
    if(fileOffset + count > filesize)
    { 
        /*gaurantees the count will only ask for at most to the end of the file*/
        temp_count = filesize - fileOffset;
    }
    else
    {
        temp_count = count;
    }

    /*do this while there is still space to read into the buffer*/
    while(bytes_read < temp_count)
    { 
        block_read(FATOffset+SUPERBLOCK.datablock_start_index, buffer);//reads in one block at a time
        /*the read spans multiple blocks*/
        if(((fileOffset%BLOCK_SIZE) + (temp_count - bytes_read)) >= BLOCK_SIZE)
        {
            memcpy(buf+bytes_read, buffer+(fileOffset%BLOCK_SIZE), (BLOCK_SIZE-(fileOffset%BLOCK_SIZE))); // copy to the end of the block
            bytes_read+=(BLOCK_SIZE-(fileOffset%BLOCK_SIZE)); //increment the bytes_read with what was able to be read
            OFILE[fd].fileOffset += BLOCK_SIZE - (fileOffset%BLOCK_SIZE); // increment the offset with what was read
            fileOffset =  OFILE[fd].fileOffset;
            FATOffset = find_next_fat(OFILE[fd].blockOffset); // move to the next block
            OFILE[fd].blockOffset = FATOffset;//update the next block offset
        }

        else if(((fileOffset%BLOCK_SIZE) + (temp_count - bytes_read)) < BLOCK_SIZE)//the read only spans 1 block
        {
            memcpy(buf+bytes_read, buffer+(fileOffset%BLOCK_SIZE), (temp_count - bytes_read));
            bytes_read+=(temp_count - bytes_read); //increment the bytes_read with what was able to be read
            OFILE[fd].fileOffset += (temp_count - bytes_read); // increment the offset with what was read
            fileOffset =  OFILE[fd].fileOffset;
        }
    }
    return bytes_read;

//  return 1;  
}

