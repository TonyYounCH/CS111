#!/usr/local/cs/bin/python

#NAME: Changhui Youn
#EMAIL: tonyyoun2@gmail.com
#ID: 304207830

from collections import defaultdict
import sys

damaged = False

class SuperBlock:
	def __init__(self, field):
		self.num_blocks = int(field[1])
		self.num_inodes = int(field[2])
		self.block_size = int(field[3])
		self.inode_size = int(field[4])
		self.blocks_per_group = int(field[5])
		self.inodes_per_group = int(field[6])
		self.first_non_reserved_inode = int(field[7])

class Group:
	def __init__(self, field):
		self.group_num = int(field[1])
		self.total_num_of_blocks = int(field[2])
		self.total_num_of_inodes = int(field[3])
		self.num_free_blocks = int(field[4])
		self.num_free_inodes = int(field[5])
		self.block_bitmap = int(field[6])
		self.inode_bitmap = int(field[7])
		self.first_block = int(field[8])

class Inode:
    def __init__(self, field):
        self.inode_num = int(field[1])
        self.file_type = (field[2])
        self.mode = field[3]
        self.uid = int(field[4])
        self.gid = int(field[5])
        self.link_count = int(field[6])
        self.ctime = field[7]
        self.mtime = field[8]
        self.atime = field[9]
        self.file_size = int(field[10])
        self.block_num = int(field[11])

class Dirent:
	def __init__(self, field):
		self.parent_inode_num = int(field[1])
		self.logical_byte = int(field[2])
		self.inode_num = int(field[3])
		self.entry_length = int(field[4])
		self.name_length = int(field[5])
		self.name = str(field[6]).rstrip('\r\n')

def block_check(super_block, group, blocks):
	first_valid_block = group.first_block + super_block.inode_size * group.total_num_of_inodes / super_block.block_size
	i = first_valid_block
	for block_num in blocks:
		if i <= block_num:
			while i != block_num and i < 64:
				sys.stdout.write('UNREFERENCED BLOCK ' + str(i) + '\n')
				damaged = True
				i += 1
			i += 1

	for block_num, infos in blocks.iteritems():
		if len(infos) > 1:
			free = False
			notfree = False
			count = 0
			for info in infos:
				if info[0] == 'BFREE':
					free = True
				else:
					notfree = True
					count += 1
			if free and notfree:
				sys.stdout.write('ALLOCATED BLOCK ' + str(block_num) + ' ON FREELIST' + '\n')
				damaged = True
			if count > 1:
				for info in infos:
					if info[0] != 'BFREE':
						sys.stdout.write('DUPLICATE ' + info[0] + 'BLOCK ' + str(block_num) + ' IN INODE ' + str(info[1]) + ' AT OFFSET ' + str(info[2]) + '\n')
						damaged = True
		for info in infos:
			if info[0] != 'BFREE':
				if block_num < 0 or block_num > group.total_num_of_blocks:
					sys.stdout.write('INVALID ' + info[0] + 'BLOCK ' + str(block_num) + ' IN INODE ' + str(info[1]) + ' AT OFFSET ' + str(info[2]) + '\n')
					damaged = True

				if block_num > 0 and block_num < first_valid_block:
					sys.stdout.write('RESERVED ' + info[0] + 'BLOCK ' + str(block_num) + ' IN INODE ' + str(info[1]) + ' AT OFFSET ' + str(info[2]) + '\n')
					damaged = True

def inode_check(super_block, free_inodes, list_dirent, inodes):
	link_count_dict = dict()
	parent_inode_dict = dict()
	inode_nums = list()

	for dirent in list_dirent:
		if dirent.inode_num not in link_count_dict:
			link_count_dict[dirent.inode_num] = 1
		else:
			link_count_dict[dirent.inode_num] = link_count_dict[dirent.inode_num] + 1
		if dirent.inode_num not in parent_inode_dict:
			parent_inode_dict[dirent.inode_num] = dirent.parent_inode_num

	for inode in inodes:
		if inode.inode_num in free_inodes:
			sys.stdout.write('ALLOCATED INODE ' + str(inode.inode_num) + ' ON FREELIST' + '\n')
			damaged = True
		inode_nums.append(inode.inode_num)
		if inode.inode_num in link_count_dict and link_count_dict[inode.inode_num] != inode.link_count:
			sys.stdout.write('INODE ' + str(inode.inode_num) + ' HAS ' + str(link_count_dict[inode.inode_num]) + ' LINKS BUT LINKCOUNT IS ' + str(inode.link_count) + '\n')
			damaged = True
		elif inode.inode_num not in link_count_dict:
			sys.stdout.write('INODE ' + str(inode.inode_num) + ' HAS 0 LINKS BUT LINKCOUNT IS ' + str(inode.link_count) + '\n')
			damaged = True

	for inode in range(super_block.first_non_reserved_inode, super_block.num_inodes + 1):
		if inode not in free_inodes and inode not in inode_nums:
			sys.stdout.write('UNALLOCATED INODE ' + str(inode) + ' NOT ON FREELIST' + '\n')
			damaged = True

	for dirent in list_dirent:
		if dirent.inode_num not in inode_nums and dirent.inode_num in range (1, super_block.num_inodes + 1):
			sys.stdout.write('DIRECTORY INODE ' + str(dirent.parent_inode_num) + ' NAME ' + str(dirent.name) + ' UNALLOCATED INODE ' + str(dirent.inode_num) + '\n')
			damaged = True
		elif dirent.inode_num < 1 or dirent.inode_num > super_block.num_inodes:
			sys.stdout.write('DIRECTORY INODE ' + str(dirent.parent_inode_num) + ' NAME ' + str(dirent.name) + ' INVALID INODE ' + str(dirent.inode_num) + '\n')
			damaged = True
		if dirent.name == "'.'":
			if dirent.inode_num != dirent.parent_inode_num:
				sys.stdout.write('DIRECTORY INODE ' + str(dirent.parent_inode_num) + ' NAME ' + str(dirent.name) + ' LINK TO INODE ' + str(dirent.inode_num) + ' SHOULD BE ' + str(dirent.parent_inode_num) + '\n')
				damaged = True
		elif dirent.name == "'..'":
			if dirent.inode_num != parent_inode_dict[dirent.parent_inode_num]:
				sys.stdout.write('DIRECTORY INODE ' + str(dirent.parent_inode_num) + ' NAME ' + str(dirent.name) + ' LINK TO INODE ' + str(dirent.inode_num) + ' SHOULD BE ' + str(parent_inode_dict[dirent.parent_inode_num]) + '\n')
				damaged = True

def process_csv(lines):
	blocks = defaultdict(list)
	super_block = None
	group = None
	inodes = list()
	list_dirent = list()
	free_inodes = list()

	for line in lines:
		field = line.split(',')
		if field[0] == 'SUPERBLOCK':
			super_block = SuperBlock(field)

		if field[0] == 'GROUP':
			group = Group(field)

		if field[0] == 'BFREE':
			blocks[int(field[1])].append(['BFREE', None, None])

		if field[0] == 'IFREE':
			free_inodes.append(int(field[1]))

		if field[0] == 'INODE':
			inodes.append(Inode(field))
			for i in range(12, 27):
				block_num = int(field[i])
				info = list()
				logical_offset = i - 12
				if i == 24:
					info.append('INDIRECT ')
				elif i == 25:
					info.append('DOUBLE INDIRECT ')
					logical_offset = 12 + 256
				elif i == 26:
					info.append('TRIPLE INDIRECT ')
					logical_offset = 12 + 256 + 256*256
				else:
					info.append('')
				info.append(int(field[1]))
				info.append(logical_offset)
				if block_num != 0:
					blocks[block_num].append(info)

		if field[0] == 'INDIRECT':
			info = ['', field[1], int(field[3])]
			blocks[int(field[5])].append(info)

		if field[0] == 'DIRENT':
			list_dirent.append(Dirent(field))

	block_check(super_block, group, blocks)
	inode_check(super_block, free_inodes, list_dirent, inodes)
 
def main():
	if len(sys.argv) != 2:
		sys.stderr.write("Error receiving filename : ./lab3b filename\n")
		exit(1)

	try:
		input_file = open(sys.argv[1], "r")
	except:
		sys.stderr.write('Failed to open file.\n')
		exit(1)

	lines = input_file.readlines()
	process_csv(lines)

	if damaged:
		exit(2)
	else :
		exit(0)

if __name__ == '__main__':
	main()