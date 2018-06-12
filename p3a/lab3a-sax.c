#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <assert.h>
#include <math.h>
#include "ext2_fs.h"

//error reporting
char *cmd_err;
int errno_temp;
//

//image given
int img_fd = -1;
//
//superblock
struct ext2_super_block img_superblock;
//
//group
struct ext2_group_desc* img_group_desc;
int num_groups;
//

//bitmaps
int* inode_bitmap;
int* blocks_bitmap;
//

//inode and directory
struct ext2_inode my_ext2_inode;
#define ext2_inode_size sizeof(struct ext2_inode)
struct inode {
	u_int32_t inode_id;
	char file_type;
	u_int16_t mode;
	u_int16_t owner;
	u_int16_t group;
	u_int16_t link_count;
  	char* time_of_last_change;
	char* modification_time;
	char* time_of_last_access;
	u_int32_t file_size;
	u_int32_t num_blocks;
};

struct directory {
  	u_int32_t parent_inode;
	u_int32_t offset;
	u_int32_t inode;
  	u_int16_t rec_len;
	u_int8_t name_len;
  	char* name[];
};

struct inode inode_data;
//

unsigned int block_size = 0;

void report_error(const char* err)
{
  errno_temp = errno;
  fprintf(stderr, "Error due to: %s.\n", err);
  fprintf(stderr, "Error code: %d.\n", errno_temp);
  fprintf(stderr, "Error message: %s.\n", strerror(errno_temp));
  exit(EXIT_FAILURE);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////SUPERBLOCK STUFF////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void superblock_summary()
{
  if (pread(img_fd, &img_superblock, sizeof(struct ext2_super_block), 1024) < 0)
  {
    cmd_err="pread";
		report_error(cmd_err);
	}

  if (img_superblock.s_magic != EXT2_SUPER_MAGIC)
  {
    fprintf(stderr, "%s\n", "Incorrect file system");
		exit(1);
	}

  block_size = EXT2_MIN_BLOCK_SIZE << img_superblock.s_log_block_size;

  printf("SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", img_superblock.s_blocks_count, img_superblock.s_inodes_count,
      block_size, img_superblock.s_inode_size, img_superblock.s_blocks_per_group, img_superblock.s_inodes_per_group,
      img_superblock.s_first_ino);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////GROUP STUFF/////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


void group_summary() {
  	num_groups = 1; //always

    double read_size = num_groups * sizeof(struct ext2_group_desc);
    img_group_desc = malloc(read_size);
    if (img_group_desc==NULL)
    {
      cmd_err="malloc";
  		report_error(cmd_err);
    }

	   if (pread(img_fd, img_group_desc, read_size, 1024 + block_size) < 0)
     {
       cmd_err="pread";
   		report_error(cmd_err);
	   }

     printf("GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n", 0, img_superblock.s_blocks_count, img_superblock.s_inodes_count,
        img_group_desc[0].bg_free_blocks_count, img_group_desc[0].bg_free_inodes_count, img_group_desc[0].bg_block_bitmap,
        img_group_desc[0].bg_inode_bitmap, img_group_desc[0].bg_inode_table);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
////////////////////FREE BLOCK AND INODE ENTRIES///////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void free_block_entries(unsigned int group_no) {
  u_int32_t block_bitmap_size = img_superblock.s_blocks_per_group / 8;
	blocks_bitmap = calloc(block_bitmap_size, sizeof(u_int8_t));
  if (blocks_bitmap==NULL)
  {
    cmd_err="calloc";
    report_error(cmd_err);
  }

  unsigned int j;
  for (j = 0; j < block_bitmap_size; j++)
  {
    if(pread(img_fd, &blocks_bitmap[j], 1, block_size * img_group_desc[group_no].bg_block_bitmap + j) < 0)
    {
      cmd_err="pread";
      report_error(cmd_err);
		}

    int mask = 1;
    unsigned int p;
    for (p = 1; p <= 8; p++) {
      int is_block = mask & blocks_bitmap[j];
      if (!is_block) {
        printf("BFREE,%d\n", (j*8)+p);
      }
      mask <<= 1;
    }
  }
}

void free_inode_entries(unsigned int group_no) {
  u_int32_t inodes_bitmap_size = img_superblock.s_inodes_per_group / 8;
	inode_bitmap = calloc(inodes_bitmap_size, sizeof(u_int8_t));
  if (inode_bitmap==NULL)
  {
    cmd_err="calloc";
    report_error(cmd_err);
  }

  unsigned int j;
  for (j = 0; j < inodes_bitmap_size; j++)
  {
    if(pread(img_fd, &inode_bitmap[j], 1, block_size * img_group_desc[group_no].bg_inode_bitmap + j) < 0)
    {
      cmd_err="pread";
      report_error(cmd_err);
    }

    int mask = 1;
    unsigned int p;
    for (p = 1; p <= 8; p++) {
				int is_inode = mask & inode_bitmap[j];
				if (!is_inode) {
					printf("IFREE,%d\n", (j*8)+p);
				}
				mask <<= 1;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/////////////////INODE AND DIRECTORY STUFF ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


////////////////////////
//DIRECTORY STUFF START
////////////////////////

void directory_entries()
{
  unsigned char block[BLOCK_SIZE];
  struct ext2_dir_entry * entry;

  unsigned int i, fileOffset, byteOffset = 0;
  ssize_t e;

  for (i = 0; i < EXT2_NDIR_BLOCKS; i++){
      fileOffset = 1024 + (my_ext2_inode.i_block[i] - 1) * BLOCK_SIZE;
      e = pread(filesystem_fd, block, BLOCK_SIZE, fileOffset);

      if (e < 0){
          fprintf(stderr, "Pread error: %s\n", strerror(errno));
          exit(EXIT_FAILURE);
      }

      entry = (struct ext2_dir_entry *) block;

      while (byteOffset < my_ext2_inode.i_size && entry->file_type){
          char fileName[EXT2_NAME_LEN + 1];
          memcpy(fileName, entry->name, entry->name_len);
          fileName[entry->name_len] = 0;
          if (entry->inode != 0 && entry->name_len > 0){
              dprintf(STDOUT_FILENO, "DIRENT,%d,%d,%d,%d,%d,'%s'\n",
                      inode_data.inode_id,
                      byteOffset,
                      entry->inode,
                      entry->rec_len,
                      entry->name_len,
                      fileName);
          }
          byteOffset += entry->rec_len;
          entry = (void *) entry + entry->rec_len;
      }
  }
}

void parse_dir_block(int j, u_int32_t indirect_block, int level)
{
  unsigned int p;
  for (p = 0; p < BLOCK_SIZE / 4; p++)
  {
    u_int32_t next_block;
    pread(filesystem_fd, &next_block, 4, indirect_block * BLOCK_SIZE + 4 * p);

    if (next_block == 0)
    {
      continue;
    }

    u_int32_t block_num;
    if (j == 12)
    {
      block_num = 12 + p;
    }
    else if (j == 13)
    {
      block_num = 12 + 256 + p;
    }
    else
    {
      block_num = 12 + 256 + 256 * 256 + p;
    }

    printf("INDIRECT,%d,%d,%d,%d,%d\n", inode_data.inode_id, j - (11+level), block_num, indirect_block, next_block);

    if (j>12 && level==0)
    {
      parse_dir_block(j, next_block, 1);
    }
    if (j>13 && level==1)
    {
      parse_dir_block(j, next_block, 2);
    }
  }
}

void indirect_block_references()
{
  int j;
  for (j=12; j<15; j++)
  {
    u_int32_t indirect_block;
		pread(filesystem_fd, &indirect_block, 4, (BLOCK_SIZE * group_list[0].inode_table_id) + ((inode_data.inode_id-1) * ext2_inode_size) + 40 + (j * 4));

		if (indirect_block == 0)
		{
			continue;
		}

    parse_dir_block(j, indirect_block, 0);

  }
}

////////////////////////
//DIRECTORY STUFF END
////////////////////////
char *format_time(uint32_t raw)
{
  time_t time_seconds = (time_t) raw;
	struct tm *_time = gmtime(&time_seconds);

	char *formatted_time = malloc(sizeof(char) * 100);
	const char *format = "%D %k:%M:%S";
	strftime(formatted_time, 100, format, _time);

	return formatted_time;
}
void print_inode_info() {
    printf("INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%d,%d,", inode_data.inode_id, inode_data.file_type, inode_data.mode & 0xFFF, inode_data.owner, inode_data.group, inode_data.link_count, inode_data.time_of_last_change,
                                                    inode_data.modification_time, inode_data.time_of_last_access, inode_data.file_size, inode_data.num_blocks);

    unsigned int i;
    for (i= 0; i < EXT2_N_BLOCKS; i++) {
        printf("%d", my_ext2_inode.i_block[i]);
        if (i < 14) printf("%c", ',');
    }

    printf("\n");

    if (inode_data.file_type == 'd')
    {
      directory_entries();
    }

    if (inode_data.file_type == 'd' || inode_data.file_type == 'f')
    {
      indirect_block_references();
    }
}

int is_inode_valid(unsigned int inode_num)
{
  int is_inode = 1;
	int is_inode_present = 0;
  unsigned int j;
  for (j=0; j<bsize; j++)
  {
    uint8_t byte = inode_bitmap[j];
    int mask = 1;
    unsigned int bit;
    for (bit=1; bit<=8; bit++)
    {
      if (((j*8)+bit)==inode_num)
      {
        is_inode_present = 1;
        if ((byte & mask) == 0)
          is_inode = 0;
        break;
      }
      mask <<= 1;
      if (is_inode_present) {
						break;
			}
    }
  }
  if (!is_inode_present)
    return -1;
  if (!is_inode)
    return 0;
  return 1;
}

void inode_summary()
{
  unsigned int i;

  for(i=2; i<img_superblock.s_inodes_count; i++)
  {
    int ret = is_inode_valid(i);

    //inode not found
    if (ret == -1)
    {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }

    //inode is free
    else if (ret == 0)
    {
      continue;
    }

    if (pread(filesystem_fd, &my_ext2_inode, ext2_inode_size, 1024 + (group_list[0].inode_table_id - 1)* superblock_data.block_size + ((i-1)*ext2_inode_size)) < 0)
    {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }
    //non zero mode and non zero link count
    if(my_ext2_inode.i_mode == 0 || my_ext2_inode.i_links_count == 0) {
        continue;
    }

    inode_data.inode_id = i;
    inode_data.mode = my_ext2_inode.i_mode;
    inode_data.link_count = my_ext2_inode.i_links_count;

    inode_data.file_type = ' ';
		if (my_ext2_inode.i_mode & 0x4000) {
			inode_data.file_type = 'd';
		} else if (my_ext2_inode.i_mode & 0x8000) {
			inode_data.file_type = 'f';
		} else if (my_ext2_inode.i_mode & 0xA000) {
			inode_data.file_type = 's';
		}

    inode_data.owner = my_ext2_inode.i_uid;  // user_id
		inode_data.group = my_ext2_inode.i_gid;  // group_id
		inode_data.link_count = my_ext2_inode.i_links_count;  // number of links

		inode_data.time_of_last_change = format_time(my_ext2_inode.i_ctime);
		inode_data.modification_time = format_time(my_ext2_inode.i_mtime);
		inode_data.time_of_last_access = format_time(my_ext2_inode.i_atime);

		inode_data.file_size = my_ext2_inode.i_size;
		inode_data.num_blocks = my_ext2_inode.i_blocks;

    print_inode_info();

    if (i == 2) {
      i = my_ext2_superblock.s_first_ino - 1;
    }
  }

}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "%s\n", "Incorrect number of arguments");
	  exit(1);
  }

  char *img = argv[1];
  img_fd = open(img, O_RDONLY);
  if (img_fd < 0)
	{
		cmd_err = "open(img)";
    report_error(cmd_err);
  }

	superblock_summary();
  group_summary();
  free_block_entries(0);
  free_inode_entries(0);

  exit(EXIT_SUCCESS);
}
