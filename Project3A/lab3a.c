/*
NAME: Changhui Youn
EMAIL: tonyyoun2@gmail.com
ID: 304207830
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <time.h>
#include <getopt.h>
#include "ext2_fs.h"

#define SUPER_OFFSET 1024


int disk_fd;

struct ext2_super_block superblock;
unsigned int block_size = 1024;


/* returns the offset for a block number */
unsigned long block_offset(unsigned int block) {
	return SUPER_OFFSET + (block - 1) * block_size;
}

/* store the time in the format mm/dd/yy hh:mm:ss, GMT
 * 'raw_time' is a 32 bit value representing the number of
 * seconds since January 1st, 1970
 */
void get_time(time_t raw_time, char* buf) {
	time_t epoch = raw_time;
	struct tm ts = *gmtime(&epoch);
	strftime(buf, 80, "%m/%d/%y %H:%M:%S", &ts);
}

/* given location of directory entry block, produce directory entry summary */
void read_dir_entry(unsigned int parent_inode, unsigned int block_num) {
	struct ext2_dir_entry dir_entry;
	unsigned long offset = block_offset(block_num);
	unsigned int num_bytes = 0;

	while(num_bytes < block_size) {
		memset(dir_entry.name, 0, 256);
		pread(disk_fd, &dir_entry, sizeof(dir_entry), offset + num_bytes);
		if (dir_entry.inode != 0) { //entry is not empty
			memset(&dir_entry.name[dir_entry.name_len], 0, 256 - dir_entry.name_len);
			fprintf(stdout, "DIRENT,%d,%d,%d,%d,%d,'%s'\n",
				parent_inode, //parent inode number
				num_bytes, //logical byte offset
				dir_entry.inode, //inode number of the referenced file
				dir_entry.rec_len, //entry length
				dir_entry.name_len, //name length
				dir_entry.name //name, string, surrounded by single-quotes
			);
		}
		num_bytes += dir_entry.rec_len;
	}
}

/* for an allocated inode, print its summary */
void read_inode(unsigned int inode_table_id, unsigned int index, unsigned int inode_num) {
	struct ext2_inode inode;

	unsigned long offset = block_offset(inode_table_id) + index * sizeof(inode);
	pread(disk_fd, &inode, sizeof(inode), offset);

	if (inode.i_mode == 0 || inode.i_links_count == 0) {
		return;
	}

	char filetype = '?';
	//get bits that determine the file type
	uint16_t file_val = (inode.i_mode >> 12) << 12;
	if (file_val == 0xa000) { //symbolic link
		filetype = 's';
	} else if (file_val == 0x8000) { //regular file
		filetype = 'f';
	} else if (file_val == 0x4000) { //directory
		filetype = 'd';
	}

	unsigned int num_blocks = 2 * (inode.i_blocks / (2 << superblock.s_log_block_size));

	fprintf(stdout, "INODE,%d,%c,%o,%d,%d,%d,",
		inode_num, //inode number
		filetype, //filetype
		inode.i_mode & 0xFFF, //mode, low order 12-bits
		inode.i_uid, //owner
		inode.i_gid, //group
		inode.i_links_count //link count
	);

	char ctime[20], mtime[20], atime[20];
    	get_time(inode.i_ctime, ctime); //creation time
    	get_time(inode.i_mtime, mtime); //modification time
    	get_time(inode.i_atime, atime); //access time
    	fprintf(stdout, "%s,%s,%s,", ctime, mtime, atime);
		
	fprintf(stdout, "%d,%d", 
	    	inode.i_size, //file size
		num_blocks //number of blocks
	);

	unsigned int i;
	for (i = 0; i < 15; i++) { //block addresses
		fprintf(stdout, ",%d", inode.i_block[i]);
	}
	fprintf(stdout, "\n");

	//if the filetype is a directory, need to create a directory entry
	for (i = 0; i < 12; i++) { //direct entries
		if (inode.i_block[i] != 0 && filetype == 'd') {
			read_dir_entry(inode_num, inode.i_block[i]);
		}
	}

	//indirect entry
	if (inode.i_block[12] != 0) {
		uint32_t *block_ptrs = malloc(block_size);
		uint32_t num_ptrs = block_size / sizeof(uint32_t);

		unsigned long indir_offset = block_offset(inode.i_block[12]);
		pread(disk_fd, block_ptrs, block_size, indir_offset);

		unsigned int j;
		for (j = 0; j < num_ptrs; j++) {
			if (block_ptrs[j] != 0) {
				if (filetype == 'd') {
					read_dir_entry(inode_num, block_ptrs[j]);
				}
				fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
					inode_num, //inode number
					1, //level of indirection
					12 + j, //logical block offset
					inode.i_block[12], //block number of indirect block being scanned
					block_ptrs[j] //block number of reference block
				);
			}
		}
		free(block_ptrs);
	}

	//doubly indirect entry
	if (inode.i_block[13] != 0) {
		uint32_t *indir_block_ptrs = malloc(block_size);
		uint32_t num_ptrs = block_size / sizeof(uint32_t);

		unsigned long indir2_offset = block_offset(inode.i_block[13]);
		pread(disk_fd, indir_block_ptrs, block_size, indir2_offset);

		unsigned int j;
		for (j = 0; j < num_ptrs; j++) {
			if (indir_block_ptrs[j] != 0) {
				fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
					inode_num, //inode number
					2, //level of indirection
					256 + 12 + j, //logical block offset
					inode.i_block[13], //block number of indirect block being scanned
					indir_block_ptrs[j] //block number of reference block
				);

				//search through this indirect block to find its directory entries
				uint32_t *block_ptrs = malloc(block_size);
				unsigned long indir_offset = block_offset(indir_block_ptrs[j]);
				pread(disk_fd, block_ptrs, block_size, indir_offset);

				unsigned int k;
				for (k = 0; k < num_ptrs; k++) {
					if (block_ptrs[k] != 0) {
						if (filetype == 'd') {
							read_dir_entry(inode_num, block_ptrs[k]);
						}
						fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
							inode_num, //inode number
							1, //level of indirection
							256 + 12 + k, //logical block offset
					 		indir_block_ptrs[j], //block number of indirect block being scanned
							block_ptrs[k] //block number of reference block
						);
					}
				}
				free(block_ptrs);
			}
		}
		free(indir_block_ptrs);
	}

	//triply indirect entry
	if (inode.i_block[14] != 0) {
		uint32_t *indir2_block_ptrs = malloc(block_size);
		uint32_t num_ptrs = block_size / sizeof(uint32_t);

		unsigned long indir3_offset = block_offset(inode.i_block[14]);
		pread(disk_fd, indir2_block_ptrs, block_size, indir3_offset);

		unsigned int j;
		for (j = 0; j < num_ptrs; j++) {
			if (indir2_block_ptrs[j] != 0) {
				fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
					inode_num, //inode number
					3, //level of indirection
					65536 + 256 + 12 + j, //logical block offset
					inode.i_block[14], //block number of indirect block being scanned
					indir2_block_ptrs[j] //block number of reference block
				);

				uint32_t *indir_block_ptrs = malloc(block_size);
				unsigned long indir2_offset = block_offset(indir2_block_ptrs[j]);
				pread(disk_fd, indir_block_ptrs, block_size, indir2_offset);

				unsigned int k;
				for (k = 0; k < num_ptrs; k++) {
					if (indir_block_ptrs[k] != 0) {
						fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
							inode_num, //inode number
							2, //level of indirection
							65536 + 256 + 12 + k, //logical block offset
				 			indir2_block_ptrs[j], //block number of indirect block being scanned
							indir_block_ptrs[k] //block number of reference block	
						);	
						uint32_t *block_ptrs = malloc(block_size);
						unsigned long indir_offset = block_offset(indir_block_ptrs[k]);
						pread(disk_fd, block_ptrs, block_size, indir_offset);

						unsigned int l;
						for (l = 0; l < num_ptrs; l++) {
							if (block_ptrs[l] != 0) {
								if (filetype == 'd') {
									read_dir_entry(inode_num, block_ptrs[l]);
								}
								fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
									inode_num, //inode number
									1, //level of indirection
									65536 + 256 + 12 + l, //logical block offset
				 					indir_block_ptrs[k], //block number of indirect block being scanned
									block_ptrs[l] //block number of reference block	
								);
							}
						}
						free(block_ptrs);
					}
				}
				free(indir_block_ptrs);
			}
		}
		free(indir2_block_ptrs);
	}
}


void superblock_summary() {
	pread(disk_fd, &superblock, sizeof(superblock), SUPER_OFFSET);
	block_size = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
	fprintf(stdout, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", superblock.s_blocks_count, superblock.s_inodes_count, block_size, superblock.s_inode_size, superblock.s_blocks_per_group, superblock.s_inodes_per_group, superblock.s_first_ino);

}

void free_block_enties(int group_num, uint32_t block_bitmap){
	char* block = (char*) malloc(block_size);
	unsigned int num_free_block = superblock.s_first_data_block + group_num * superblock.s_blocks_per_group;
	pread(disk_fd, block, block_size, SUPER_OFFSET + (block_bitmap - 1) * block_size);

	unsigned int i, j;
	for (i = 0; i < block_size; i++) {
		char data = block[i];
		for (j = 0; j < 8; j++) {
			// if not used
			if (!(1 & data)) {
				fprintf(stdout, "BFREE,%d\n", num_free_block);
			}
			data = data >> 1;
			num_free_block++;
		}
	}
	free(block);
}

void free_inode_entries(int group_num, uint32_t inode_bitmap, uint32_t inode_table){
	int num_bytes = superblock.s_inodes_per_group / 8;
	char* bytes = (char*) malloc(num_bytes);

	unsigned long offset = SUPER_OFFSET + (inode_bitmap - 1) * block_size;
	unsigned int num_free_inode = group_num * superblock.s_inodes_per_group + 1;
	unsigned int start = num_free_inode;
	pread(disk_fd, bytes, num_bytes, offset);

	int i, j;
	for (i = 0; i < num_bytes; i++) {
		char data = bytes[i];
		for (j = 0; j < 8; j++) {
			int used = 1 & data;
			if (used) { //inode is allocated
				read_inode(inode_table, num_free_inode - start, num_free_inode);
			} else { //free inode
				fprintf(stdout, "IFREE,%d\n", num_free_inode);
			}
			data = data >> 1;
			num_free_inode++;
		}
	}
	free(bytes);
}

void group_summary(int group_num, int num_group) {
	struct ext2_group_desc group_desc;
	pread(disk_fd, &group_desc, sizeof(struct ext2_group_desc), SUPER_OFFSET + sizeof(struct ext2_super_block));

	int num_block = (group_num == num_group - 1 ? superblock.s_blocks_count - (superblock.s_blocks_per_group * (num_group - 1)) : superblock.s_blocks_per_group);
	int num_inode = (group_num == num_group - 1 ? superblock.s_inodes_count - (superblock.s_inodes_per_group * (num_group - 1)) : superblock.s_inodes_per_group);

	fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", group_num, num_block, num_inode, group_desc.bg_free_blocks_count, group_desc.bg_free_inodes_count, group_desc.bg_block_bitmap, group_desc.bg_inode_bitmap, group_desc.bg_inode_table);

	free_block_enties(group_num, group_desc.bg_block_bitmap);
	free_inode_entries(group_num, group_desc.bg_inode_bitmap, group_desc.bg_inode_table);
}

int main(int argc, char const *argv[])
{
	if(argc != 2){
		fprintf(stderr, "%s\n", "Incorrect arguments");
		exit(1);
	}
	if((disk_fd = open(argv[1], O_RDONLY)) < 0)
	{
		fprintf(stderr, "%s\n", "Could not mount" );
		exit(2);
	}
	superblock_summary();
	int num_group = superblock.s_blocks_count / superblock.s_blocks_per_group;
	int i;
	for (i = 0; i < num_group; i++){
		group_summary(i, num_group);
	}

	return 0;
}