// NAME: Dinkar Khattar, Suyash Saxena
// EMAIL: dinkarkhattar@ucla.edu, suyashs@ucla.edu
// ID: 204818138, 404735401

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

//image given
int img_fd = -1;

//superblock
struct ext2_super_block img_superblock;

//group
struct ext2_group_desc* img_group_desc;
int num_groups;


//bitmaps
int* inode_bitmap;
int* blocks_bitmap;


//inode and directory
struct ext2_inode img_inode;

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
struct inode inode_data;

struct directory {
    u_int32_t parent_inode;
    u_int32_t offset;
    u_int32_t inode;
    u_int16_t rec_len;
    u_int8_t name_len;
    char* name[];
};
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

///////////////////////////SUPERBLOCK STUFF////////////////////////////////////

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

///////////////////////////GROUP STUFF/////////////////////////////////////////

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

////////////////////FREE BLOCK ENTRIES///////////////////////////////

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

////////////////////FREE INODE ENTRIES///////////////////////////////

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

/////////////////DIRECTORY STUFF ////////////////////////////////////

void directory_entries()
{
    struct ext2_dir_entry* all_dir;
    all_dir = calloc(EXT2_NDIR_BLOCKS, sizeof(struct ext2_dir_entry));
    
    unsigned int i, fileOffset, byteOffset = 0;
    
    for (i = 0; i < EXT2_NDIR_BLOCKS; i++){
        fileOffset = 1024 + (img_inode.i_block[i] - 1) * block_size;
        if (pread(img_fd, all_dir, block_size, fileOffset) < 0)
        {
            cmd_err="pread";
            report_error(cmd_err);
        }
        
        while (byteOffset < img_inode.i_size && all_dir->file_type)
        {
            char fileName[EXT2_NAME_LEN + 1];
            memcpy(fileName, all_dir->name, all_dir->name_len);
            fileName[all_dir->name_len] = 0;
            if (all_dir->inode != 0 && all_dir->name_len > 0)
            {
                printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n", inode_data.inode_id, byteOffset, all_dir->inode, all_dir->rec_len, all_dir->name_len, fileName);
            }
            byteOffset += all_dir->rec_len;
            all_dir = (void*) all_dir + all_dir->rec_len;
        }
    }
}

void parse_dir_block(int j, u_int32_t indirect_block, int level)
{
    unsigned int p;
    for (p = 0; p < block_size / 4; p++)
    {
        u_int32_t next_block;
        pread(img_fd, &next_block, 4, indirect_block * block_size + 4 * p);
        
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
        pread(img_fd, &indirect_block, 4, (block_size * img_group_desc[0].bg_inode_table) + ((inode_data.inode_id-1) * sizeof(struct ext2_inode)) + 40 + (j * 4));
        
        if (indirect_block == 0)
        {
            continue;
        }
        
        parse_dir_block(j, indirect_block, 0);
        
    }
}

char *format_time(uint32_t raw)
{
    time_t time_seconds = (time_t) raw;
    struct tm *_time = gmtime(&time_seconds);
    
    char *formatted_time = malloc(sizeof(char) * 100);
    const char *format = "%D %k:%M:%S";
    strftime(formatted_time, 100, format, _time);
    
    return formatted_time;
}

/////////////////INODE STUFF ////////////////////////////////////

int is_inode_valid(unsigned int inode_num)
{
    unsigned int j;
    for (j=0; j<block_size; j++)
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
    
    for(i=2; i<img_superblock.s_inodes_count; i++)
    {
        int ret = is_inode_valid(i);
        
        //inode not found
        if (ret == -1)
        {
            fprintf(stderr, "%s\n", "Inode not found, bitmap incorrect");
            exit(1);
        }
        
        //inode is free
        else if (ret == 0)
        {
            continue;
        }
        
        if (pread(img_fd, &img_inode, sizeof(struct ext2_inode), 1024 + (img_group_desc[0].bg_inode_table - 1)* block_size + ((i-1)*sizeof(struct ext2_inode))) < 0)
        {
            cmd_err="pread";
            report_error(cmd_err);
        }
        
        if(img_inode.i_mode == 0 || img_inode.i_links_count == 0)
            continue;
        
        inode_data.inode_id = i;
        inode_data.mode = img_inode.i_mode;
        inode_data.link_count = img_inode.i_links_count;
        
        inode_data.file_type = ' ';
        if (img_inode.i_mode & 0x4000) {
            inode_data.file_type = 'd';
        } else if (img_inode.i_mode & 0x8000) {
            inode_data.file_type = 'f';
        } else if (img_inode.i_mode & 0xA000) {
            inode_data.file_type = 's';
        }
        
        inode_data.owner = img_inode.i_uid;  // user_id
        inode_data.group = img_inode.i_gid;  // group_id
        inode_data.link_count = img_inode.i_links_count;  // number of links
        
        inode_data.time_of_last_change = format_time(img_inode.i_ctime);
        inode_data.modification_time = format_time(img_inode.i_mtime);
        inode_data.time_of_last_access = format_time(img_inode.i_atime);
        
        inode_data.file_size = img_inode.i_size;
        inode_data.num_blocks = img_inode.i_blocks;
        
        printf("INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%d,%d,", inode_data.inode_id, inode_data.file_type, inode_data.mode & 0xFFF,
               inode_data.owner, inode_data.group, inode_data.link_count, inode_data.time_of_last_change, inode_data.modification_time,
               inode_data.time_of_last_access, inode_data.file_size, inode_data.num_blocks);
        
        unsigned int j;
        for (j = 0; j < EXT2_N_BLOCKS; j++)
        {
            printf("%d", img_inode.i_block[j]);
            if (j < 14) printf("%c", ',');
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
        
        if (i == 2) {
            i = img_superblock.s_first_ino - 1;
        }
    }
    
}

///////////////////////MAIN///////////////////////////////////////

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
    inode_summary();
    
    exit(EXIT_SUCCESS);
}

//////////////////////////////////////////////////////////////
