#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include "ext2_fs.h"

const char* filesystem_image;
int filesystem_fd = -1;

char* buffer;
int BUFFER_SIZE = 1024;

struct ext2_super_block my_ext2_superblock;
#define ext2_superblock_size sizeof(struct ext2_super_block)

struct ext2_group_desc* my_ext2_groups;
#define ext2_group_size sizeof(struct ext2_group_desc)

struct ext2_inode my_ext2_inode;
#define ext2_inode_size sizeof(struct ext2_inode)
int* inode_bitmap;

u_int32_t num_groups;

uint32_t BLOCK_SIZE = 0;

struct superblock {
  	u_int32_t num_blocks;
	u_int32_t num_inodes;
	u_int32_t block_size;
	u_int16_t inode_size;
	u_int32_t blocks_per_group;
	u_int32_t inodes_per_group;
	u_int32_t first_inode;
};
#define superblock_size sizeof(struct superblock)

struct group {
  	u_int32_t group_id;
	u_int32_t num_blocks;
	u_int32_t num_inodes;
	u_int16_t num_free_blocks;
	u_int16_t num_free_inodes;
	u_int32_t block_bitmap_id;
	u_int32_t inode_bitmap_id;
	u_int32_t inode_table_id;
};
#define group_size sizeof(struct group)

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

struct superblock superblock_data;
struct group* group_list;
struct inode inode_data;

void print_superblock_info() {
	memset(buffer, 0, BUFFER_SIZE);
	sprintf(buffer, "SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", superblock_data.num_blocks, superblock_data.num_inodes,
            superblock_data.block_size, superblock_data.inode_size, superblock_data.blocks_per_group, superblock_data.inodes_per_group,
            superblock_data.first_inode);

	if (write(1, buffer, strlen(buffer)) < 0) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void superblock_summary() {

	if (pread(filesystem_fd, &my_ext2_superblock, ext2_superblock_size, 1024) < 0) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

  	superblock_data.num_blocks = my_ext2_superblock.s_blocks_count;
  	superblock_data.num_inodes = my_ext2_superblock.s_inodes_count;
	  superblock_data.block_size = BLOCK_SIZE = EXT2_MIN_BLOCK_SIZE << my_ext2_superblock.s_log_block_size;	// given by header file
    superblock_data.inode_size = my_ext2_superblock.s_inode_size;
    superblock_data.blocks_per_group = my_ext2_superblock.s_blocks_per_group;
    superblock_data.inodes_per_group = my_ext2_superblock.s_inodes_per_group;
    superblock_data.first_inode = my_ext2_superblock.s_first_ino;

  	print_superblock_info();
}


void free_block_entries(unsigned int list_index) {
    u_int32_t bytes_block_bitmap = superblock_data.blocks_per_group / 8;
	u_int8_t* block_bitmap = (u_int8_t *) malloc(sizeof(u_int8_t) * bytes_block_bitmap);

    unsigned int j;
    for (j = 0; j < bytes_block_bitmap; j++) {
        if(pread(filesystem_fd, &block_bitmap[list_index * bytes_block_bitmap + j], 1, superblock_data.block_size * group_list[list_index].block_bitmap_id + j) < 0) {
			exit(1);
		}
	}
    unsigned int k;
    unsigned int p;
    for (k = 0; k < bytes_block_bitmap; k++) {
        u_int32_t bitmap = block_bitmap[list_index * bytes_block_bitmap + k];
        for (p = 1; p <= 8; p++) {
				int block_allocated = 1 & bitmap;
				if (block_allocated == 0) {
					dprintf(1, "BFREE,%d\n", (k*8)+p);
				}
				bitmap = bitmap >> 1;
		}
    }
}

void free_inode_entries(unsigned int list_index) {
  u_int32_t inodes_bitmap_size = superblock_data.inodes_per_group / 8;
	inode_bitmap = calloc(inodes_bitmap_size, sizeof(u_int8_t));
  if (inode_bitmap==NULL)
  {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  unsigned int j;
  for (j = 0; j < BLOCK_SIZE; j++)
  {
    if(pread(filesystem_fd, &inode_bitmap[j], 1, superblock_data.block_size * group_list[list_index].inode_bitmap_id + j) < 0) {
      exit(1);
    }

    int mask = 1;
    unsigned int p;
    for (p = 1; p <= 8; p++) {
				int inode_allocated = mask & inode_bitmap[j];
				if (inode_allocated == 0) {
					dprintf(1, "IFREE,%d\n", (j*8)+p);
				}
				mask <<= 1;
    }
  }
}

void print_group_info() {
    unsigned int j;
    for(j=0; j < num_groups; j++) {
        memset(buffer, 0, BUFFER_SIZE);
        sprintf(buffer, "GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n", group_list[j].group_id, group_list[j].num_blocks,
                group_list[j].num_inodes, group_list[j].num_free_blocks, group_list[j].num_free_inodes,
                group_list[j].block_bitmap_id, group_list[j].inode_bitmap_id, group_list[j].inode_table_id);

        if(write(1, buffer, strlen(buffer)) < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
        }
        free_block_entries(j);
        free_inode_entries(j);
    }
}

void group_summary() {
  	num_groups = (((3*superblock_data.num_blocks)/2)/superblock_data.blocks_per_group);
    if(num_groups == 0) {
        num_groups = 1;
    }
  	group_list = (struct group *) malloc(sizeof(struct group) * num_groups);

    double read_size = num_groups * sizeof(struct ext2_group_desc);

  	my_ext2_groups = (struct ext2_group_desc *) malloc(read_size);
	if (pread(filesystem_fd, my_ext2_groups, read_size, 1024 + ext2_superblock_size) < 0) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}

  	unsigned int i;
    unsigned int remaining_blocks = superblock_data.num_blocks;
    unsigned int remaining_inodes = superblock_data.num_inodes;
  	for(i=0; i < num_groups; i++) {

    	group_list[i].group_id = i;
        group_list[i].num_blocks = 0;
        group_list[i].num_inodes = 0;

      	//change this a little bit
        if(remaining_blocks > superblock_data.blocks_per_group) {
            group_list[i].num_blocks = superblock_data.blocks_per_group;
            remaining_blocks -= superblock_data.blocks_per_group;
        }
        else {
            group_list[i].num_blocks = remaining_blocks;
        }

        if(remaining_inodes > superblock_data.inodes_per_group) {
            group_list[i].num_inodes = superblock_data.inodes_per_group;
            remaining_inodes -= superblock_data.inodes_per_group;
        }
        else {
            group_list[i].num_inodes = remaining_inodes;
        }

        group_list[i].block_bitmap_id = my_ext2_groups[i].bg_block_bitmap;

        group_list[i].inode_bitmap_id = my_ext2_groups[i].bg_inode_bitmap;

        group_list[i].inode_table_id = my_ext2_groups[i].bg_inode_table;

        group_list[i].num_free_blocks = my_ext2_groups[i].bg_free_blocks_count;

        group_list[i].num_free_inodes = my_ext2_groups[i].bg_free_inodes_count;
    }
    print_group_info();
}

////////////////////////////////////////////////////////////////////////////////////
//INODE SUMMARY FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////
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
  unsigned int j;
  for (j=0; j<superblock_data.block_size; j++)
  {
    uint8_t byte = inode_bitmap[j];
    int mask = 1;
    unsigned int bit;
    for (bit=1; bit<=8; bit++)
    {
      if (((j*8)+bit)==inode_num)
      {
        if ((byte & mask) == 0)
          return 0;
        else
          return 1;
      }
      mask <<= 1;
    }
  }
  return -1;
}

void inode_summary()
{
  unsigned int i;

  for(i=2; i<my_ext2_superblock.s_inodes_count; i++)
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
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "Wrong number of arguments. Correct usage: ./lab3a [fs.img] \n");
		exit(EXIT_FAILURE);
	}

	filesystem_image = argv[1];
	if ((filesystem_fd = open(filesystem_image, O_RDONLY)) == -1) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}

	buffer = (char*) malloc(sizeof(char) * BUFFER_SIZE);

    //free memory at exit

	superblock_summary();
	group_summary();
  	inode_summary();

	exit(EXIT_SUCCESS);
}
