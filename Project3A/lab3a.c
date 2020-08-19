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

char* get_time(time_t time) {
	char* time_format = malloc(sizeof(char)*32);
	time_t epoch = time;
	struct tm ts = *gmtime(&epoch);
	strftime(time_format, 32, "%m/%d/%y %H:%M:%S", &ts);
	return time_format;
}


void directory_entries(uint32_t parent_inode, uint32_t block_num)
{
	struct ext2_dir_entry dir_entry;
	uint32_t offset = block_num * block_size;
	uint32_t start = offset; 
	while(offset < start + block_size)
	{
		pread(disk_fd, &dir_entry, sizeof(struct ext2_dir_entry), offset);
		uint32_t logical_byte = offset - start;
		if(dir_entry.inode != 0)
			fprintf(stdout, "DIRENT,%d,%d,%d,%d,%d,'%s'\n", parent_inode, logical_byte, dir_entry.inode, dir_entry.rec_len, dir_entry.name_len, dir_entry.name);
		offset += dir_entry.rec_len;
	}

}

void superblock_summary() {
	pread(disk_fd, &superblock, sizeof(struct ext2_super_block), SUPER_OFFSET);
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

void single_indirect_block(struct ext2_inode inode, uint32_t num_free_inode, char file_type) {
	if (inode.i_block[12] != 0) {
		uint32_t offset = inode.i_block[12] * block_size;
		uint32_t block;		

		uint32_t i;
		for (i = 0; i < block_size / sizeof(uint32_t); i++) {
			pread(disk_fd, &block, sizeof(uint32_t), offset + i * sizeof(uint32_t));
			if (block != 0) {
				if (file_type == 'd') {
					directory_entries(num_free_inode, block);
				}
				fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", num_free_inode, 1, 12 + i, inode.i_block[12], block);
			}
		}
	}

}

void double_indirect_block(struct ext2_inode inode, uint32_t num_free_inode, char file_type) {
	if (inode.i_block[13] != 0) {
		uint32_t indr_offset = inode.i_block[13] * block_size;
		uint32_t indr_block;		

		uint32_t i, j;
		for (i = 0; i < block_size / sizeof(uint32_t); i++) {
			uint32_t offset, block;
			pread(disk_fd, &indr_block, sizeof(uint32_t), indr_offset + i * sizeof(uint32_t));
			if (indr_block != 0) {
				offset = indr_block * block_size;
				fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", num_free_inode, 2, (int) (block_size / sizeof(uint32_t)) + 12 + i, inode.i_block[13], indr_block);
				for (j = 0; j < block_size / sizeof(uint32_t); j++){
					pread(disk_fd, &block, sizeof(uint32_t), offset + j * sizeof(uint32_t));
					if(block != 0) {
						if (file_type == 'd') {
							directory_entries(num_free_inode, block);
						}
						fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", num_free_inode, 1, (int) (block_size / sizeof(uint32_t)) + 12 + j, indr_block, block);

					}
				}
			}
		}
	}

}

void tripple_indirect_block(struct ext2_inode inode, uint32_t num_free_inode, char file_type) {

	if (inode.i_block[14] != 0) {
		uint32_t indr1_offset = inode.i_block[14] * block_size;
		uint32_t indr1_block;		

		uint32_t i, j, k;
		for (i = 0; i < block_size / sizeof(uint32_t); i++) {
			uint32_t indr_offset, indr_block;
			pread(disk_fd, &indr1_block, sizeof(uint32_t), indr1_offset + i * sizeof(uint32_t));
			if (indr1_block != 0) {
				indr_offset = indr1_block * block_size;
				fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", num_free_inode, 3, (int) (block_size / sizeof(uint32_t) * block_size / sizeof(uint32_t)) + (int) (block_size / sizeof(uint32_t)) + 12 + i, inode.i_block[14], indr1_block);

				uint32_t offset, block;
				for (j = 0; j < block_size / sizeof(uint32_t); j++){
					pread(disk_fd, &indr_block, sizeof(uint32_t), indr_offset + j * sizeof(uint32_t));
					if(indr_block != 0) {
						offset = indr_block * block_size;
						fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", num_free_inode, 2, (int) (block_size / sizeof(uint32_t) * block_size / sizeof(uint32_t)) + (int) (block_size / sizeof(uint32_t)) + 12 + j, indr1_block, indr_block);

						for (k = 0; k < block_size / sizeof(uint32_t); k++){
							pread(disk_fd, &block, sizeof(uint32_t), offset + k * sizeof(uint32_t));
							if(block != 0) {
								if (file_type == 'd') {
									directory_entries(num_free_inode, block);
								}
								fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", num_free_inode, 1, (int) (block_size / sizeof(uint32_t) * block_size / sizeof(uint32_t)) + (int) (block_size / sizeof(uint32_t)) + 12 + k, indr_block, block);
							}
						}
					}
				}
			}
		}
	}
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
	char* ctime = get_time(inode.i_ctime);
	char* mtime = get_time(inode.i_mtime);
	char* atime = get_time(inode.i_atime);
	uint32_t file_size = inode.i_size;
	uint32_t num_blocks = inode.i_blocks;
	fprintf(stdout, "INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d", num_free_inode, file_type, imode, owner, group, link_count, ctime, mtime, atime, file_size, num_blocks);

	uint32_t i;
	for (i = 0; i < 15; i++) {
		// block address
		fprintf(stdout, ",%d", inode.i_block[i]);
	}
	fprintf(stdout, "\n");

	// i_block[0..11] point directly to the first 12 data blocks of the file
	for (i = 0; i < 12; i++) {
		if (inode.i_block[i] != 0 && file_type == 'd') {
			directory_entries(num_free_inode, inode.i_block[i]);
		}
	}

	single_indirect_block(inode, num_free_inode, file_type);
	double_indirect_block(inode, num_free_inode, file_type);
	tripple_indirect_block(inode, num_free_inode, file_type);


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