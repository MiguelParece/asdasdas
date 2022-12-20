#ifndef STATE_H
#define STATE_H

#include "config.h"
#include "operations.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

/**
 * Directory entry
 */
typedef struct {
    char d_name[MAX_FILE_NAME];
    int d_inumber;
} dir_entry_t;

typedef enum { T_FILE, T_DIRECTORY, T_SYM_LINK } inode_type;

typedef struct {
    inode_type i_node_type;
    size_t i_size;
    int i_data_block;
    int number_hard_links;
    char name_of_destination[40];
    // in a more complete FS, more fields could exist here
} inode_t;

typedef enum { FREE = 0, TAKEN = 1 } allocation_state_t;

extern pthread_mutex_t inode_Whole_locks;
extern pthread_mutex_t open_Whole_file_entries;
extern pthread_rwlock_t inode_locks[64];
extern pthread_rwlock_t open_file_locks[16];

/**
 * Open file entry (in open file table)
 */
typedef struct {
    int of_inumber;
    size_t of_offset;
    pthread_t lock;
} open_file_entry_t;

int state_init(tfs_params);
int state_destroy(void);

size_t state_block_size(void);

int inode_create(inode_type n_type);
void inode_delete(int inumber);
inode_t *inode_get(int inumber);

int clear_dir_entry(inode_t *inode, char const *sub_name);
int add_dir_entry(inode_t *inode, char const *sub_name, int sub_inumber);
int find_in_dir(inode_t const *inode, char const *sub_name);

int data_block_alloc(void);
void data_block_free(int block_number);
void *data_block_get(int block_number);

int add_to_open_file_table(int inumber, size_t offset);
void remove_from_open_file_table(int fhandle);
open_file_entry_t *get_open_file_entry(int fhandle);

void mutex_init(pthread_mutex_t *mutex);
void mutex_destroy(pthread_mutex_t *mutex);
void mutex_lock(pthread_mutex_t *mutex);
void mutex_unlock(pthread_mutex_t *mutex);
#endif // STATE_H
