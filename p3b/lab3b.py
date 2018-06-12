#!/usr/bin/python

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
true_allocated_inode_list = []

errors = False

def parse_csv(csv_file):
    global my_superblock, inodes, indirects, dirents
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
                "block_num": int(line[4]),
                "ref": int(line[5])
            }
            indirects.append(my_indir)
        elif col == 'DIRENT':
            my_dir = {
                "parent_inum": int(line[1]),
                "size": int(line[2]),
                "entry_inum": int(line[3]),
                "entry_file_name": line[6]
            }
            dirents.append(my_dir)

l2s = {0: "BLOCK ", 1: "INDIRECT BLOCK ", 2: "DOUBLE INDIRECT BLOCK ", 3: "TRIPLE INDIRECT BLOCK "}
block_map = {}

def block_util(block_num, inum, offset, level):
    if block_num != 0:
        if block_num + 1 > my_superblock["num_blocks"]:
            errors = True
            print "INVALID " + l2s[level] + str(block_num) + " IN INODE " + str(inum) + " AT OFFSET " + str(offset)
        elif block_num < 8:
            errors = True
            print "RESERVED " + l2s[level] + str(block_num) + " IN INODE " + str(inum) + " AT OFFSET " + str(offset)
        elif block_num in block_map:
            block_map[block_num].append((level, offset, inum))
        else:
            block_map[block_num] = [(level, offset, inum)]

def block_audit():
    for inode in inodes:
        for offset, block_num in enumerate(inode["blocks"]):
            block_util(block_num, inode["inum"], offset, 0)

        block_util(inode["indirect_1"], inode["inum"], 12, 1)
        block_util(inode["indirect_2"], inode["inum"], 268, 2)
        block_util(inode["indirect_3"], inode["inum"], 65804, 3)

    for indirect in indirects:
        block_util(indirect["ref"], indirect["inum"], indirect["offset"], indirect["level"])

    for block in range(8, my_superblock["num_blocks"]):
        if block in free_block_list and block in block_map:
            errors = True
            print "ALLOCATED BLOCK " + str(block) + " ON FREELIST"

        elif block not in block_map and block not in free_block_list:
            errors = True
            print "UNREFERENCED BLOCK " + str(block)

        elif block in block_map and len(block_map[block]) > 1:
            for l, o, inum in block_map[block]:
                errors = True
                print "DUPLICATE " + l2s[l] + str(block) + " IN INODE " + str(inum) + " AT OFFSET " + str(o)

def inode_audit():
    global my_superblock, true_free_inode_list
    true_free_inode_list = free_inode_list
    for inode in inodes:
        if inode["file_type"] == '0':
            if inode["inum"] not in free_inode_list:
            	errors = True
                print "UNALLOCATED INODE " + str(inode["inum"]) + " NOT ON FREELIST"
                true_free_inode_list.append(inode["inum"])
        else:
            if inode["inum"] in free_inode_list:
            	errors = True
                print "ALLOCATED INODE " + str(inode["inum"]) + " ON FREELIST"
                true_free_inode_list.remove(inode["inum"])
            allocated_inode_list.append(inode["inum"])
            allocated_inodes.append(inode)
    for inode_num in range(my_superblock["first_inode"], my_superblock["num_inodes"]):
        if inode_num not in free_inode_list and inode_num not in allocated_inode_list:
            errors = True
            print "UNALLOCATED INODE " + str(inode_num) + " NOT ON FREELIST"
            true_free_inode_list.append(inode_num)


def confirm_links():

    inode_to_parent = {2: 2}	# idg how this works

    for dirent in dirents:

        curr_dir_entry_inum = dirent["entry_inum"]
        curr_dir_fileName = dirent["entry_file_name"]

        if curr_dir_entry_inum <= my_superblock["num_inodes"] and curr_dir_entry_inum not in true_free_inode_list:
            if curr_dir_fileName != "'.'" and curr_dir_fileName != "'..'":
                inode_to_parent[curr_dir_entry_inum] = dirent["parent_inum"]

    for dirent in dirents:
        if dirent["entry_file_name"] == "'.'" and dirent["entry_inum"] != dirent["parent_inum"]:
            errors = True
            print "DIRECTORY INODE " + str(dirent["parent_inum"]) + " NAME '.' LINK TO INODE " + str(dirent["entry_inum"]) + " SHOULD BE " + str(dirent["parent_inum"])
        elif dirent["entry_file_name"] == "'..'" and dirent["entry_inum"] != inode_to_parent[dirent["parent_inum"]]:
            errors = True
            print "DIRECTORY INODE " + str(dirent["parent_inum"]) + " NAME '..' LINK TO INODE " + str(dirent["entry_inum"]) + " SHOULD BE " + str(inode_to_parent[dirent["parent_inum"]])

    # print inode_to_parent.get(1,-1)

def directory_audit():
    global true_free_inode_list
    # global to keep track of errors?
    num_inodes = my_superblock["num_inodes"]
    inode_links = {}
    for dirent in dirents:
        if dirent["entry_inum"] > num_inodes:
            errors = True
            print "DIRECTORY INODE " + str(dirent["parent_inum"]) + " NAME " + dirent["entry_file_name"] + " INVALID INODE " + str(dirent["entry_inum"])
        elif dirent["entry_inum"] in true_free_inode_list:
            errors = True
            print "DIRECTORY INODE " + str(dirent["parent_inum"]) + " NAME " + dirent["entry_file_name"] + " UNALLOCATED INODE " + str(dirent["entry_inum"])
        else:
            n_links = inode_links.get(dirent["entry_inum"], 0) + 1
            inode_links[dirent["entry_inum"]] = n_links

    for inode in allocated_inodes:
        if inode["inum"] in inode_links:
            if inode_links[inode["inum"]] != inode["link_count"]:
                errors = True
                print "INODE " + str(inode["inum"]) + " HAS " + str(inode_links[inode["inum"]]) + " LINKS BUT LINKCOUNT IS " + str(inode["link_count"])
        else:
            if inode["link_count"] != 0:
                errors = True
                print "INODE " + str(inode["inum"]) + " HAS 0 LINKS BUT LINKCOUNT IS " + str(inode["link_count"])

    confirm_links()



if __name__ == '__main__':
    if len(sys.argv) != 2:
        print "Correct usage: ./lab3b [fs]"
        sys.exit(1)

    f = open(sys.argv[1], 'r')
    f = csv.reader(f)

    parse_csv(f)

    block_audit()
    inode_audit()
    directory_audit()

    if errors:
        exit(2)
    else:
        exit(0)
