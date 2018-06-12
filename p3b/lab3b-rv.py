#!/usr/bin/python

# NAME: Ritam Sarmah, Rohan Varma
# EMAIL: rsarmah@ucla.edu, rvarm1@ucla.edu

import sys
import csv
import os

sb = None
free_blocks = []
free_inodes = []
inodes = []
indirects = []
dirents = []

unallocated_inode_nos = []  # inode numbers only
allocated_inodes = []       # inode objects

errors = 0

# String values for indirection levels
level_str = ["", "INDIRECT ", "DOUBLE INDIRECT ", "TRIPPLE INDIRECT "]


class SuperBlock:
	def __init__(self, row):
		self.n_blocks = int(row[1])
		self.n_inodes = int(row[2])
		self.block_size = int(row[3])
		self.inode_size = int(row[4])
		self.blocks_per_group = int(row[5])
		self.inodes_per_group = int(row[6])
		self.first_inode = int(row[7])


class Inode:
	def __init__(self, row):
		self.inode_no = int(row[1])
		self.file_type = row[2]
		self.mode = int(row[3])
		self.owner = int(row[4])
		self.group = int(row[5])
		self.link_count = int(row[6])
		self.ctime = row[7]
		self.mtime = row[8]
		self.atime = row[9]
		self.size = int(row[10])
		self.n_blocks = int(row[11])
		self.blocks = map(int, row[12:24])
		self.single_ind = int(row[24])
		self.double_ind = int(row[25])
		self.triple_ind = int(row[26])


class Indirect:
	def __init__(self, row):
		self.inode_no = int(row[1])
		self.level = int(row[2])
		self.offset = int(row[3])
		self.block_no = int(row[4])
		self.reference_no = int(row[5])


class Dirent:
	def __init__(self, row):
		self.parent_inode = int(row[1])
		self.size = int(row[2])
		self.entry_inode_num = int(row[3])
		self.entry_rec_len = int(row[4])
		self.entry_name_len = int(row[5])
		self.entry_file_name = row[6].rstrip()


def print_error(msg):
	sys.stderr.write(msg)
	exit(1)


def parse_csv(filename):
	global sb, free_blocks, free_inodes, inodes, indirects

	f = open(filename, 'r')
	if not f:
		print_error("Error opening file\n")

	if os.path.getsize(filename) <= 0:
		print_error("Error: file is empty\n")

	reader = csv.reader(f)
	for row in reader:
		if len(row) <= 0:
			print_error("Error: file contains blank line\n")

		category = row[0]
		if category == 'SUPERBLOCK':
			sb = SuperBlock(row)
		elif category == 'GROUP':
			pass
		elif category == 'BFREE':
			free_blocks.append(int(row[1]))
		elif category == 'IFREE':
			free_inodes.append(int(row[1]))
		elif category == 'DIRENT':
			dirents.append(Dirent(row))
		elif category == 'INODE':
			inodes.append(Inode(row))
		elif category == 'INDIRECT':
			indirects.append(Indirect(row))
		else:
			print_error("Error: unrecognized line in csv\n")


def audit_blocks():

	# block_refs maps block number to list of tuples with (level, offset inode_no)
	# Ex: block_refs[6] : [(1, 13, 12)]
	block_refs = {}
	global errors

	def check_block(block, inode_no, offset, level):
		global errors
		if block != 0:
			# Invalid block
			if block > sb.n_blocks - 1:
				print("INVALID {}BLOCK {} IN INODE {} AT OFFSET {}".format(
					level_str[level], block, inode_no, offset))
				errors += 1

			# Reserved block
			elif block < 8:
				print("RESERVED {}BLOCK {} IN INODE {} AT OFFSET {}".format(
					level_str[level], block, inode_no, offset))
				errors += 1

			# Valid block, add inode to block reference count
			elif block in block_refs:
				block_refs[block].append((level, offset, inode_no))
			else:
				block_refs[block] = [(level, offset, inode_no)]

	# Examine direct block pointers in inodes
	for inode in inodes:
		for offset, block in enumerate(inode.blocks):
			check_block(block, inode.inode_no, offset, 0)

		check_block(inode.single_ind, inode.inode_no, 12, 1)
		check_block(inode.double_ind, inode.inode_no, 268, 2)
		check_block(inode.triple_ind, inode.inode_no, 65804, 3)

	# Examine indirect entries
	for ind_block in indirects:
		check_block(ind_block.reference_no, ind_block.inode_no, ind_block.offset, ind_block.level)

	# Iterate through all non-reserved data blocks
	for block in range(8, sb.n_blocks):
		if block not in free_blocks and block not in block_refs:
			print("UNREFERENCED BLOCK {}".format(block))
			errors += 1
		elif block in free_blocks and block in block_refs:
			print("ALLOCATED BLOCK {} ON FREELIST".format(block))
			errors += 1
		elif block in block_refs and len(block_refs[block]) > 1:
			inode_refs = block_refs[block]
			for level, offset, inode_no in inode_refs:
				print("DUPLICATE {}BLOCK {} IN INODE {} AT OFFSET {}".format(
					level_str[level], block, inode_no, offset))
				errors += 1


def audit_inodes():
	global errors, inodes, allocated_inodes, unallocated_inode_nos

	# List is corrected by audit to contain "real" free inodes
	unallocated_inode_nos = free_inodes

	for inode in inodes:
		if inode.file_type == '0':
			if inode.inode_no not in free_inodes:
				print("UNALLOCATED INODE {} NOT ON FREELIST".format(inode.inode_no))
				errors += 1
				unallocated_inode_nos.append(inode.inode_no)
		else:
			if inode.inode_no in free_inodes:
				print("ALLOCATED INODE {} ON FREELIST".format(inode.inode_no))
				errors += 1
				unallocated_inode_nos.remove(inode.inode_no)

			allocated_inodes.append(inode)

	# Iterate through all non-reserved inodes
	for inode in range(sb.first_inode, sb.n_inodes):
		used = True if len(list(filter(lambda x: x.inode_no == inode, inodes))) > 0 else False
		if inode not in free_inodes and not used:
			print("UNALLOCATED INODE {} NOT ON FREELIST".format(inode))
			errors += 1
			unallocated_inode_nos.append(inode)


def check_links():
	global errors
	inode_to_parent = {2: 2}  # include special root directory case
	for dirent in dirents:
		if dirent.entry_inode_num <= sb.n_inodes and dirent.entry_inode_num not in unallocated_inode_nos:
			if dirent.entry_file_name != "'..'" and dirent.entry_file_name != "'.'":
				inode_to_parent[dirent.entry_inode_num] = dirent.parent_inode

	for dirent in dirents:
		if dirent.entry_file_name == "'.'":
			if dirent.entry_inode_num != dirent.parent_inode:
				print("DIRECTORY INODE {} NAME '.' LINK TO INODE {} SHOULD BE {}".format(dirent.parent_inode, dirent.entry_inode_num, dirent.parent_inode))
				errors +=1
		elif dirent.entry_file_name == "'..'":
			if dirent.entry_inode_num != inode_to_parent[dirent.parent_inode]:
				print ("DIRECTORY INODE {} NAME '..' LINK TO INODE {} SHOULD BE {}".format(dirent.parent_inode, dirent.entry_inode_num, inode_to_parent[dirent.parent_inode]))
				errors +=1


def audit_dirents():
	global errors
	print unallocated_inode_nos
	print free_inodes
	total_inodes = sb.n_inodes
	inode_link_map = {}
	for dirent in dirents:
		if dirent.entry_inode_num > total_inodes:
			print("DIRECTORY INODE {} NAME {} INVALID INODE {}".format(dirent.parent_inode, dirent.entry_file_name, dirent.entry_inode_num))
			errors += 1
		elif dirent.entry_inode_num in unallocated_inode_nos:
			print("DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}".format(dirent.parent_inode, dirent.entry_file_name, dirent.entry_inode_num))
			errors += 1
		else:
			inode_link_map[dirent.entry_inode_num] = inode_link_map.get(dirent.entry_inode_num, 0) + 1

	for inode in allocated_inodes:
		if inode.inode_no in inode_link_map:
			if inode.link_count != inode_link_map[inode.inode_no]:
				print("INODE {} HAS {} LINKS BUT LINKCOUNT IS {}".format(inode.inode_no, inode_link_map[inode.inode_no], inode.link_count))
				errors += 1
		else:
			if inode.link_count != 0:
				print("INODE {} HAS 0 LINKS BUT LINKCOUNT IS {}".format(inode.inode_no, inode.link_count))
				errors += 1
	check_links()


if __name__ == '__main__':
	filename='P3B-test_15.csv'
	if not os.path.isfile(filename):
		print_error("File does not exist\nUsage: ./lab3b FILENAME\n")

	parse_csv(filename)

	audit_blocks()
	audit_inodes()
	audit_dirents()

	exit(2) if errors != 0 else exit(0)
