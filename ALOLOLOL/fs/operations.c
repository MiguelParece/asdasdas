#include "operations.h"
#include "config.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


#include "betterassert.h"


tfs_params tfs_default_params() {
    tfs_params params = {
        .max_inode_count = 64,
        .max_block_count = 1024,
        .max_open_files_count = 16,
        .block_size = 1024,
    };
    return params;
}

int tfs_init(tfs_params const *params_ptr) {
    tfs_params params;
    if (params_ptr != NULL) {
        params = *params_ptr;
    } else {
        params = tfs_default_params();
    }

    if (state_init(params) != 0) {
        return -1;
    }

    // create root inode
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }
    return 0;
}

int tfs_destroy() {
    if (state_destroy() != 0) {
        return -1;
    }
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}



/**
 * Looks for a file.
 *
 * Note: as a simplification, only a plain directory space (root directory only)
 * is supported.
 *
 * Input:
 *   - name: absolute path name
 *   - root_inode: the root directory inode
 * Returns the inumber of the file, -1 if unsuccessful.
 */
static int tfs_lookup(char const *name, inode_t const *root_inode,int Flag) {
    // TODO: assert that root_inode is the root directory
    //assert(root_inode == ROOT_DIR_INUM);
    if (!valid_pathname(name)) {
        return -1;
    }
    if(Flag){
    // skip the initial '/' character
    name++;
    mutex_lock(&inode_Whole_locks);
    int r = find_in_dir(root_inode, name);
    mutex_unlock(&inode_Whole_locks);
    return r;
    }
    
    else{
    // skip the initial '/' character
    name++;
    return find_in_dir(root_inode, name);
    }

    
    
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    int inum;
    size_t offset;

    if (!valid_pathname(name)) {
        return -1;
    }
    mutex_lock(&inode_Whole_locks);

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");
    
    inum = tfs_lookup(name, root_dir_inode,0);

    if (inum >= 0) { 
        // The file already exists
        //Unlocks the table and locks the specific inode's lock
        mutex_unlock(&inode_Whole_locks);
        pthread_rwlock_wrlock(&inode_locks[inum]);
        
        inode_t *inode = inode_get(inum);
        ALWAYS_ASSERT(inode != NULL,
                      "tfs_open: directory files must have an inode");

        if(inode->i_node_type==T_SYM_LINK){
            if(tfs_lookup(inode->name_of_destination,root_dir_inode,1)==-1){
                tfs_close(inum);
                pthread_rwlock_unlock(&inode_locks[inum]);
                return -1;
            }
                tfs_close(inum);
                pthread_rwlock_unlock(&inode_locks[inum]);
                return tfs_open(inode->name_of_destination,mode);
        }

        // Truncate (if requested)
        if (mode & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                data_block_free(inode->i_data_block);
                inode->i_size = 0;
            }
        }
        // Determine initial offset
        if (mode & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (mode & TFS_O_CREAT) {
        // The file does not exist; the mode specified that it should be created
        // Create inode
        inum = inode_create(T_FILE);
        
        if (inum == -1) {
            mutex_unlock(&inode_Whole_locks);
            return -1; // no space in inode table
        }

        // Add entry in the root directory
        if (add_dir_entry(root_dir_inode, name + 1, inum) == -1) {
            inode_delete(inum);
            mutex_unlock(&inode_Whole_locks);
            return -1; // no space in directory
        }
        
        offset = 0;
        mutex_unlock(&inode_Whole_locks);
    } else {
        mutex_unlock(&inode_Whole_locks);
        return -1;
    }
    
    // Finally, add entry to the open file table and return the corresponding
    // handle
    return add_to_open_file_table(inum, offset);

    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened but it remains created
}


int tfs_sym_link(char const *target, char const *link_name) {
    inode_t *root_inode = inode_get(ROOT_DIR_INUM);
    if(tfs_lookup(target,root_inode,1)==-1){
        return -1;
    }
    mutex_lock(&inode_Whole_locks);
    int inumber= inode_create(T_SYM_LINK);

    if(inumber==-1){
        mutex_unlock(&inode_Whole_locks);
        return -1;
    }
    inode_t *inode= inode_get(inumber);
    strcpy(inode->name_of_destination,target);
    if(add_dir_entry(root_inode,link_name+1,inumber)==-1){
        mutex_unlock(&inode_Whole_locks);
        return -1;
    }
    mutex_unlock(&inode_Whole_locks);
    return 0;

    PANIC("TODO: tfs_sym_link");
}

int tfs_link(char const *target, char const *link_name) {
    inode_t *inode_root= inode_get(ROOT_DIR_INUM);
    mutex_lock(&inode_Whole_locks);
    int inumber=find_in_dir(inode_root,target+1);
    inode_t *inodeOfTarget = inode_get(inumber);    
    if (inumber == -1) {
        mutex_unlock(&inode_Whole_locks);
        return -1;
    }
    if(inodeOfTarget->i_node_type==T_SYM_LINK){
        mutex_unlock(&inode_Whole_locks);
        return -1;
    }
    if(add_dir_entry(inode_root, link_name+1, inumber)==-1){
        mutex_unlock(&inode_Whole_locks);
        return -1;
    }
    inodeOfTarget->number_hard_links++;
    mutex_unlock(&inode_Whole_locks);
    return 0;

    PANIC("TODO: tfs_link");
}


int tfs_close(int fhandle) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1; // invalid fd
    }

    remove_from_open_file_table(fhandle);

    return 0;
}


ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }
    
    //  From the open file table entry, we get the inode
    pthread_rwlock_wrlock(&inode_locks[file->of_inumber]);
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");

    // Determine how many bytes to write

    size_t block_size = state_block_size();
    if (to_write + file->of_offset > block_size) {
        to_write = block_size - file->of_offset;
    }

    if (to_write > 0) {
        if (inode->i_size == 0) {
            // If empty file, allocate new block
            int bnum = data_block_alloc();
            if (bnum == -1) {
                pthread_rwlock_unlock(&inode_locks[file->of_inumber]);
                return -1; // no space
            }
            inode->i_data_block = bnum;
        }

        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_write: data block deleted mid-write");

        // Perform the actual write
        memcpy(block + file->of_offset, buffer, to_write);

        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_write;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }
    pthread_rwlock_unlock(&inode_locks[file->of_inumber]);
    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {

    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    // From the open file table entry, we get the inode
    pthread_rwlock_rdlock(&inode_locks[file->of_inumber]);
    inode_t const *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_read: inode of open file deleted");

    // Determine how many bytes to read

    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    if (to_read > 0) {
        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_read: data block deleted mid-read");

        // Perform the actual read
        memcpy(buffer, block + file->of_offset, to_read);
        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_read;
    }
    pthread_rwlock_unlock(&inode_locks[file->of_inumber]);
    return (ssize_t)to_read;
}

int tfs_unlink(char const *target) {
    mutex_lock(&inode_Whole_locks);
    inode_t *inodeOfRoot= inode_get(ROOT_DIR_INUM);
    int inumber = tfs_lookup(target,inodeOfRoot,0);
    if(inumber==-1){
        mutex_unlock(&inode_Whole_locks);
        return -1;
    }
    inode_t *inodeOfTarget= inode_get(inumber);
    if(inodeOfTarget->i_node_type==T_SYM_LINK){
        clear_dir_entry(inodeOfRoot,target+1);
        inode_delete(inumber);
        mutex_unlock(&inode_Whole_locks);
        return 0;
    }
    else{
        inodeOfTarget->number_hard_links--;
        if(inodeOfTarget->number_hard_links==0){
            clear_dir_entry(inodeOfRoot,target+1);
            inode_delete(inumber);
        }
        else{
            clear_dir_entry(inodeOfRoot,target+1);
        }
    mutex_unlock(&inode_Whole_locks);
    return 0;
    }

    PANIC("TODO: tfs_unlink");
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {

    FILE *file = fopen(source_path, "r");
    if (!file) {
        return -1;
    }
    tfs_params params = tfs_default_params();
    int fhandle;
    fhandle = tfs_open(dest_path, 0b011);
    if (fhandle == -1){
        fclose(file);
        return -1;
    }
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));

    ssize_t bytes_read = 0;
    ssize_t bytes_written = 0;
    ssize_t count=0;
    while (1) {
        bytes_read = (ssize_t)fread(buffer, 1, sizeof(buffer), file);
        if (bytes_read == 0) {
            if (!feof(file)) {
                fclose(file);
                return -1;
            } else {
                break;
            }
        }
        count+=bytes_read;
        if(count>params.block_size){
            return -1;
        }

        while (bytes_read > bytes_written) {
            bytes_written += tfs_write(fhandle, buffer + bytes_written,
                                       (size_t)(bytes_read - bytes_written));
        }
        bytes_read = 0;
        bytes_written = 0;
    }
    /* close the file */
    if (fclose(file) == -1) {
        return -1;
    }
    if (tfs_close(fhandle) == -1) {
        return -1;
    }
    return 0;
}
