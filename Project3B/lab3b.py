#!/usr/local/cs/bin/python

#NAME: Changhui Youn
#EMAIL: tonyyoun2@gmail.com
#ID: 304207830

from collections import defaultdict
from sets import Set
import sys

blocks = defaultdict(list)
damaged = False

def blockData(file):
	file.seek(0)
	for line in file:
		fields = line.split(',')
		if fields[0] == 'SUPERBLOCK':
			inode_size = int(fields[4])
			block_size = int(fields[3])

		if fields[0] == 'GROUP':
			num_blocks = int(fields[2])
			num_inodes = int(fields[3])
			first_valid_block = int(fields[8]) + inode_size * num_inodes / block_size

		if fields[0] == 'BFREE':
			info = ['free']
			blocks[int(fields[1])].append(info)

		if fields[0] == 'INODE':
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
			'''
			if fields[2] == 1:
				typ = 'INDIRECT'
			if fields[2] == 2:
				typ = 'DOUBLE INDIRECT'
			if fields[2] == 3:
				typ = 'TRIPLE INDIRECT'
			'''
			info = [typ, inode_num, offset]
			blocks[block_num].append(info)

################################################################################################
#####   NOW PROCESS THE BLOCK DATA AND OUTPUT STUFF  ###########################################

	#first figure out which are unreferenced
	i = first_valid_block
	for blocknum in blocks:
		if i > blocknum:
			continue
		while i != blocknum and i < 64:
			sys.stdout.write('UNREFERENCED BLOCK '+str(i)+'\n')
			damaged = True
			i = i + 1
		i = i + 1

	#iterate over dict entries in blocks
	for blocknum, lst in blocks.iteritems():
		#print blocknum, lst, len(lst)
		#check for allocated blocks on freelist and for duplicate references
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
				for item in lst: #yeah, I looped over it again... maybe there's a better way to do this
					if item[0] != 'free':
						typ = item[0]
						if typ != '':
							typ += ' '
						inum = item[1]
						offset = item[2]
						sys.stdout.write('DUPLICATE '+typ+'BLOCK '+str(blocknum)+' IN INODE '+str(inum)+' AT OFFSET '+str(offset)+'\n')
						damaged = True


			#sys.stdout.write('DUPLICATE')

		#iterate over the lists in lst
		for item in lst:
			typ = item[0]
			if typ != 'free':
				inum = item[1]
				offset =  item[2]
			#check in bounds
			if blocknum < 0 or blocknum > num_blocks:
				if typ != '':
					typ += ' '
				sys.stdout.write('INVALID '+typ+'BLOCK '+str(blocknum)+' IN INODE '+str(inum)+' AT OFFSET '+str(offset)+'\n')
				damaged = True

			#check reserved
			if blocknum > 0 and blocknum < first_valid_block:
				if typ != '':
					typ += ' '
				sys.stdout.write('RESERVED '+typ+'BLOCK '+str(blocknum)+' IN INODE '+str(inum)+' AT OFFSET '+str(offset)+'\n')
				damaged = True

def inodeDirCheck(file):
	freenodes = Set()
	linkCounts = dict()
	file.seek(0)
	parentInode = dict()
	#Collect the raw data
	for rawline in file:
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
	file.seek(0)
	for line in file:
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
	file.seek(0)
	for rawline in file:
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
		print('Usage Error: ./lab3b fileName', file=sys.stderr)
		exit(1)

	try:
		input_file = open(sys.argv[1], "r")
	except:
		sys.stderr.write("Error. Cannot open file" + '\n')
		exit(1)

	exitcode = 0;
	blockData(input_file)
	inodeDirCheck(input_file)
	input_file.close()

	if damaged:
		exit(2)
	else:
		exit(0)

if __name__ == '__main__':
	main()