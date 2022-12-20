#include "operations.h"
#include "config.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include "betterassert.h"

// static pthread_mutex_t global_lock;
// static pthread_cond_t cond;

//Sc - mexer com inodes e mexer com ficheiros abertos
//Fazer 3 testes para o 1.3
//Fazer 1 teste para o 1.1 e para o 1.2
//Mutex é mais facil mas dá menos nota, ptt usar os rwlock


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
static int tfs_lookup(char const *name, inode_t const *root_inode) {
    // TODO: assert that root_inode is the root directory
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(root_inode, name);
}

// New
int get_hard_link_inum(int inum) {
    inode_t *inode = inode_get(inum);
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);

    while (inode -> i_node_type == SYM_LINK){
        inum = tfs_lookup(inode -> sym_path, root_dir_inode);
        if (inum < 0)
            return -1;
        inode = inode_get(inum);
    }

    return inum;
}



int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    if (!valid_pathname(name)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");
    int inum = tfs_lookup(name, root_dir_inode);

    size_t offset;

    if (inum >= 0) {

        // The file already exists
        inode_t *inode = inode_get(inum);

        // If inode is a symbolic link
        if (inode -> i_node_type == SYM_LINK){
            inum = get_hard_link_inum(inum);
            if (inum == -1)
                return -1;
            if (isFreeInode(inum))
                return -1;
        }

        ALWAYS_ASSERT(inode != NULL,
                      "tfs_open: directory files must have an inode");

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
            return -1; // no space in inode table
        }

        // Add entry in the root directory
        if (add_dir_entry(root_dir_inode, name + 1, inum) == -1) {
            inode_delete(inum);
            return -1; // no space in directory
        }

        offset = 0;
    } else {
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
    // Link must have a valid name
    if (!valid_pathname(link_name))
        return -1;

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);

    // Create inode for the symbolic link 
    int sym_inumber = inode_create(SYM_LINK);

    // Add entry in the root directory
    if (add_dir_entry(root_dir_inode, link_name + 1, sym_inumber) == -1) {
        inode_delete(sym_inumber);
        return -1; // no space in directory
    }
    
    inode_t *sym_inode = inode_get(sym_inumber);
    sym_inode -> i_node_type = SYM_LINK;
    strcpy(sym_inode -> sym_path, target);  

    return 0;
}

int tfs_link(char const *target, char const *link_name) {
    // Link must have a valid name
    if (!valid_pathname(link_name) || !valid_pathname(target))
        return -1;

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);  
    int target_inum = tfs_lookup(target, root_dir_inode);

    // If target is not a valid entry
    if (target_inum == -1)
        return -1;

    inode_t *target_inode = inode_get(target_inum);

    // If target is a sym link
    if (target_inode -> i_node_type == SYM_LINK)
        return -1;      

    // If target has no hard links
    if (target_inode -> hl_count == 0)
        return -1;

    // Add entry in the root directory
    if (add_dir_entry(root_dir_inode, link_name + 1, target_inum) == -1) {
        inode_delete(target_inum);
        return -1; // no space in directory
    }
    
    // Updating hard link counter
    target_inode -> hl_count = target_inode -> hl_count + 1;

    return 0;
} 

int tfs_unlink(char const *target) {

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    int link_inum = tfs_lookup(target, root_dir_inode);
    inode_t *link_inode = inode_get(link_inum);

    // If inode is soft
    if (link_inode -> i_node_type == SYM_LINK){
        clear_dir_entry(root_dir_inode, target + 1);
        inode_delete(link_inum);
    }

    // If inode is hard
    else {
        clear_dir_entry(root_dir_inode, target + 1);
        link_inode -> hl_count = link_inode -> hl_count - 1;

        if (link_inode -> hl_count == 0){
            // TODO: Chek if any processes have the file open
            inode_delete(link_inum);
        }
    }

    return 0;
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

    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    // From the open file table entry, we get the inode
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

    return (ssize_t)to_read;
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
    /** TODO: Limit the dimension to 1 block
     * if dest_path doesn't exist, this should be created
     * if dest_path already exists, the new contents should replace, not append, the old contents
     */

    FILE *myfile;
    char buffer[128];
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    int dest_fhandle;

    // Source path doesn't exist
    myfile = fopen(source_path, "r");
    if (myfile == NULL){
        return -1;
    }

    // Destination path already exists    
    if (find_in_dir(root_dir_inode, dest_path + 1) != -1){
        dest_fhandle = tfs_open(dest_path, TFS_O_TRUNC);
        dest_fhandle = tfs_open(dest_path, TFS_O_APPEND);
    }
    
    // Destination path doesn't exist
    else dest_fhandle = tfs_open(dest_path, TFS_O_CREAT);

    
    size_t aux = 1;
    while (aux > 0  ){
        aux = fread(buffer, sizeof(char), sizeof(buffer) - 1, myfile);
        tfs_write(dest_fhandle, buffer, aux);
        memset(buffer, 0, sizeof(buffer));
        }
   return 0;
}
