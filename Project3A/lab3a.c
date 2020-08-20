/*
NAME: Changhui Youn
EMAIL: tonyyoun2@gmail.com
ID: 304207830
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdint.h>
#include <time.h>
#include <getopt.h>
#include "ext2_fs.h"

#define SUPER_OFFSET 1024
#define EXT2_FILE 0x8000
#define EXT2_DIR 0x4000
#define EXT2_SYM 0xA000

int mount_fd;

struct ext2_super_block superblock;
uint32_t block_size = 1024;

void pread_error(){
	fprintf(stderr, "Error while pread\n");
	exit(1);	
}

// This function converts time_t time into "%m/%d/%y %H:%M:%S" time format
char* get_time(time_t time) {
	char* time_format = malloc(sizeof(char)*20);
	struct tm t = *gmtime(&time);
	strftime(time_format, 20, "%m/%d/%y %H:%M:%S", &t);
	return time_format;
}

// This function prints directory entry summary for entries that are not empty
void directory_entries(uint32_t parent_inode, uint32_t block_num) {
	struct ext2_dir_entry dir_entry;
	uint32_t offset = block_num * block_size;
	uint32_t start = offset; 
	while(offset < start + block_size) {
		if((pread(mount_fd, &dir_entry, sizeof(struct ext2_dir_entry), offset)) < 0)
			pread_error();
		uint32_t logical_byte = offset - start;
		if(dir_entry.inode != 0)
			fprintf(stdout, "DIRENT,%d,%d,%d,%d,%d,'%s'\n", parent_inode, logical_byte, dir_entry.inode, dir_entry.rec_len, dir_entry.name_len, dir_entry.name);
		offset += dir_entry.rec_len;
	}
}

// This function prints out superblock summary
void superblock_summary() {
	if((pread(mount_fd, &superblock, sizeof(struct ext2_super_block), SUPER_OFFSET)) < 0)
		pread_error();
	block_size = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
	fprintf(stdout, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", superblock.s_blocks_count, superblock.s_inodes_count, block_size, superblock.s_inode_size, superblock.s_blocks_per_group, superblock.s_inodes_per_group, superblock.s_first_ino);
}

// It prints number of free blocks for each not used block
void free_block_enties(uint32_t block_bitmap, int group_num){
	char* block = (char*) malloc(block_size);
	uint32_t num_free_block = superblock.s_first_data_block + superblock.s_blocks_per_group * group_num;
	if((pread(mount_fd, block, block_size, SUPER_OFFSET + (block_bitmap - 1) * block_size)) < 0)
		pread_error();

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

// i_block[12] points to a single indirect block
void single_indirect_block(struct ext2_inode inode, uint32_t num_free_inode, char file_type) {
	if (inode.i_block[12] != 0) {
		uint32_t offset = inode.i_block[12] * block_size;
		uint32_t block;		

		uint32_t i;
		for (i = 0; i < block_size / sizeof(uint32_t); i++) {
			if((pread(mount_fd, &block, sizeof(uint32_t), offset + i * sizeof(uint32_t))) < 0)
				pread_error();
			if (block != 0) {
				if (file_type == 'd') {
					directory_entries(num_free_inode, block);
				}
				fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", num_free_inode, 1, 12 + i, inode.i_block[12], block);
			}
		}
	}

}


// i_block[13] points to a double indirect block
void double_indirect_block(struct ext2_inode inode, uint32_t num_free_inode, char file_type) {
	if (inode.i_block[13] != 0) {
		uint32_t indr_offset = inode.i_block[13] * block_size;
		uint32_t indr_block;		

		uint32_t i, j;
		for (i = 0; i < block_size / sizeof(uint32_t); i++) {
			uint32_t offset, block;
			if((pread(mount_fd, &indr_block, sizeof(uint32_t), indr_offset + i * sizeof(uint32_t))) < 0)
				pread_error();
			if (indr_block != 0) {
				offset = indr_block * block_size;
				fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", num_free_inode, 2, 268 + i, inode.i_block[13], indr_block);
				for (j = 0; j < block_size / sizeof(uint32_t); j++){
					if((pread(mount_fd, &block, sizeof(uint32_t), offset + j * sizeof(uint32_t))) < 0)
						pread_error();
					if(block != 0) {
						if (file_type == 'd') {
							directory_entries(num_free_inode, block);
						}
						fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", num_free_inode, 1, 268 + j, indr_block, block);

					}
				}
			}
		}
	}

}

// i_block[14] points to a triple indirect block
void triple_indirect_block(struct ext2_inode inode, uint32_t num_free_inode, char file_type) {

	if (inode.i_block[14] != 0) {
		uint32_t indr1_offset = inode.i_block[14] * block_size;
		uint32_t indr1_block;		

		uint32_t i, j, k;
		for (i = 0; i < block_size / sizeof(uint32_t); i++) {
			uint32_t indr_offset, indr_block;
			if((pread(mount_fd, &indr1_block, sizeof(uint32_t), indr1_offset + i * sizeof(uint32_t))) < 0)
				pread_error();
			if (indr1_block != 0) {
				indr_offset = indr1_block * block_size;
				fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", num_free_inode, 3, 65804 + i, inode.i_block[14], indr1_block);

				uint32_t offset, block;
				for (j = 0; j < block_size / sizeof(uint32_t); j++){
					if((pread(mount_fd, &indr_block, sizeof(uint32_t), indr_offset + j * sizeof(uint32_t))) < 0)
						pread_error();
					if(indr_block != 0) {
						offset = indr_block * block_size;
						fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", num_free_inode, 2, 65804 + j, indr1_block, indr_block);

						for (k = 0; k < block_size / sizeof(uint32_t); k++){
							if((pread(mount_fd, &block, sizeof(uint32_t), offset + k * sizeof(uint32_t))) < 0)
								pread_error();
							if(block != 0) {
								if (file_type == 'd') {
									directory_entries(num_free_inode, block);
								}
								fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", num_free_inode, 1, 65804 + k, indr_block, block);
							}
						}
					}
				}
			}
		}
	}
}

// This function produces inode summary for an allocated inode
void inode_summary(uint32_t inode_table, int index, uint32_t num_free_inode) {
	struct ext2_inode inode;

	uint32_t offset = SUPER_OFFSET + (inode_table - 1) * block_size + sizeof(struct ext2_inode) * index;
	if((pread(mount_fd, &inode, sizeof(struct ext2_inode), offset)) < 0)
		pread_error();

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
	uint16_t uid = inode.i_uid;
	uint16_t gid = inode.i_gid;
	uint16_t link_count = inode.i_links_count;
	char* ctime = get_time(inode.i_ctime);
	char* mtime = get_time(inode.i_mtime);
	char* atime = get_time(inode.i_atime);
	uint32_t file_size = inode.i_size;
	uint32_t num_blocks = inode.i_blocks;
	fprintf(stdout, "INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d", num_free_inode, file_type, imode, uid, gid, link_count, ctime, mtime, atime, file_size, num_blocks);

	uint32_t i;
	// block address
	if(file_type == 's' || file_size <= 60)
		fprintf(stdout, ",%d", inode.i_block[0]);
	else {
		for (i = 0; i < 15; i++) {
			fprintf(stdout, ",%d", inode.i_block[i]);
		}
	}
	fprintf(stdout, "\n");

	// i_block[0..11] point directly to the first 12 data blocks of the file
	for (i = 0; i < 12; i++) {
		if (inode.i_block[i] != 0 && file_type == 'd') {
			directory_entries(num_free_inode, inode.i_block[i]);
		}
	}

	// indirect entries
	single_indirect_block(inode, num_free_inode, file_type);
	double_indirect_block(inode, num_free_inode, file_type);
	triple_indirect_block(inode, num_free_inode, file_type);


}

// In this function, it prints IFREE if inode is not used.
// If used, print inode summary
void free_inode_entries(uint32_t inode_bitmap, uint32_t inode_table, int group_num){
	uint32_t inode_size = superblock.s_inodes_per_group / 8;
	char* inode = (char*) malloc(inode_size);
	uint32_t num_free_inode = superblock.s_inodes_per_group * group_num + 1;
	if((pread(mount_fd, inode, inode_size, SUPER_OFFSET + (inode_bitmap - 1) * block_size)) < 0)
		pread_error();

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

// This function prints group summary and  reads bitmaps for blocks and inodes
void group_summary(int group_num, int num_group) {
	struct ext2_group_desc group_desc;
	if((pread(mount_fd, &group_desc, sizeof(struct ext2_group_desc), SUPER_OFFSET + sizeof(struct ext2_super_block))) < 0)
		pread_error();

	int num_block = (group_num == num_group - 1 ? superblock.s_blocks_count - (superblock.s_blocks_per_group * (num_group - 1)) : superblock.s_blocks_per_group);
	int num_inode = (group_num == num_group - 1 ? superblock.s_inodes_count - (superblock.s_inodes_per_group * (num_group - 1)) : superblock.s_inodes_per_group);

	fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", group_num, num_block, num_inode, group_desc.bg_free_blocks_count, group_desc.bg_free_inodes_count, group_desc.bg_block_bitmap, group_desc.bg_inode_bitmap, group_desc.bg_inode_table);

	free_block_enties(group_desc.bg_block_bitmap, group_num);
	free_inode_entries(group_desc.bg_inode_bitmap, group_desc.bg_inode_table, group_num);
}

int main(int argc, char const *argv[]) {
	if(argc != 2) {
		fprintf(stderr, "Invalid arguments\n");
		exit(1);
	}
	if((mount_fd = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "Could not mount\n");
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