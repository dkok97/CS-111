import csv
import sys

my_superblock = {}
inodes = []
free_block_list = []
indirects = []
dirents = []
free_inode_list = []
true_free_inode_list = []
allocated_inode_list = []
allocated_inodes = []
true_allocated_inode_list=[]

def parse_csv(csv_file):
    global my_superblock, Inode, Indirect, Dirent
    for line in csv_file:
        col = line[0]
        if col == 'SUPERBLOCK':
            my_superblock = {
                "num_blocks": int(line[1]),
                "num_inodes": int(line[2]),
                "inode_size": int(line[4]),
                "first_inode": int(line[7])
            }
        elif col == 'BFREE':
            free_block_list.append(int(line[1]))
        elif col == 'IFREE':
            free_inode_list.append(int(line[1]))
        elif col == 'INODE':
            my_inode = {
                "inum":int(line[1]),
                "file_type":line[2],
                "link_count":int(line[6]),
                "size":int(line[10]),
                "n_blocks":int(line[11]),
                "indirect_1":int(line[24]),
                "indirect_2":int(line[25]),
                "indirect_3":int(line[26]),
                "blocks": [int(val) for val in line[12:24]],
                "ref": int(line[5])
            }
            inodes.append(my_inode)
        elif col == 'INDIRECT':
            my_indir = {
                "inum": int(line[1]),
                "level": int(line[2]),
                "offset": int(line[3]),
                "block_num": int(line[4])
            }
            indirects.append(my_indir)
        elif col == 'DIRENT':
            my_dir = {
                "parent_inum": int(line[1]),
                "size": int(line[2]),
                "entry_inum": int(line[3]),
                "entry_file_name": int(line[4])
            }
            dirents.append(my_dir)

if __name__ == '__main__':

    f = open('trivial.csv', 'r')
    f = csv.reader(f)

    parse_csv(f)

    print my_superblock
    print inodes
