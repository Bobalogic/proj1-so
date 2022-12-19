#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {

    char str_ext_file[] = "testtttttt";
    char final_content_should_be[] = "test";
    char *path_copied_file = "/f1";
    char *path_src = "tests/file_to_write.txt";
    char buffer[30];

    assert(tfs_init(NULL) != -1);

    int f;
    ssize_t r;

    // Creating a file
    f = tfs_open(path_copied_file, TFS_O_CREAT);

    // Writing "test" on new file
    tfs_write(f, str_ext_file, sizeof(str_ext_file) - 1);

    // The function should overwrite previous content, since the destination path already exists
    f = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(f != -1);

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    // Reading the updated file
    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(final_content_should_be) + 1); // +1 character because of EOF
    assert(!memcmp(buffer, final_content_should_be, strlen(final_content_should_be)));

    printf("Successful test.\n");

    return 0;
}
