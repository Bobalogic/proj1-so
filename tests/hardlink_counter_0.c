#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t const file_contents[] = "AAA!";
char const target_path1[] = "/f1";
char const link_path1[] = "/l1";
char const target_path2[] = "/f2";
char const link_path2[] = "/l2";
char const target_path3[] = "/f3";
char const link_path3[] = "/l3";
char const bad_link[] = "f3";
char const sym_link_path1[] = "/sl1";
char const sym_link_path2[] = "/sl2";

void assert_contents_ok(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void assert_empty_file(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void write_contents(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
}

int main() {
    assert(tfs_init(NULL) != -1);

    // Write to symlink and read original file
    {
        int f = tfs_open(target_path1, TFS_O_CREAT);
        assert(f != -1);
        assert(tfs_close(f) != -1);

        assert_empty_file(target_path1); // sanity check
    }

    // Create two hardlinks to the same file
    assert(tfs_link(target_path1, link_path1) != -1);
    assert_empty_file(link_path1);
    assert(tfs_link(target_path1, link_path2) != -1);
    assert_empty_file(link_path2);

    // Create symbolic link to the file
    assert(tfs_sym_link(target_path1, sym_link_path1) != -1);
    assert_empty_file(sym_link_path1);

    // Create a hardlink with invalid name (should not be created)
    assert(tfs_link(target_path1, bad_link) == -1);

    // Remove the two hardlinks and the original file 
    assert(tfs_unlink(link_path1) != -1);
    assert(tfs_unlink(link_path2) != -1);
    assert(tfs_unlink(target_path1) != -1);

    // Create a hardlink to a file that does not exist (should not be created)
    assert(tfs_link(target_path1, link_path1) == -1);

    // Remove the symbolic link 
    assert(tfs_unlink(sym_link_path1) != -1);

    // Open sym link (shouldn't open because it's pointing to a null entry)
    assert(tfs_open(sym_link_path1, TFS_O_APPEND) == -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
