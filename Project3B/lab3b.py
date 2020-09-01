#!/usr/local/cs/bin/python

#NAME: Changhui Youn
#EMAIL: tonyyoun2@gmail.com
#ID: 304207830

from collections import defaultdict
from sets import Set
import sys


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
    def read_blocks(self, field):
        list_blocks = []
        if field[2] != 's':
            for i in range(12, 24):
                list_blocks.append(int(field[i]))
        return list_blocks

    def read_pointers(self, field):
        list_pointers = []
        if field[2] != 's':
            for i in range(24, 27):
                list_pointers.append(int(field[i]))
        return list_pointers

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
        self.list_blocks = self.read_blocks(field)
        self.list_pointers = self.read_pointers(field)


class Dirent:
    def __init__(self, field):
        self.parent_inode_num = int(field[1])
        self.offset = int(field[2])
        self.inode_num = int(field[3])
        self.entry_length = int(field[4])
        self.name_length = int(field[5])
        self.name = str(field[6])

class Indirect:
    def __init__(self, field):
        self.inode_num = int(field[1])
        self.indirection_level = int(field[2])
        self.offset = int(field[3])
        self.indirect_block_num = int(field[4])
        self.ref_block_num = int(field[5])


blocks = defaultdict(list)
damaged = False

def blockData(lines):
	super_block = None
	group = None
	inodes = list()

	for line in lines:
		fields = line.split(',')
		if fields[0] == 'SUPERBLOCK':
			super_block = SuperBlock(fields)
			block_size = int(fields[3])
			inode_size = int(fields[4])

		if fields[0] == 'GROUP':
			group = Group(fields)
			num_blocks = int(fields[2])
			num_inodes = int(fields[3])
			first_valid_block = int(fields[8]) + inode_size * num_inodes / block_size

		if fields[0] == 'BFREE':
			blocks[int(fields[1])].append(['free'])

		if fields[0] == 'INODE':
			inodes.append(Inode(fields))
			inode_num = int(fields[1])
			for i in range(12, 27):
				block_num = int(fields[i])
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

				info = [typ, inode_num, offset] # a 2 element structure with {'type', inode #}
				if block_num != 0:
					blocks[block_num].append(info)

		if fields[0] == 'INDIRECT':
			typ = ''
			inode_num = fields[1]
			block_num = int(fields[5])
			offset = int(fields[3])
			info = [typ, inode_num, offset]
			blocks[block_num].append(info)


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
			if blocknum < 0 or blocknum > num_blocks:
				if typ != '':
					typ += ' '
				sys.stdout.write('INVALID '+typ+'BLOCK '+str(blocknum)+' IN INODE '+str(inum)+' AT OFFSET '+str(offset)+'\n')
				damaged = True

			if blocknum > 0 and blocknum < first_valid_block:
				if typ != '':
					typ += ' '
				sys.stdout.write('RESERVED '+typ+'BLOCK '+str(blocknum)+' IN INODE '+str(inum)+' AT OFFSET '+str(offset)+'\n')
				damaged = True

def inodeDirCheck(lines):
	freenodes = Set()
	linkCounts = dict()
	parentInode = dict()
	#Collect the raw data
	for rawline in lines:
		line = rawline.rstrip('\r\n')
		fields = line.split(',') 
		if fields[0] == 'IFREE':
			freenodes.add(int(fields[1]))
		if fields[0] == 'SUPERBLOCK':
			firstNode = int(fields[7])
			numNodes = int(fields[2])
		if fields[0] == 'DIRENT':
			inodeNum = int(fields[3])
			if inodeNum not in linkCounts:
				linkCounts[inodeNum] = 1
			else:
				linkCounts[inodeNum] = linkCounts[inodeNum] + 1
			if inodeNum not in parentInode:
				parentInode[inodeNum] = int(fields[1])
	allocnodes = Set()
	for line in lines:
		fields = line.split(',')
		#Check which nodes are allocated and if linkage numbers are correct
		if fields[0] == 'INODE':
			inodeNum = int(fields[1])
			if inodeNum in freenodes:
				sys.stdout.write('ALLOCATED INODE ' + str(inodeNum) + ' ON FREELIST'+'\n')
				damaged = True
			allocnodes.add(inodeNum)
			linkCnt = int(fields[6])
			if inodeNum in linkCounts and linkCounts[inodeNum] != linkCnt:
				sys.stdout.write('INODE ' + str(inodeNum) + ' HAS ' + str(linkCounts[inodeNum]) + ' LINKS BUT LINKCOUNT IS ' + str(linkCnt) + '\n')
				damaged = True
			elif inodeNum not in linkCounts:
				sys.stdout.write('INODE ' + str(inodeNum) + ' HAS 0 LINKS BUT LINKCOUNT IS ' + str(linkCnt) + '\n')
				damaged = True
	#After we know which nodes are allocated, check for missing unallocated node entries
	for x in range(firstNode,numNodes + 1):
		if x not in freenodes and x not in allocnodes:
			sys.stdout.write('UNALLOCATED INODE ' + str(x) + ' NOT ON FREELIST' + '\n')
			damaged = True

	for rawline in lines:
		line = rawline.rstrip('\r\n')
		fields = line.split(',')
		#Check that directory information is correct, including unallocated, invalid inodes, . , and .. files
		if fields[0] == 'DIRENT':
			inodeNum = int(fields[3])
			dirNum = int(fields[1])
			if inodeNum not in allocnodes and inodeNum in range (1,numNodes + 1):
				sys.stdout.write('DIRECTORY INODE ' + str(dirNum) + ' NAME ' + fields[6] + ' UNALLOCATED INODE ' + str(inodeNum) + '\n')
				damaged = True
			elif inodeNum < 1 or inodeNum > numNodes:
				sys.stdout.write('DIRECTORY INODE ' + str(dirNum) + ' NAME ' + fields[6] + ' INVALID INODE ' + str(inodeNum) + '\n')
				damaged = True
			name = fields[6]
			if fields[6] == "'.'":
				if fields[3] != fields[1]:
					sys.stdout.write('DIRECTORY INODE ' + str(dirNum) + ' NAME ' + fields[6] + ' LINK TO INODE ' + fields[3] + ' SHOULD BE ' + fields[1] + '\n')
					damaged = True
			if fields[6] == "'..'":
				if int(fields[3]) != parentInode[int(fields[1])]:
					sys.stdout.write('DIRECTORY INODE ' + str(dirNum) + ' NAME ' + fields[6] + ' LINK TO INODE ' + fields[3] + ' SHOULD BE ' + str(parentInode[int(fields[1])]) + '\n')
					damaged = True


 
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
	blockData(lines)
	inodeDirCheck(lines)

	if damaged:
		exit(2)
	else :
		exit(0)

if __name__ == '__main__':
	main()