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
#define EXT2_FILE 0x8000
#define EXT2_DIR 0x4000
#define EXT2_SYM 0xA000

int disk_fd;

struct ext2_super_block superblock;
uint32_t block_size = 1024;


/* returns the offset for a block number */
unsigned long block_offset(unsigned int block) {
	return SUPER_OFFSET + (block - 1) * block_size;
}

char* format_time(uint32_t time) {
	char* formattedDate = malloc(sizeof(char)*32);
	time_t rawtime = time;
	struct tm* info = gmtime(&rawtime);
	strftime(formattedDate, 32, "%m/%d/%y %H:%M:%S", info);
	return formattedDate;
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


void superblock_summary() {
	pread(disk_fd, &superblock, sizeof(superblock), SUPER_OFFSET);
	block_size = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
	fprintf(stdout, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", superblock.s_blocks_count, superblock.s_inodes_count, block_size, superblock.s_inode_size, superblock.s_blocks_per_group, superblock.s_inodes_per_group, superblock.s_first_ino);

}

void free_block_enties(uint32_t block_bitmap, int group_num){
	char* block = (char*) malloc(block_size);
	uint32_t num_free_block = superblock.s_first_data_block + superblock.s_blocks_per_group * group_num;
	pread(disk_fd, block, block_size, SUPER_OFFSET + (block_bitmap - 1) * block_size);

	uint32_t i, j;
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

void inode_summary(uint32_t inode_table, int index, uint32_t num_free_inode) {
	struct ext2_inode inode;

	uint32_t offset = SUPER_OFFSET + (inode_table - 1) * block_size + sizeof(struct ext2_inode) * index;
	pread(disk_fd, &inode, sizeof(struct ext2_inode), offset);

	if (inode.i_mode == 0 || inode.i_links_count == 0) {
		return;
	}

	char file_type = '?';

	uint16_t file_bit = inode.i_mode & 0xF000;
	if (file_bit == EXT2_FILE) {
		file_type = 'f';
	} else if (file_bit == EXT2_DIR) { 
		file_type = 'd';
	} else if (file_bit == EXT2_SYM) {
		file_type = 's';
	}

	uint16_t imode = inode.i_mode & 0xFFF;
	uint16_t owner = inode.i_uid;
	uint16_t group = inode.i_gid;
	uint16_t link_count = inode.i_links_count;
	char* ctime = format_time(inode.i_ctime);
	char* mtime = format_time(inode.i_mtime);
	char* atime = format_time(inode.i_atime);
	uint32_t file_size = inode.i_size;
	uint32_t num_blocks = inode.i_blocks;
	fprintf(stdout, "INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d", num_free_inode, file_type, imode, owner, group, link_count, ctime, mtime, atime, file_size, num_blocks);


	uint32_t i;
	for (i = 0; i < 15; i++) { //block addresses
		fprintf(stdout, ",%d", inode.i_block[i]);
	}
	fprintf(stdout, "\n");

	//if the file_type is a directory, need to create a directory entry
	for (i = 0; i < 12; i++) { //direct entries
		if (inode.i_block[i] != 0 && file_type == 'd') {
			read_dir_entry(num_free_inode, inode.i_block[i]);
		}
	}

	//indirect entry
	if (inode.i_block[12] != 0) {
		uint32_t *block_ptrs = malloc(block_size);
		uint32_t num_ptrs = block_size / sizeof(uint32_t);

		uint32_t indir_offset = block_offset(inode.i_block[12]);
		pread(disk_fd, block_ptrs, block_size, indir_offset);

		uint32_t j;
		for (j = 0; j < num_ptrs; j++) {
			if (block_ptrs[j] != 0) {
				if (file_type == 'd') {
					read_dir_entry(num_free_inode, block_ptrs[j]);
				}
				fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
					num_free_inode, //inode number
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

		uint32_t indir2_offset = block_offset(inode.i_block[13]);
		pread(disk_fd, indir_block_ptrs, block_size, indir2_offset);

		uint32_t j;
		for (j = 0; j < num_ptrs; j++) {
			if (indir_block_ptrs[j] != 0) {
				fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
					num_free_inode, //inode number
					2, //level of indirection
					256 + 12 + j, //logical block offset
					inode.i_block[13], //block number of indirect block being scanned
					indir_block_ptrs[j] //block number of reference block
				);

				//search through this indirect block to find its directory entries
				uint32_t *block_ptrs = malloc(block_size);
				uint32_t indir_offset = block_offset(indir_block_ptrs[j]);
				pread(disk_fd, block_ptrs, block_size, indir_offset);

				uint32_t k;
				for (k = 0; k < num_ptrs; k++) {
					if (block_ptrs[k] != 0) {
						if (file_type == 'd') {
							read_dir_entry(num_free_inode, block_ptrs[k]);
						}
						fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
							num_free_inode, //inode number
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

		uint32_t indir3_offset = block_offset(inode.i_block[14]);
		pread(disk_fd, indir2_block_ptrs, block_size, indir3_offset);

		uint32_t j;
		for (j = 0; j < num_ptrs; j++) {
			if (indir2_block_ptrs[j] != 0) {
				fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
					num_free_inode, //inode number
					3, //level of indirection
					65536 + 256 + 12 + j, //logical block offset
					inode.i_block[14], //block number of indirect block being scanned
					indir2_block_ptrs[j] //block number of reference block
				);

				uint32_t *indir_block_ptrs = malloc(block_size);
				uint32_t indir2_offset = block_offset(indir2_block_ptrs[j]);
				pread(disk_fd, indir_block_ptrs, block_size, indir2_offset);

				uint32_t k;
				for (k = 0; k < num_ptrs; k++) {
					if (indir_block_ptrs[k] != 0) {
						fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
							num_free_inode, //inode number
							2, //level of indirection
							65536 + 256 + 12 + k, //logical block offset
				 			indir2_block_ptrs[j], //block number of indirect block being scanned
							indir_block_ptrs[k] //block number of reference block	
						);	
						uint32_t *block_ptrs = malloc(block_size);
						uint32_t indir_offset = block_offset(indir_block_ptrs[k]);
						pread(disk_fd, block_ptrs, block_size, indir_offset);

						uint32_t l;
						for (l = 0; l < num_ptrs; l++) {
							if (block_ptrs[l] != 0) {
								if (file_type == 'd') {
									read_dir_entry(num_free_inode, block_ptrs[l]);
								}
								fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
									num_free_inode, //inode number
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

void free_inode_entries(uint32_t inode_bitmap, uint32_t inode_table, int group_num){
	uint32_t inode_size = superblock.s_inodes_per_group / 8;
	char* inode = (char*) malloc(inode_size);
	uint32_t num_free_inode = superblock.s_inodes_per_group * group_num + 1;
	pread(disk_fd, inode, inode_size, SUPER_OFFSET + (inode_bitmap - 1) * block_size);

	uint32_t start = num_free_inode;
	uint32_t i, j;
	for (i = 0; i < inode_size; i++) {
		char data = inode[i];
		for (j = 0; j < 8; j++) {
			// if not used
			if (!(1 & data)) {
				fprintf(stdout, "IFREE,%d\n", num_free_inode);
			} else {
				int index = num_free_inode - start;
				inode_summary(inode_table, index, num_free_inode);
			}
			data = data >> 1;
			num_free_inode++;
		}
	}
	free(inode);
}

void group_summary(int group_num, int num_group) {
	struct ext2_group_desc group_desc;
	pread(disk_fd, &group_desc, sizeof(struct ext2_group_desc), SUPER_OFFSET + sizeof(struct ext2_super_block));

	int num_block = (group_num == num_group - 1 ? superblock.s_blocks_count - (superblock.s_blocks_per_group * (num_group - 1)) : superblock.s_blocks_per_group);
	int num_inode = (group_num == num_group - 1 ? superblock.s_inodes_count - (superblock.s_inodes_per_group * (num_group - 1)) : superblock.s_inodes_per_group);

	fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", group_num, num_block, num_inode, group_desc.bg_free_blocks_count, group_desc.bg_free_inodes_count, group_desc.bg_block_bitmap, group_desc.bg_inode_bitmap, group_desc.bg_inode_table);

	free_block_enties(group_desc.bg_block_bitmap, group_num);
	free_inode_entries(group_desc.bg_inode_bitmap, group_desc.bg_inode_table, group_num);
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
	int num_group = superblock.s_blocks_count / superblock.s_blocks_per_group + 1;
	int i;
	for (i = 0; i < num_group; i++){
		group_summary(i, num_group);
	}

	return 0;
}