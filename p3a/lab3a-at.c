/*
NAME: Ashwin Vivek, Taasin Saquib
EMAIL: ashwinvivek@ucla.edu, taasin.saquib@gmail.com
ID: 204705339, 304757196
*/

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

//only 1 group
struct ext2_group_desc my_ext2_group;
#define ext2_group_size sizeof(struct ext2_group_desc)

struct ext2_inode my_ext2_inode;
#define ext2_inode_size sizeof(struct ext2_inode)

int* inode_bitmap;
char inode_file_type;
u_int32_t inode_id;
uint32_t BLOCK_SIZE = 0;

struct group {
  	u_int32_t group_id;
	u_int32_t num_blocks;
	u_int32_t num_inodes;
};
#define group_size sizeof(struct group)

struct group my_group_data;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
// SUPERBLOCK
//////////////////////////////////////////////////////////////////

void print_superblock_info() {
	memset(buffer, 0, BUFFER_SIZE);
    BLOCK_SIZE = EXT2_MIN_BLOCK_SIZE << my_ext2_superblock.s_log_block_size;
	sprintf(buffer, "SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", my_ext2_superblock.s_blocks_count, my_ext2_superblock.s_inodes_count,
            BLOCK_SIZE, my_ext2_superblock.s_inode_size, my_ext2_superblock.s_blocks_per_group, my_ext2_superblock.s_inodes_per_group,
            my_ext2_superblock.s_first_ino);

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

  	print_superblock_info();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// GROUP
//////////////////////////////////////////////////////////////////

void free_block_entries() {
    u_int32_t bytes_block_bitmap = my_ext2_superblock.s_blocks_per_group / 8;
	u_int8_t* block_bitmap = (u_int8_t *) malloc(sizeof(u_int8_t) * bytes_block_bitmap);

    unsigned int j = 0;
  	while(j < bytes_block_bitmap){
        if(pread(filesystem_fd, &block_bitmap[j], 1, BLOCK_SIZE * my_ext2_group.bg_block_bitmap + j) < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(1);
		}
      	j++;
	}
    unsigned int k = 0;
    unsigned int p;
  	u_int32_t bitmap;

  	while(k < bytes_block_bitmap){
       	bitmap = block_bitmap[k];
      	p = 1;
      	while(p <= 8){
            int block_allocated = 1 & bitmap;
            if (block_allocated == 0) {
                dprintf(1, "BFREE,%d\n", (k*8)+p);
            }
            bitmap = bitmap >> 1;
            p++;
		}
        k++;
    }
}

void free_inode_entries() {
    u_int32_t inodes_bitmap_size =  my_ext2_superblock.s_inodes_per_group / 8;
	inode_bitmap = calloc(inodes_bitmap_size, sizeof(int));
    if (inode_bitmap==NULL)
    {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
    }

    unsigned int j = 0;
  	unsigned int p;
  	int mask;

  	while(j < BLOCK_SIZE){
        if(pread(filesystem_fd, &inode_bitmap[j], 1, BLOCK_SIZE * my_ext2_group.bg_inode_bitmap + j) < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(1);
        }

        mask = 1;
        p = 1;
      	while(p <= 8){
            int inode_allocated = mask & inode_bitmap[j];
            if (inode_allocated == 0) {
                dprintf(1, "IFREE,%d\n", (j*8)+p);
            }
            mask <<= 1;
            p++;
        }
        j++;
    }
}

void print_group_info() {
    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n", my_group_data.group_id, my_group_data.num_blocks,
            my_group_data.num_inodes, my_ext2_group.bg_free_blocks_count, my_ext2_group.bg_free_inodes_count,
            my_ext2_group.bg_block_bitmap, my_ext2_group.bg_inode_bitmap, my_ext2_group.bg_inode_table);

    if(write(1, buffer, strlen(buffer)) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
    }
    free_block_entries();
    free_inode_entries();
}

void group_summary() {

	if (pread(filesystem_fd, &my_ext2_group, sizeof(struct ext2_group_desc), 1024 + ext2_superblock_size) < 0) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}

    my_group_data.group_id = 0;
    my_group_data.num_blocks = my_ext2_superblock.s_blocks_count;
    my_group_data.num_inodes = my_ext2_superblock.s_inodes_count;
    print_group_info();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// INODE
//////////////////////////////////////////////////////////////////



void recursive_dir_func(int indirect_index, u_int32_t indirect_block, int level) {
    unsigned int p;
    for (p = 0; p < BLOCK_SIZE / 4; p++) {
        u_int32_t next_block;
        if(pread(filesystem_fd, &next_block, 4, indirect_block * BLOCK_SIZE + 4 * p) < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(1);
        }

        if (next_block == 0)
            continue;

        switch(indirect_index) {
            case 12:
                printf("INDIRECT,%d,%d,%d,%d,%d\n", inode_id, indirect_index - (11+level), 12 + p, indirect_block, next_block);
                break;
            case 13:
                printf("INDIRECT,%d,%d,%d,%d,%d\n", inode_id, indirect_index - (11+level), 12 + 256 + p, indirect_block, next_block);
                break;
            default:
            		printf("INDIRECT,%d,%d,%d,%d,%d\n", inode_id, indirect_index - (11+level), 12 + 256 + 65536 + p, indirect_block, next_block);
                break;
        }
        if (indirect_index>12 && level==0)
            recursive_dir_func(indirect_index, next_block, 1);
        if (indirect_index>13 && level==1)
            recursive_dir_func(indirect_index, next_block, 2);
    }
}


void indirect_directory_entries(struct ext2_inode* inode, uint32_t block_no, unsigned int size, int level) {

    uint32_t entries_size = BLOCK_SIZE/sizeof(uint32_t);
    uint32_t dir_entries[entries_size];

    memset(dir_entries, 0, sizeof(dir_entries));
    if (pread(filesystem_fd, dir_entries, BLOCK_SIZE, 1024 + (block_no - 1) * BLOCK_SIZE) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
    }

    unsigned char block[BLOCK_SIZE];
    struct ext2_dir_entry *dir_entry;

    unsigned int i = 0;
    while(i < entries_size) {
        if (dir_entries[i] != 0) {
            if (level == 2 || level == 3) {
                indirect_directory_entries(inode, dir_entries[i], size, level-1);
            }

            if (pread(filesystem_fd, block, BLOCK_SIZE, 1024 + (dir_entries[i] - 1) * BLOCK_SIZE) < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
                exit(1);
            }

            dir_entry = (struct ext2_dir_entry *) block;

            while(1) {
                if(!(size < inode->i_size) || !(dir_entry->file_type)) {
                    break;
                }
                char file_name[EXT2_NAME_LEN+1];
                memcpy(file_name, dir_entry->name, dir_entry->name_len);
                file_name[dir_entry->name_len] = 0;
                if (dir_entry->inode) {
                    printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n", inode_id, size, dir_entry->inode, dir_entry->rec_len, dir_entry->name_len, file_name);
                }
                size = size + dir_entry->rec_len;
                dir_entry = (void*) dir_entry + dir_entry->rec_len;
            }
        }
        i++;
    }
}

void directory_entries(){
    struct ext2_dir_entry* directories;
    directories = calloc(1, sizeof(struct ext2_dir_entry));
    unsigned int file_offset;
    unsigned int byte_offset = 0;
    unsigned int i = 0;

    while(i < EXT2_NDIR_BLOCKS) {
        file_offset = 1024 + (BLOCK_SIZE * (my_ext2_inode.i_block[i] - 1));
        if (pread(filesystem_fd, directories, BLOCK_SIZE, file_offset) < 0){
            fprintf(stderr, "%s\n", strerror(errno));
            exit(1);
        }

        while(1){
            if(!(byte_offset < my_ext2_inode.i_size) || !(directories->file_type)) {
                break;
            }
            char file_name_arr[EXT2_NAME_LEN + 1];
            memcpy(file_name_arr, directories->name, directories->name_len);
            file_name_arr[directories->name_len] = 0;
            if(directories->inode  && directories->name_len > 0){
                printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n", inode_id, byte_offset, directories->inode, directories->rec_len, directories->name_len, file_name_arr);
            }
            byte_offset = byte_offset+ directories->rec_len;
            directories = (void*) directories + directories->rec_len;
        }
        i++;
    }
    indirect_directory_entries(&my_ext2_inode, my_ext2_inode.i_block[EXT2_IND_BLOCK], byte_offset, 1);
}


void indirect_block_references() {
    int j;
    for (j=12; j<15; j++) {
        u_int32_t indirect_block;
		if(pread(filesystem_fd, &indirect_block, 4, (j * 4) + ((inode_id-1) * ext2_inode_size) + (BLOCK_SIZE * my_ext2_group.bg_inode_table)+ 40) < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(1);
        }
		if (!indirect_block) {
			continue;
		}
        recursive_dir_func(j, indirect_block, 0);
    }
}

char* get_time(uint32_t unformatted) {
    time_t seconds = (time_t) unformatted;
    char *formatted_time = malloc(sizeof(char) * 100);
	struct tm *_time = gmtime(&seconds);
	strftime(formatted_time, 100, "%D %k:%M:%S", _time);

	return formatted_time;
}

// change first half
int is_inode_valid(unsigned int inode_num) {

    int inode_is_valid = 1;
    int inode_found = 0;
    int bitMask;
    uint8_t curr_byte;
    unsigned int curr_bit;

    unsigned int j = 0;
    while(j < BLOCK_SIZE){

        curr_byte = inode_bitmap[j];
        bitMask = 1;

        curr_bit = 1;
        while(curr_bit <= 8){
            if (((j*8)+curr_bit) == inode_num) {

                if ((curr_byte & bitMask) == 0)
                    inode_is_valid = 0;

                inode_found = 1;

                break;
            }

            bitMask = bitMask << 1;

            if (inode_found)
                break;

            curr_bit++;
        }
        j++;
    }

    if (!inode_found)
        return -1;

    if (!inode_is_valid)
        return 0;

    return 1;
}


void print_inode_info() {
    char* time_last_changed = get_time(my_ext2_inode.i_ctime);
    char* time_modified = get_time(my_ext2_inode.i_mtime);
    char* time_last_access = get_time(my_ext2_inode.i_atime);

    printf("INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%d,%d,", inode_id, inode_file_type, my_ext2_inode.i_mode & 0xFFF, my_ext2_inode.i_uid, my_ext2_inode.i_gid, my_ext2_inode.i_links_count, time_last_changed, time_modified, time_last_access, my_ext2_inode.i_size, my_ext2_inode.i_blocks);

    unsigned int i = 0;
  	while(i < EXT2_N_BLOCKS){
        printf("%d", my_ext2_inode.i_block[i]);
        if (i < 14) printf(",");
      	i++;
    }

    printf("\n");

    if (inode_file_type == 'd') {
      directory_entries();
    }
    if (inode_file_type == 'd' || inode_file_type == 'f') {
      indirect_block_references();
    }
}



void inode_summary() {
    unsigned int i;

    for(i=2; i<my_ext2_superblock.s_inodes_count; i++) {
        int ret = is_inode_valid(i);

        //inode not found
        if (ret == -1) {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(1);
        }

        //inode is free
        else if (ret == 0) {
            continue;
        }

        if (pread(filesystem_fd, &my_ext2_inode, ext2_inode_size, 1024 + (my_ext2_group.bg_inode_table - 1)* BLOCK_SIZE + ((i-1)*ext2_inode_size)) < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(1);
        }
    //non zero mode and non zero link count
        if(my_ext2_inode.i_mode == 0 || my_ext2_inode.i_links_count == 0) {
            continue;
        }

        inode_file_type = ' ';
        if (my_ext2_inode.i_mode & 0x4000) {
            inode_file_type = 'd';
        }
        else if (my_ext2_inode.i_mode & 0x8000) {
            inode_file_type = 'f';
        }
        else if (my_ext2_inode.i_mode & 0xA000) {
            inode_file_type = 's';
        }
        inode_id = i;


        print_inode_info();

        if (i == 2) {
            i = my_ext2_superblock.s_first_ino - 1;
        }
    }
}



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
