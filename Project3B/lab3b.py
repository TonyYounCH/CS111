#!/usr/local/cs/bin/python

#NAME: Changhui Youn
#EMAIL: tonyyoun2@gmail.com
#ID: 304207830

from collections import defaultdict
from sets import Set
import sys

damaged = False

class SuperBlock:
	def __init__(self, field):
		self.total_blocks = int(field[1])
		self.total_inodes = int(field[2])
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
		self.bitmap = int(field[6])
		self.imap = int(field[7])
		self.first_block = int(field[8])

class Inode:
    def __init__(self, field):
        self.inode_num = int(field[1])
        self.file_type = (field[2])
        self.mode = field[3]
        self.owner = int(field[4])
        self.group = int(field[5])
        self.link_count = int(field[6])
        self.change_time = field[7]
        self.mod_time = field[8]
        self.access_time = field[9]
        self.file_size = int(field[10])
        self.block_num = int(field[11])

class Dirent:
	def __init__(self, field):
		self.parent_inode_num = int(field[1])
		self.offset = int(field[2])
		self.inode_num = int(field[3])
		self.entry_length = int(field[4])
		self.name_length = int(field[5])
		self.name = str(field[6])

def blockData(super_block, group, blocks):

	first_valid_block = group.first_block + super_block.inode_size * group.total_num_of_inodes / super_block.block_size
	i = first_valid_block
	for blocknum in blocks:
		if i > blocknum:
			continue
		while i != blocknum and i < 64:
			sys.stdout.write('UNREFERENCED BLOCK '+str(i)+'\n')
			damaged = True
			i = i + 1
		i = i + 1


	for blocknum, lst in blocks.iteritems():
		if len(lst) > 1:
			allocated_and_free = [0, 0]
			num_references = 0
			for item in lst:
				if item[0] == 'free':
					allocated_and_free[0] = 1
				if item[0] != 'free':
					allocated_and_free[1] = 1
					num_references = num_references + 1
			if allocated_and_free[0] == 1 and allocated_and_free[1] == 1:
				sys.stdout.write('ALLOCATED BLOCK '+str(blocknum)+' ON FREELIST'+'\n')
				damaged = True
			if num_references > 1:
				for item in lst:
					if item[0] != 'free':
						typ = item[0]
						if typ != '':
							typ += ' '
						inum = item[1]
						offset = item[2]
						sys.stdout.write('DUPLICATE '+typ+'BLOCK '+str(blocknum)+' IN INODE '+str(inum)+' AT OFFSET '+str(offset)+'\n')
						damaged = True


		for item in lst:
			typ = item[0]
			if typ != 'free':
				inum = item[1]
				offset =  item[2]
			if blocknum < 0 or blocknum > group.total_num_of_blocks:
				if typ != '':
					typ += ' '
				sys.stdout.write('INVALID '+typ+'BLOCK '+str(blocknum)+' IN INODE '+str(inum)+' AT OFFSET '+str(offset)+'\n')
				damaged = True

			if blocknum > 0 and blocknum < first_valid_block:
				if typ != '':
					typ += ' '
				sys.stdout.write('RESERVED '+typ+'BLOCK '+str(blocknum)+' IN INODE '+str(inum)+' AT OFFSET '+str(offset)+'\n')
				damaged = True

def inodeDirCheck(super_block, freenodes, list_dirent, inodes, lines):
	linkCounts = dict()
	parentInode = dict()
	allocnodes = Set()

	for dirent in list_dirent:
		if dirent.inode_num not in linkCounts:
			linkCounts[dirent.inode_num] = 1
		else:
			linkCounts[dirent.inode_num] = linkCounts[dirent.inode_num] + 1
		if dirent.inode_num not in parentInode:
			parentInode[dirent.inode_num] = dirent.parent_inode_num

	for inode in inodes:
		if inode.inode_num in freenodes:
			sys.stdout.write('ALLOCATED INODE ' + str(inode.inode_num) + ' ON FREELIST'+'\n')
			damaged = True
		allocnodes.add(inode.inode_num)
		if inode.inode_num in linkCounts and linkCounts[inode.inode_num] != inode.link_count:
			sys.stdout.write('INODE ' + str(inode.inode_num) + ' HAS ' + str(linkCounts[inode.inode_num]) + ' LINKS BUT LINKCOUNT IS ' + str(inode.link_count) + '\n')
			damaged = True
		elif inode.inode_num not in linkCounts:
			sys.stdout.write('INODE ' + str(inode.inode_num) + ' HAS 0 LINKS BUT LINKCOUNT IS ' + str(inode.link_count) + '\n')
			damaged = True

	#After we know which nodes are allocated, check for missing unallocated node entries
	for x in range(super_block.first_non_reserved_inode, super_block.total_inodes + 1):
		if x not in freenodes and x not in allocnodes:
			sys.stdout.write('UNALLOCATED INODE ' + str(x) + ' NOT ON FREELIST' + '\n')
			damaged = True

	for dirent in list_dirent:
		if dirent.inode_num not in allocnodes and dirent.inode_num in range (1,super_block.total_inodes + 1):
			sys.stdout.write('DIRECTORY INODE ' + str(dirent.parent_inode_num) + ' NAME ' + str(dirent.name) + ' UNALLOCATED INODE ' + str(dirent.inode_num) + '\n')
			damaged = True
		elif dirent.inode_num < 1 or dirent.inode_num > super_block.total_inodes:
			sys.stdout.write('DIRECTORY INODE ' + str(dirent.parent_inode_num) + ' NAME ' + str(dirent.name) + ' INVALID INODE ' + str(dirent.inode_num) + '\n')
			damaged = True
		if dirent.name == "'.'":
			if dirent.inode_num != dirent.parent_inode_num:
				sys.stdout.write('DIRECTORY INODE ' + str(dirent.parent_inode_num) + ' NAME ' + str(dirent.name) + ' LINK TO INODE ' + dirent.inode_num + ' SHOULD BE ' + dirent.parent_inode_num + '\n')
				damaged = True
		if dirent.name == "'..'":
			if dirent.inode_num != parentInode[dirent.parent_inode_num]:
				sys.stdout.write('DIRECTORY INODE ' + str(dirent.parent_inode_num) + ' NAME ' + str(dirent.name) + ' LINK TO INODE ' + dirent.inode_num + ' SHOULD BE ' + str(parentInode[dirent.parent_inode_num]) + '\n')
				damaged = True

def process_csv(lines):
	blocks = defaultdict(list)
	super_block = None
	group = None
	inodes = list()
	list_dirent = list()

	freenodes = Set()

	for line in lines:
		field = line.split(',')
		if field[0] == 'SUPERBLOCK':
			super_block = SuperBlock(field)

		if field[0] == 'GROUP':
			group = Group(field)

		if field[0] == 'BFREE':
			blocks[int(field[1])].append(['free'])

		if field[0] == 'IFREE':
			freenodes.add(int(field[1]))

		if field[0] == 'INODE':
			inodes.append(Inode(field))
			for i in range(12, 27):
				block_num = int(field[i])
				offset = i - 12
				if i < 24:
					typ = ''
				elif i == 24:
					typ = 'INDIRECT'
				elif i == 25:
					typ = 'DOUBLE INDIRECT'
					offset = 12 + 256
				elif i == 26:
					typ = 'TRIPLE INDIRECT'
					offset = 12 + 256 + 256*256

				info = [typ, int(field[1]), offset]
				if block_num != 0:
					blocks[block_num].append(info)

		if field[0] == 'INDIRECT':
			typ = ''
			inode_num = field[1]
			block_num = int(field[5])
			offset = int(field[3])
			info = [typ, inode_num, offset]
			blocks[block_num].append(info)

		if field[0] == 'DIRENT':
			list_dirent.append(Dirent(field))

	blockData(super_block, group, blocks)
	inodeDirCheck(super_block, freenodes, list_dirent, inodes, lines)
 
def main():
	if len(sys.argv) != 2:
		sys.stderr.write("Must provide filename : ./lab3b fiilename\n")
		exit(1)

	try:
		input_file = open(sys.argv[1], "r")
	except IOError:
		sys.stderr.write("File cannot be opened\n")
		exit(1)

	exitcode = 0;
	lines = input_file.readlines()
	process_csv(lines)

	if damaged:
		exit(2)
	else :
		exit(0)

if __name__ == '__main__':
	main()